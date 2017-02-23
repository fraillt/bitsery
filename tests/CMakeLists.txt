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
project(${TEST_PROJECT_NAME} C CXX)

find_package(Threads REQUIRED)

#add googletest external project
#USE_GMOCK enable gmock
#exports variables GTEST_INCLUDE_DIRS, GTEST_LIBS_DIR, GTEST_LIBNAME, GTEST_MAIN_LIBNAME
set(EXT_PROJECTS_DIR ${CMAKE_SOURCE_DIR}/ext)
set(USE_GMOCK ON)
add_subdirectory(${EXT_PROJECTS_DIR}/gtest ${CMAKE_BINARY_DIR}/gtest)

# set common include folder for module
include_directories(SYSTEM ${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB TEST_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

FOREACH(TEST_PROJECT_FILE ${TEST_SRC_FILES})
    get_filename_component(SEPARATE_TEST_NAME ${TEST_PROJECT_FILE} NAME_WE)
    set(SEPARATE_TEST_NAME TEST_${SEPARATE_TEST_NAME})
    add_executable(${SEPARATE_TEST_NAME} ${TEST_PROJECT_FILE})
    add_dependencies(${SEPARATE_TEST_NAME} googletest)

    set_property(TARGET ${SEPARATE_TEST_NAME} PROPERTY CXX_STANDARD 14)
    set_property(TARGET ${SEPARATE_TEST_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)


    if(NOT WIN32 OR MINGW)
        FOREACH(LIBNAME ${GTEST_LINK_LIBNAMES})
            target_link_libraries(${SEPARATE_TEST_NAME} ${GTEST_LIBS_DIR}/lib${LIBNAME}.a )
        ENDFOREACH()
    else()
        FOREACH(LIBNAME ${GTEST_LINK_LIBNAMES})
            target_link_libraries(${SEPARATE_TEST_NAME}
                    debug ${GTEST_LIBS_DIR}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}${LIBNAME}${CMAKE_FIND_LIBRARY_SUFFIXES}
                    optimized ${GTEST_LIBS_DIR}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}${LIBNAME}${CMAKE_FIND_LIBRARY_SUFFIXES})
        ENDFOREACH()
    endif()

    target_link_libraries(${SEPARATE_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

    add_test(NAME ${SEPARATE_TEST_NAME} COMMAND $<TARGET_FILE:${SEPARATE_TEST_NAME}>)

ENDFOREACH()

#all in one tests for code coverage
add_executable(${TEST_PROJECT_NAME} ${TEST_SRC_FILES})
add_dependencies(${TEST_PROJECT_NAME} googletest)

set_property(TARGET ${TEST_PROJECT_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${TEST_PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)


if(NOT WIN32 OR MINGW)
    FOREACH(LIBNAME ${GTEST_LINK_LIBNAMES})
        target_link_libraries(${TEST_PROJECT_NAME} ${GTEST_LIBS_DIR}/lib${LIBNAME}.a )
    ENDFOREACH()
else()
    FOREACH(LIBNAME ${GTEST_LINK_LIBNAMES})
        target_link_libraries(${TEST_PROJECT_NAME}
                debug ${GTEST_LIBS_DIR}/DebugLibs/${CMAKE_FIND_LIBRARY_PREFIXES}${LIBNAME}${CMAKE_FIND_LIBRARY_SUFFIXES}
                optimized ${GTEST_LIBS_DIR}/ReleaseLibs/${CMAKE_FIND_LIBRARY_PREFIXES}${LIBNAME}${CMAKE_FIND_LIBRARY_SUFFIXES})
    ENDFOREACH()
endif()

target_link_libraries(${TEST_PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})








