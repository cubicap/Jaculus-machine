#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>

#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <jac/features/moduleLoaderFeature.h>
#include <jac/features/filesystemFeature.h>
#include <jac/features/timersFeature.h>
#include <jac/features/eventQueueFeature.h>
#include <jac/features/eventLoopFeature.h>

#include "util.h"


TEST_CASE("Imported promise", "[moduleLoader]") {
    using Machine =
        jac::EventLoopTerminal<
        jac::TimersFeature<
        jac::ModuleLoaderFeature<
        jac::FilesystemFeature<
        jac::EventLoopFeature<
        jac::EventQueueFeature<
        TestReportFeature<
        jac::MachineBase
    >>>>>>>;

    Machine machine;

    machine.setCodeDir("test_files/regression/importPromise");
    machine.initialize();

    evalFile(machine, "main.js");
    machine.runEventLoop();

    REQUIRE(machine.getReports() == std::vector<std::string> { "before", "after", "then" });
}


TEST_CASE("Timer leak", "[timers]") {
    using Machine =
        jac::EventLoopTerminal<
        jac::TimersFeature<
        jac::EventLoopFeature<
        jac::EventQueueFeature<
        TestReportFeature<
        jac::MachineBase
    >>>>>;

    Machine machine;

    machine.initialize();

    machine.eval("( async () => { for (let i = 0; i < 100000; i++) { await sleep(0); }; exit(0); } )()", "<eval>");

    machine.runEventLoop();

    REQUIRE(machine.getReports() == std::vector<std::string> {});
}



TEST_CASE("Exception", "[values]") {
    using Machine = jac::MachineBase;

    Machine machine;

    machine.initialize();

    evalCodeModuleThrows(machine, "throw new Error('test');", "<eval>");
    evalCodeGlobalThrows(machine, "throw new Error('test');", "<eval>");
}
