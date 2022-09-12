#pragma once

#include <cstdint>  // size_t
#include <unistd.h> // uint8_t

#include "serial_parameters_types.h"

class SerialPolicyUnix {
  public:
    ~SerialPolicyUnix();
    bool openSerial(const char* name, baudrate_t baudrate = BAUDRATE_9600,
                    databits_t databits = DATABITS_8,
                    parity_t parity = PARITY_NONE,
                    stopbits_t stopbits = STOPBITS_1);
    void closeSerial();
    size_t tx(const std::uint8_t* data, const std::size_t data_sz);
    size_t rx(std::uint8_t* data, const std::size_t max_data_sz);

  private:
    int _fd;
};
