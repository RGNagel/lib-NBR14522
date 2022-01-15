#pragma once

#include <NBR14522.h>

// NBR14522 uses CRC16 (X16 + X15 + X2 + 1) i.e. 0x8005 (MSB-first code) or
// 0xA001 (LSB-first code)

uint16_t CRC16(const byte_t* data, const size_t data_sz);
