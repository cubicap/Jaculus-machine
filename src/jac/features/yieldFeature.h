#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <noal_func.h>
#include <chrono>
#include <deque>


template<class Next>
class YieldFeature : public Next {
private:
    std::deque<jac::Function> _resolvers;
    int counter = 0;

    static void paralelRunner(YieldFeature* self, jac::Function resolve, std::vector<jac::Object> funcs) {
        if (funcs.empty()) {
            resolve.call<void>();
            return;
        }

        for (int i = 0; i < 100; i++) {
        auto it = funcs.begin();
        while (it != funcs.end()) {
            auto& func = *it;
            auto next = func.get<jac::ValueConst>("next");
            auto result = next.to<jac::FunctionConst>().callThis<jac::Value>(func);

            auto obj = result.to<jac::ObjectConst>();
            if (obj.get<bool>("done")) {
                it = funcs.erase(it);
            } else {
                it++;
            }
        }
        }
        self->eventLoop_schedule([self, resolve = std::move(resolve), funcs = std::move(funcs)]() mutable {
            paralelRunner(self, std::move(resolve), std::move(funcs));
        });
    }

    jac::Promise runParalel(std::vector<jac::ValueConst> args) {
        std::vector<jac::Object> funcs;
        for (auto& arg : args) {
            funcs.emplace_back(arg.to<jac::Object>());
        }

        auto [promise, resolve, reject] = jac::Promise::create(this->_context);

        this->eventLoop_schedule([this, resolve = std::move(resolve), funcs = std::move(funcs)]() mutable {
            paralelRunner(this, std::move(resolve), std::move(funcs));
        });

        return promise;
    }
public:
    void initialize() {
        Next::initialize();
        jac::FunctionFactory ff(this->_context);

        this->registerGlobal("_yield", ff.newFunction([this]() {
            auto [promise, resolve, _] = jac::Promise::create(this->_context);
            _resolvers.emplace_back(resolve);
            return promise;
        }));

        this->registerGlobal("yieldCount", ff.newFunction([this]() {
            return counter;
        }));

        this->registerGlobal("runParalel", ff.newFunctionVariadic(noal::function(&YieldFeature::runParalel, this)));
    }

    void onEventLoop() {
        Next::onEventLoop();
        while (!_resolvers.empty()) {
            auto& resolver = _resolvers.front();
            resolver.call<void>();
            _resolvers.pop_front();
            counter++;
        }
    }
};
