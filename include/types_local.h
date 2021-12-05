#pragma once

#include <array>

typedef unsigned char byte_t;

namespace NBR14522 {

typedef std::array<byte_t, 66> comando_t;
typedef std::array<byte_t, 258> resposta_t;
typedef std::array<byte_t, 3> leitor_num_serie_t;

enum Sinalizador {
    ENQ = 0x05,
    ACK = 0x06,
    NAK = 0x15,
    WAIT = 0x10
};

enum Regra {
    // numero maximo de NAK para um mesmo bloco
    MAX_BLOCO_NAK = 7,
    // numero maximo de WAIT para um mesmo bloco
    MAX_BLOCO_WAIT = 12,
    // numero maximo de envio de um comando que nao obtém resposta do medidor
    MAX_COMANDO_SEM_RESPOSTA = 7,
    // tempo máximo entre dois WAIT subsequentes (tempo máximo sem WAIT)
    TSEMWAIT_SECONDS = 305
};

} // NBR14522