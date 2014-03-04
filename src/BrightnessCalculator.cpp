#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>

#include "IByteReader.h"
#include "InputFile.h"
#include "IOBasicTypes.h"

#include "jpeglib.h"

using namespace Hummus;

typedef std::vector<Byte> ByteVector;

struct BrightnessState
{
	jpeg_decompress_struct mJPGState;
	jpeg_error_mgr mJPGError;
	IByteReader* mStream;
	JSAMPARRAY mSamplesBuffer;
	LongBufferSizeType mIndexInRow;
	LongBufferSizeType mCurrentSampleRow;
	LongBufferSizeType mTotalSampleRows;
};

class HummusJPGException
{
};

METHODDEF(void) HummusJPGErrorExit (j_common_ptr cinfo)
{
    (*cinfo->err->output_message) (cinfo);
    throw HummusJPGException();
}

METHODDEF(void) HummusJPGOutputMessage(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    
    (*cinfo->err->format_message) (cinfo, buffer);
}

struct HummusSourceManager
{
    struct jpeg_source_mgr pub;	/* public fields */
    
    IByteReader *mReader;	/* source stream */
    JOCTET * buffer;		/* start of buffer */
};

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */

METHODDEF(boolean) HummusFillInputBuffer (j_decompress_ptr cinfo)
{
    HummusSourceManager* src = (HummusSourceManager*) cinfo->src;
    size_t nbytes;
    
    nbytes =  src->mReader->Read((Byte*)(src->buffer),INPUT_BUF_SIZE);
    
    if (nbytes <= 0) {
        /* Insert a fake EOI marker */
        src->buffer[0] = (JOCTET) 0xFF;
        src->buffer[1] = (JOCTET) JPEG_EOI;
        nbytes = 2;
    }
    
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = nbytes;
    
    return TRUE;
}

METHODDEF(void) HummusSkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
    
    struct jpeg_source_mgr * src = cinfo->src;
    
    if (num_bytes > 0) {
        while (num_bytes > (long) src->bytes_in_buffer) {
            num_bytes -= (long) src->bytes_in_buffer;
            (void) (*src->fill_input_buffer) (cinfo);
            /* note we assume that fill_input_buffer will never return FALSE,
             * so suspension need not be handled.
             */
        }
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

METHODDEF(void) HummusNoOp (j_decompress_ptr cinfo)
{
    /* no work necessary here */
}

METHODDEF(void) HummusJPGSourceInitialization (j_decompress_ptr cinfo, IByteReader * inSourceStream)
{
    HummusSourceManager* src;
    
    /* The source object and input buffer are made permanent so that a series
     * of JPEG images can be read from the same file by calling jpeg_stdio_src
     * only before the first one.  (If we discarded the buffer at the end of
     * one image, we'd likely lose the start of the next one.)
     * This makes it unsafe to use this manager and a different source
     * manager serially with the same JPEG object.  Caveat programmer.
     */
    if (cinfo->src == NULL) {	/* first time for this JPEG object? */
        cinfo->src = (struct jpeg_source_mgr *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                    sizeof(HummusSourceManager));
        src = (HummusSourceManager*) cinfo->src;
        src->buffer = (JOCTET *)
        (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                    INPUT_BUF_SIZE * sizeof(JOCTET));
    }
    
    src = (HummusSourceManager*) cinfo->src;
    src->pub.init_source = HummusNoOp;
    src->pub.fill_input_buffer = HummusFillInputBuffer;
    src->pub.skip_input_data = HummusSkipInputData;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = HummusNoOp;
    src->mReader = inSourceStream;
    src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
    src->pub.next_input_byte = NULL; /* until buffer loaded */
}

EStatusCode InitializeDecodingState(BrightnessState& inState)
{
    EStatusCode status = eSuccess;
    inState.mJPGState.err = jpeg_std_error(&(inState.mJPGError));
    inState.mJPGError.error_exit = HummusJPGErrorExit;
    inState.mJPGError.output_message = HummusJPGOutputMessage;
    
    try {
        jpeg_create_decompress(&(inState.mJPGState));
        HummusJPGSourceInitialization(&(inState.mJPGState),inState.mStream);
    }
    catch(HummusJPGException)
    {
        status = eFailure;
    }
    return status;
}

EStatusCode StartRead(BrightnessState& inState)
{
    EStatusCode status = eSuccess;
    
    try
    {
        jpeg_read_header(&(inState.mJPGState), TRUE);
        
        // require RGB color, so i can make simple conversion to hsb
        inState.mJPGState.out_color_space = JCS_RGB;
        
        jpeg_start_decompress(&(inState.mJPGState));
        
        int row_stride = inState.mJPGState.output_width * inState.mJPGState.output_components;
        inState.mSamplesBuffer = (*(inState.mJPGState).mem->alloc_sarray)
        ((j_common_ptr) &(inState.mJPGState), JPOOL_IMAGE, row_stride, (inState.mJPGState).rec_outbuf_height);
        inState.mCurrentSampleRow = 0;
        inState.mTotalSampleRows = 0;
        inState.mIndexInRow = 0;
        
    }
    catch(HummusJPGException)
    {
        status = eFailure;
    }
    return status;
    
}

void FinalizeDecoding(BrightnessState& inState)
{
    jpeg_finish_decompress(&(inState.mJPGState));
    jpeg_destroy_decompress(&(inState.mJPGState));
    inState.mSamplesBuffer = NULL;
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//		if s == 0, then h = -1 (undefined)
Byte RGBtoHSVtoBrightness(Byte inR,Byte inG,Byte inB)
{
    float r,g,b,h,s,v;
    r = (float)inR/255.0;
    g = (float)inG/255.0;
    b = (float)inB/255.0;
    
	float min, max, delta;
	min = std::min(r,std::min(g,b));
	max = std::max(r,std::max(g,b));
	v = max;				// v
	delta = max - min;
	if( max != 0 )
		s = delta / max;		// s
	else {
		// r = g = b = 0		// s = 0, v is undefined
		s = 0;
		h = -1;
		return 0;
	}
	if( r == max )
		h = ( g - b ) / delta;		// between yellow & magenta
	else if( g == max )
		h = 2 + ( b - r ) / delta;	// between cyan & yellow
	else
		h = 4 + ( r - g ) / delta;	// between magenta & cyan
	h *= 60;				// degrees
	if( h < 0 )
		h += 360;

    return v*255;
}

unsigned long calculateDistance(Byte inBaseValue,const ByteVector& inVector)
{
    unsigned long result = 0;
    
    ByteVector::const_iterator it = inVector.begin();
    for(; it != inVector.end();++it)
        result+=std::abs(*it-inBaseValue);
    
    return result;
}

struct DistanceAndBrightness {
    Byte brightness;
    unsigned long distance;
    
    DistanceAndBrightness(Byte inBrightness,unsigned long inDistance)
    {
        brightness = inBrightness;
        distance = inDistance;
    }
};
typedef std::vector<DistanceAndBrightness> DistanceAndBrightnessVector;

static bool DistanceAndBrightnessSort(const DistanceAndBrightness& inLeft, const DistanceAndBrightness& inRight)
{
	return inLeft.distance < inRight.distance;
}

Byte calculateImageBrightnessFactor(const std::string& inImageFilePath)
{
	BrightnessState state;
    InputFile inputFile;
    ByteVector brightnessValues;
    DistanceAndBrightnessVector valuesForAverage;
    
    inputFile.OpenFile(inImageFilePath);
    
    state.mStream = inputFile.GetInputStream();
    InitializeDecodingState(state);
    StartRead(state);
    
    LongBufferSizeType samplesSkipping = 1; // max samples 20
    if(state.mJPGState.output_width > 500)
        samplesSkipping = state.mJPGState.output_width / 50; // max samples - 50
    else if(state.mJPGState.output_width > 20)
        samplesSkipping = 10; // max samples - 50
    
    LongBufferSizeType rowsSkipping = 1; // max samples 20
    if(state.mJPGState.output_height > 500)
        rowsSkipping = state.mJPGState.output_height / 50; // max samples - 50
    else if(state.mJPGState.output_height > 20)
        rowsSkipping = 10; // max samples - 50
    
    LongBufferSizeType rowCounter = 0;
    
    
    // read samples from image, converting to hsb and keeping the "b" component, as a brightness factor
    while(state.mJPGState.output_scanline < state.mJPGState.output_height)
    {
        try
        {
            state.mTotalSampleRows = jpeg_read_scanlines(&(state.mJPGState), state.mSamplesBuffer, state.mJPGState.rec_outbuf_height);
            ++rowCounter;
            if(rowCounter >= rowsSkipping)
                rowCounter = 0;
            else if(rowCounter != 1)
                continue;
        }
        catch(HummusJPGException)
        {
            state.mTotalSampleRows = 0;
        }
        state.mIndexInRow = 0;
        state.mCurrentSampleRow = 0;
        
        while(state.mCurrentSampleRow < state.mTotalSampleRows)
        {
            LongBufferSizeType row_stride = state.mJPGState.output_width * state.mJPGState.output_components;
            
            // convert samples to HSB (note that some samples are skipped)
            for(LongBufferSizeType i=0;i<row_stride;i+=(state.mJPGState.output_components*samplesSkipping))
            {
                // get rgb
                Byte r = state.mSamplesBuffer[state.mCurrentSampleRow][i];
                Byte g = state.mSamplesBuffer[state.mCurrentSampleRow][i+1];
                Byte b = state.mSamplesBuffer[state.mCurrentSampleRow][i+2];
                
                // calculate brightness [converting to HSB]
                brightnessValues.push_back(RGBtoHSVtoBrightness(r,g,b));
            }
            
            
            ++state.mCurrentSampleRow;
        }
    }
    FinalizeDecoding(state);
    
    // prepare distance data and sort, to remove extremes from calculation
    ByteVector::const_iterator it = brightnessValues.begin();
    for(;it!=brightnessValues.end();++it)
        valuesForAverage.push_back(DistanceAndBrightness(*it,calculateDistance(*it,brightnessValues)));
    
    std::sort(valuesForAverage.begin(),valuesForAverage.end(),DistanceAndBrightnessSort);
    
    
    // now simply calculate the average based on the first 2/3 of the vector, to reduce the effects of extremes
    double average = 0;
    DistanceAndBrightnessVector::const_iterator itCurrent = valuesForAverage.begin();
	unsigned long interestingItemsCount = floor(valuesForAverage.size()*2.0/3.0);
    DistanceAndBrightnessVector::const_iterator itEnd = valuesForAverage.begin()+interestingItemsCount;
    for(itCurrent = valuesForAverage.begin();itCurrent!=itEnd;++itCurrent)
        average+=(double)(itCurrent->brightness)/interestingItemsCount;
    return ceil(average);
}

int main(int argc, char* argv[])
{
    if(argc >=2)
        cout<<"Brightness :"<<(unsigned int)calculateImageBrightnessFactor(
                                argv[1])<<"\n";
    
	return 0;
}

