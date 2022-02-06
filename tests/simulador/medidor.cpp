#include "doctest/doctest.h"
#include <CRC.h>
#include <NBR14522.h>
#include <array>
#include <functional>
#include <iporta.h>
#include <ring_buffer.h>
#include <simulador/gerador_de_respostas.h>
#include <simulador/medidor.h>
#include <string.h>
#include <task_scheduler.h>
#include <thread>

using namespace NBR14522;

// TEST_CASE("Medidor Simulado" * doctest::skip()) {
TEST_CASE("Medidor Simulado") {
    medidor_num_serie_t serie = {5, 6, 7, 8};
    Simulador::Medidor medidor(serie);

    std::thread t1(&Simulador::Medidor::run, &medidor);

    std::this_thread::sleep_for(5ms);

    std::array<byte_t, 258> data;
    data.fill(0x00);

    comando_t cmd;
    resposta_t rsp;

    cmd.fill(0x00);

    // read all ENQs and empty medidor read buffer
    size_t read = medidor.read(data.data(), data.size());
    for (size_t i = 0; i < read; i++)
        CHECK(data.at(i) == ENQ);

    SUBCASE("0x14 valido") {
        cmd.at(0) = 0x14;
        setCRC(cmd, CRC16(cmd.data(), cmd.size() - 2));

        // wait for next ENQ and then send cmd

        byte_t enq = 0;
        medidor.read(&enq, 1);
        while (enq != ENQ) {
            std::this_thread::sleep_for(5ms);
            medidor.read(&enq, 1);
        }

        medidor.write(cmd.data(), cmd.size());

        std::this_thread::sleep_for(
            std::chrono::milliseconds(TMAXRSP_MSEC / 2));

        // read resposta
        read = medidor.read(rsp.data(), rsp.size());
        CHECK(read == rsp.size());
        CHECK(rsp.at(0) == 0x14);
        CHECK(rsp.at(1) == serie.at(0));
        CHECK(rsp.at(2) == serie.at(1));
        CHECK(rsp.at(3) == serie.at(2));
        CHECK(rsp.at(4) == serie.at(3));
        CHECK(CRC16(rsp.data(), rsp.size() - 2) == getCRC(rsp));
    }

    SUBCASE("comando composto") {
        cmd.at(0) = 0x52;
        setCRC(cmd, CRC16(cmd.data(), cmd.size() - 2));

        // wait for next ENQ and then send cmd

        byte_t enq = 0;
        medidor.read(&enq, 1);
        while (enq != ENQ) {
            std::this_thread::sleep_for(5ms);
            medidor.read(&enq, 1);
        }

        medidor.write(cmd.data(), cmd.size());

        std::this_thread::sleep_for(
            std::chrono::milliseconds(TMAXRSP_MSEC / 2));

        // read resposta
        read = medidor.read(rsp.data(), rsp.size());
        CHECK(read == rsp.size());

        while (read == rsp.size()) {
            CHECK(rsp.at(0) == 0x52);
            CHECK(rsp.at(1) == serie.at(0));
            CHECK(rsp.at(2) == serie.at(1));
            CHECK(rsp.at(3) == serie.at(2));
            CHECK(rsp.at(4) == serie.at(3));
            CHECK(CRC16(rsp.data(), rsp.size() - 2) == getCRC(rsp));

            // devemos enviar um ACk
            byte_t ACK = static_cast<byte_t>(ACK);
            medidor.write(&ACK, 1);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(TMAXRSP_MSEC / 2));

            // read next resposta
            read = medidor.read(rsp.data(), rsp.size());
        }
    }

    medidor.runStop();
    t1.join();
}
