#pragma once
#include <iostream>

class LogPolicyNull {
  public:
    static void log(const std::string& input) {}
};

class LogPolicyStdout {
  public:
    static void log(const std::string& input) { std::cout << input; }
    // static void log(...) {
    //   fprintf(stdout, __VA_ARGS__);
    // }
};
