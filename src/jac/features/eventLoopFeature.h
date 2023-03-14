#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>
#include <deque>
#include <mutex>
#include <condition_variable>

#include "eventLoopTerminal.h"


template<class Next>
class EventLoopFeature : public Next {
private:
    bool _running = false;
    bool _shouldExit = false;
    int _exitCode = 1;
public:

    void eventLoop_run() {
        _running = true;

        JSContext* ctx = this->_context;
        JSContext* ctx1;
        int err;

        bool didJob = true;

        while (!_shouldExit) {
            runOnEventLoop();

            bool gotEvent = this->runEvent(!didJob);
            if (!didJob && !gotEvent) {
                continue;
            }

            didJob = false;
            while (!_shouldExit) {
                err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
                if (err <= 0) {
                    if (err < 0) {
                        // js_std_dump_error(ctx1);
                        this->stdio.err->println(std::string("Error executing job: ") + std::to_string(err));
                    }
                    break;
                }
                didJob = true;
            }
        }
        _running = false;
    }

    void eventLoop_stop() {
        _shouldExit = true;
        this->notifyEventLoop();
    }

    void eventLoop_exit(int code = 0) {
        _exitCode = code;
        eventLoop_stop();
    }

    int eventLoop_getExitCode() {
        return _exitCode;
    }

    virtual void runOnEventLoop() = 0;

    void initialize() {
        Next::initialize();
        jac::FunctionFactory ff(this->_context);
        this->registerGlobal("exit", ff.newFunction(noal::function(&EventLoopFeature::eventLoop_exit, this)));
    }

    void onEventLoop() {
        // last in stack
    }
};
