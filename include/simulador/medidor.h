#pragma once

#include "gerador_de_respostas.h"
#include <NBR14522.h>
#include <chrono>
#include <iporta.h>
#include <log_policy.h>
#include <ring_buffer.h>
#include <timer/timer.h>

namespace Simulador {

using namespace NBR14522;
using ms = std::chrono::milliseconds;
using namespace std::literals;
using SysTimer = Timer<std::chrono::system_clock>;

/**
 *
 * @brief implementa uma porta serial que se comunica com o medidor simulado.
 * Os dados transmitidos pelo medidor são disponibilizados na função
 * _read() e os dados recebidos na função _write(), de maneira que essa classe
 * possa ser usada como uma porta serial do ponto de vista do leitor.
 *
 */
template <class LogPolicy = LogPolicyNull> class Medidor : public IPorta {
  private:
    RingBuffer<byte_t, 1024> _rb_received;
    RingBuffer<byte_t, 1024> _rb_transmitted;

    GeradorDeRespostas _gerador;

    bool _stop = false;

    void _flush(RingBuffer<byte_t, 1024>& rb) {
        while (rb.toread() >= 1)
            rb.read();
    }

  protected:
    // _write() e _read() são chamadas pelo leitor somente (API)

    size_t _write(const byte_t* data, const size_t data_sz) {
        // dados vindo do "leitor"

        for (size_t i = 0; i < data_sz; i++)
            _rb_received.write(data[i]);

        return data_sz;
    }

    size_t _read(byte_t* data, const size_t max_data_sz) {
        // dados lidos do medidor

        size_t until = MIN(max_data_sz, _rb_transmitted.toread());
        for (size_t i = 0; i < until; i++)
            data[i] = _rb_transmitted.read();

        return until;
    }

  public:
    Medidor(NBR14522::medidor_num_serie_t medidor = {1, 2, 3, 4})
        : _gerador(medidor) {}

    Medidor(uint16_t qntdRespostasCmd26, uint16_t qntdRespostasCmd27,
            uint16_t qntdRespostasCmd52)
        : _gerador(qntdRespostasCmd26, qntdRespostasCmd27, qntdRespostasCmd52) {
    }

    Medidor(NBR14522::medidor_num_serie_t medidor, uint16_t qntdRespostasCmd26,
            uint16_t qntdRespostasCmd27, uint16_t qntdRespostasCmd52)
        : _gerador(medidor, qntdRespostasCmd26, qntdRespostasCmd27,
                   qntdRespostasCmd52) {}

    void run() {
        LogPolicy::log("Iniciando execução...\n");

        // ----------------------------
        // Envia ENQ e aguarda comando
        // ----------------------------

    proximoENQ:

        // esvazia dados recebidos e transmitidos até agora
        _flush(_rb_received);
        _flush(_rb_transmitted);

        // de acordo com a norma, deveriamos receber o PRIMEIRO byte de
        // dado após no máximo ~TMAXSINC_MSEC depois de enviar o
        // ENQ.
        SysTimer timerShouldRecvDataWithin;
        timerShouldRecvDataWithin.setTimeout(ms(TMAXSINC_MSEC));

        // envia ENQ e agenda próximo ENQ
        LogPolicy::log("Enviando ENQ\n");
        _rb_transmitted.write(NBR14522::ENQ);
        SysTimer timerSendNextENQ;
        timerSendNextENQ.setTimeout(ms(TMINENQ_MSEC));

        while (1) {
            // guard
            if (_stop) {
                _stop = false;
                return;
            }

            if (timerSendNextENQ.timedOut()) {
                // envia ENQ e agenda próximo ENQ
                LogPolicy::log("Enviando ENQ\n");
                _rb_transmitted.write(NBR14522::ENQ);
                timerSendNextENQ.setTimeout(ms(TMINENQ_MSEC));
                timerShouldRecvDataWithin.setTimeout(ms(TMAXSINC_MSEC));
            }

            if (_rb_received.toread() &&
                !timerShouldRecvDataWithin.timedOut()) {
                LogPolicy::log("Recebeu dado(s) dentro do período\n");
                break;
            } else if (_rb_received.toread() &&
                       timerShouldRecvDataWithin.timedOut()) {
                LogPolicy::log(
                    "Recebeu dado(s) FORA do período. Descarta dado(s)\n");
                _flush(_rb_received);
            }
        }

        // ------------------------------------------------------------
        // recebeu o(s) dado(s) dentro do tempo estabelecido pela norma.
        // ------------------------------------------------------------

        size_t countSentNAK = 0;

    lerComando:

        comando_t comando;
        size_t comandoIndex = 0;

        while (_rb_received.toread() && comandoIndex < comando.size())
            comando.at(comandoIndex++) = _rb_received.read();

        SysTimer timerShouldRecvNextPieceWithin;
        timerShouldRecvNextPieceWithin.setTimeout(ms(TMAXCAR_MSEC));

        while (comandoIndex < comando.size()) {
            if (timerShouldRecvNextPieceWithin.timedOut()) {
                LogPolicy::log("Tempo excedido ao receber comando completo\n");
                SysTimer::wait(250ms);
                goto proximoENQ;
            }

            while (_rb_received.toread())
                comando.at(comandoIndex++) = _rb_received.read();
        }

        LogPolicy::log("Comando completo recebido\n");

        uint16_t crcRecv = NBR14522::getCRC(comando);
        uint16_t crcCalc = CRC16(comando.data(), comando.size() - 2);

        if (crcRecv != crcCalc) {
            // se CRC com erro, o medidor deve enviar um NAK para o
            // leitor e aguardar novo COMANDO. O máximo de NAKs enviados
            // para um mesmo comando é 7.

            LogPolicy::log("Comando com erro de CRC.\n");

            if (countSentNAK >= MAX_BLOCO_NAK) {
                LogPolicy::log("Envio de NAKs excedido\n");
                // quebra de sequencia o medidor envia o ENQ
                // TODO (MAYBE): calcular valor do próximo ENQ levando em
                // consideração o tempo de leitura do comando

                SysTimer::wait(250ms);
                goto proximoENQ;
            }

            LogPolicy::log("Enviando NAK\n");
            _rb_transmitted.write(NAK);
            countSentNAK++;
            goto lerComando;

        } else if (!isValidCodeCommand(comando.at(0))) {
            // a norma não define qual deve ser o comportamento do
            // medidor após receber um comando inválido mas com CRC OK.
            // vamos quebrar a sessão.

            LogPolicy::log("Comando recebido inválido\n");

            // TODO: subst. esses waits por uma logica que aguarda todo "lixo"
            // do leitor vir e então só após disso vai para o ENQ
            SysTimer::wait(250ms);
            goto proximoENQ;
        }

        // ------------------------------------
        // Comando recebido OK. Gera respostas.
        // -----------------------------------

        LogPolicy::log("Comando recebido OK. Gera resposta(s).\n");

        size_t quantidadeDeRespostas = _gerador.gerar(comando);

        LogPolicy::log("Quant. de respostas geradas: " +
                       std::to_string(quantidadeDeRespostas) + "\n");

        for (size_t r = 0; r < quantidadeDeRespostas; r++) {
            resposta_t resposta;
            _gerador.getNextResposta(resposta);

            LogPolicy::log("Enviando resposta[" + std::to_string(r) +
                           "]. Aguarda ACK.\n");

            for (size_t i = 0; i < resposta.size(); i++)
                _rb_transmitted.write(resposta.at(i));

            // resposta enviada, aguarda ACK
            SysTimer timerAguardaACK;
            timerAguardaACK.setTimeout(ms(TMAXRSP_MSEC));
            size_t timedOutAguardaACK = 0;
            size_t countRecvNAK = 0;

            while (1) {
                if (timerAguardaACK.timedOut()) {
                    // NBR14522: "se após o tempo permitido para a leitora
                    // enviar o ACK este ainda não foi enviado, o medidor deve
                    // enviar ENQ aguardando o recebimento do ACK"

                    // TODO: Mas quantas vezes até dar timeout?
                    timedOutAguardaACK++;
                    if (timedOutAguardaACK >= MAX_BLOCO_NAK) {
                        LogPolicy::log(
                            "Tempo total para receber ACK expirou.\n");
                        goto proximoENQ;
                    }

                    LogPolicy::log("Enviando ENQ. Aguarda ACK novamente\n");

                    _rb_transmitted.write(ENQ);
                    timerAguardaACK.setTimeout(ms(TMINENQ_MSEC));
                }

                if (!_rb_received.toread())
                    continue;

                byte_t dado = _rb_received.read();

                if (dado == ACK) {
                    LogPolicy::log("ACK recebido\n");
                    break;
                }

                if (dado == NAK) {
                    LogPolicy::log("NAK recebido\n");

                    countRecvNAK++;
                    if (countRecvNAK >= MAX_BLOCO_NAK) {
                        SysTimer::wait(ms(TMINREV_MSEC)); // TODO Precisa mesmo?
                        LogPolicy::log("Reenvios excedidos\n");
                        goto proximoENQ;
                    }

                    LogPolicy::log("Reenviando resposta\n");

                    for (size_t i = 0; i < resposta.size(); i++)
                        _rb_transmitted.write(resposta.at(i));

                    timerAguardaACK.setTimeout(ms(TMAXRSP_MSEC));
                } else {
                    LogPolicy::log("Sinalizador inválido recebido\n");

                    SysTimer::wait(250ms);
                    goto proximoENQ;
                }
            }
        }

        LogPolicy::log("Respostas(s) enviadas com sucesso\n");
        goto proximoENQ;
    }

    void runStop() {
        LogPolicy::log("Parando execução...\n");
        _stop = true;
    }
};

} // namespace Simulador
