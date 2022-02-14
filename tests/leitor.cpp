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
using namespace std::literals;

using MedidorSimulado = Simulador::Medidor<>;

// TEST_CASE("leitor com medidor simulado" * doctest::skip()) {
TEST_CASE("leitor com medidor simulado") {
    medidor_num_serie_t serie = {5, 6, 7, 8};
    std::shared_ptr<MedidorSimulado> medidor =
        std::make_shared<MedidorSimulado>(serie);

    Leitor<> leitor(medidor);
    std::vector<resposta_t> respostas;
    comando_t comando;
    Leitor<>::estado_t estado;

    // timeout deve acontecer, pois medidor nao foi posto para executar ainda
    estado = leitor.read(
        comando, [](const resposta_t& rsp) {}, 100ms);
    REQUIRE(estado == Leitor<>::ERR_SINC_TEMPO_EXCEDIDO);

    // coloca medidor para executar
    std::thread t1(&MedidorSimulado::run, medidor);
    std::this_thread::sleep_for(5ms);

    Timer<> timer;

    // verifica se ENQ está sendo enviado pelo medidor
    timer.setTimeout(std::chrono::milliseconds(2 * TMAXSINC_MSEC));
    byte_t enq = 0;
    while (!timer.timedOut()) {
        if (medidor->read(&enq, 1) == 1)
            break;
    }
    REQUIRE(enq == ENQ);
    REQUIRE(!timer.timedOut());

    SUBCASE("0x14") {

        comando.at(0) = 0x14;
        estado = leitor.read(
            comando, [&](const resposta_t& rsp) { respostas.push_back(rsp); },
            1000ms);

        CHECK(estado == Leitor<>::RSP_RECEBIDA);
        CHECK(respostas.size() == 1);
        CHECK(respostas.at(0).at(0) == 0x14);
    }

    medidor->runStop();
    t1.join();
}
