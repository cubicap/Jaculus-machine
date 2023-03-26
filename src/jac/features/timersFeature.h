#pragma once

#include <jac/machine/machine.h>
#include <jac/machine/functionFactory.h>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>


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
        std::function<void()> _callback;
        int _id;
        bool _isRepeating = false;
        bool cancelled = false;

    public:
        Timer(std::function<void()> callback, std::chrono::milliseconds duration, int id, bool isRepeating = false):
            _duration(duration),
            _callback(callback),
            _id(id),
            _isRepeating(isRepeating)
        {
            _startTime = std::chrono::steady_clock::now();
        }

        bool operator<(const Timer& other) const {
            return getEndTime() > other.getEndTime();
        }

        void update() {
            if (_isRepeating) {
                _startTime = std::chrono::steady_clock::now();
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

        bool isCancelled() const {
            return cancelled;
        }

        int getId() const {
            return _id;
        }

        void cancel() {
            cancelled = true;
        }
    };

    class CompareTimer {
    public:
        bool operator()(const std::shared_ptr<Timer>& a, const std::shared_ptr<Timer>& b) const {
            return *a < *b;
        }
    };

    std::priority_queue<std::shared_ptr<Timer>, std::vector<std::shared_ptr<Timer>>, CompareTimer> _timers;
    std::unordered_map<int, std::shared_ptr<Timer>> _timersById;
    std::mutex _timersMutex;
    std::condition_variable _timersCondition;
    std::thread _timerThread;
    bool _stop = false;

    int nextId = 1;

    int createTimer(std::function<void()> func, std::chrono::milliseconds millis, bool isRepeating) {
        std::lock_guard<std::mutex> lock(_timersMutex);
        auto timer = std::make_shared<Timer>(func, millis, nextId++, isRepeating);
        _timers.emplace(timer);
        _timersById[nextId - 1] = std::move(timer);

        _timersCondition.notify_one();
        return nextId - 1;
    }

    void clearTimer(int id) {
        std::lock_guard<std::mutex> lock(_timersMutex);
        auto it = _timersById.find(id);
        if (it != _timersById.end()) {
            it->second->cancel();
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

                if (timer->getEndTime() > std::chrono::steady_clock::now()) {
                    _timersCondition.wait_until(lock, timer->getEndTime());
                    continue;
                }

                _timers.pop();

                lock.unlock();

                if (timer->isCancelled()) {
                    continue;
                }

                this->scheduleEvent([timer, this]() mutable {
                    if (timer->isCancelled()) {
                        return;
                    }
                    timer->getCallback()();

                    if (timer->isRepeating()) {
                        timer->update();
                        _timers.push(timer);
                        _timersCondition.notify_one();
                    }
                });
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
