#pragma once
#include <iostream>

class LogPolicyNull {
  public:
    template <typename... Args>
    static void log(char const* const format, Args const&... args) noexcept {
        // printf(format, args...);
    }
};

class LogPolicyStdout {
  public:
    template <typename... Args>
    static void log(char const* const format, Args const&... args) noexcept {
        printf(format, args...);
    }
};
