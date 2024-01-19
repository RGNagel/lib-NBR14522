#pragma once

#include <BCD.h>
#include <vector>

namespace NBR14522 {

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

typedef enum {
    REPOSICAO_DE_DEMANDA,
    VERIFICACAO,
    RECUPERACAO,
    REPOSICAO_DE_DEMANDA_RESUMIDA,
    VERIFICACAO_RESUMIDA,
    RECUPERACAO_RESUMIDA,
    VERIFICACAO_DA_MEMORIA_DE_MASSA
} leitura_padrao_t;

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

} // namespace NBR14522