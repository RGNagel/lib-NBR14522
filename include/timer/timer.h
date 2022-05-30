#pragma once

struct TimerImplementacao;

class Timer {
  public:
    Timer();
    ~Timer();
    void setTimeout(unsigned int milliseconds);
    bool timedOut();

  private:
    struct TimerImplementacao* impl;
};
