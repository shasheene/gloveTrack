cmake_minimum_required (VERSION 2.8)
project (gloveTrack)

# ExternalProject module allows triggering of build AND install targets (required for eg, some third-party libs)
include(ExternalProject)

# We assume the libhand repository is a sibling directory to gloveTrack
SET(HAND_LIB_ROOT_DIR "${CMAKE_SOURCE_DIR}/third-party/libhand")

SET(HAND_LIB_DIST_DIR "${HAND_LIB_ROOT_DIR}/dist")
SET(HAND_LIB_EXAMPLES_DIR "${HAND_LIB_ROOT_DIR}/examples")
SET(HAND_LIB_API_DIR "${HAND_LIB_ROOT_DIR}/hand_cpp/source/")

ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/src")
