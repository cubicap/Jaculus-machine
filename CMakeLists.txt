cmake_minimum_required(VERSION 3.0)

project(Jaculus-machine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-Wall -Wold-style-cast -Wshadow -fdiagnostics-color")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_subdirectory(src)

enable_testing()

add_subdirectory(tests)
