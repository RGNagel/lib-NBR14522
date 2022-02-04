#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <thread>
#include <timer.h>

typedef uint32_t tick_t;
typedef int task_t;

inline tick_t ms2ticks(const milliseconds_t ms,
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

template <milliseconds_t tick_period = 1, size_t tasks_sz = 8> class TaskScheduler {
  private:
    tick_t _counter;
    
    typedef struct {
        std::function<void()> fun;
        tick_t triggerAt;
    } task_t;

    std::array<task_t, tasks_sz> _tasks;

  public:
    TaskScheduler() : _counter(0) {
        _tasks.fill({nullptr,0});
    }

    void addTask(std::function<void()> callback,
                 milliseconds_t waitAtLeast = 0) {
        tick_t triggerAt = _counter + ms2ticks(waitAtLeast, tick_period);

        for (auto t = _tasks.begin(); t != _tasks.end(); t++) {
            if (!t->fun) {
                t->fun = callback;
                t->triggerAt = triggerAt;
                break;
            }
        }
    }

    [[noreturn]] void run() {
        while (1) {

            if (_tasks.size() <= 0) {
                _counter = 0;
            } else {

                for (auto i = _tasks.begin(); i != _tasks.end(); i++) {
                    if (i->fun && i->triggerAt <= _counter) {
                        i->fun();
                        i->fun = nullptr;
                    }
                }

                _counter++;
            }

            Timer::wait(tick_period);
        }
    }
};
