#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>

#include "util.h"


TEST_CASE("Register global", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    machine.registerGlobal("test", jac::Value::from<std::string>(machine._context, "test string"));


    evalCode(machine, "report(test);", "test", JS_EVAL_TYPE_MODULE);

    REQUIRE(machine.getReports() == std::vector<std::string>{"test string"});
}


TEST_CASE("Cpp Module", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("Simple") {
        auto& module = machine.newModule("testModule");
        module.addExport("test", jac::Value::from<std::string>(machine._context, "test string"));

        evalCode(machine, "import * as testModule from 'testModule'; report(testModule.test);", "test", JS_EVAL_TYPE_MODULE);

        REQUIRE(machine.getReports() == std::vector<std::string>{"test string"});
    }

    SECTION("Not imported") {
        auto& module = machine.newModule("testModule");
        module.addExport("test", jac::Value::from<std::string>(machine._context, "test string"));

        evalCode(machine, "report('nothing');", "test", JS_EVAL_TYPE_MODULE);

        REQUIRE(machine.getReports() == std::vector<std::string>{"nothing"});
    }

    SECTION("Two modules") {
        auto& module = machine.newModule("testModule1");
        module.addExport("test1", jac::Value::from<std::string>(machine._context, "test string 1"));

        auto& module2 = machine.newModule("testModule2");
        module2.addExport("test2", jac::Value::from<std::string>(machine._context, "test string 2"));

        evalCode(machine, R"(
            import * as testModule1 from 'testModule1';
            import * as testModule2 from 'testModule2';
            report(testModule1.test1);
            report(testModule2.test2);
        )", "test", JS_EVAL_TYPE_MODULE);

        REQUIRE(machine.getReports() == std::vector<std::string>{"test string 1", "test string 2"});
    }
}
