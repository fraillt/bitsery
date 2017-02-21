#MIT License
#
#Copyright (c) 2017 Mindaugas Vinkelis
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.


cmake_minimum_required(VERSION 3.2)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB EXAMPLE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

FOREACH(EXAMPLE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE})

    set_property(TARGET ${EXAMPLE_NAME} PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${EXAMPLE_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

ENDFOREACH()

