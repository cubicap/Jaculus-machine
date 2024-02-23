#pragma once

#include <jac/machine/values.h>
#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>
#include <cmath>



template<typename T>
bool approxEqual(T a, T b) {
    return std::fabs(a - b) < std::numeric_limits<T>::epsilon();
}


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



void evalFileThrowsJS(auto& machine, std::string path_) {
    try {
        jac::Value res = machine.evalFile(path_);
        REQUIRE(res.to<jac::Promise>().state() == jac::Promise::Rejected);
    }
    catch (std::exception& e) {
        std::string message(e.what());
        INFO("Expected exception in JS, got \"" + message + "\"");
        REQUIRE(false);
    }
    catch (...) {
        INFO("Expected exception in JS, got unknown exception");
        REQUIRE(false);
    }
};

void evalFileThrowsOut(auto& machine, std::string path_) {
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

void evalCodeGlobalThrows(auto& machine, std::string code, std::string filename) {
    try {
        machine.eval(code, filename, jac::EvalFlags::Global);
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


void evalCodeModuleThrows(auto& machine, std::string code, std::string filename) {
    try {
        jac::Value res = machine.eval(code, filename, jac::EvalFlags::Module);
        REQUIRE(res.to<jac::Promise>().state() == jac::Promise::Rejected);
    }
    catch (std::exception& e) {
        std::string message(e.what());
        INFO("Expected exception in JS, got \"" + message + "\"");
        REQUIRE(false);
    }
    catch (...) {
        INFO("Expected exception in JS, got unknown exception");
        REQUIRE(false);
    }
};
