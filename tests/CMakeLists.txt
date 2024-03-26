cmake_minimum_required(VERSION 3.0)

Include(FetchContent)
include(CTest)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.2.0
)

FetchContent_MakeAvailable(Catch2)


add_executable(main main.cpp)
target_link_libraries(main PUBLIC jac-machine)


function(add_test_executable name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PUBLIC jac-machine Catch2::Catch2WithMain)
    add_test(NAME ${name} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${name})
endfunction()

add_test_executable(filesystem)
add_test_executable(eval)
add_test_executable(moduleLoader)
add_test_executable(machineBase)
add_test_executable(values)
add_test_executable(functionFactory)
add_test_executable(class)
add_test_executable(plugins)
add_test_executable(regression)

add_test_executable(scanner)
add_test_executable(parser)

add_test_executable(asScanner)

file(COPY test_files DESTINATION "./")
