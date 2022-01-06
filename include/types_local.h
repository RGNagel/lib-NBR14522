#pragma once

// TODO: mudar o nome deste header para NBR14522.h

#include <array>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef unsigned char byte_t;

namespace NBR14522 {

constexpr size_t COMANDO_SZ = 66;
constexpr size_t RESPOSTA_SZ = 258;

typedef std::array<byte_t, COMANDO_SZ> comando_t;
typedef std::array<byte_t, RESPOSTA_SZ> resposta_t;
typedef std::array<byte_t, 3> leitor_num_serie_t;
typedef std::array<byte_t, 4> medidor_num_serie_t;

template <size_t S> uint16_t getCRC(std::array<byte_t, S>& cmd_ou_rsp);
inline medidor_num_serie_t getNumSerieMedidor(resposta_t& resposta);
inline medidor_num_serie_t getNumSerieMedidor(resposta_t& resposta);

inline bool isValidCodeCommand(byte_t code);

template <size_t S> uint16_t getCRC(std::array<byte_t, S>& cmd_ou_rsp) {
    byte_t msb = cmd_ou_rsp.at(S - 1);
    byte_t lsb = cmd_ou_rsp.at(S - 2);

    return static_cast<uint16_t>((msb << 8) + lsb);
}

inline medidor_num_serie_t getNumSerieMedidor(resposta_t& resposta) {
    medidor_num_serie_t num = {resposta.at(1), resposta.at(2), resposta.at(3),
                               resposta.at(4)};
    return num;
}

inline bool isValidCodeCommand(byte_t code) {
    // TODO: realizar verificação em O(1) (lookup table?)

    static const byte_t codes[] = {0x14, 0x20, 0x21, 0x22, 0x51, 0x23, 0x24,
                                   0x41, 0x44, 0x42, 0x43, 0x45, 0x46, 0x25,
                                   0x26, 0x27, 0x52, 0x28, 0x80};

    for (size_t i = 0; i < sizeof(codes); i++)
        if (code == codes[i])
            return true;

    return false;
}

enum Sinalizador { ENQ = 0x05, ACK = 0x06, NAK = 0x15, WAIT = 0x10 };

constexpr uint32_t BAUDRATE = 9600;

// TCAR (caracter): tempo de transmissão de um caracter (10 bits: 1 start, 8
// dados, 1 stop): 10/9600baud = ~1,042 ms
constexpr uint32_t TCAR_MSEC = 1;

// TENTCAR: tempo entre os start bits de dois caracteres consecutivos de um
// mesmo COMANDO ou RESPOSTA TMAXCAR: tempo máximo que TENTCAR pode ter
constexpr uint32_t TMAXCAR_MSEC = TCAR_MSEC + 5;

// TREV (reversão): tempo entre inicio do start bit do último caracter recebido
// e o inicio do start bit do primeiro caracter a transmitir

constexpr uint32_t TMINREV_MSEC = TCAR_MSEC + 1;

// TMAXENQ (tempo máximo entre ENQs subsequentes) = TMINREV + 500ms = ~502ms
constexpr uint32_t TMAXENQ_MSEC = TMINREV_MSEC + 500;
// TMINENQ (tempo mínimo entre ENQs subsequentes) = TMINREV + 20ms  = ~22ms
constexpr uint32_t TMINENQ_MSEC = TMINREV_MSEC + 20;
// tempo médio entre ENQs subsequentes (não é definido na norma)
constexpr uint32_t TAVGENQ_MSEC = (TMAXENQ_MSEC + TMINENQ_MSEC) >> 1;

// TSINC (sincronização): tempo entre inicio do start bit de um ENQ (enviado
// pelo medidor) e o inicio do start bit do primeiro caractere enviado
// subsequentemente pelo leitor

// obs.: somente TSINC máximo (TMAXSINC) é definido pela norma.
constexpr uint32_t TMAXSINC_MSEC = TMINREV_MSEC + 10;

// Trsp (tempo de resposta): tempo entre inicio do start bit do ultimo caracter
// de COMANDO ou RESPOSTA ou SINALIZADOR e o início do start bit do primeiro
// caractere subsequente recebido

// TMAXRSP: tempo máximo que Trsp pode ter. Tmaxsinc é uma exceção a esta
// especificação.
constexpr uint32_t TMAXRSP_MSEC = TMINREV_MSEC + 500;

enum Regra {
    // numero maximo de NAK para um mesmo bloco
    MAX_BLOCO_NAK = 7,
    // numero maximo de WAIT para um mesmo bloco
    MAX_BLOCO_WAIT = 12,
    // numero maximo de envio de um comando que nao obtém resposta do medidor
    MAX_COMANDO_SEM_RESPOSTA = 7,
    // tempo máximo entre dois WAIT subsequentes (tempo máximo sem WAIT)
    TSEMWAIT_SEC = 305,
};

} // namespace NBR14522
