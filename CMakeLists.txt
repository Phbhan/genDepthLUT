cmake_minimum_required(VERSION 3.16)

option(LINUX_BUILD "Build FLAG for linux" OFF)
project(GenTopLut LANGUAGES CXX C)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_THREAD_LIBS_INIT "-lpthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -w -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

message("Compiler Version ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

message(STATUS "Building executable for linux")

# find the required packages

find_package(Eigen3 3.3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})
find_package(OpenCV REQUIRED)

include(FindPkgConfig)

find_library(GLES2-lib GLESv2)
find_library(EGL_LIBRARY EGL)

find_package(PNG REQUIRED)
include_directories (${LIBPNG_INCLUDE_DIRS})
link_directories (${LIBPNG_LIBRARY_DIRS})
link_libraries (${LIBPNG_LIBRARIES})

include_directories(${catkin_INCLUDE_DIRS})
include_directories(
  ${CMAKE_SOURCE_DIR}/includes/
  ${CMAKE_SOURCE_DIR}/includes/json
  ${CMAKE_SOURCE_DIR}/src/include
)

file(GLOB sources src/*.cpp)

list(REMOVE_ITEM sources ${CMAKE_SOURCE_DIR}/src/Test_JNI.cpp)

add_executable(genTopLut ${sources})
target_link_libraries(genTopLut
  ${catkin_LIBRARIES} ${OpenCV_LIBS} ${LIBS} stdc++fs EGL GLESv2 ${PNG_LIBRARY}
)