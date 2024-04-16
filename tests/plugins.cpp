#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <jac/features/evalFeature.h>
#include <jac/machine/machine.h>
#include <jac/machine/plugins.h>

#include "util.h"


class ReportPlugin : public jac::Plugin {
    std::vector<std::string> _reports;
public:
    void report(std::string report) {
        _reports.push_back(report);
    }

    const std::vector<std::string>& getReports() {
        return _reports;
    }
public:
    ReportPlugin(jac::MachineBase& machine) {
        jac::FunctionFactory ff(machine.context());
        jac::Object global = machine.context().getGlobalObject();

        global.defineProperty("report", ff.newFunction([this](jac::ValueWeak val) {
            this->report(val.to<std::string>());
        }));
    }
};


TEST_CASE("ReportPlugin") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::EvalFeature,
        jac::PluginHolderFeature
    >;

    Machine machine;
    machine.initialize();

    jac::PluginManager pm;
    auto off = pm.addPlugin<ReportPlugin>();

    SECTION("report") {
        auto handle = pm.initialize(machine);

        evalCode(machine, "report('hello');", "test.js", jac::EvalFlags::Global);
        evalCode(machine, "report('world');", "test.js", jac::EvalFlags::Global);

        auto reports = machine.getPlugin<ReportPlugin>(handle + off).getReports();

        REQUIRE(reports == std::vector<std::string>{"hello", "world"});
    }
}


class MathPlugin : public jac::Plugin {
    size_t _calls = 0;
public:
    MathPlugin(jac::MachineBase& machine) {
        jac::FunctionFactory ff(machine.context());
        jac::Object global = machine.context().getGlobalObject();

        global.defineProperty("add", ff.newFunction([this](int a, int b) {
            _calls++;
            return a + b;
        }));
    }

    size_t calls() {
        return _calls;
    }
};

TEST_CASE("MathPlugin") {
    using Machine = jac::ComposeMachine<
        jac::MachineBase,
        jac::EvalFeature,
        jac::PluginHolderFeature
    >;

    Machine machine;
    machine.initialize();

    jac::PluginManager pm;
    auto mathOff = pm.addPlugin<MathPlugin>();
    auto reportOff = pm.addPlugin<ReportPlugin>();

    SECTION("add") {
        auto handle = pm.initialize(machine);

        evalCode(machine, "report(add(1, 2));", "test.js", jac::EvalFlags::Global);

        auto reports = machine.getPlugin<ReportPlugin>(handle + reportOff).getReports();

        REQUIRE(reports == std::vector<std::string>{"3"});
        REQUIRE(machine.getPlugin<MathPlugin>(handle + mathOff).calls() == 1);
    }
}
