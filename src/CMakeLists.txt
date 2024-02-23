cmake_minimum_required(VERSION 3.0)

set(JAC_MACHINE_SRC
    "jac/machine/machine.cpp"
    "jac/machine/context.cpp"
)


if(ESP_PLATFORM)
    idf_component_register(
        SRCS ${JAC_MACHINE_SRC}
        INCLUDE_DIRS .
    )

    set(QUICKJS_ENABLE_BIGNUM_EXT ${CONFIG_QUICKJS_ENABLE_BIGNUM_EXT})

else()
    option(QUICKJS_ENABLE_BIGNUM_EXT "Enable QuickJS Bignum extensions" OFF)

endif()


Include(FetchContent)
FetchContent_Declare(
    QuickJS
    URL               https://github.com/bellard/quickjs/archive/27928ce49170a17170bb01649fc27624c0494b05.tar.gz
    PATCH_COMMAND     ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/quickjs.CMakeLists.txt <SOURCE_DIR>/CMakeLists.txt
    CMAKE_ARGS        -DESP_PLATFORM=${ESP_PLATFORM} -DQUICKJS_ENABLE_BIGNUM_EXT=${QUICKJS_ENABLE_BIGNUM_EXT}
)
FetchContent_MakeAvailable(QuickJS)


if(ESP_PLATFORM)
    target_link_libraries(${COMPONENT_LIB} PUBLIC quickjs)
else()

    add_library(jac-machine STATIC ${JAC_MACHINE_SRC})
    target_include_directories(jac-machine PUBLIC .)
    set_target_properties(jac-machine PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(jac-machine PUBLIC quickjs)

endif()
