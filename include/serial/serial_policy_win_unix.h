#pragma once

#include "serialib.h"
#include <porta_serial/porta_serial.h>

class SerialPolicyWinUnix {
  public:
    ~SerialPolicyWinUnix() { _serial.closeDevice(); }
    bool open(const char* name, const unsigned int baudrate) {
        if (_serial.openDevice(name, baudrate) == 1)
            return true;
        else
            return false;
    }
    void close() { _serial.closeDevice(); }

    size_t tx(const byte_t* data, const size_t data_sz) {
        if (_serial.writeBytes(data, data_sz) == 1)
            return data_sz;
        else
            return 0;
    }

    size_t rx(byte_t* data, const size_t max_data_sz) {
        int toread = _serial.available();
        if (toread <= 0)
            return 0;

        if (toread > static_cast<int>(max_data_sz))
            toread = max_data_sz;

        int read = _serial.readBytes(data, toread);
        if (read >= 0)
            return static_cast<size_t>(read);
        else
            return 0;
    }

  private:
    serialib _serial;
};
