cmake_minimum_required(VERSION 3.2)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB EXAMPLE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

FOREACH(EXAMPLE ${EXAMPLE_FILES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE} NAME_WE)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE})
ENDFOREACH()

