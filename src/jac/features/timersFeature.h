#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>


template<>
struct jac::ConvTraits<std::chrono::milliseconds> {
    static std::chrono::milliseconds from(ContextRef ctx, ValueConst value) {
        return std::chrono::milliseconds(value.to<int>());
    }

    static Value to(ContextRef ctx, std::chrono::milliseconds value) {
        return Value::from(ctx, static_cast<int>(value.count()));
    }
};

template<class Next>
class TimersFeature : public Next {
private:
    class Timer {
    private:
        std::chrono::time_point<std::chrono::steady_clock> _startTime;
        std::chrono::milliseconds _duration;
        bool _isRepeating = false;
        std::function<void()> _callback;
        int _id;
    public:
        Timer(std::function<void()> callback, std::chrono::milliseconds duration, int id, bool isRepeating = false):
            _duration(duration),
            _isRepeating(isRepeating),
            _callback(callback),
            _id(id)
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

        int getId() const {
            return _id;
        }
    };

    std::priority_queue<Timer> _timers;
    std::mutex _timersMutex;
    std::condition_variable _timersCondition;
    std::thread _timerThread;
    bool _stop = false;

    int nextId = 1;

    int createTimer(std::function<void()> func, std::chrono::milliseconds millis, bool isRepeating) {
        std::lock_guard<std::mutex> lock(_timersMutex);
        _timers.emplace(func, millis, nextId++, isRepeating);
        _timersCondition.notify_one();
        return nextId - 1;
    }

    void clearTimer(int id) {
        std::lock_guard<std::mutex> lock(_timersMutex);
        std::priority_queue<Timer> newTimers;
        while (!_timers.empty()) {
            auto timer = _timers.top();
            _timers.pop();
            if (timer.getId() != id) {
                newTimers.push(timer);
            }
        }
    }
public:
    int setInterval(std::function<void()> func, std::chrono::milliseconds millis) {
        return createTimer(func, millis, true);
    }

    int setTimeout(std::function<void()> func, std::chrono::milliseconds millis) {
        return createTimer(func, millis, false);
    }

    void clearInterval(int id) {
        clearTimer(id);
    }

    void clearTimeout(int id) {
        clearTimer(id);
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

        this->registerGlobal("setInterval", ff.newFunction([this](jac::Function func, std::chrono::milliseconds millis) {
            return setInterval([func]() mutable {
                func.call<void>();
            }, millis);
        }));

        this->registerGlobal("setTimeout", ff.newFunction([this](jac::Function func, std::chrono::milliseconds millis) {
            return setTimeout([func]() mutable {
                func.call<void>();
            }, millis);
        }));

        this->registerGlobal("clearInterval", ff.newFunction([this](int id) {
            clearInterval(id);
        }));

        this->registerGlobal("clearTimeout", ff.newFunction([this](int id) {
            clearTimeout(id);
        }));

        this->registerGlobal("sleep", ff.newFunction([this](int millis) {
            auto [promise, resolve, _] = jac::Promise::create(this->_context);
            createTimer([resolve]() {
                const_cast<jac::Function&>(resolve).call<void>();
            }, std::chrono::milliseconds(millis), false);
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
