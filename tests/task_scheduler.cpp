#include "doctest/doctest.h"
#include <functional>

// class TaskScheduler {
//     class Task {
//       private:
//         std::function<void()> _callback;
//         bool _repeat;
//         uint32_t _waitAtLeastMiliseconds;
//         bool _canceled;



//       public:
//         Task(std::function<void()> callback,
//              uint32_t waitAtLeastMiliseconds = 0, bool repeat = false)
//             : _callback(callback), _repeat(repeat),
//               _waitAtLeastMiliseconds(waitAtLeastMiliseconds),
//               _canceled(false) {}

//         void cancel(){_canceled = true;};
//         bool isCanceled() {return _canceled; };
//         bool repeat() {return _repeat; };
//     };

//   private:
//     RingBuffer<Task*, 16> _tasks;
//     Timer _timer;

//   public:
//     void [[noreturn]] run() {
//         while (1) {

//             if (_tasks.toread() >= 1) {
//                 Task* task = _tasks.read();
//                 if (!task->isCanceled()) {
//                     task->call();

//                     if (task.repeat())
//                         _tasks.write(task);
//                 }
//             }

//             timer.waitMiliseconds(10);
//         }
//     }
// };

// class ActiveObject {
//   private:
//     RingBuffer<Event, 16> _events;

//   public:
//     void [[noreturn]] run() { _events.run(); }
// };


typedef uint32_t tick_t;
typedef uint32_t miliseconds_t;
typedef int task_t;

tick_t miliseconds2ticks(const miliseconds_t ms, const miliseconds_t tickPeriod) {
    if (ms == 0 || tickPeriod == 0 || ms < tickPeriod)
        return 0;

    tick_t retval = static_cast<tick_t>(ms/tickPeriod);

    // round up the tick quantity so we can assure it will consider AT LEAST the
    // ms informed
    if (ms % tickPeriod != 0)
        retval++;

    return retval;
}

TEST_CASE("miliseconds to ticks") {
    CHECK(0 == miliseconds2ticks(0, 0));
    CHECK(0 == miliseconds2ticks(0, 100));
    CHECK(0 == miliseconds2ticks(100, 0));

    CHECK(1 == miliseconds2ticks(10,10));

    CHECK(6 == miliseconds2ticks(55,10));
    CHECK(6 == miliseconds2ticks(59,10));
    CHECK(6 == miliseconds2ticks(51,10));
    CHECK(5 == miliseconds2ticks(50,10));
}

class TaskScheduler {
    private:
        class Task {
            private:
                task_t _taskID;
                tick_t _tickValueWhenCreated;
                tick_t _waitAtLeastTicks;
            public:
            Task(tick_t tickBase, tick_t waitAtLeast) : _tickValueWhenCreated(tickBase), _waitAtLeastTicks(waitAtLeast) {
                static task_t id = 0;
                _taskID = id;
                id++;
            }

            task_t getID() {return _taskID;}

        };
        tick_t _counter;
        miliseconds_t _tickPeriod;
    public:

        TaskScheduler() : _counter(0) {}

        task_t addTask(std::function<void()> callback, miliseconds_t waitAtLeast = 0) {

            Task task(_counter + 1, miliseconds2ticks(waitAtLeast, _tickPeriod));

            
            return task.getID();
        }

        bool removeTask(task_t task) {

        }

        void [[noreturn]] run() {
            while (1) {
                
                _counter++;
                // timer.waitMiliseconds(10);
            }
        }
};

TEST_CASE("Task Scheduler") {
    TaskScheduler scheduler;

    scheduler.addTask({...});
    scheduler.addTask({...}, 1000);
    scheduler.addTask({...}, 1000);

    int taskID = scheduler.addTask({...}, 100);
    scheduler.removeTask(taskID);


}
