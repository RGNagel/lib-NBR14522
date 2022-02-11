#pragma once

#include <chrono>

template <typename clock_type = std::chrono::system_clock> class Timer {
  private:
    using moment = std::chrono::time_point<clock_type>;

    moment timeoutAt;

  public:
    void setTimeout(std::chrono::milliseconds duration) {
        timeoutAt = clock_type::now() + duration;
    }

    bool timedOut() { return clock_type::now() >= timeoutAt; }
};
