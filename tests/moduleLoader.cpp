#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>

#include "util.h"


TEST_CASE("Eval file", "[moduleLoader]") {
    using Machine =
        jac::ModuleLoaderFeature<
        jac::FilesystemFeature<
        TestReportFeature<
        jac::MachineBase
    >>>;
    Machine machine;

    using sgn = typename std::tuple<std::string, std::string, std::vector<std::string>>;
    auto [comment, path, expected] = GENERATE(
        sgn {
            "Eval file",
            "test_files/moduleLoader/test.js",
            { "first", "second" }
        }
    );

    DYNAMIC_SECTION(comment) {
        machine.setCodeDir(machine.path.dirname(path));
        machine.initialize();

        machine.evalFile(machine.path.basename(path));
        REQUIRE(machine.getReports() == expected);
    }

    SECTION("File not found") {
        machine.initialize();
        evalFileThrowsOut(machine, "test_files/moduleLoader/notFound.js");
    }
}


TEST_CASE("Import file", "[moduleLoader]") {
    using Machine =
        jac::ModuleLoaderFeature<
        jac::FilesystemFeature<
        TestReportFeature<
        jac::MachineBase
    >>>;
    Machine machine;

    using sgn = typename std::tuple<std::string, std::string, std::vector<std::string>>;
    auto [comment, path, expected] = GENERATE(
        sgn {
            "Neighbor",
            "test_files/moduleLoader/importNeighbor/main.js",
            { "callNeighbor" }
        },
        sgn {
            "Subdirectory",
            "test_files/moduleLoader/importSubdir/main.js",
            { "callSubdir" }
        },
        sgn {
            "Subdirectory and up",
            "test_files/moduleLoader/importSubdirUp/main.js",
            { "callUp", "callSubdir" }
        }
    );

    DYNAMIC_SECTION(comment) {
        machine.setCodeDir(machine.path.dirname(path));
        machine.initialize();

        evalFile(machine, machine.path.basename(path));

        REQUIRE(machine.getReports() == expected);
    }

    SECTION("Import not found") {
        machine.initialize();
        evalFileThrowsOut(machine, "test_files/moduleLoader/importNotFound/main.js");
    }

    SECTION("Neighbor exception") {
        machine.initialize();
        evalFileThrowsJS(machine, "test_files/moduleLoader/importException/main.js");
    }

    SECTION("Main exception") {
        machine.initialize();
        evalFileThrowsJS(machine, "test_files/moduleLoader/exception.js");
    }
}
