#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <filesystem>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/filesystemFeature.h>

#include "util.h"


std::string readFile(auto f) {
    std::string buffer;
    std::string read = f.read();
    while (read.size() > 0) {
        buffer += read;
        read = f.read();
    }
    return buffer;
}


TEST_CASE("Filesystem", "[filesystem]") {
    using Machine =
        FilesystemFeature<
        jac::MachineBase
    >;
    Machine machine;


    SECTION("code - existing") {
        std::string file("test_files/moduleLoader/test.js");
        std::string expected("report(\"first\")\nreport(\"second\")\n");
        machine.setCodeDir(machine.path.dirname(file));
        machine.initialize();

        REQUIRE(machine.fs.loadCode(machine.path.basename(file)) == expected);
    }

    SECTION("code - not existing") {
        std::string file("not_existing.js");
        machine.setCodeDir(".");
        machine.initialize();

        REQUIRE_THROWS_AS(machine.fs.loadCode(file), std::runtime_error);
    }

    SECTION("data - existing") {
        std::string file("test_files/moduleLoader/test.js");
        std::string expected("report(\"first\")\nreport(\"second\")\n");
        machine.setDataDir(machine.path.dirname(file));
        machine.initialize();

        REQUIRE(readFile(machine.fs.openData(machine.path.basename(file), "r")) == expected);
    }

    SECTION("data - not existing") {
        std::string file("not_existing.js");
        machine.setDataDir(".");
        machine.initialize();

        REQUIRE_THROWS_AS(readFile(machine.fs.openData(file, "r")), std::runtime_error);
    }

    SECTION("data - write") {
        std::string file("test_files/fs/write.txt");
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
        }
        machine.setDataDir(".");
        machine.initialize();

        auto f = machine.fs.openData(file, "wt");
        f.write("test");
        f.close();

        REQUIRE(std::filesystem::exists(file));
        REQUIRE(readFile(machine.fs.openData(file, "r")) == "test");

        std::filesystem::remove(file);
    }
}

TEST_CASE("File class js", "[filesystem]") {
    using Machine =
        FilesystemFeature<
        TestReportFeature<
        jac::MachineBase
    >>;

    Machine machine;
    machine.setCodeDir("test_files/fs/");
    machine.setDataDir("test_files/fs/");
    machine.initialize();

    SECTION("read") {
        std::string code("import { open } from 'fs'\n"
                         "var file = open('testRead.txt', 'r');\n"
                         "report(file.read(1024));\n");

        evalCode(machine, code, "test.js", jac::EvalFlags::Module);
        REQUIRE(machine.getReports() == std::vector<std::string> { "test1\ntest2\n" });
    }

    SECTION("write") {
        std::string code("import { open } from 'fs'\n"
                         "var file = open('testWrite.txt', 'w');\n"
                         "file.write('test');\n"
                         "file.close();\n");

        evalCode(machine, code, "test.js", jac::EvalFlags::Module);
        REQUIRE(readFile(machine.fs.openData("testWrite.txt", "r")) == "test");
        std::filesystem::remove("test_files/fs/testWrite.txt");
    }

    SECTION("close") {
        std::string code("import { open } from 'fs'\n"
                         "var file = open('testClose.txt', 'w');\n"
                         "file.write('test');\n"
                         "file.close();\n"
                         "report(file.isOpen());\n");

        evalCode(machine, code, "test.js", jac::EvalFlags::Module);
        REQUIRE(machine.getReports() == std::vector<std::string> { "false" });

        std::filesystem::remove("test_files/fs/testClose.txt");
    }
}
