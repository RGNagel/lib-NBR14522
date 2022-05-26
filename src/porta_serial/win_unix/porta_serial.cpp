#include "serialib.h"
#include <porta_serial/porta_serial.h>

struct PortaSerialImplementacao {
    serialib serial;
};

PortaSerial::PortaSerial() { this->impl = new PortaSerialImplementacao(); }

PortaSerial::~PortaSerial() { this->impl->serial.closeDevice(); }

bool PortaSerial::open(const char* name, const unsigned int baudrate) {
    if (this->impl->serial.openDevice(name, baudrate) == 1)
        return true;
    else
        return false;
}

bool PortaSerial::close() {
    this->impl->serial.closeDevice();
    return true;
}

size_t PortaSerial::write(const byte_t* data, const size_t data_sz) {
    if (this->impl->serial.writeBytes(data, data_sz) == 1)
        return data_sz;
    else
        return 0;
}

size_t PortaSerial::read(byte_t* data, const size_t max_data_sz) {
    int toread = this->impl->serial.available();
    if (toread <= 0)
        return 0;

    if (toread > static_cast<int>(max_data_sz))
        toread = max_data_sz;

    int read = this->impl->serial.readBytes(data, toread);
    if (read >= 0)
        return static_cast<size_t>(read);
    else
        return 0;
}