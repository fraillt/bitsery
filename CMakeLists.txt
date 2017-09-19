cmake_minimum_required(VERSION 3.2)

project(bitsery)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
