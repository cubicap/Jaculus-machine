#pragma once

#include <jac/machine/values.h>
#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>


template<class Next>
class TestReportFeature : public Next {
    std::vector<std::string> _reports;
public:
    void report(std::string report) {
        _reports.push_back(report);
    }

    const std::vector<std::string>& getReports() {
        return _reports;
    }

    void initialize() {
        Next::initialize();
        jac::FunctionFactory ff(this->context());
        jac::Object global = this->context().getGlobalObject();

        global.defineProperty("report", ff.newFunction([this](jac::ValueWeak val) {
            this->report(val.to<std::string>());
        }));
    }
};


jac::Value evalFile(auto& machine, std::string path_) {
    try {
        return machine.evalFile(path_);
    }
    catch (jac::Exception& e) {
        std::string message(e.what());
        CAPTURE(message);
        REQUIRE(false);
    }

    return jac::Value::undefined(machine.context());
};


jac::Value evalCode(auto& machine, std::string code, std::string filename, jac::EvalFlags flags) {
    try {
        return machine.eval(code, filename, flags);
    }
    catch (jac::Exception& e) {
        std::string message(e.what());
        CAPTURE(message);
        REQUIRE(false);
    }

    return jac::Value::undefined(machine.context());
};



void evalFileThrows(auto& machine, std::string path_) {
    try {
        machine.evalFile(path_);
    }
    catch (jac::Exception& e) {
        return;
    }
    catch (std::exception& e) {
        std::string message(e.what());
        INFO("Expected jac::Exception, got \"" + message + "\"");
        REQUIRE(false);
    }
    catch (...) {
        INFO("Expected jac::Exception, got unknown exception");
        REQUIRE(false);
    }

    INFO("Expected jac::Exception, but no exception was thrown");
    REQUIRE(false);
};


void evalCodeThrows(auto& machine, std::string code, std::string filename, jac::EvalFlags flags) {
    try {
        machine.eval(code, filename, flags);
    }
    catch (jac::Exception& e) {
        return;
    }
    catch (std::exception& e) {
        std::string message(e.what());
        INFO("Expected jac::Exception, got \"" + message + "\"");
        REQUIRE(false);
    }
    catch (...) {
        INFO("Expected jac::Exception, got unknown exception");
        REQUIRE(false);
    }

    INFO("Expected jac::Exception, but no exception was thrown");
    REQUIRE(false);
};
