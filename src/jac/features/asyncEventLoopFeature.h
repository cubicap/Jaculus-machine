#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <iostream>

#include "eventLoopTerminal.h"


template<class Next>
class AsyncEventLoopFeature : public Next {
private:
    bool _running = false;
    bool _shouldExit = false;
    int _exitCode = 1;
    std::deque<std::function<void()>> _scheduledFunctions;
    std::mutex _scheduledFunctionsMutex;
    std::condition_variable _scheduledFunctionsCondition;
public:
    bool checkEventQueue(bool wait) {
        std::unique_lock lock(_scheduledFunctionsMutex);
        if (wait && _scheduledFunctions.empty()) {
            _scheduledFunctionsCondition.wait(lock);
        }
        if (_scheduledFunctions.empty()) {
            return false;
        }
        auto func = _scheduledFunctions.front();
        _scheduledFunctions.pop_front();
        lock.unlock();

        func();
        return true;
    }

    void eventLoop_exit(int code = 0) {
        std::unique_lock lock(_scheduledFunctionsMutex);
        _shouldExit = true;
        _exitCode = code;
        _scheduledFunctionsCondition.notify_one();
    }

    int eventLoop_getExitCode() {
        return _exitCode;
    }

    void eventLoop_schedule(std::function<void()> func) {
        {
            std::scoped_lock lock(_scheduledFunctionsMutex);
            _scheduledFunctions.push_back(std::move(func));
        }
        _scheduledFunctionsCondition.notify_one();
    }

    void eventLoop_run() {
        _running = true;

        JSContext* ctx = this->_context;
        JSContext* ctx1;
        int err;

        bool didJob = true;

        while (!_shouldExit) {
            runOnEventLoop();

            bool gotEvent = checkEventQueue(!didJob);
            if (!didJob && !gotEvent) {
                continue;
            }

            didJob = false;
            while (!_shouldExit) {
                err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
                if (err <= 0) {
                    if (err < 0) {
                        // js_std_dump_error(ctx1);
                        std::cerr << "Error: " << err << std::endl;
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
        _scheduledFunctionsCondition.notify_one();
    }

    virtual void runOnEventLoop() = 0;

    void initialize() {
        Next::initialize();
        jac::FunctionFactory ff(this->_context);
        this->registerGlobal("exit", ff.newFunction(noal::function(&AsyncEventLoopFeature::eventLoop_exit, this)));
    }

    void onEventLoop() {
        // last in stack
    }
};
