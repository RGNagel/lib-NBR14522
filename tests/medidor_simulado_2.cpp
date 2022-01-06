#include "doctest/doctest.h"
#include <types_local.h>
#include <task_scheduler.h>
#include <iporta.h>
#include <ring_buffer.h>
#include <CRC.h>

#include <array>
#include <functional>
#include <string.h>

using namespace NBR14522;

// class GeradorDeRespostas {
//   public:
//     void set(comando_t& comando) {
//         // ...
//     }

//     resposta_t getNextResposta() {
//         // ...

//         resposta_t res;

//         return res;
//     }
// };

/**
 *
 * @brief implementa uma porta serial que se comunica com o medidor simulado.
 * Os dados transmitidos pelo medidor são disponibilizados na função
 * _read() e os dados recebidos na função _write(), de maneira que essa classe
 * possa ser usada como uma porta serial do ponto de vista do leitor.
 *
 */
template <size_t readBufferLen> class MedidorSimulado : public IPorta {
  private:
    RingBuffer<byte_t, 1024> _rb_received;
    RingBuffer<byte_t, 1024> _rb_transmitted;
    
    TaskScheduler _tasks;
    uint8_t _nak_transmitted = 0;

    // comando vindo do leitor
    comando_t _comando;
    size_t _comandoIndex = 0;

    enum {
            CONECTADO,
            COMANDO_RECEBIDO,
            NAK_ENVIADO,
            RESPOSTA_ENVIADA
    } _state = CONECTADO;

    void _flush(RingBuffer<byte_t, 1024> &rb) {
        while (rb.toread() >= 1)
            rb.read();
    }

    bool _comandoRecebido() {

        uint16_t crcReceived = NBR14522::getCRC(_comando);
        uint16_t crcCalculated =
            CRC16(_comando.data(), _comando.size() - 2);

        if (crcReceived != crcCalculated) {
            // se CRC com erro, o medidor deve enviar um NAK para o
            // leitor e aguardar novo COMANDO. O máximo de NAKs enviados
            // para um mesmo comando é 7.

            if (_nak_transmitted >= MAX_BLOCO_NAK) {
                // quebra de sequencia o medidor envia o ENQ
                _nak_transmitted = 0;
                // TODO (MAYBE): calcular valor do próximo ENQ levando em consideração o tempo de leitura do comando
                _tasks.addTask(std::bind(_ENQ, this), TMINENQ_MSEC);
            }
            else {
                _rb_transmitted.write(NBR14522::NAK);
                _nak_transmitted++;
                _state = NAK_ENVIADO;
                _tasks.addTask(std::bind(timedOutTMAXRSP, this), TMAXRSP_MSEC);
            }
        }
        // verifica se o comando recebido existe e/ou é valido
        else if (!NBR14522::isValidCodeCommand(comandoRecebido.at(0))) {
            // a norma não define qual deve ser o comportamento do
            // medidor após receber um comando inválido mas com CRC OK.
            // vamos quebrar a sessão.

            // TODO (MAYBE): calcular valor do próximo ENQ levando em consideração o tempo de leitura do comando
            _tasks.addTask(std::bind(_ENQ, this), TMINENQ_MSEC);
        }
        // comando recebido OK
        else {
            // TODO ...
        }


    }

    void _readNextPieceOfComando() {
        if (_rb_received.toread() <= 0) {
            // TODO (MAYBE): calcular valor do próximo ENQ levando em consideração o tempo das leituras dos bytes anteriores
            _tasks.addTask(std:;bind(_ENQ, this), TMINENQ_MSEC);
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
        }
        else {
            _tasks.addTask(std::bind(_readNextPieceOfComando, this), TMAXCAR_MSEC);
        }
    }

    void timedOutTMAXSINC() {

        if (_rb_received.toread() <= 0) {
            _tasks.addTask(std::bind(_ENQ, this), TMINENQ_MSEC - TMAXSINC_MSEC);
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
                    // TODO (MAYBE): calculate next ENQ regarding the last bytes txed
                    _tasks.addTask(std::bind(_ENQ, this), TMINENQ_MSEC);
                }
                else {
                    _comandoIndex = 0;
                    _readPieceOfComando();
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
        _tasks.addTask(std::bind(timedOutTMAXSINC, this), TMAXSINC_MSEC);
    }

  protected:
    // _write() e _read() são chamadas pelo leitor somente

    size_t _write(const byte_t* data, const size_t data_sz) {
        // data coming from leitor

        for (auto i = 0; i < data_sz; i++)
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

    MedidorSimulado() {
        _tasks.addTask(std::bind(_ENQ, this), TMINENQ_MSEC);
    }
    
    [[noreturn]] void run() {
        _tasks.run();
    }

    [[noreturn]] void engine() {
        static enum {
            CONECTADO,
            COMANDO_RECEBIDO,
            TRANSMITE_NAK,
            RESPOSTA_ENVIADA
        } engineFSM = CONECTADO;

        size_t previous_toread = 0;

        comando_t comandoRecebido;

        resposta_t resposta;

        int nak_transmitted = 0;

        // GeradorDeRespostas geradorDeResposta;

        _flushReadBuffer();

        while (1) {
            switch (engineFSM) {

            case COMANDO_RECEBIDO: {

                uint16_t crcReceived = NBR14522::getCRC(comandoRecebido);
                uint16_t crcCalculated =
                    CRC16(comandoRecebido.data(), comandoRecebido.size() - 2);

                if (crcReceived != crcCalculated) {
                    // se CRC com erro, o medidor deve enviar um NAK para o
                    // leitor e aguardar novo COMANDO. O máximo de NAKs enviados
                    // para um mesmo comando é 7.

                    _rb_transmitted.write(NBR14522::NAK);
                    nak_transmitted++;

                    if (nak_transmitted == NBR14522::MAX_BLOCO_NAK) {
                        nak_transmitted = 0;
                        // quebra de sequencia, vai imediatamente para o estado
                        // CONECTADO no qual o medidor envia o ENQ
                        engineFSM = CONECTADO;
                        break;
                    } else {
                        // TODO: nak_transmitted tem que ser zerado de alguma
                        // maneira

                        // aguarda tempo máximo de resposta (TMAXRSP) antes de
                        // ir para CONECTADO, para ver se o leitor envia
                        // novamente o comando neste periodo. Caso o leitor
                        // enviar, o estado CONECTADO trata este novo comando
                        // antes de enviar próximo ENQ (quebra de sequência).

                        // _timer.waitMiliseconds(TMAXRSP_MSEC);
                        engineFSM = CONECTADO;
                        break;
                    }
                }

                // verifica se o comando recebido existe e/ou é valido
                if (!NBR14522::isValidCodeCommand(comandoRecebido.at(0))) {
                    // a norma não define qual deve ser o comportamento do
                    // medidor após receber um comando inválido mas com CRC OK.
                    // vamos quebrar a sessão.

                    engineFSM = CONECTADO;
                    break;
                }

                // gera a resposta para o comando solicitado

                // geradorDeResposta.set(comandoRecebido);

                // resposta = geradorDeResposta.getNextResposta();

                for (size_t i = 0; i < resposta.size(); i++)
                    _rb_transmitted.write(resposta.at(i));

                _flushReadBuffer();

                engineFSM = RESPOSTA_ENVIADA;
            } break;
            case TRANSMITE_NAK: {
                _rb_transmitted.write(NBR14522::NAK);
                nak_transmitted++;

                if (nak_transmitted == NBR14522::MAX_BLOCO_NAK) {
                    nak_transmitted = 0;

                    // quebra de sequencia, vai imediatamente para o estado
                    // CONECTADO no qual o medidor envia o ENQ
                    engineFSM = CONECTADO;
                    break;
                }

            } break;
            case RESPOSTA_ENVIADA: {
                static uint32_t counter = 0;

                // somente um caracter de sinalização é esperado pelo
                // medidor
                if (_rb_received.toread() > 1) {
                    // quebra de
                    counter = 0;
                    engineFSM = CONECTADO;
                    break;
                } else if (_rb_received.toread() > 1)

                    // fica aguardando resposta do leitor até estourar o tempo
                    // TMAXRSP_MSEC
                    if (counter == 10) {
                        // tempo de resposta estourado
                        counter = 0;
                        engineFSM = CONECTADO;
                        break;
                    }
                // _timer.waitMiliseconds(TMAXRSP_MSEC/10);
                counter++;
            } break;
            }
        }
    }
};
