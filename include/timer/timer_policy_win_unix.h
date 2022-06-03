#pragma once
#include <chrono>

using clock_type = std::chrono::system_clock;
using moment = std::chrono::time_point<clock_type>;

class TimerPolicyWinUnix {
  public:
    void setTimeout(unsigned int milliseconds) {
        _deadline = clock_type::now() + std::chrono::milliseconds(milliseconds);
    }
    bool timedOut() { return clock_type::now() >= _deadline; }

  private:
    moment _deadline;
};
