cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME bitsery)
set(TEST_PROJECT_NAME ${PROJECT_NAME}_tests)
project(${PROJECT_NAME})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O0 -fprofile-arcs -ftest-coverage")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/ext)
if(CMAKE_COMPILER_IS_GNUCXX)
    #include(CodeCoverage)
    #setup_target_for_coverage(${PROJECT_NAME}_coverage ${TEST_PROJECT_NAME} coverage)
endif()

add_subdirectory(examples)

#add tests
enable_testing()
add_subdirectory(tests)
