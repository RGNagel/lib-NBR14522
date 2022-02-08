#include "doctest/doctest.h"
#include <chrono>
#include <functional>
#include <map>
#include <task_scheduler.h>
#include <thread>

using namespace std::literals;

// TEST_CASE("class TaskScheduler" * doctest::skip()) {
TEST_CASE("class TaskScheduler") {
    TaskScheduler<> ts;

    std::thread runthread(&TaskScheduler<>::run, &ts);

    SUBCASE("No timeout") {
        int t1 = 0, t2 = 0, t3 = 0;

        ts.addTask([&t1]() { t1++; });
        ts.addTask([&t2]() { t2++; });
        ts.addTask([&t3]() { t3++; });

        // I think we can consider undefined behaviour if before adding some
        // delay the tasks will be trigger or not.

        std::this_thread::sleep_for(1ms);

        CHECK(t1 == 1);
        CHECK(t2 == 1);
        CHECK(t3 == 1);
    }

    SUBCASE("With timeout") {
        int t1 = 0, t2 = 0, t3 = 0;

        ts.addTask([&t1]() { t1++; }, 50ms);
        ts.addTask([&t2]() { t2++; }, 100ms);
        ts.addTask([&t3]() { t3++; }, 150ms);

        std::this_thread::sleep_for(1ms);

        CHECK(t1 == 0);
        CHECK(t2 == 0);
        CHECK(t3 == 0);

        std::this_thread::sleep_for(50ms);
        std::this_thread::sleep_for(5ms); // add extra delay to assure process has enough time to execute it

        CHECK(t1 == 1);
        CHECK(t2 == 0);
        CHECK(t3 == 0);

        std::this_thread::sleep_for(50ms);
        std::this_thread::sleep_for(5ms); // add extra delay to assure process has enough time to execute it

        CHECK(t1 == 1);
        CHECK(t2 == 1);
        CHECK(t3 == 0);

        std::this_thread::sleep_for(50ms);
        std::this_thread::sleep_for(5ms); // add extra delay to assure process has enough time to execute it

        CHECK(t1 == 1);
        CHECK(t2 == 1);
        CHECK(t3 == 1);
    }

    ts.runStop();
    runthread.join();
}
