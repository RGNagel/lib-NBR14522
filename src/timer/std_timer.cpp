#include <timer.h>
#include <thread>

void Timer::wait(milliseconds_t period) {
        std::this_thread::sleep_for(std::chrono::milliseconds(period));
}