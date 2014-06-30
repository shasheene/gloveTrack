cmake_minimum_required (VERSION 2.8)
project (gloveTrack)

find_package( OpenCV REQUIRED )

set (Glovetrack_VERSION_MAJOR 0)
set (Glovetrack_VERSION_MINOR 1)

configure_file (
  "${PROJECT_SOURCE_DIR}/gloveTrackConfig.h.in"
  "${PROJECT_BINARY_DIR}/gloveTrackConfig.h"
)

include_directories("$(PROJECT_BINARY_DIR)")

#find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR})


add_executable(gloveTrack gloveTrack.cpp isolateGlove.cpp lookupDatabase.cpp)
target_link_libraries( gloveTrack ${OpenCV_LIBS})
