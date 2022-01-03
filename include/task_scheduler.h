#pragma once

#include <functional>
#include <map>
typedef uint32_t tick_t;
typedef uint32_t miliseconds_t;
typedef int task_t;

inline tick_t miliseconds2ticks(const miliseconds_t ms,
                         const miliseconds_t tickPeriod) {
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
  public:
    class Task {
      private:
        std::function<void()> _callback;
        task_t _id;

      public:
        Task(std::function<void()> callback) : _callback(callback) {
            static task_t id = 0;
            _id = id++;
        }

        void callback() {
            _callback();
        }

        task_t id() {
            return _id;
        }
    };

  private:

    tick_t _counter;
    miliseconds_t _tickPeriod;
    std::multimap<tick_t, Task> _tasks;

  public:

    TaskScheduler() : _counter(0) {}

    void addTask(std::function<void()> callback,
                   miliseconds_t waitAtLeast = 0) {

        Task task(callback);
        // _tasks.insert(task);
        tick_t key = _counter + miliseconds2ticks(waitAtLeast, _tickPeriod);
        _tasks.insert({key, task});
    }

    void [[noreturn]] run() {
        while (1) {
            
            if (_tasks.size() <= 0) {
                _counter = 0;
                continue;
            }

            for (auto i = _tasks.find(_counter); i->first == _counter; i++) {
                i->second.callback();
            }


            _counter++;
            // timer.waitMiliseconds(10);
        }
    }
};