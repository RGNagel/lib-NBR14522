#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <CRC.h>
#include <NBR14522.h>
#include <iporta.h>
#include <leitor.h>
#include <simulador/medidor.h>
#include <task_scheduler.h>

using namespace NBR14522;

TEST_CASE("leitor com medidor simulado") {

    std::shared_ptr<Simulador::Medidor> medidor =
        std::make_shared<Simulador::Medidor>();

    Leitor leitor(medidor);
    
    comando_t comando;
    // timeout deve acontecer, pois medidor nao foi posto para executar ainda
    CHECK(Leitor::SINCRONIZACAO_FALHOU == leitor.tx(comando, [](resposta_t &rsp){}, 100ms));
    
    // coloca medidor para executar
    std::thread t1(&Simulador::Medidor::run, medidor);

    SUBCASE("0x14") {

        comando.at(0) = 0x14;
        leitor.tx(
            comando, [](resposta_t& rsp) { CHECK(rsp.at(0) == 0x14); }, 2000ms);
    }

    medidor->runStop();
    t1.join();
}
