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
set(TestProjectName bitsery_tests)
project(${TestProjectName} C CXX)


find_package(Threads REQUIRED)

#add googletest external project
#USE_GMOCK enable gmock
#exports variables GTEST_INCLUDE_DIRS, GTEST_LIBS_DIR, GTEST_LIBNAME, GTEST_MAIN_LIBNAME
set(ExtCMakeFilesDir ${CMAKE_SOURCE_DIR}/ext)
set(UseGMock ON)
add_subdirectory(${ExtCMakeFilesDir}/gtest ${CMAKE_BINARY_DIR}/gtest)

#this helps idea to know which files are actually used
file(GLOB_RECURSE IncludeHeaders ${CMAKE_SOURCE_DIR}/include/bitsery/*.h)
# set common include folder for module
include_directories(SYSTEM ${GTestIncludeDirs})
include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB TestSourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(WARNING "extension tests for optional is disable for VS, because VS currenty doesn't have <optional>")
    list(REMOVE_ITEM TestSourceFiles ${CMAKE_CURRENT_SOURCE_DIR}/serialization_ext_std_optional.cpp)
endif()

include(${ExtCMakeFilesDir}/LinkTestLib.cmake)

FOREACH(TestFile ${TestSourceFiles})
    get_filename_component(TestName ${TestFile} NAME_WE)
    set(TestName TEST_${TestName})
    add_executable(${TestName} ${TestFile} ${IncludeHeaders} serialization_test_utils.h)
    LinkTestLib(${TestName})

    add_test(NAME ${TestName} COMMAND $<TARGET_FILE:${TestName}>)

ENDFOREACH()

#all in one tests for code coverage
add_executable(${TestProjectName} ${TestSourceFiles})

if(CMAKE_COMPILER_IS_GNUCXX)
    include(${ExtCMakeFilesDir}/CodeCoverage.cmake)
    target_compile_options(${TestProjectName} PUBLIC -O0 -fprofile-arcs -ftest-coverage)
    target_link_libraries(${TestProjectName} -O0 -fprofile-arcs -ftest-coverage)
    setup_target_for_coverage(tests_coverage ${TestProjectName} coverage)
endif()

LinkTestLib(${TestProjectName})


