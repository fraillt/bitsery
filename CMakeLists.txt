cmake_minimum_required(VERSION 3.2)

project(bitsery)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(COMPILE_FLAGS "")
else()
    set(COMPILE_FLAGS "-Wall -Wextra -Wno-missing-braces -Wpedantic -Weffc++")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(examples)

enable_testing()
add_subdirectory(tests)
