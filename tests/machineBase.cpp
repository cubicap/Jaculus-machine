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
    jac::Object global = machine.context().getGlobalObject();

    global.defineProperty("test", jac::Value::from<std::string>(machine.context(), "test string"));


    evalCode(machine, "report(test);", "test", jac::EvalFlags::Module);

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
        module.addExport("test", jac::Value::from<std::string>(machine.context(), "test string"));

        evalCode(machine, "import * as testModule from 'testModule'; report(testModule.test);", "test", jac::EvalFlags::Module);

        REQUIRE(machine.getReports() == std::vector<std::string>{"test string"});
    }

    SECTION("Not imported") {
        auto& module = machine.newModule("testModule");
        module.addExport("test", jac::Value::from<std::string>(machine.context(), "test string"));

        evalCode(machine, "report('nothing');", "test", jac::EvalFlags::Module);

        REQUIRE(machine.getReports() == std::vector<std::string>{"nothing"});
    }

    SECTION("Two modules") {
        auto& module = machine.newModule("testModule1");
        module.addExport("test1", jac::Value::from<std::string>(machine.context(), "test string 1"));

        auto& module2 = machine.newModule("testModule2");
        module2.addExport("test2", jac::Value::from<std::string>(machine.context(), "test string 2"));

        evalCode(machine, R"(
            import * as testModule1 from 'testModule1';
            import * as testModule2 from 'testModule2';
            report(testModule1.test1);
            report(testModule2.test2);
        )", "test", jac::EvalFlags::Module);

        REQUIRE(machine.getReports() == std::vector<std::string>{"test string 1", "test string 2"});
    }
}

TEST_CASE("watchdog", "[base]") {
    using Machine =
        TestReportFeature<
        jac::MachineBase
    >;

    Machine machine;
    machine.initialize();

    SECTION("Stop") {
        int triggerCount = 0;
        machine.setWatchdogTimeout(std::chrono::milliseconds(50));
        machine.setWatchdogHandler(std::function<bool()>([&triggerCount]() {
            triggerCount++;
            return true;
        }));

        evalCodeThrows(machine, R"(
            report('start');
            let until = Date.now() + 100;
            while (Date.now() < until) {}
            report('end');
        )", "test", jac::EvalFlags::Module);

        REQUIRE(machine.getReports() == std::vector<std::string>{"start"});
    }

    SECTION("Ok") {
        int triggerCount = 0;
        machine.setWatchdogTimeout(std::chrono::milliseconds(50));
        machine.setWatchdogHandler(std::function<bool()>([&triggerCount]() {
            triggerCount++;
            return true;
        }));

        evalCode(machine, R"(
            report('start');
            let until = Date.now() + 20;
            while (Date.now() < until) {}
            report('end');
        )", "test", jac::EvalFlags::Module);

        REQUIRE(machine.getReports() == std::vector<std::string>{"start", "end"});
    }
}
