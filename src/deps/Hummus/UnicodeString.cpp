/*
   Source File : UnicodeString.cpp


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
#include "UnicodeString.h"
#include <sstream>

using namespace Hummus;

UnicodeString::UnicodeString(void)
{
}

UnicodeString::~UnicodeString(void)
{
}

UnicodeString::UnicodeString(const UnicodeString& inOtherString)
{
	mUnicodeCharacters = inOtherString.mUnicodeCharacters;
}

UnicodeString::UnicodeString(const ULongList& inOtherList)
{
	mUnicodeCharacters = inOtherList;
}

UnicodeString& UnicodeString::operator =(const UnicodeString& inOtherString)
{
	mUnicodeCharacters = inOtherString.mUnicodeCharacters;
	return *this;
}

UnicodeString& UnicodeString::operator =(const ULongList& inOtherList)
{
	mUnicodeCharacters = inOtherList;
	return *this;
}

bool UnicodeString::operator==(const UnicodeString& inOtherString) const
{
	return mUnicodeCharacters == inOtherString.mUnicodeCharacters;
}


const ULongList& UnicodeString::GetUnicodeList() const
{
	return mUnicodeCharacters;
}

ULongList& UnicodeString::GetUnicodeList()
{
	return mUnicodeCharacters;
}

EStatusCode UnicodeString::FromUTF8(const string& inString)
{
	mUnicodeCharacters.clear();
	string::const_iterator it = inString.begin();
	EStatusCode status = eSuccess;
	unsigned long unicodeCharacter;


	for(; it != inString.end() && eSuccess == status; ++it)
	{
		if((unsigned char)*it <= 0x7F)
		{
			unicodeCharacter = (unsigned char)*it;
		}
		else if(((unsigned char)*it>>5) == 0x6) // 2 bytes encoding
		{
			unicodeCharacter = (unsigned char)*it & 0x1F;
			++it;
			if(it == inString.end() || ((unsigned char)*it>>6 != 0x2))
			{
				status = eFailure;
				break;
			}

			unicodeCharacter = (unicodeCharacter << 6) | ((unsigned char)*it & 0x3F);
		}
		else if(((unsigned char)*it>>4) == 0xE) // 3 bytes encoding
		{
			unicodeCharacter = (unsigned char)*it & 0xF;
			for(int i =0 ; i < 2 && eSuccess == status; ++i)
			{
				++it;
				if(it == inString.end() || ((unsigned char)*it>>6 != 0x2))
				{
					status = eFailure;
					break;
				}
				unicodeCharacter = (unicodeCharacter << 6) | ((unsigned char)*it & 0x3F);
			}
			if(status != eSuccess)
				break;
		}
		else if(((unsigned char)*it>>3) == 0x1E) // 4 bytes encoding
		{
			unicodeCharacter = (unsigned char)*it & 0x7;
			for(int i =0 ; i < 3 && eSuccess == status; ++i)
			{
				++it;
				if(it == inString.end() || ((unsigned char)*it>>6 != 0x2))
				{
					status = eFailure;
					break;
				}
				unicodeCharacter = (unicodeCharacter << 6) | ((unsigned char)*it & 0x3F);
			}
			if(status != eSuccess)
				break;
		}
		else
		{
			status = eFailure;
			break;
		}

		mUnicodeCharacters.push_back(unicodeCharacter);
	}

	return status;
}

EStatusCodeAndString UnicodeString::ToUTF8() const
{
	ULongList::const_iterator it = mUnicodeCharacters.begin();
	EStatusCode status = eSuccess;
	stringstream result;

	for(; it != mUnicodeCharacters.end() && eSuccess == status; ++it)
	{
		// Encode Unicode to UTF8
		if(*it <= 0x7F)
		{
			result.put((unsigned char)*it);
		}
		else if(0x7F < *it && *it <= 0x7FF)
		{
			result.put((unsigned char)((0xC0 | (*it>>6))));
			result.put((unsigned char)(0x80 | (*it & 0x3F)));
		}
		else if(0x7FF < *it && *it <= 0xFFFF)
		{
			result.put((unsigned char)(0xE0 | (*it>>12)));
			result.put((unsigned char)(0x80 | ((*it>>6) & 0x3F)));
			result.put((unsigned char)(0x80 | (*it & 0x3F)));
		}
		else if(0xFFFF < *it && *it <= 0x10FFFF)
		{
			result.put((unsigned char)(0xF0 | (*it>>18)));
			result.put((unsigned char)(0x80 | ((*it>>12) & 0x3F)));
			result.put((unsigned char)(0x80 | ((*it>>6) & 0x3F)));
			result.put((unsigned char)(0x80 | (*it & 0x3F)));
		}
		else
		{
			status = eFailure;
		}
	}
	
	return EStatusCodeAndString(status,result.str());
}

EStatusCode UnicodeString::FromUTF16(const string& inString)
{
	return FromUTF16((const unsigned char*)inString.c_str(),(unsigned long)inString.length());
}

EStatusCode UnicodeString::FromUTF16(const unsigned char* inString, unsigned long inLength)
{
	// Read BOM
	if(inLength < 2)
		return eFailure;

	if(inString[0] == 0xFE && inString[1] == 0xFF)
		return FromUTF16BE(inString+2,inLength-2);
	else if(inString[0] == 0xFF && inString[1] == 0xFE)
		return FromUTF16LE(inString+2,inLength-2);
	else
		return eFailure; // no bom
}

EStatusCode UnicodeString::FromUTF16BE(const string& inString)
{
	return FromUTF16BE((const unsigned char*)inString.c_str(),(unsigned long)inString.length());
}

EStatusCode UnicodeString::FromUTF16BE(const unsigned char* inString, unsigned long inLength)
{
	mUnicodeCharacters.clear();
	EStatusCode status = eSuccess;

	if(inLength % 2 != 0)
	{
		return eFailure;
	}

	for(unsigned long i = 0; i < inLength-1 && eSuccess == status; i+=2)
	{
		unsigned short buffer = (((unsigned short)inString[i])<<8) + inString[i+1];

		if(0xD800 <= buffer && buffer <= 0xDBFF) 
		{
			// Aha! high surrogate! this means that this character requires 2 w_chars
			unsigned short highSurrogate = buffer;
			i+=2;
			if(i>=inLength-1)
			{
				status = eFailure;
				break;
			}

			unsigned short buffer = (((unsigned short)inString[i])<<8) + inString[i+1];
			if(0xDC00 > buffer|| buffer > 0xDFFF)
			{
				status = eFailure;
				break;
			}

			unsigned short lowSurrogate = buffer;

			mUnicodeCharacters.push_back(0x10000 + ((highSurrogate - 0xD800) << 5) + (lowSurrogate - 0xDC00));
		}
		else
			mUnicodeCharacters.push_back(buffer);		
	}

	return status;
}

EStatusCode UnicodeString::FromUTF16LE(const string& inString)
{
	return FromUTF16LE((const unsigned char*)inString.c_str(),(unsigned long)inString.length());
}


EStatusCode UnicodeString::FromUTF16LE(const unsigned char* inString, unsigned long inLength)
{
	mUnicodeCharacters.clear();
	EStatusCode status = eSuccess;

	if(inLength % 2 != 0)
	{
		return eFailure;
	}

	for(unsigned long i = 0; i < inLength-1 && eSuccess == status; i+=2)
	{
		unsigned short buffer = (((unsigned short)inString[i+1])<<8) + inString[i];

		if(0xD800 <= buffer && buffer <= 0xDBFF) 
		{
			// Aha! high surrogate! this means that this character requires 2 w_chars
			unsigned short highSurrogate = buffer;
			i+=2;
			if(i>=inLength-1)
			{
				status = eFailure;
				break;
			}

			unsigned short buffer = (((unsigned short)inString[i+1])<<8) + inString[i];
			if(0xDC00 > buffer|| buffer > 0xDFFF)
			{
				status = eFailure;
				break;
			}

			unsigned short lowSurrogate = buffer;

			mUnicodeCharacters.push_back(0x10000 + ((highSurrogate - 0xD800) << 5) + (lowSurrogate - 0xDC00));
		}
		else
			mUnicodeCharacters.push_back(buffer);		
	}

	return status;
}

EStatusCode UnicodeString::FromUTF16UShort(const unsigned short* inShorts, unsigned long inLength)
{
	mUnicodeCharacters.clear();
	EStatusCode status = eSuccess;

	for(unsigned long i = 0; i < inLength && eSuccess == status; ++i)
	{
		if(0xD800 <= inShorts[i] && inShorts[i] <= 0xDBFF) 
		{
			// Aha! high surrogate! this means that this character requires 2 w_chars
			++i;
			if(i>=inLength)
			{
				status = eFailure;
				break;
			}

			if(0xDC00 > inShorts[i] || inShorts[i] > 0xDFFF)
			{
				status = eFailure;
				break;
			}

			mUnicodeCharacters.push_back(0x10000 + ((inShorts[i-1] - 0xD800) << 5) + (inShorts[i] - 0xDC00));
		}
		else
			mUnicodeCharacters.push_back(inShorts[i]);		
	}

	return status;		
}


EStatusCodeAndString UnicodeString::ToUTF16BE(bool inPrependWithBom) const
{
	ULongList::const_iterator it = mUnicodeCharacters.begin();
	EStatusCode status = eSuccess;
	stringstream result;

	if(inPrependWithBom)
	{
		result.put((unsigned char)0xFE);
		result.put((unsigned char)0xFF);
	}

	for(; it != mUnicodeCharacters.end() && eSuccess == status; ++it)
	{
		if(*it < 0xD7FF || (0xE000 < *it && *it < 0xFFFF))
		{
			result.put((unsigned char)(*it>>8));
			result.put((unsigned char)(*it & 0xFF));
		}
		else if(0xFFFF < *it && *it <= 0x10FFFF)
		{
			unsigned short highSurrogate = (unsigned short)(((*it - 0x10000) >> 10) + 0xD800);
			unsigned short lowSurrogate = (unsigned short)(((*it - 0x10000) & 0x3FF) + 0xDC00);
			
			result.put((unsigned char)(highSurrogate>>8));
			result.put((unsigned char)(highSurrogate & 0xFF));
			result.put((unsigned char)(lowSurrogate>>8));
			result.put((unsigned char)(lowSurrogate & 0xFF));
		}
		else
		{
			status = eFailure;
			break;
		}	
	}
	
	return EStatusCodeAndString(status,result.str());
}

EStatusCodeAndString UnicodeString::ToUTF16LE(bool inPrependWithBom) const
{
	ULongList::const_iterator it = mUnicodeCharacters.begin();
	EStatusCode status = eSuccess;
	stringstream result;

	if(inPrependWithBom)
	{
		result.put((unsigned char)0xFF);
		result.put((unsigned char)0xFE);
	}

	for(; it != mUnicodeCharacters.end() && eSuccess == status; ++it)
	{
		if(*it < 0xD7FF || (0xE000 < *it && *it < 0xFFFF))
		{
			result.put((unsigned char)(*it & 0xFF));
			result.put((unsigned char)(*it>>8));
		}
		else if(0xFFFF < *it && *it <= 0x10FFFF)
		{
			unsigned short highSurrogate = (unsigned short)(((*it - 0x10000) >> 10) + 0xD800);
			unsigned short lowSurrogate = (unsigned short)(((*it - 0x10000) & 0x3FF) + 0xDC00);
			
			result.put((unsigned char)(highSurrogate & 0xFF));
			result.put((unsigned char)(highSurrogate>>8));
			result.put((unsigned char)(lowSurrogate & 0xFF));
			result.put((unsigned char)(lowSurrogate>>8));
		}
		else
		{
			status = eFailure;
			break;
		}	
	}
	
	return EStatusCodeAndString(status,result.str());
}

EStatusCodeAndUShortList UnicodeString::ToUTF16UShort() const
{
	ULongList::const_iterator it = mUnicodeCharacters.begin();
	EStatusCode status = eSuccess;
	UShortList result;

	for(; it != mUnicodeCharacters.end() && eSuccess == status; ++it)
	{
		if(*it < 0xD7FF || (0xE000 < *it && *it < 0xFFFF))
		{
			result.push_back((unsigned short)*it);
		}
		else if(0xFFFF < *it && *it <= 0x10FFFF)
		{
			unsigned short highSurrogate = (unsigned short)(((*it - 0x10000) >> 10) + 0xD800);
			unsigned short lowSurrogate = (unsigned short)(((*it - 0x10000) & 0x3FF) + 0xDC00);

			result.push_back(highSurrogate);
			result.push_back(lowSurrogate);
		}
		else
		{
			status = eFailure;
			break;
		}	
	}
	
	return EStatusCodeAndUShortList(status,result);
}


