#include "doctest/doctest.h"
#include <chrono>
#include <functional>
#include <map>
#include <task_scheduler.h>
#include <thread>

TEST_CASE("ms2ticks") {
    CHECK(0 == ms2ticks(0, 0));
    CHECK(0 == ms2ticks(0, 100));
    CHECK(0 == ms2ticks(100, 0));

    CHECK(1 == ms2ticks(10, 10));

    CHECK(6 == ms2ticks(55, 10));
    CHECK(6 == ms2ticks(59, 10));
    CHECK(6 == ms2ticks(51, 10));
    CHECK(5 == ms2ticks(50, 10));
}

TEST_CASE("class TaskScheduler") {
    TaskScheduler<> ts;

    std::thread runthread(&TaskScheduler<1>::run, &ts);

    SUBCASE("No timeout") {
        int t1 = 0, t2 = 0, t3 = 0;

        ts.addTask([&t1]() { t1++; });
        ts.addTask([&t2]() { t2++; });
        ts.addTask([&t3]() { t3++; });

        // I think we can consider undefined behaviour if before adding some
        // delay the tasks will be trigger or not.

        Timer::wait(1);

        CHECK(t1 == 1);
        CHECK(t2 == 1);
        CHECK(t3 == 1);
    }

    SUBCASE("With timeout") {
        int t1 = 0, t2 = 0, t3 = 0;

        ts.addTask([&t1]() { t1++; }, 50);
        ts.addTask([&t2]() { t2++; }, 100);
        ts.addTask([&t3]() { t3++; }, 150);

        Timer::wait(1);

        CHECK(t1 == 0);
        CHECK(t2 == 0);
        CHECK(t3 == 0);

        Timer::wait(52);

        // CHECK(t1 == 1);
        CHECK(t2 == 0);
        CHECK(t3 == 0);
    }

    ts.runStop();
    runthread.join();
}
