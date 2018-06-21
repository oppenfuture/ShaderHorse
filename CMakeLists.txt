project(ShaderHorse)

if(WIN32)
  add_definitions(-DGLI_BUILD_WINDOWS)
endif(WIN32)

if(UNIX)
  add_definitions(-DGLI_BUILD_LINUX)
endif(UNIX)

include_directories(../../Common)
include_directories(.)

file(GLOB SRC_FILES *.h *.cpp ../../Common/InputUtils.cpp)
add_library(ShaderHorse SHARED ${SRC_FILES})
