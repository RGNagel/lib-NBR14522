#pragma once

#include "types_local.h"
#include <vector>

using namespace NBR14522;

namespace LeituraPadrao {

/**
 * Quais informações serão necessárias para uma leitura padrão?
 * começam com os comandos 20, 21, 22, 51 que possuem os parâmetros:
 * 1 - grupo de canais (00,01,02,03,04)
 * 2 - quantidade (em tempo) da memoria de massa a ser lida
 * 3 - tempo em horas ou dias
 *
 */

typedef struct {
    uint8_t tempo_mm;
    bool tempo_mm_unidade; // 0: horas, 1: dias
    uint8_t grupo_de_canais;
} parametros_t;

typedef enum {
    REPOSICAO_DE_DEMANDA,
    VERIFICACAO,
    RECUPERACAO,
    REPOSICAO_DE_DEMANDA_RESUMIDA,
    VERIFICACAO_RESUMIDA,
    RECUPERACAO_RESUMIDA,
    VERIFICACAO_DA_MEMORIA_DE_MASSA
} tipo_t;

std::vector<comando_t> getComandos(tipo_t tipo, parametros_t params) {
    std::vector<comando_t> cmds;

    // ...

    return cmds;
}

}; // namespace LeituraPadrao
