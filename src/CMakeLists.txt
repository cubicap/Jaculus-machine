cmake_minimum_required(VERSION 3.0)

set(JAC_MACHINE_SRC
    "jac/machine/machine.cpp"
    "jac/machine/context.cpp"
    "jac/machine/compiler/cfg.cpp"
)


if(ESP_PLATFORM)
    idf_component_register(
        SRCS ${JAC_MACHINE_SRC}
        INCLUDE_DIRS .
    )

    set(QUICKJS_ENABLE_BIGNUM_EXT ${CONFIG_QUICKJS_ENABLE_BIGNUM_EXT})

else()
    option(QUICKJS_ENABLE_BIGNUM_EXT "Enable QuickJS Bignum extensions" OFF)
    option(QUICKJS_DUMP_LEAKS "Dump memory leaks" OFF)

endif()


Include(FetchContent)
FetchContent_Declare(
    QuickJS
    URL               https://github.com/bellard/quickjs/archive/b5e62895c619d4ffc75c9d822c8d85f1ece77e5b.tar.gz
    PATCH_COMMAND     ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/quickjs.CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS        -DESP_PLATFORM=${ESP_PLATFORM} -DQUICKJS_ENABLE_BIGNUM_EXT=${QUICKJS_ENABLE_BIGNUM_EXT} -DQUICKJS_DUMP_LEAKS=${QUICKJS_DUMP_LEAKS}
)
FetchContent_MakeAvailable(QuickJS)

FetchContent_Declare(
    Cwerg
    URL               https://github.com/robertmuth/Cwerg/archive/b36921f76d61c93a2836073e6110dcd118627a12.tar.gz
)
FetchContent_MakeAvailable(Cwerg)
set(CWERG_DIR ${cwerg_SOURCE_DIR})


if(ESP_PLATFORM)
    target_link_libraries(${COMPONENT_LIB} PUBLIC quickjs)
else()

    add_library(jac-machine STATIC ${JAC_MACHINE_SRC})
    target_include_directories(jac-machine PUBLIC . ${CWERG_DIR})
    set_target_properties(jac-machine PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(jac-machine PUBLIC quickjs UTIL_LIB BASE_LIB CODEGENX64_LIB CPUX64_LIB ELF_LIB)
    target_compile_options(jac-machine PRIVATE -Wall -Wold-style-cast -Wshadow)


endif()
