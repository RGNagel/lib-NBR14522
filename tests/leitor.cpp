#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <CRC.h>
#include <NBR14522.h>
#include <iporta.h>
#include <leitor.h>
#include <simulador/medidor.h>
#include <task_scheduler.h>
#include <thread>

using namespace NBR14522;

TEST_CASE("leitor com medidor simulado") {

    std::shared_ptr<Simulador::Medidor> medidor =
        std::make_shared<Simulador::Medidor>();

    Leitor leitor(medidor);
    std::vector<resposta_t> respostas;
    comando_t comando;
    Leitor::estado_t estado;

    // timeout deve acontecer, pois medidor nao foi posto para executar ainda
    estado = leitor.tx(
        comando, [](const resposta_t& rsp) {}, 100ms);
    CHECK(estado == Leitor::SINCRONIZACAO_FALHOU);

    // coloca medidor para executar
    std::thread t1(&Simulador::Medidor::run, medidor);

    SUBCASE("0x14") {

        comando.at(0) = 0x14;
        estado = leitor.tx(
            comando,
            [&](const resposta_t& rsp) {
                CHECK(1 == 0);
                respostas.push_back(rsp);
                //
            },
            2000ms);

        CHECK(estado == Leitor::RESPOSTA_RECEBIDA);

        // CHECK(respostas.size() == 1);
        // CHECK(respostas.at(0).at(0) == 0x14);
    }

    medidor->runStop();
    t1.join();
}
