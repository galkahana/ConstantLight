/*
   Source File : InputFile.cpp


   Copyright 2011 Gal Kahana Hummus

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   
*/
#include "InputFile.h"
#include "InputBufferedStream.h"
#include "InputFileStream.h"

using namespace Hummus;

InputFile::InputFile(void)
{
	mInputStream = NULL;
	mFileStream = NULL;
}

InputFile::~InputFile(void)
{
	CloseFile();
}

EStatusCode InputFile::OpenFile(const string& inFilePath)
{
	EStatusCode status;
	do
	{
		status = CloseFile();
		if(status != eSuccess)
			break;
	
		InputFileStream* inputFileStream = new InputFileStream();
		status = inputFileStream->Open(inFilePath); // explicitly open, so status may be retrieved
		if(status != eSuccess)
		{
			delete inputFileStream;
			break;
		}

		mInputStream = new InputBufferedStream(inputFileStream);
		mFileStream = inputFileStream;
		mFilePath = inFilePath;
	} while(false);
	return status;
}

EStatusCode InputFile::CloseFile()
{
	if(NULL == mInputStream)
	{
		return eSuccess;
	}
	else
	{
		EStatusCode status = mFileStream->Close(); // explicitly close, so status may be retrieved

		delete mInputStream; // will delete the referenced file stream as well
		mInputStream = NULL;
		mFileStream = NULL;
		return status;
	}
}


IByteReaderWithPosition* InputFile::GetInputStream()
{
	return mInputStream;
}

const string& InputFile::GetFilePath()
{
	return mFilePath;
}

LongFilePositionType InputFile::GetFileSize()
{
	if(mInputStream)
	{
		InputFileStream* inputFileStream = (InputFileStream*)mInputStream->GetSourceStream();

		return inputFileStream->GetFileSize();
	}
	else
		return 0;
}