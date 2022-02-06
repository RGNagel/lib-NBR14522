#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <thread>

using namespace std::literals;

/**
 * @brief
 *
 * @tparam clock_type    std::chrono clock. Para sistemas embarcados deve-se
 * criar um novo. Veja ref.:
 * https://stackoverflow.com/questions/46736323/using-chrono-as-a-timer-in-bare-metal-microcontroller
 * @tparam tasks_sz      quantidade máxima de tarefas
 */

template <typename clock_type = std::chrono::system_clock, size_t tasks_sz = 8>
class TaskScheduler {
  private:
    using time_point = std::chrono::time_point<clock_type>;

    using Task = struct {
        std::function<void()> fun;
        time_point triggerAt;
    };

    std::array<Task, tasks_sz> _tasks;
    bool _stop;

  public:
    TaskScheduler() : _stop(false) { _tasks.fill({nullptr, {}}); }

    void addTask(std::function<void()> callback,
                 std::chrono::milliseconds waitAtLeast = 0ms) {
        std::chrono::time_point triggerAt =
            std::chrono::high_resolution_clock::now() + waitAtLeast;

        for (auto t = _tasks.begin(); t != _tasks.end(); t++) {
            if (!t->fun) {
                t->fun = callback;
                t->triggerAt = triggerAt;
                break;
            }
        }
    }

    void run() {
        _stop = false;

        while (1) {
            if (_stop)
                break;

            for (auto i = _tasks.begin(); i != _tasks.end(); i++) {
                if (i->fun && i->triggerAt <= clock_type::now()) {
                    i->fun();
                    i->fun = nullptr;
                }
            }
        }
    }

    void runStop() { _stop = true; }
};
