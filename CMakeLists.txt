cmake_minimum_required(VERSION 3.2)

project(serialization)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#add tests
enable_testing()
add_subdirectory(tests)
