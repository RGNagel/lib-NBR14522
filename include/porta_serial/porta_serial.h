#pragma once

#include <stddef.h>

typedef unsigned char byte_t;

struct PortaSerialImplementacao;

// interface para implementação da porta serial

class PortaSerial {
  public:
    PortaSerial();
    ~PortaSerial();
    bool open(const char* name, const unsigned int baudrate = 9600);
    bool close();
    size_t write(const byte_t* data, const size_t data_sz);
    size_t read(byte_t* data, const size_t max_data_sz);

  private:
    PortaSerialImplementacao* impl;
};
