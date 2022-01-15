#pragma once

#include "NBR14522.h"

namespace NBR14522 {

class ILeitor {
  public:
    enum status { SUCESSO = 0, DESCONECTADO, CRC_ERR, TIMED_OUT, MEMORY_OUT };

    virtual void tx(comando_t comando) = 0;
};

} // namespace NBR14522