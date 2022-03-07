#pragma once

#include <stdint.h>

inline uint8_t bcd2dec(uint8_t bcd) {
    // e.g. 0x14 -> 14
    return (0x0F & bcd) + 10 * ((0xF0 & bcd) >> 4);
}

inline uint8_t dec2bcd(uint8_t dec) {
    // e.g. 14 -> 0x14
    uint8_t MSDigit = static_cast<uint8_t>(dec / 10);
    uint8_t LSDigit = dec - 10 * MSDigit;
    return (MSDigit << 4) + LSDigit;
}
