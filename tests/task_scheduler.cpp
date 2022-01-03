#include "doctest/doctest.h"
#include <task_scheduler.h>
#include <functional>
#include <map>
#include <future>
#include <chrono>
#include <thread>

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

TEST_CASE("miliseconds to ticks") {
    CHECK(0 == miliseconds2ticks(0, 0));
    CHECK(0 == miliseconds2ticks(0, 100));
    CHECK(0 == miliseconds2ticks(100, 0));

    CHECK(1 == miliseconds2ticks(10, 10));

    CHECK(6 == miliseconds2ticks(55, 10));
    CHECK(6 == miliseconds2ticks(59, 10));
    CHECK(6 == miliseconds2ticks(51, 10));
    CHECK(5 == miliseconds2ticks(50, 10));
}

// bool operator>(const TaskScheduler::Task)



void schedulerRun(TaskScheduler *scheduler) {
    scheduler->run();
}

// TEST_CASE("Task Scheduler") {
//     TaskScheduler scheduler;

//     auto schedulerRunResult = std::async(std::launch::async, schedulerRun, &scheduler);

//     scheduler.addTask([](){
//         printf("task 0\n");
//     });
//     scheduler.addTask([](){
//         printf("task 1\n");
//     });
//     scheduler.addTask([](){
//         printf("task 2\n");
//     });

//     int x = 5;

//     // scheduler.addTask([x]() mutable {
//     //     // x += 5;
//     // });

//     // std::this_thread::sleep_for(std::chrono::seconds(1));

//     CHECK(x == 10);
// }


