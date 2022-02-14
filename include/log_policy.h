#pragma once
#include <iostream>

class LogPolicyNull {
  public:
    static void log(std::string input) {}
};

class LogPolicyStdout {
  public:
    static void log(std::string input) { std::cout << input; }
};
