#pragma once

#include <iporta.h>
#include <serialib.h>

class PortaSerial : public IPorta {
  private:
    serialib serial;

  public:
    PortaSerial() : serial() {}

    bool open(const std::string& porta, const int baudrate = 9600) {
        if (serial.openDevice(porta.c_str(), baudrate) == 1)
            return true;
        else
            return false;
    }

    void close() { serial.closeDevice(); }

    size_t _write(const byte_t* data, const size_t data_sz) {
        if (1 == serial.writeBytes(data, data_sz))
            return data_sz;
        else
            return 0;
    }
    size_t _read(byte_t* data, const size_t max_data_sz) {
        int toread = serial.available();
        if (toread <= 0)
            return 0;

        if (toread > static_cast<int>(max_data_sz))
            toread = max_data_sz;

        int read = serial.readBytes(data, toread);
        if (read >= 0)
            return static_cast<size_t>(read);
        else
            return 0;
    }
};