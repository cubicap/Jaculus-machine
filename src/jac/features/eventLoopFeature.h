#pragma once

#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>

#include <atomic>
#include <optional>
#include <string>

#include <noal_func.h>


#include "eventLoopTerminal.h"


namespace jac {


/**
 * @note The EventLoopFeature must be companied by EventLoopTerminal at the top of the Machine stack.
 */
template<class Next>
class EventLoopFeature : public Next {
private:
    std::atomic<bool> _shouldExit = false;
    int _exitCode = 1;

protected:
    std::optional<Exception> _error = std::nullopt;

    void evalWithEventLoopCommon(Value& promise) {
        auto promiseObj = promise.to<ObjectWeak>();

        FunctionFactory ff(this->context());
        auto fail = ff.newFunction(noal::function([this](Value err) {
            this->_error = err.to<Exception>();
            this->kill();
        }));

        auto catch_ = promiseObj.get<Function>("catch");
        catch_.callThis<void>(promise, fail);

        this->runEventLoop();
    }
public:

    void runEventLoop() {
        try {
            JSRuntime* rt = JS_GetRuntime(this->context());
            JSContext* ctx1;

            bool didJob = true;
            while (!_shouldExit) {
                runOnEventLoop();

                auto event = this->getEvent(!didJob);
                this->resetWatchdog();
                if (event) {
                    (*event)();
                }
                else if (!didJob) {
                    continue;
                }

                didJob = false;
                while (!_shouldExit) {
                    this->resetWatchdog();
                    int err = JS_ExecutePendingJob(rt, &ctx1);
                    if (err <= 0) {
                        if (err < 0) {
                            throw ContextRef(ctx1).getException();
                        }
                        break;
                    }
                    didJob = true;
                }
            }
        }
        catch (...) {
            if (_shouldExit) {
                // ignore
            }
            else {
                throw;
            }
        }

        if (_error) {
            throw (*_error);
        }
    }

    void kill() {
        _shouldExit = true;
        this->interruptRuntime();
        this->notifyEventLoop();
        _exitCode = 1;
    }

    void exit(int code) {
        kill();
        _exitCode = code;
    }

    int getExitCode() {
        return _exitCode;
    }

    void evalModuleWithEventLoop(std::string code, std::string filename) {
        Value promise = this->eval(std::move(code), filename, EvalFlags::Module);
        this->evalWithEventLoopCommon(promise);
    }

    virtual void runOnEventLoop() = 0;

    void initialize() {
        Next::initialize();
        FunctionFactory ff(this->context());
        Object global = this->context().getGlobalObject();

        global.defineProperty("exit", ff.newFunction(noal::function(&EventLoopFeature::exit, this)), PropFlags::Enumerable);
    }

    void onEventLoop() {
        // last in stack
    }
};


} // namespace jac
