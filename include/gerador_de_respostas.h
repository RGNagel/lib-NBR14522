#pragma once

#include <types_local.h>

using namespace NBR14522;

class GeradorDeRespostas {
  private:
    byte_t _code;

  public:
    // retorna numero de respostas geradas
    uint16_t gerar(comando_t& comando) {

        if (!isValidCodeCommand(comando.at(0)))
            return 0;

        _code = comando.at(0);
    }

    bool getNextResposta(resposta_t* res) {
        // ...
    }
};