#pragma once

#include <jac/machine/values.h>
#include <jac/machine/machine.h>
#include <mutex>
#include <deque>
#include <condition_variable>

template<class Next>
class ReplFeature : public Next {
public:
    jac::Value prompt(std::string prompt) {
        std::mutex _promptMutex;
        std::condition_variable _resultCondition;


        auto result = jac::Value::undefined(this->_context);

        this->scheduleEvent([&]() {
            std::unique_lock _(_promptMutex);
            try {
                result = this->eval(prompt, "repl", JS_EVAL_TYPE_GLOBAL);
            } catch (jac::Exception& e) {
                std::cout << "Exception: " << e.what() << std::endl;
                std::cout << "Stack: " << e.stackTrace() << std::endl;
            }

            _resultCondition.notify_one();
        });

        std::unique_lock lock(_promptMutex);
        _resultCondition.wait(lock);

        return result;
    }
};
