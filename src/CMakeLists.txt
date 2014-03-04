project(BrightnessCalculator)
cmake_minimum_required (VERSION 2.6)

ADD_SUBDIRECTORY(deps/LibJpeg)
ADD_SUBDIRECTORY(deps/Hummus)

add_executable(BrightnessCalculator 

#sources
BrightnessCalculator.cpp
)

source_group(Main FILES
BrightnessCalculator.cpp
)

include_directories (${LibJpeg_SOURCE_DIR})
include_directories (${Hummus_SOURCE_DIR})
target_link_libraries (BrightnessCalculator LibJpeg)
target_link_libraries (BrightnessCalculator Hummus)

if(APPLE)
	set(CMAKE_EXE_LINKER_FLAGS "-framework CoreFoundation")
endif(APPLE)
