cmake_minimum_required (VERSION 2.8)
project (gloveTrack)

find_package( OpenCV REQUIRED )

set (Glovetrack_VERSION_MAJOR 0)
set (Glovetrack_VERSION_MINOR 1)

configure_file (
  "${PROJECT_SOURCE_DIR}/gloveTrackConfig.hpp.in"
  "${PROJECT_BINARY_DIR}/gloveTrackConfig.hpp"
)

include_directories("$(PROJECT_BINARY_DIR)")

#find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR})

add_executable(gloveTrack gloveTrack.cpp isolateGlove.cpp lookupDatabase.cpp commandLineArguments.cpp manifest.cpp)
target_link_libraries( gloveTrack ${OpenCV_LIBS})

# Ubuntu 12.04 repositories only have CMake 2.8.12.2. Latter versions have explicit
# support to force C++11 compilation. Not ideal, but gets the job done.
list( APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS} -g -ftest-coverage -fprofile-arcs -lpthread")

# Copy database directory (containing training and testing image sets) to build/ directory
add_custom_command(TARGET gloveTrack POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/db/ $<TARGET_FILE_DIR:gloveTrack>/db/)
