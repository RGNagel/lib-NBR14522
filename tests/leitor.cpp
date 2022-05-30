#include "doctest/doctest.h"
#include <CRC.h>
#include <NBR14522.h>
#include <leitor.h>
#include <memory>
#include <porta_serial/porta_serial.h>
#include <ring_buffer.h>

using namespace NBR14522;

static void waitFor(unsigned int milliseconds) {
    Timer timer;
    timer.setTimeout(milliseconds);
    while (!timer.timedOut())
        ;
}

// porta serial fake para testes

static RingBuffer<byte_t, 300> dados_para_o_leitor;
static RingBuffer<byte_t, 300> dados_para_o_medidor;

struct PortaSerialImplementacao {};
PortaSerial::PortaSerial() {}
PortaSerial::~PortaSerial() {}
bool PortaSerial::open(const char* name, const unsigned int baudrate){};
bool PortaSerial::close(){};
size_t PortaSerial::write(const byte_t* data, const size_t data_sz) {
    for (size_t i = 0; i < data_sz; i++)
        dados_para_o_medidor.write(data[i]);
    return data_sz;
}
size_t PortaSerial::read(byte_t* data, const size_t max_data_sz) {
    size_t toread = dados_para_o_leitor.toread();
    size_t sz = max_data_sz >= toread ? toread : max_data_sz;
    for (size_t i = 0; i < sz; i++)
        data[i] = dados_para_o_leitor.read();
    return sz;
}

// fim da porta serial fake

TEST_CASE("Leitor") {

    // esvazia ring buffers
    while (dados_para_o_medidor.toread())
        dados_para_o_medidor.read();
    while (dados_para_o_leitor.toread())
        dados_para_o_leitor.read();

    sptr<PortaSerial> porta = std::make_shared<PortaSerial>();

    using Leitor = Leitor<>;

    Leitor leitor(porta);

    const byte_t enq = ENQ;
    const byte_t nak = NAK;

    comando_t cmd_transmitido, cmd_recebido_pelo_medidor;
    cmd_transmitido.fill(0xAA);
    cmd_transmitido.at(0) = 0x14;
    setCRC(cmd_transmitido,
           CRC16(cmd_transmitido.data(), cmd_transmitido.size() - 2));
    leitor.setComando(cmd_transmitido);

    CHECK(leitor.processaEstado() == Leitor::estado_t::Desconectado);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Desconectado);
    waitFor(1000);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Desconectado);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Desconectado);

    SUBCASE("Timed out TMAXENQ") {
        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        waitFor(TMAXENQ_MSEC);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Desconectado);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Desconectado);
    }

    SUBCASE("limite de transmissões sem respostas excedido") {
        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        for (int i = 1; i < MAX_COMANDO_SEM_RESPOSTA; i++) {
            // verifica que o comando foi enviado pelo leitor e está correto
            REQUIRE(dados_para_o_medidor.toread() == COMANDO_SZ);
            for (size_t j = 0; j < COMANDO_SZ; j++)
                cmd_recebido_pelo_medidor.at(j) = dados_para_o_medidor.read();
            CHECK(cmd_transmitido == cmd_recebido_pelo_medidor);
            // aguarda timeout TMAXRSP
            waitFor(TMAXRSP_MSEC);
            // processa e retransmite o comando
            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            // verifica estado e vars. após retransmissão
            CHECK(leitor.counterSemResposta() == i);
            CHECK(leitor.status() == Leitor::status_t::Processando);
        }

        // ultima tentativa
        // verifica que o comando foi enviado pelo leitor e está correto
        REQUIRE(dados_para_o_medidor.toread() == COMANDO_SZ);
        for (size_t j = 0; j < COMANDO_SZ; j++)
            cmd_recebido_pelo_medidor.at(j) = dados_para_o_medidor.read();
        CHECK(cmd_transmitido == cmd_recebido_pelo_medidor);
        // aguarda timeout TMAXRSP
        waitFor(TMAXRSP_MSEC);

        // processa e nao retransmite comando, pois estourou o limite de
        // retransmissões
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);

        CHECK(leitor.status() ==
              Leitor::status_t::ErroLimiteDeTransmissoesSemRespostas);
    }

    SUBCASE("limite de NAKs recebidos excedido") {
        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        dados_para_o_leitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        for (int i = 1; i < MAX_BLOCO_NAK; i++) {
            dados_para_o_leitor.write(nak);
            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            CHECK(leitor.counterNakRecebido() == i);
        }

        dados_para_o_leitor.write(nak);

        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);

        CHECK(leitor.status() == Leitor::status_t::ErroLimiteDeNAKsRecebidos);
    }

    SUBCASE("limite de NAKs transmitidos excedido (CRC das respostas sempre "
            "com erro)") {
        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        dados_para_o_leitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        for (int i = 1; i < MAX_BLOCO_NAK; i++) {
            // envia 1o byte da resposta (código do comando)
            dados_para_o_leitor.write(cmd_transmitido.at(0));

            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);

            // envia restante do comando (dados errados para o CRC nao bater)
            for (size_t j = 1; j < RESPOSTA_SZ; j++)
                dados_para_o_leitor.write(0xDE);

            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            CHECK(leitor.counterNakTransmitido() == i);
        }

        // envia 1o byte da resposta (código do comando)
        dados_para_o_leitor.write(cmd_transmitido.at(0));

        CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);

        // envia restante do comando (dados errados para o CRC nao bater)
        for (size_t j = 1; j < RESPOSTA_SZ; j++)
            dados_para_o_leitor.write(0xDE);

        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);

        CHECK(leitor.status() ==
              Leitor::status_t::ErroLimiteDeNAKsTransmitidos);
    }

    SUBCASE("Resposta simples válida recebida") {
        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        dados_para_o_leitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        // gera resposta válida
        resposta_t resposta;
        resposta.at(0) = cmd_transmitido.at(0);
        NBR14522::setCRC(resposta, CRC16(resposta.data(), resposta.size() - 2));

        for (size_t i = 0; i < (NBR14522::RESPOSTA_SZ - 1); i++) {
            dados_para_o_leitor.write(resposta.at(i));
            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
        }

        dados_para_o_leitor.write(resposta.at(NBR14522::RESPOSTA_SZ - 1));
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.status() == Leitor::status_t::Sucesso);

        resposta_t resposta_recebida_pelo_leitor = leitor.resposta();
        CHECK(resposta_recebida_pelo_leitor == resposta);
    }

    SUBCASE("Resposta composta válida recebida") {

        cmd_transmitido.at(0) = 0x26; // composto

        setCRC(cmd_transmitido,
               CRC16(cmd_transmitido.data(), cmd_transmitido.size() - 2));
        leitor.setComando(cmd_transmitido);

        dados_para_o_leitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        dados_para_o_leitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        REQUIRE(dados_para_o_medidor.toread() == COMANDO_SZ);
        for (size_t j = 0; j < COMANDO_SZ; j++)
            cmd_recebido_pelo_medidor.at(j) = dados_para_o_medidor.read();
        CHECK(cmd_transmitido == cmd_recebido_pelo_medidor);

        resposta_t rsp;

        // resposta intermediaria 1

        rsp.fill(0x00);
        rsp.at(0) = 0x26;
        setCRC(rsp, CRC16(rsp.data(), rsp.size() - 2));

        for (size_t j = 0; j < RESPOSTA_SZ; j++) {
            dados_para_o_leitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(dados_para_o_medidor.read() == ACK);
        CHECK(leitor.status() == Leitor::status_t::Processando);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.resposta() == rsp);

        // resposta intermediaria 2

        rsp.fill(0x00);
        rsp.at(0) = 0x26;
        rsp.at(47) = 0xAB; // só pra ser diferente da resposta 1
        setCRC(rsp, CRC16(rsp.data(), rsp.size() - 2));

        for (size_t j = 0; j < RESPOSTA_SZ; j++) {
            dados_para_o_leitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(dados_para_o_medidor.read() == ACK);
        CHECK(leitor.status() == Leitor::status_t::Processando);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.resposta() == rsp);

        // resposta intermediaria 3

        rsp.fill(0x00);
        rsp.at(0) = 0x26;
        rsp.at(22) = 0xCD; // só pra ser diferente da resposta 1 e 2
        setCRC(rsp, CRC16(rsp.data(), rsp.size() - 2));

        for (size_t j = 0; j < RESPOSTA_SZ; j++) {
            dados_para_o_leitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(dados_para_o_medidor.read() == ACK);
        CHECK(leitor.status() == Leitor::status_t::Processando);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.processaEstado() ==
              Leitor::estado_t::RespostaCompostaParcialRecebida);
        CHECK(leitor.resposta() == rsp);

        // resposta final

        rsp.fill(0x00);
        rsp.at(0) = 0x26;
        rsp.at(5) = 0x10; // indica que é resposta final
        setCRC(rsp, CRC16(rsp.data(), rsp.size() - 2));

        for (size_t j = 0; j < RESPOSTA_SZ; j++) {
            dados_para_o_leitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(dados_para_o_medidor.read() == ACK);
        CHECK(leitor.status() == Leitor::status_t::Sucesso);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.resposta() == rsp);
    }
}
