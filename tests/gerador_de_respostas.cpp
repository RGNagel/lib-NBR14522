#include "doctest/doctest.h"
#include <CRC.h>
#include <gerador_de_respostas.h>
#include <NBR14522.h>
using namespace NBR14522;

void _CHECK_CRC(resposta_t& resposta) {
    uint16_t crcRes = getCRC(resposta);
    uint16_t crcCalc = CRC16(resposta.data(), resposta.size() - 2);
    CHECK(crcRes == crcCalc);
}

TEST_CASE("Gerador") {
    GeradorDeRespostas gerador;

    comando_t comando;
    resposta_t resposta;

    SUBCASE("0x14") {
        comando.at(0) = 0x14;
        CHECK(1 == gerador.gerar(comando));
        CHECK(gerador.getNextResposta(resposta));
        CHECK(resposta.at(0) == 0x14);
        _CHECK_CRC(resposta);
    }

    SUBCASE("Composto 0x52") {
        comando.at(0) = 0x52;

        CHECK(gerador.gerar(comando) > 1);
        CHECK(gerador.getNextResposta(resposta));
        CHECK(resposta.at(0) == 0x52);
        _CHECK_CRC(resposta);
        CHECK(gerador.getNextResposta(resposta));
        CHECK(resposta.at(0) == 0x52);
        _CHECK_CRC(resposta);
    }
}
