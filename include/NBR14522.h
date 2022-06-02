#pragma once

#include <BCD.h>
#include <array>
#include <vector>

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

typedef enum {
    CANAIS_1_2_3 = 0,
    CANAIS_4_5_6,
    CANAIS_7_8_9,
    CANAIS_10_11_12,
    CANAIS_13_14_15,
    CANAIS_16_17_18,
    CANAIS_19_20_21,
    CANAIS_22_23_24,
    CANAIS_25_26_27,
    CANAIS_28_29_30,
    CANAIS_31_32_33,
    CANAIS_34_35_36,
    CANAIS_37_38_39,
    CANAIS_40_41_42,
    CANAIS_43_44_45,
    CANAIS_46_47_48,
    CANAIS_49_50_51,
    CANAIS_52_53_54,
    CANAIS_55_56_57,
    CANAIS_58_59_60,
    CANAIS_61_62_63,
    CANAIS_64_65_66,
    CANAIS_67_68_69,
    CANAIS_70_71_72,
    CANAIS_73_74_75,
    CANAIS_76_77_78,
    CANAIS_79_80_81,
    CANAIS_82_83_84,
    CANAIS_85_86_87,
    CANAIS_88_89_90,
    CANAIS_91_92_93,
    CANAIS_94_95_96,
    CANAIS_97_98_99 = 32
} canal_t;

typedef enum {
    REPOSICAO_DE_DEMANDA,
    VERIFICACAO,
    RECUPERACAO,
    REPOSICAO_DE_DEMANDA_RESUMIDA,
    VERIFICACAO_RESUMIDA,
    RECUPERACAO_RESUMIDA,
    VERIFICACAO_DA_MEMORIA_DE_MASSA
} leitura_padrao_t;

/**
 * Quais informações serão necessárias para uma leitura padrão?
 * começam com os comandos 20, 21, 22, 51 que possuem os parâmetros:
 * 1 - grupo de canais (00,01,02,03,04)
 * 2 - quantidade (em tempo) da memoria de massa a ser lida
 * 3 - tempo em horas ou dias
 *
 */

// typedef struct {
//     uint8_t tempo_mm;
//     bool tempo_mm_unidade; // 0: horas, 1: dias
//     uint8_t grupo_de_canais;
// } parametros_t;

inline void leituraPadrao(std::vector<comando_t>& comandos,
                          const leitura_padrao_t tipo,
                          const canal_t canal = CANAIS_1_2_3) {
    switch (tipo) {
    case REPOSICAO_DE_DEMANDA:
        comandos.push_back(
            {0x20, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x24});
        comandos.push_back({0x25});
        comandos.push_back({0x28});
        comandos.push_back({0x27});
        break;
    case VERIFICACAO:
        comandos.push_back(
            {0x21, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x23});
        comandos.push_back({0x25});
        comandos.push_back({0x28});
        comandos.push_back({0x26});
        break;
    case RECUPERACAO:
        comandos.push_back(
            {0x22, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x24});
        comandos.push_back({0x25});
        comandos.push_back({0x28});
        comandos.push_back({0x27});
        break;
    case REPOSICAO_DE_DEMANDA_RESUMIDA:
        comandos.push_back(
            {0x20, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x24});
        comandos.push_back({0x41});
        comandos.push_back({0x42});
        comandos.push_back({0x43});
        comandos.push_back({0x25});
        comandos.push_back({0x28});
        break;
    case VERIFICACAO_RESUMIDA:
        comandos.push_back(
            {0x21, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x23});
        comandos.push_back({0x44});
        comandos.push_back({0x45});
        comandos.push_back({0x46});
        comandos.push_back({0x25});
        comandos.push_back({0x28});
        break;
    case RECUPERACAO_RESUMIDA:
        comandos.push_back(
            {0x22, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x24});
        comandos.push_back({0x41});
        comandos.push_back({0x42});
        comandos.push_back({0x43});
        comandos.push_back({0x25});
        comandos.push_back({0x28});
        break;
    case VERIFICACAO_DA_MEMORIA_DE_MASSA:
        comandos.push_back(
            {0x51, 0, 0, 0, 0, dec2bcd(static_cast<byte_t>(canal))});
        comandos.push_back({0x80});
        comandos.push_back({0x52});
        break;
    }
}

template <size_t S> uint16_t getCRC(std::array<byte_t, S>& cmd_ou_rsp);
inline medidor_num_serie_t getNumSerieMedidor(resposta_t& resposta);
inline medidor_num_serie_t getNumSerieMedidor(resposta_t& resposta);

inline bool isValidCodeCommand(byte_t code);

template <size_t S> uint16_t getCRC(std::array<byte_t, S>& cmd_ou_rsp) {
    byte_t msb = cmd_ou_rsp.at(S - 1);
    byte_t lsb = cmd_ou_rsp.at(S - 2);

    return static_cast<uint16_t>((msb << 8) + lsb);
}

template <size_t S>
void setCRC(std::array<byte_t, S>& cmd_ou_rsp, uint16_t crc) {
    cmd_ou_rsp.at(cmd_ou_rsp.size() - 1) = (crc & 0xFF00) >> 8;
    cmd_ou_rsp.at(cmd_ou_rsp.size() - 2) = (crc & 0x00FF);
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

inline bool isComposedCodeCommand(byte_t code) {
    if (code == 0x26 || code == 0x27 || code == 0x52)
        return true;
    else
        return false;
}

inline bool isLastRespostaOfComposed(const resposta_t& rsp) {
    return rsp.at(5) & 0x10;
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


constexpr byte_t CodigoInformacaoDeOcorrenciaNoMedidor = 0x40;

} // namespace NBR14522
