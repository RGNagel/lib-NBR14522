#include <CRC.h>

uint16_t CRC16(const byte_t* data, const size_t data_sz) {
    const uint16_t POLY = 0xa001;

    uint16_t crc = 0x0000;

    for (size_t i = 0; i < data_sz; i++) {
        crc ^= data[i];

        size_t j = 8;
        while (j--) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= POLY;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}
