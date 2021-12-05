#pragma once

#include <stdio.h>

#include <types_local.h>

class IPorta {
    public:
    virtual size_t write(const byte_t *data, const size_t data_sz) = 0;
    virtual size_t read(byte_t *data, const size_t max_data_sz);
    virtual size_t available() = 0;
};

