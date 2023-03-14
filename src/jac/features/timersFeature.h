#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>


template<class Next>
class TimersFeature : public Next {
private:
    class Timer {
    private:
        std::chrono::time_point<std::chrono::steady_clock> _startTime;
        std::chrono::milliseconds _duration;
        bool _isRepeating = false;
        std::function<void()> _callback;
    public:
        Timer(std::chrono::milliseconds duration, std::function<void()> callback, bool isRepeating = false):
            _duration(duration),
            _isRepeating(isRepeating),
            _callback(callback)
        {
            _startTime = std::chrono::steady_clock::now();
        }

        bool operator<(const Timer& other) const {
            return getEndTime() > other.getEndTime();
        }

        void update() {
            if (_isRepeating) {
                _startTime = getEndTime();
            }
        }

        std::function<void()> getCallback() {
            return _callback;
        }

        std::chrono::time_point<std::chrono::steady_clock> getEndTime() const {
            return _startTime + _duration;
        }

        bool isRepeating() const {
            return _isRepeating;
        }
    };

    std::priority_queue<Timer> _timers;
    std::mutex _timersMutex;
    std::condition_variable _timersCondition;
    std::thread _timerThread;
    bool _stop = false;

public:
    void createTimer(std::chrono::milliseconds millis, std::function<void()> func, bool isRepeating) {
        std::lock_guard<std::mutex> lock(_timersMutex);
        _timers.emplace(millis, func, isRepeating);
        _timersCondition.notify_one();
    }

    void initialize() {
        Next::initialize();

        _stop = false;
        _timerThread = std::thread([this]() {
            while (!_stop) {
                std::unique_lock<std::mutex> lock(_timersMutex);
                if (_timers.empty()) {
                    _timersCondition.wait(lock);
                    if (_timers.empty()) {
                        continue;
                    }
                }

                auto timer = _timers.top();
                if (timer.getEndTime() > std::chrono::steady_clock::now()) {
                    _timersCondition.wait_until(lock, timer.getEndTime());
                    continue;
                }

                _timers.pop();

                if (timer.isRepeating()) {
                    timer.update();
                    _timers.push(timer);
                }
                auto callback = timer.getCallback();
                lock.unlock();

                this->scheduleEvent(callback);
            }
        });

        jac::FunctionFactory ff(this->_context);

        this->registerGlobal("createTimer", ff.newFunction([this](int millis, jac::Function func, bool isRepeating) {
            this->createTimer(std::chrono::milliseconds(millis), [func]() {
                const_cast<jac::Function&>(func).call<void>();
            }, isRepeating);
        }));

        this->registerGlobal("sleep", ff.newFunction([this](int millis) {
            auto [promise, resolve, _] = jac::Promise::create(this->_context);
            this->createTimer(std::chrono::milliseconds(millis), [resolve]() {
                const_cast<jac::Function&>(resolve).call<void>();
            }, false);
            return promise;
        }));
    }

    ~TimersFeature() {
        {
            std::scoped_lock lock(_timersMutex);
            _timers = {};
        }
        _stop = true;
        _timersCondition.notify_one();

        if (_timerThread.joinable()) {
            _timerThread.join();
        }
    }
};
