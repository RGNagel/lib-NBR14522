#pragma once

#include <stdint.h>

typedef uint32_t milliseconds_t;

namespace Timer {
    void wait(milliseconds_t period);
}