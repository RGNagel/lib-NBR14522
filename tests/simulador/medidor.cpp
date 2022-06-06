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
#include <thread>
#include <timer.h>

using namespace NBR14522;
using namespace std::literals;

using Medidor = Simulador::Medidor<>;

// TEST_CASE("Medidor Simulado" * doctest::skip()) {
TEST_CASE("Medidor Simulado") {
    medidor_num_serie_t serie = {5, 6, 7, 8};

    Medidor medidor(serie);

    std::thread t1(&Medidor::run, &medidor);
    std::this_thread::sleep_for(5ms);

    comando_t cmd;
    resposta_t rsp;

    Timer<> timer;

    cmd.fill(0x00);

    // wait for next ENQ and then send cmd
    byte_t enq = 0;
    timer.setTimeout(std::chrono::milliseconds(TMAXENQ_MSEC));
    while (!timer.timedOut()) {
        if (medidor.read(&enq, 1)) {
            REQUIRE(enq == ENQ);
            break;
        }
    }
    REQUIRE(!timer.timedOut());

    SUBCASE("Single NAK") {
        cmd.at(0) = 0x14;
        setCRC(cmd, 0xDEAD); // wrong CRC
        medidor.write(cmd.data(), cmd.size());

        timer.setTimeout(std::chrono::milliseconds(TMAXRSP_MSEC));
        while (!timer.timedOut()) {
            byte_t nak = 0;
            if (medidor.read(&nak, 1) == 1) {
                CHECK(nak == NAK);
                break;
            }
        }
        CHECK(!timer.timedOut());
    }

    SUBCASE("force all NAKs") {
        cmd.at(0) = 0x14;
        setCRC(cmd, 0xDEAD); // wrong CRC

        for (auto i = 0; i < MAX_BLOCO_NAK; i++) {
            medidor.write(cmd.data(), cmd.size());

            timer.setTimeout(std::chrono::milliseconds(TMAXRSP_MSEC));

            while (!timer.timedOut()) {
                byte_t nak = 0;
                if (medidor.read(&nak, 1) == 1) {
                    CHECK(nak == NAK);
                    break;
                }
            }
            CHECK(!timer.timedOut());
            // TODO sem esse delay TMINREV esse teste nao passa. Provavelemnte
            // pq o medidor tá trabalhando com os valores máximos de timeout
            std::this_thread::sleep_for(
                std::chrono::milliseconds(TMINREV_MSEC));
        }

        // medidor ja mandou o máximo de NAKs, nao deve mais envia-los
        medidor.write(cmd.data(), cmd.size());
        timer.setTimeout(std::chrono::milliseconds(TMAXRSP_MSEC));

        while (!timer.timedOut()) {
            enq = 0;
            if (medidor.read(&enq, 1) == 1) {
                CHECK(enq == ENQ);
                break;
            }
        }
        CHECK(!timer.timedOut());
    }

    SUBCASE("0x14 valido") {
        cmd.at(0) = 0x14;
        setCRC(cmd, CRC16(cmd.data(), cmd.size() - 2));

        medidor.write(cmd.data(), cmd.size());

        timer.setTimeout(std::chrono::milliseconds(TMAXRSP_MSEC));

        size_t totalRead = 0;
        while (!timer.timedOut()) {
            size_t read = medidor.read(&rsp.at(totalRead), rsp.size());
            totalRead += read;
            if (totalRead >= rsp.size())
                break;
        }
        CHECK(!timer.timedOut());

        byte_t ack = ACK;
        medidor.write(&ack, 1);

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

        medidor.write(cmd.data(), cmd.size());

        timer.setTimeout(std::chrono::milliseconds(TMAXRSP_MSEC));
        size_t read = 0;
        size_t countRespostas = 0;

        while (!timer.timedOut()) {
            read += medidor.read(&rsp[read], rsp.size() - read);

            if (read >= rsp.size()) {
                CHECK(rsp.at(0) == 0x52);
                CHECK(rsp.at(1) == serie.at(0));
                CHECK(rsp.at(2) == serie.at(1));
                CHECK(rsp.at(3) == serie.at(2));
                CHECK(rsp.at(4) == serie.at(3));
                CHECK(CRC16(rsp.data(), rsp.size() - 2) == getCRC(rsp));

                countRespostas++;

                byte_t ack = ACK;
                medidor.write(&ack, 1);

                read = 0;
                timer.setTimeout(std::chrono::milliseconds(TMAXRSP_MSEC));

                if (isLastRespostaOfComposed(rsp))
                    break;
            }
        }

        CHECK(!timer.timedOut());
        CHECK(countRespostas > 1);
    }

    medidor.runStop();
    t1.join();
}
