#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

#include "eventLoopTerminal.h"


template<class Next>
class EventLoopFeature : public Next {
private:
    bool _shouldExit = false;
    int _exitCode = 1;
public:

    void runEventLoop() {
        try {
            JSRuntime* rt = JS_GetRuntime(this->_context);
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
                            throw jac::ContextRef(ctx1).getException();
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

    virtual void runOnEventLoop() = 0;

    void initialize() {
        Next::initialize();
        jac::FunctionFactory ff(this->_context);
        jac::Object global = this->_context.getGlobalObject();

        global.defineProperty("exit", ff.newFunction(noal::function(&EventLoopFeature::exit, this)), jac::PropFlags::Enumerable);
    }

    void onEventLoop() {
        // last in stack
    }
};
