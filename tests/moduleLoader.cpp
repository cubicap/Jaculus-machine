#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>

#include "util.h"


TEST_CASE("Filesystem", "[filesystem]") {
    using Machine =
        FilesystemFeature<
        TestReportFeature<
        jac::MachineBase
    >>;
    Machine machine;


    SECTION("code - existing") {
        std::string file("test_files/moduleLoader/test.js");
        std::string expected("report(\"first\")\nreport(\"second\")\n");
        machine.setCodeDir(machine.getParentDir(file));
        machine.initialize();

        REQUIRE(machine.loadCode(machine.getFilename(file)) == expected);
    }

    SECTION("code - not existing") {
        std::string file("not_existing.js");
        machine.setCodeDir(".");
        machine.initialize();

        REQUIRE_THROWS_AS(machine.loadCode(file), std::runtime_error);
    }

    SECTION("data - existing") {
        std::string file("test_files/moduleLoader/test.js");
        std::string expected("report(\"first\")\nreport(\"second\")\n");
        machine.setDataDir(machine.getParentDir(file));
        machine.initialize();

        REQUIRE(machine.loadData(machine.getFilename(file)) == expected);
    }

    SECTION("data - not existing") {
        std::string file("not_existing.js");
        machine.setDataDir(".");
        machine.initialize();

        REQUIRE_THROWS_AS(machine.loadData(file), std::runtime_error);
    }
}


TEST_CASE("Eval file", "[filesystem, moduleLoader]") {
    using Machine =
        ModuleLoaderFeature<
        FilesystemFeature<
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
        machine.setCodeDir(machine.getParentDir(path));
        machine.initialize();

        machine.evalFile(machine.getFilename(path));
        REQUIRE(machine.getReports() == expected);
    }
}


TEST_CASE("Import file", "[filesystem, moduleLoader]") {
    using Machine =
        ModuleLoaderFeature<
        FilesystemFeature<
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
        machine.setCodeDir(machine.getParentDir(path));
        machine.initialize();

        evalFile(machine, machine.getFilename(path));

        REQUIRE(machine.getReports() == expected);
    }
}
