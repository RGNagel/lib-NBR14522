#pragma once
#include <iostream>

class LogPolicyNull {
  public:
    static void log(const std::string& input) {}
};

class LogPolicyStdout {
  public:
    template <typename... Args>
    static void log(char const* const format, Args const&... args) noexcept {
        printf(format, args...);
    }
};
