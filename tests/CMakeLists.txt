cmake_minimum_required(VERSION 3.0)

Include(FetchContent)
include(CTest)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.0
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

add_test_executable(scanner)
add_test_executable(parser)

add_executable(compileCfg compile.cpp)
target_compile_definitions(compileCfg PRIVATE COMPILE_CFG_ONLY)
target_link_libraries(compileCfg PUBLIC jac-machine Catch2::Catch2WithMain)
add_test(NAME compileCfg COMMAND ${EXECUTABLE_OUTPUT_PATH}/compileCfg)

add_executable(compileNative compile.cpp)
target_link_libraries(compileNative PUBLIC jac-machine Catch2::Catch2WithMain)
add_test(NAME compileNative COMMAND ${EXECUTABLE_OUTPUT_PATH}/compileNative)

add_test_executable(cfgInterp)
add_test_executable(regression)

add_executable(bench bench.cpp)
target_link_libraries(bench PUBLIC jac-machine Catch2::Catch2WithMain)

file(COPY test_files DESTINATION "./")
