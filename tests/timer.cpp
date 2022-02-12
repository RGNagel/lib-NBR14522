#include <chrono>
#include <doctest/doctest.h>
#include <thread>
#include <timer.h>

using namespace std::literals;

TEST_CASE("Timer") {

    Timer<> t;

    t.setTimeout(10ms);
    CHECK(!t.timedOut());
    std::this_thread::sleep_for(10ms);
    CHECK(t.timedOut());

    t.setTimeout(20ms);
    CHECK(!t.timedOut());
    std::this_thread::sleep_for(20ms);
    CHECK(t.timedOut());
}
