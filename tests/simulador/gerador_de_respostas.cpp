#include "doctest/doctest.h"
#include <BCD.h>
#include <CRC.h>
#include <NBR14522.h>
#include <simulador/gerador_de_respostas.h>

using namespace NBR14522;

void _CHECK_CRC(resposta_t& resposta);

TEST_CASE("Gerador") {
    GeradorDeRespostas gerador(2, 2, 30);

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

        uint16_t qty = gerador.gerar(comando);
        CHECK(qty == 30);

        for (uint16_t i = 0; i < qty; i++) {
            CHECK(gerador.getNextResposta(resposta));
            CHECK(resposta.at(0) == 0x52);
            _CHECK_CRC(resposta);

            byte_t octeto006 = resposta.at(5);
            byte_t octeto007 = resposta.at(6);

            uint16_t numeroDoBloco =
                100 * bcd2dec(octeto006 & 0x0F) + bcd2dec(octeto007);

            CHECK(numeroDoBloco == (i + 1));

            // ultimo bloco?
            if (i == (qty - 1))
                CHECK((octeto006 & 0xF0) == 0x10);
            else
                CHECK((octeto006 & 0xF0) == 0x00);
        }
    }
}

void _CHECK_CRC(resposta_t& resposta) {
    uint16_t crcRes = getCRC(resposta);
    uint16_t crcCalc = CRC16(resposta.data(), resposta.size() - 2);
    CHECK(crcRes == crcCalc);
}
