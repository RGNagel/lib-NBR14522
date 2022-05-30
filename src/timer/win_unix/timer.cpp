#include <chrono>
#include <timer/timer.h>

using clock_type = std::chrono::system_clock;
using moment = std::chrono::time_point<clock_type>;

struct TimerImplementacao {
    moment timeoutAt;
};

Timer::Timer() { this->impl = new TimerImplementacao(); }

Timer::~Timer() { delete this->impl; }

void Timer::setTimeout(unsigned int milliseconds) {
    this->impl->timeoutAt =
        clock_type::now() + std::chrono::milliseconds(milliseconds);
}

bool Timer::timedOut() { return clock_type::now() >= this->impl->timeoutAt; }
