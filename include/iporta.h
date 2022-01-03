#pragma once

#include <types_local.h>

class IPorta {
  public:
    size_t write(const byte_t* data, const size_t data_sz) {
        return _write(data, data_sz);
    }
    size_t read(byte_t* data, const size_t max_data_sz) {
        return _read(data, max_data_sz);
    }

    virtual ~IPorta() {}

  protected:
    virtual size_t _write(const byte_t* data, const size_t data_sz) = 0;
    virtual size_t _read(byte_t* data, const size_t max_data_sz) = 0;
};
