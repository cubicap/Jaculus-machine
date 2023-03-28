#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>


template<class Next>
class EventQueueFeature : public Next {
private:
    std::deque<std::function<void()>> _scheduledFunctions;
    std::mutex _scheduledFunctionsMutex;
    std::condition_variable _scheduledFunctionsCondition;
public:
    /**
     * @brief Check event queue for events and run one if available
     * @param wait Wait for event if no event is available
     * @return true if event was run, false if no event was run
     */
    std::optional<std::function<void()>> getEvent(bool wait) {
        std::unique_lock lock(_scheduledFunctionsMutex);
        if (wait && _scheduledFunctions.empty()) {
            _scheduledFunctionsCondition.wait(lock);
        }
        if (_scheduledFunctions.empty()) {
            return std::nullopt;
        }
        auto func = std::move(_scheduledFunctions.front());
        _scheduledFunctions.pop_front();
        lock.unlock();

        return func;
    }

    /**
     * @brief Schedule an event to be run
     * @param func Function to be run
     */
    void scheduleEvent(std::function<void()> func) {
        {
            std::scoped_lock lock(_scheduledFunctionsMutex);
            _scheduledFunctions.push_back(std::move(func));
        }
        _scheduledFunctionsCondition.notify_one();
    }

    /**
     * @brief Wake up event loop if it is waiting for events
     */
    void notifyEventLoop() {
        _scheduledFunctionsCondition.notify_one();
    }

    ~EventQueueFeature() {
        notifyEventLoop();
        std::scoped_lock lock(_scheduledFunctionsMutex);
        _scheduledFunctions.clear();
    }
};
