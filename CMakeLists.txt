cmake_minimum_required(VERSION 3.0)

project(Jaculus-machine)


if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_EXTENSIONS ON)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_EXTENSIONS ON)

    if (CMAKE_COMPILER_IS_GNUCXX)
        add_compile_options(-Wall -Wold-style-cast -Wshadow)
    endif()

    add_compile_options(-pthread -fdiagnostics-color)

endif()


add_subdirectory(src)


if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)

endif()
