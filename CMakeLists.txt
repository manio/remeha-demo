CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT("remeha")

set (CMAKE_CXX_FLAGS "-O2 --std=c++0x ${CMAKE_CXX_FLAGS}")

file(GLOB SOURCES *.cpp)
ADD_EXECUTABLE(remeha ${SOURCES})
