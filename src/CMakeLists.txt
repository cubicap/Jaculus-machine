cmake_minimum_required(VERSION 3.0)

set(QUICKJS_CPP
    "jac/machine/machine.cpp"
    "jac/machine/context.cpp"
)


if(NOT ESP_PLATFORM)
    Include(FetchContent)

    FetchContent_Declare(
        QuickJS
        GIT_REPOSITORY https://github.com/cubicap/quickjs.git
        GIT_TAG        669afd182b5707eb8b70104bc942309506033697
    )

    FetchContent_MakeAvailable(QuickJS)

    add_library(jac-machine STATIC ${QUICKJS_CPP})
    target_include_directories(jac-machine PUBLIC .)
    set_target_properties(jac-machine PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(jac-machine PUBLIC quickjs)

else()
    idf_component_register(
        SRCS ${QUICKJS_CPP}
        INCLUDE_DIRS .
        REQUIRES quickjs
    )

endif()
