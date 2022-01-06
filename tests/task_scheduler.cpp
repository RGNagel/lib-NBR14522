#include "doctest/doctest.h"
#include <task_scheduler.h>
#include <functional>
#include <map>
#include <future>
#include <chrono>
#include <thread>

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

