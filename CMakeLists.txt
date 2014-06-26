cmake_minimum_required (VERSION 2.8)
project (Glovetrack)

find_package( OpenCV REQUIRED )

set (Glovetrack_VERSION_MAJOR 0)
set (Glovetrack_VERSION_MINOR 1)

configure_file (
  "${PROJECT_SOURCE_DIR}/GlovetrackConfig.h.in"
  "${PROJECT_BINARY_DIR}/GlovetrackConfig.h"
)

include_directories("$(PROJECT_BINARY_DIR)")

#find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR})



add_executable(Glovetrack gloveTrack.cpp)
target_link_libraries( Glovetrack ${OpenCV_LIBS} )
