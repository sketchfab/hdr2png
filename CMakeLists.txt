cmake_minimum_required(VERSION 2.8)
project(hdr2png)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules)

find_package(OpenImageIO REQUIRED)
include_directories(${OIIO_INCLUDE_DIR})

add_executable(hdr2png hdr2png.cpp)
target_link_libraries(hdr2png ${OIIO_LIBRARY})
