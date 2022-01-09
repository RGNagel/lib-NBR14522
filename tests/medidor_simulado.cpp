#include "doctest/doctest.h"
#include <CRC.h>
#include <array>
#include <functional>
#include <gerador_de_respostas.h>
#include <iporta.h>
#include <ring_buffer.h>
#include <string.h>
#include <task_scheduler.h>
#include <types_local.h>
#include <thread>

using namespace NBR14522;

/**
 *
 * @brief implementa uma porta serial que se comunica com o medidor simulado.
 * Os dados transmitidos pelo medidor são disponibilizados na função
 * _read() e os dados recebidos na função _write(), de maneira que essa classe
 * possa ser usada como uma porta serial do ponto de vista do leitor.
 *
 */
class MedidorSimulado : public IPorta {
  private:
    RingBuffer<byte_t, 1024> _rb_received;
    RingBuffer<byte_t, 1024> _rb_transmitted;

    TaskScheduler _tasks;
    uint8_t _nak_transmitted = 0;
    uint8_t _nak_received = 0;

    // comando vindo do leitor
    comando_t _comando;
    size_t _comandoIndex = 0;

    // variaveis relacionadas a(s) resposta(s) dadas pelo medidor
    GeradorDeRespostas _gerador;
    uint16_t _quantidadeDeRespostas;
    resposta_t _nextResposta;

    enum {
        CONECTADO,
        COMANDO_RECEBIDO,
        NAK_ENVIADO,
        RESPOSTA_ENVIADA
    } _state = CONECTADO;

    void _flush(RingBuffer<byte_t, 1024>& rb) {
        while (rb.toread() >= 1)
            rb.read();
    }

    void _comandoRecebido() {

        uint16_t crcReceived = NBR14522::getCRC(_comando);
        uint16_t crcCalculated = CRC16(_comando.data(), _comando.size() - 2);

        if (crcReceived != crcCalculated) {
            // se CRC com erro, o medidor deve enviar um NAK para o
            // leitor e aguardar novo COMANDO. O máximo de NAKs enviados
            // para um mesmo comando é 7.

            if (_nak_transmitted >= MAX_BLOCO_NAK) {
                // quebra de sequencia o medidor envia o ENQ
                _nak_transmitted = 0;
                // TODO (MAYBE): calcular valor do próximo ENQ levando em
                // consideração o tempo de leitura do comando
                _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this),
                               TMINENQ_MSEC);
            } else {
                _rb_transmitted.write(NBR14522::NAK);
                _nak_transmitted++;
                _state = NAK_ENVIADO;
                _tasks.addTask(
                    std::bind(&MedidorSimulado::timedOutTMAXRSP, this),
                    TMAXRSP_MSEC);
            }
        }
        // verifica se o comando recebido existe e/ou é valido
        else if (!NBR14522::isValidCodeCommand(_comando.at(0))) {
            // a norma não define qual deve ser o comportamento do
            // medidor após receber um comando inválido mas com CRC OK.
            // vamos quebrar a sessão.

            // TODO (MAYBE): calcular valor do próximo ENQ levando em
            // consideração o tempo de leitura do comando
            _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this),
                           TMINENQ_MSEC);
        }
        // comando recebido OK
        else {
            // TODO ...
            _quantidadeDeRespostas = _gerador.gerar(_comando);

            _gerador.getNextResposta(_nextResposta);
            _sendResposta(_nextResposta);
            _quantidadeDeRespostas--;
        }
    }

    void _sendResposta(const resposta_t& resposta) {
        // garante que nenhum lixo esteja presente se nao pode ser confundido
        // com o ACK/NAK esperado
        _flush(_rb_received);

        _nak_received = 0;

        for (size_t i = 0; i < resposta.size(); i++)
            _rb_transmitted.write(resposta.at(i));

        _tasks.addTask(std::bind(&MedidorSimulado::timedOutTMAXRSP, this),
                       TMAXRSP_MSEC);
        _state = RESPOSTA_ENVIADA;
    }

    void _readNextPieceOfComando() {
        if (_rb_received.toread() <= 0) {
            // TODO (MAYBE): calcular valor do próximo ENQ levando em
            // consideração o tempo das leituras dos bytes anteriores
            _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this),
                           TMINENQ_MSEC);
            return;
        }

        _readPieceOfComando();
    }

    void _readPieceOfComando() {
        while (_rb_received.toread() && _comandoIndex < COMANDO_SZ)
            _comando.at(_comandoIndex++) = _rb_received.read();

        if (_comandoIndex == COMANDO_SZ) {
            _comandoIndex = 0;
            _comandoRecebido();
        } else {
            _tasks.addTask(
                std::bind(&MedidorSimulado::_readNextPieceOfComando, this),
                TMAXCAR_MSEC);
        }
    }

    void timedOutTMAXSINC() {

        if (_rb_received.toread() <= 0) {
            _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this),
                           TMINENQ_MSEC - TMAXSINC_MSEC);
            return;
        }

        // has rxed at least one byte

        _comandoIndex = 0;
        _readPieceOfComando();
    }

    void timedOutTMAXRSP() {
        switch (_state) {
        case NAK_ENVIADO:
            if (_rb_received.toread() <= 0) {
                // TODO (MAYBE): calculate next ENQ regarding the last bytes
                // txed
                _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this),
                               TMINENQ_MSEC);
            } else {
                _comandoIndex = 0;
                _readPieceOfComando();
            }
            break;
        case RESPOSTA_ENVIADA:
            if (_rb_received.toread() <= 0) {
                // NBR14522: "se após o tempo permitido para a leitora enviar o
                // ACK este ainda não foi enviado, o medidor deve enviar ENQ
                // aguardando o recebimento do ACK"

                _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this));
                return;
            }

            switch (_rb_received.read()) {
            case ACK:
                if (_quantidadeDeRespostas <= 0) {
                    // todas respostas enviadas
                    _state = CONECTADO;
                    _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this));
                    return;
                }

                _gerador.getNextResposta(_nextResposta);
                _sendResposta(_nextResposta);
                _quantidadeDeRespostas--;

                break;
            case NAK:
                _nak_received++;

                if (_nak_received >= MAX_BLOCO_NAK) {
                    _state = CONECTADO;
                    _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this));
                    return;
                }

                _sendResposta(_nextResposta);

                break;
            default:
                _state = CONECTADO;
                _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this));

                break;
            }
            break;
        }
    }

    void _ENQ() {
        // discard all bytes received so far
        _flush(_rb_received);

        // send ENQ
        _rb_transmitted.write(NBR14522::ENQ);

        // de acordo com a norma, deveriamos receber o primeiro byte de
        // dado após no máximo ~TMAXSINC_MSEC depois de enviar o
        // ENQ.
        _tasks.addTask(std::bind(&MedidorSimulado::timedOutTMAXSINC, this),
                       TMAXSINC_MSEC);
    }

  protected:
    // _write() e _read() são chamadas pelo leitor somente (API)

    size_t _write(const byte_t* data, const size_t data_sz) {
        // data coming from leitor

        for (size_t i = 0; i < data_sz; i++)
            _rb_received.write(data[i]);

        return data_sz;
    }

    size_t _read(byte_t* data, const size_t max_data_sz) {
        size_t until = MIN(max_data_sz, _rb_transmitted.toread());
        for (size_t i = 0; i < until; i++)
            data[i] = _rb_transmitted.read();

        return until;
    }

  public:
    MedidorSimulado(NBR14522::medidor_num_serie_t medidor = {1, 2, 3, 4})
        : _gerador(medidor), _tasks(1) {
        _tasks.addTask(std::bind(&MedidorSimulado::_ENQ, this), TMINENQ_MSEC);
    }

    [[noreturn]] void run() { _tasks.run(); }
};

TEST_CASE("Medidor Simulado") {
    medidor_num_serie_t serie = {5,6,7,8};
    MedidorSimulado medidor(serie);

    std::thread t1(&MedidorSimulado::run, &medidor);

    std::this_thread::sleep_for(std::chrono::milliseconds(TMINENQ_MSEC*2));

    // std::array<byte_t, 258> data;
    // data.fill(0x00);
    
    // comando_t cmd;
    // resposta_t rsp;

    // cmd.fill(0x00);

    // SUBCASE("0x14 valido") {
    //     // read all ENQs and empty medidor read buffer
    //     size_t read = medidor.read(data.data(), data.size());

    //     for (size_t i = 0; i < read; i++)
    //         CHECK(data.at(i) == ENQ);
    //     cmd.at(0) = 0x14;
    //     uint16_t crc = CRC16(cmd.data(), cmd.size() - 2);
    //     setCRC(cmd, crc);

    //     // wait for next ENQ and then send cmd

    //     while (medidor.read(data.data(), 1) != ENQ) {
    //         std::this_thread::sleep_for(std::chrono::milliseconds(5));
    //     }
    //     medidor.write(cmd.data(), cmd.size());

    //     // read resposta

    //     std::this_thread::sleep_for(std::chrono::milliseconds(TMAXRSP_MSEC/2));
    //     read = medidor.read(rsp.data(), rsp.size());
    //     CHECK(read == rsp.size());
    //     CHECK(rsp.at(0) == 0x14);
    //     CHECK(rsp.at(1) == serie.at(0));
    //     CHECK(rsp.at(2) == serie.at(1));
    //     CHECK(rsp.at(3) == serie.at(2));
    //     CHECK(rsp.at(4) == serie.at(3));
    //     CHECK(CRC16(rsp.data(), rsp.size() - 2) == getCRC(rsp));
    // }


    t1.detach();
}
