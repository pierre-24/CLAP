cmake_minimum_required(VERSION 3.2)
project(CLAP)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99")

set(SOURCE_FILES example.c CLAP.c)
add_executable(CLAP ${SOURCE_FILES})