cmake_minimum_required(VERSION 3.0)

project(Jaculus-machine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAGE_C_STANDARD 17)
set(CMAGE_C_EXTENSIONS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wold-style-cast -Wshadow -fdiagnostics-color")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_subdirectory(src)

enable_testing()

add_subdirectory(tests)
