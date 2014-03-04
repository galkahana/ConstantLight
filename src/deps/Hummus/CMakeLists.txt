cmake_minimum_required (VERSION 2.6) 
project(Hummus)
add_library (Hummus 
#sources
InputBufferedStream.cpp
InputFile.cpp
InputFileStream.cpp
UnicodeString.cpp

#headers
EStatusCode.h
IByteReader.h
IByteReaderWithPosition.h
InputBufferedStream.h
InputFile.h
InputFileStream.h
IOBasicTypes.h
SafeBufferMacrosDefs.h
UnicodeString.h
)
