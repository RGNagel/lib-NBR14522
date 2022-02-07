#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <NBR14522.h>
#include <iporta.h>
#include <CRC.h>
#include <leitor.h>
#include <task_scheduler.h>
#include <simulador/medidor.h>

using namespace NBR14522;
// using std::literals;

TEST_CASE("leitor com medidor simulado") {

    std::shared_ptr<Simulador::Medidor> porta;
    Leitor leitor(porta);

    comando_t comando;
    comando.at(0) = 0x14;
    leitor.tx(comando, [](resposta_t &rsp) {
        // CHECK();
    }, 2000ms);

}
