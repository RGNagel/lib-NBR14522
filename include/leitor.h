#pragma once

#include <CRC.h>
#include <NBR14522.h>
#include <log_policy.h>
#include <task_scheduler.h>
#include <timer.h>

namespace NBR14522 {

template <typename T> using sptr = std::shared_ptr<T>;

using ms = std::chrono::milliseconds;

template <class LogPolicy = LogPolicyStdout> class Leitor {
  public:
    typedef enum {
        RSP_RECEBIDA,

        ERR_SINC_TEMPO_EXCEDIDO,
        ERR_SINC_SINALIZADOR,

        ERR_RSP_NAK_EXCEDIDO,
        ERR_RSP_CODIGO_TEMPO_EXCEDIDO,

        ERR_RSP_NAK_TX_EXCEDIDO,
        ERR_RSP_TOTAL_TEMPO_EXCEDIDO,

        ERR_RSP_CODIGO_DIFERE,
        ERR_RSP_CODIGO_INVALIDO_ENQ
    } estado_t;

  private:
    sptr<IPorta> _porta;

  public:
    Leitor(sptr<IPorta> porta) : _porta(porta) {}

    estado_t read(comando_t& comando,
                  std::function<void(const resposta_t& rsp)> cb_rsp_received,
                  std::chrono::milliseconds timeout) {

        Timer<> txTimer(timeout);

        resposta_t resposta;
        size_t respostaIndex = 0;

        // discarta todos os bytes até agora da porta
        byte_t data;
        while (_porta->read(&data, 1))
            ;

        // aguarda ENQ da porta
        Timer<> timer;
        byte_t enq = 0;
        timer.setTimeout(ms(TMAXENQ_MSEC));
        while (!timer.timedOut() && !txTimer.timedOut()) {
            if (_porta->read(&enq, 1) == 1)
                break;
        }

        if (txTimer.timedOut() || timer.timedOut())
            return ERR_SINC_TEMPO_EXCEDIDO;

        if (enq != ENQ)
            return ERR_SINC_SINALIZADOR;

        // sincronizado

        setCRC(comando, CRC16(comando.data(), comando.size() - 2));
        _porta->write(comando.data(), comando.size());

        // ---------------------------------
        // Aguarda primeiro byte da resposta
        // ---------------------------------

    waitFirstByte:
        size_t countReceivedNAK = 0;

        // devemos receber o primeiro byte da resposta em até TMAXRSP
        timer.setTimeout(ms(TMAXRSP_MSEC));
        while (!timer.timedOut() && !txTimer.timedOut()) {

            byte_t firstByte = 0;
            if (_porta->read(&firstByte, 1) == 1) {
                if (firstByte == NAK) {
                    countReceivedNAK++;
                    // devemos enviar novamente o comando ao medidor, caso menos
                    // de 7 NAKs recebidos
                    if (countReceivedNAK >= MAX_BLOCO_NAK) {
                        return ERR_RSP_NAK_EXCEDIDO;
                    } else {
                        // retransmite comando
                        _porta->write(comando.data(), comando.size());
                        continue;
                    }
                } else if (firstByte == ENQ) {
                    return ERR_RSP_CODIGO_INVALIDO_ENQ;
                } else if (firstByte != comando.at(0)) {
                    return ERR_RSP_CODIGO_DIFERE;
                }

                // primeiro byte recebido OK!

                resposta.at(0) = firstByte;
                respostaIndex = 1;
                break;
            }
        }

        if (txTimer.timedOut() || timer.timedOut())
            return ERR_RSP_CODIGO_TEMPO_EXCEDIDO;

        size_t countTxNAK = 0;

        // ----------------------------------------------------------
        // Primeiro byte recebido. Agora aguarda restante da resposta
        // ----------------------------------------------------------

        timer.setTimeout(ms(TMAXCAR_MSEC));
        while (!timer.timedOut() || !txTimer.timedOut()) {
            size_t read = _porta->read(&resposta[respostaIndex],
                                       resposta.size() - respostaIndex);
            respostaIndex += read;

            if (read)
                timer.setTimeout(
                    ms(TMAXCAR_MSEC)); // recarrega timeout para
                                       // receber próximo(s) byte(s)
            if (respostaIndex < resposta.size())
                continue;

            // ------------------------------------
            // resposta completa recebida. Trata-a.
            // ------------------------------------

            uint16_t crcRecv = getCRC(resposta);
            uint16_t crcCalc = CRC16(resposta.data(), resposta.size() - 2);

            if (crcRecv != crcCalc) {
                // se CRC com erro, o medidor deve enviar um NAK para o
                // leitor e aguardar novo COMANDO. O máximo de NAKs
                // enviados para um mesmo comando é 7.

                if (countTxNAK >= MAX_BLOCO_NAK)
                    return ERR_RSP_NAK_TX_EXCEDIDO;

                byte_t sinalizador = NAK;
                _porta->write(&sinalizador, 1);

                countTxNAK++;

                timer.setTimeout(ms(TMAXRSP_MSEC));
                continue;
            }

            // -----------------------------------
            // resposta completa recebida está OK!
            // -----------------------------------

            byte_t sinalizador = ACK;
            _porta->write(&sinalizador, 1);
            // TODO (maybe): talvez tenha q add um tempo aqui pra ter certeza
            // que o medidor recebeu o ACK

            cb_rsp_received(resposta);

            if (isComposedCodeCommand(resposta.at(0))) {
                // os unicos 3 comandos compostos existentes na norma (0x26,
                // 0x27 e 0x52) possuem o octeto 006 (5o byte), cujo valor:
                // 0N -> resposta/bloco intermediário
                // 1N -> resposta/bloco final

                if (resposta.at(5) & 0x10) {
                    // resposta final do comando composto
                    return RSP_RECEBIDA;
                } else {
                    // resposta intermediária, aguarda próxima resposta

                    // recarrega timeout da leitura
                    txTimer.setTimeout(timeout);
                    goto waitFirstByte;
                }

            } else {
                return RSP_RECEBIDA;
            }
        }

        return ERR_RSP_TOTAL_TEMPO_EXCEDIDO;
    }
};

} // namespace NBR14522
