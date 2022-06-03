#include "doctest/doctest.h"
#include <CRC.h>
#include <NBR14522.h>
#include <leitor.h>
#include <memory>
#include <ring_buffer.h>
#include <timer/timer_policy_win_unix.h>

using namespace NBR14522;

static void waitFor(unsigned int milliseconds) {
    TimerPolicyWinUnix timer;
    timer.setTimeout(milliseconds);
    while (!timer.timedOut())
        ;
}

// porta serial fake para testes

class SerialPolicyDummy {
  public:
    RingBuffer<byte_t, 300> toLeitor;
    RingBuffer<byte_t, 300> toMedidor;
    SerialPolicyDummy() {
        // esvazia ring buffers
        while (this->toMedidor.toread())
            this->toMedidor.read();
        while (this->toLeitor.toread())
            this->toLeitor.read();
    }
    size_t tx(const byte_t* data, const size_t data_sz) {
        for (size_t i = 0; i < data_sz; i++)
            toMedidor.write(data[i]);
        return data_sz;
    }
    size_t rx(byte_t* data, const size_t max_data_sz) {
        size_t toread = toLeitor.toread();
        size_t sz = max_data_sz >= toread ? toread : max_data_sz;
        for (size_t i = 0; i < sz; i++)
            data[i] = toLeitor.read();
        return sz;
    }
};

TEST_CASE("Leitor") {

    sptr<SerialPolicyDummy> porta = std::make_shared<SerialPolicyDummy>();

    using Leitor = Leitor<TimerPolicyWinUnix, SerialPolicyDummy>;

    Leitor leitor(porta);

    const byte_t enq = ENQ;
    const byte_t nak = NAK;

    comando_t cmd_transmitido, cmd_recebido_pelo_medidor;
    cmd_transmitido.fill(0xAA);
    cmd_transmitido.at(0) = 0x14;
    setCRC(cmd_transmitido,
           CRC16(cmd_transmitido.data(), cmd_transmitido.size() - 2));
    leitor.setComando(cmd_transmitido);

    CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);
    waitFor(1000);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);

    SUBCASE("Timed out TMAXENQ") {
        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        waitFor(TMAXENQ_MSEC);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);
    }

    SUBCASE("limite de transmissões sem respostas excedido") {
        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        for (int i = 1; i < MAX_COMANDO_SEM_RESPOSTA; i++) {
            // verifica que o comando foi enviado pelo leitor e está correto
            REQUIRE(porta->toMedidor.toread() == COMANDO_SZ);
            for (size_t j = 0; j < COMANDO_SZ; j++)
                cmd_recebido_pelo_medidor.at(j) = porta->toMedidor.read();
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
        REQUIRE(porta->toMedidor.toread() == COMANDO_SZ);
        for (size_t j = 0; j < COMANDO_SZ; j++)
            cmd_recebido_pelo_medidor.at(j) = porta->toMedidor.read();
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
        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        porta->toLeitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        for (int i = 1; i < MAX_BLOCO_NAK; i++) {
            porta->toLeitor.write(nak);
            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            CHECK(leitor.counterNakRecebido() == i);
        }

        porta->toLeitor.write(nak);

        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);

        CHECK(leitor.status() == Leitor::status_t::ErroLimiteDeNAKsRecebidos);
    }

    SUBCASE("limite de NAKs transmitidos excedido (CRC das respostas sempre "
            "com erro)") {
        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        porta->toLeitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        for (int i = 1; i < MAX_BLOCO_NAK; i++) {
            // envia 1o byte da resposta (código do comando)
            porta->toLeitor.write(cmd_transmitido.at(0));

            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);

            // envia restante do comando (dados errados para o CRC nao bater)
            for (size_t j = 1; j < RESPOSTA_SZ; j++)
                porta->toLeitor.write(0xDE);

            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::ComandoTransmitido);
            CHECK(leitor.counterNakTransmitido() == i);
        }

        // envia 1o byte da resposta (código do comando)
        porta->toLeitor.write(cmd_transmitido.at(0));

        CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);

        // envia restante do comando (dados errados para o CRC nao bater)
        for (size_t j = 1; j < RESPOSTA_SZ; j++)
            porta->toLeitor.write(0xDE);

        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);

        CHECK(leitor.status() ==
              Leitor::status_t::ErroLimiteDeNAKsTransmitidos);
    }

    SUBCASE("Resposta simples válida recebida") {
        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        porta->toLeitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        // gera resposta válida
        resposta_t resposta;
        resposta.at(0) = cmd_transmitido.at(0);
        NBR14522::setCRC(resposta, CRC16(resposta.data(), resposta.size() - 2));

        for (size_t i = 0; i < (NBR14522::RESPOSTA_SZ - 1); i++) {
            porta->toLeitor.write(resposta.at(i));
            CHECK(leitor.processaEstado() == Leitor::estado_t::CodigoRecebido);
        }

        porta->toLeitor.write(resposta.at(NBR14522::RESPOSTA_SZ - 1));
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

        porta->toLeitor.write(enq);
        CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
        porta->toLeitor.write(enq);

        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        REQUIRE(porta->toMedidor.toread() == COMANDO_SZ);
        for (size_t j = 0; j < COMANDO_SZ; j++)
            cmd_recebido_pelo_medidor.at(j) = porta->toMedidor.read();
        CHECK(cmd_transmitido == cmd_recebido_pelo_medidor);

        resposta_t rsp;

        // resposta intermediaria 1

        rsp.fill(0x00);
        rsp.at(0) = 0x26;
        setCRC(rsp, CRC16(rsp.data(), rsp.size() - 2));

        for (size_t j = 0; j < RESPOSTA_SZ; j++) {
            porta->toLeitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(porta->toMedidor.read() == ACK);
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
            porta->toLeitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(porta->toMedidor.read() == ACK);
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
            porta->toLeitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(porta->toMedidor.read() == ACK);
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
            porta->toLeitor.write(rsp.at(j));
            leitor.processaEstado();
        }
        CHECK(porta->toMedidor.read() == ACK);
        CHECK(leitor.status() == Leitor::status_t::Sucesso);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.resposta() == rsp);
    }
}

class TimerPolicyReduzTempoDeWAIT {
  public:
    static const unsigned int TSEMWAIT_REDUZIDO_MSEC;
    void setTimeout(unsigned int milliseconds) {
        if (milliseconds == (NBR14522::TSEMWAIT_SEC * 1000))
            _deadline = clock_type::now() +
                        std::chrono::milliseconds(TSEMWAIT_REDUZIDO_MSEC);
        else
            _deadline =
                clock_type::now() + std::chrono::milliseconds(milliseconds);
    }
    bool timedOut() { return clock_type::now() >= _deadline; }

  private:
    moment _deadline;
};

const unsigned int TimerPolicyReduzTempoDeWAIT::TSEMWAIT_REDUZIDO_MSEC = 1000;

TEST_CASE("Atraso de sequência (WAIT) com timer dummy") {

    sptr<SerialPolicyDummy> porta = std::make_shared<SerialPolicyDummy>();

    using Leitor = Leitor<TimerPolicyReduzTempoDeWAIT, SerialPolicyDummy>;

    Leitor leitor(porta);

    comando_t cmd_transmitido, cmd_recebido_pelo_medidor;
    cmd_transmitido.fill(0xAA);
    cmd_transmitido.at(0) = 0x14;
    setCRC(cmd_transmitido,
           CRC16(cmd_transmitido.data(), cmd_transmitido.size() - 2));
    leitor.setComando(cmd_transmitido);

    CHECK(leitor.processaEstado() == Leitor::estado_t::Dessincronizado);
    porta->toLeitor.write(ENQ);
    CHECK(leitor.processaEstado() == Leitor::estado_t::Sincronizado);
    porta->toLeitor.write(ENQ);
    CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

    CHECK(porta->toMedidor.toread() == COMANDO_SZ);
    for (size_t i = 0; i < COMANDO_SZ; i++)
        cmd_recebido_pelo_medidor.at(i) = porta->toMedidor.read();
    CHECK(cmd_transmitido == cmd_recebido_pelo_medidor);

    porta->toLeitor.write(WAIT);
    CHECK(leitor.processaEstado() ==
          Leitor::estado_t::AtrasoDeSequenciaRecebido);

    SUBCASE("com retorno à sequência") {
        porta->toLeitor.write(ENQ);
        CHECK(leitor.processaEstado() == Leitor::estado_t::ComandoTransmitido);

        CHECK(porta->toMedidor.toread() == COMANDO_SZ);
        for (size_t i = 0; i < COMANDO_SZ; i++)
            cmd_recebido_pelo_medidor.at(i) = porta->toMedidor.read();
        CHECK(cmd_transmitido == cmd_recebido_pelo_medidor);
    }

    SUBCASE("com falha timeout TSEMWAIT") {
        waitFor(TimerPolicyReduzTempoDeWAIT::TSEMWAIT_REDUZIDO_MSEC);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.status() == leitor.ErroTempoSemWaitEsgotado);
    }

    SUBCASE("com falha MAX_BLOCO_WAIT") {

        for (int i = 1; i < MAX_BLOCO_WAIT; i++) {
            porta->toLeitor.write(WAIT);
            CHECK(leitor.processaEstado() ==
                  Leitor::estado_t::AtrasoDeSequenciaRecebido);
            CHECK(leitor.counterWaitRecebido() == i);
        }

        porta->toLeitor.write(WAIT);
        CHECK(leitor.processaEstado() == Leitor::estado_t::AguardaNovoComando);
        CHECK(leitor.counterWaitRecebido() == MAX_BLOCO_WAIT);
        CHECK(leitor.status() == Leitor::status_t::ErroLimiteDeWaitsRecebidos);
    }
}
