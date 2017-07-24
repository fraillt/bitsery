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
project(gtest_builder C CXX)
include(ExternalProject)

set(ForceSharedCrt ON)
set(DisablePThreads OFF)

if(MINGW)
    set(DisablePThreads ON)
endif()

if (${UseGMock})
    message("use gmock")
    set(BuildArgs -DBUILD_GTEST=OFF -DBUILD_GMOCK=ON)
else ()
    message("use gtest only")
    set(BuildArgs -DBUILD_GTEST=ON -DBUILD_GMOCK=OFF)
endif()

if (WIN32 AND NOT MINGW)
    set(BuildArgs ${BuildArgs}
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
            -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs)
endif()

ExternalProject_Add(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        CMAKE_ARGS ${BuildArgs}
        -Dgtest_force_shared_crt=${ForceSharedCrt}
        -Dgtest_disable_pthreads=${DisablePThreads}
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
        # disable update command
        UPDATE_COMMAND ""
        # disable install step
        INSTALL_COMMAND ""
        )

#export variables
ExternalProject_Get_Property(googletest source_dir)
ExternalProject_Get_Property(googletest binary_dir)
if (${UseGMock})
    # need to include both googletest and googlemock
    set(GTestIncludeDirs ${source_dir}/googlemock/include ${source_dir}/googletest/include PARENT_SCOPE)
    set(GTestLibsDir ${binary_dir}/googlemock PARENT_SCOPE)
    set(GTestLibName gmock PARENT_SCOPE)
    set(GTestMainLibName gmock_main PARENT_SCOPE)
    set(GTestLinkLibNames gmock_main PARENT_SCOPE)
else()
    set(GTestIncludeDirs ${source_dir}/googletest/include PARENT_SCOPE)
    set(GTestLibsDir ${binary_dir}/googletest PARENT_SCOPE)
    set(GTestLibName gtest PARENT_SCOPE)
    set(GTestMainLibName gtest_main PARENT_SCOPE)
    # need to include both libs gtest and gtest_main
    set(GTestLinkLibNames gtest gtest_main PARENT_SCOPE)
endif()


