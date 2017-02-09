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

add_test(NAME all_tests COMMAND $<TARGET_FILE:${TEST_PROJECT_NAME}>)










