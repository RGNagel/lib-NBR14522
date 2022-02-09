#pragma once

inline unsigned char bcd2dec(unsigned char bcd) {
        // e.g. 0x14 -> 14
        return (0x0F & bcd) + 10*((0xF0 & bcd) >> 4);
}

inline unsigned char dec2bcd(unsigned char dec) {
    // e.g. 14 -> 0x14
    unsigned char MSDigit = static_cast<unsigned char>(dec/10);
    unsigned char LSDigit = dec - 10*MSDigit;
    return (MSDigit << 4) + LSDigit;
}

