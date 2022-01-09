#pragma once

#include <chrono>
#include <thread>

#include <functional>
#include <map>
typedef uint32_t tick_t;
typedef uint32_t milliseconds_t;
typedef int task_t;

inline tick_t miliseconds2ticks(const milliseconds_t ms,
                                const milliseconds_t tickPeriod) {
    if (ms == 0 || tickPeriod == 0 || ms < tickPeriod)
        return 0;

    tick_t retval = static_cast<tick_t>(ms / tickPeriod);

    // round up the tick quantity so we can assure it will consider AT LEAST the
    // ms informed
    if (ms % tickPeriod != 0)
        retval++;

    return retval;
}

class TaskScheduler {
  private:
    tick_t _counter;
    milliseconds_t _tickPeriod;
    std::multimap<tick_t, std::function<void()>> _tasks;

  public:
    TaskScheduler(milliseconds_t tickPeriod = 10)
        : _counter(0), _tickPeriod(tickPeriod) {}

    void addTask(std::function<void()> callback,
                 milliseconds_t waitAtLeast = 0) {
        tick_t key = _counter + miliseconds2ticks(waitAtLeast, _tickPeriod);
        _tasks.insert({key, callback});
    }

    [[noreturn]] void run() {
        while (1) {

            if (_tasks.size() <= 0) {
                _counter = 0;
                continue;
            }

            for (auto i = _tasks.find(_counter); i->first == _counter; i++) {
                i->second();
            }

            _counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(_tickPeriod));
        }
    }
};
