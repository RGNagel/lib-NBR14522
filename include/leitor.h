#pragma once

#include <NBR14522.h>
#include <task_scheduler.h>

namespace NBR14522 {

template <typename T> using sptr = std::shared_ptr<T>;

class Leitor {
  public:
    typedef enum {
        RESPOSTA_RECEBIDA,
        RESPOSTA_PEDACO_RECEBIDO,
        DESCONECTADO,
        SINCRONIZADO,
        ESPERANDO_RESPOSTA,
        // erros:
        NAK_TX_EXCEDIDO,
        NAK_RX_EXCEDIDO,
        TEMPO_EXCEDIDO,
        RESPOSTA_CODIGO_INVALIDO,
        SINCRONIZACAO_FALHOU
    } estado_t;

  private:
    estado_t _estado;
    sptr<IPorta> _porta;
    TaskScheduler<> _tasks;
    resposta_t _resposta;
    std::function<void(resposta_t& rsp)> _callbackDeResposta;
    comando_t _comando;
    size_t _respostaIndex;
    size_t _countTransmittedNAK;
    size_t _countReceivedNAK;

    void _setEstado(estado_t estado) {
        _estado = estado;

        bool stopTasks = false;

        switch (_estado) {
        case SINCRONIZADO:
            _countReceivedNAK = 0;
            _countTransmittedNAK = 0;
            _transmiteComando(_comando);
            break;
        case RESPOSTA_PEDACO_RECEBIDO:
            _tasks.addTask(std::bind(&Leitor::_readPieceOfResposta, this),
                           std::chrono::milliseconds(TMAXCAR_MSEC));
            break;
        case RESPOSTA_RECEBIDA:
            _callbackDeResposta(_resposta);

            if (!isComposedCodeCommand(_resposta.at(0))) {
                // os unicos 3 comandos compostos existentes na norma (0x26,
                // 0x27 e 0x52) possuem o octeto 006 (5o byte), cujo valor:
                // 0N -> resposta/bloco intermediário
                // 1N -> resposta/bloco final

                if (_resposta.at(5) & 0x10) {
                    // resposta final do comando composto
                    stopTasks = true;
                } else {
                    // resposta intermediária, aguarda próxima resposta
                    _setEstado(ESPERANDO_RESPOSTA);
                }

            } else {
                // comando simples, e unica resposta foi recebida
                stopTasks = true;
            }
            break;
        case ESPERANDO_RESPOSTA:
            // TODO: é possivel otimizar esse trecho, colocando TMAXRSP somente
            // como timeout e um loop/task periodico de tempo menor p/ checar se
            // recebeu a resposta.
            _respostaIndex = 0;
            _tasks.addTask(std::bind(&Leitor::_readPieceOfResposta, this),
                           std::chrono::milliseconds(TMAXRSP_MSEC));
            break;
        case NAK_TX_EXCEDIDO:
        case NAK_RX_EXCEDIDO:
        case TEMPO_EXCEDIDO:
        case RESPOSTA_CODIGO_INVALIDO:
        case SINCRONIZACAO_FALHOU:
            stopTasks = true;
            break;
        default:
            break;
        }

        if (stopTasks)
            _tasks.runStop();
    }
    estado_t _getEstado() { return _estado; }

    void _transmiteComando(comando_t& comando) {
        setCRC(comando, CRC16(comando.data(), comando.size() - 2));
        _porta->write(comando.data(), comando.size());
        _setEstado(ESPERANDO_RESPOSTA);
    }

    void _tentaSincronizarPeriodicamente() {
        // tentar sincronizar com medidor periodicamente até conseguir
        // sincronizar

        byte_t data;
        size_t read = _porta->read(&data, 1);
        if (read == 1 && data == ENQ) {
            _setEstado(SINCRONIZADO);
        } else {
            std::chrono::milliseconds tryAgainIn =
                static_cast<std::chrono::milliseconds>(TMINENQ_MSEC / 4);

            _tasks.addTask(
                std::bind(&Leitor::_tentaSincronizarPeriodicamente, this),
                tryAgainIn);
        }
    }

    void _readPieceOfResposta() {
        size_t read =
            _porta->read(&_resposta[_respostaIndex], _resposta.size());

        // no data within TMAXRSP or TMAXCAR?
        if (read <= 0) {
            // response from meter has timed out
            _setEstado(TEMPO_EXCEDIDO);
        }
        // check if NAK received
        else if (_respostaIndex == 0 && _resposta.at(0) == NAK) {
            _countReceivedNAK++;
            // devemos enviar novamente o comando ao medidor, caso menos de
            // 7 NAKs recebidos
            if (_countReceivedNAK >= MAX_BLOCO_NAK) {
                _setEstado(NAK_RX_EXCEDIDO);
            } else {
                // retransmite comando
                _transmiteComando(_comando);
            }
            // No NAK received
        } else {

            _respostaIndex += read;

            if (_respostaIndex >= _resposta.size()) {
                // resposta completa recebida
                // _respostaRecebida(_resposta);
                uint16_t crcReceived = getCRC(_resposta);
                uint16_t crcCalculated =
                    CRC16(_resposta.data(), _resposta.size() - 2);

                if (crcReceived != crcCalculated) {
                    // se CRC com erro, o medidor deve enviar um NAK para o
                    // leitor e aguardar novo COMANDO. O máximo de NAKs
                    // enviados para um mesmo comando é 7.

                    if (_countTransmittedNAK >= MAX_BLOCO_NAK) {
                        _setEstado(NAK_TX_EXCEDIDO);
                    } else {
                        byte_t sinalizador = NAK;
                        _porta->write(&sinalizador, 1);
                        _countTransmittedNAK++;

                        // aguarda retransmissao da resposta pelo medidor
                        _setEstado(ESPERANDO_RESPOSTA);
                    }
                } else if (!isValidCodeCommand(_resposta.at(0)) ||
                           _comando.at(0) != _resposta.at(0)) {
                    // a norma não define qual deve ser o comportamento do
                    // leitor após receber uma resposta inválida mas com CRC
                    // OK. vamos quebrar a sessão.

                    _setEstado(RESPOSTA_CODIGO_INVALIDO);
                } else {
                    // resposta recebida está OK!

                    // transmite um ACK para o medidor
                    byte_t sinalizador = ACK;
                    _porta->write(&sinalizador, 1);

                    _setEstado(RESPOSTA_RECEBIDA);
                }
            } else {
                // resposta parcial recebida
                _setEstado(RESPOSTA_PEDACO_RECEBIDO);
            }
        }
    }

  public:
    Leitor(sptr<IPorta> porta) : _porta(porta) { _setEstado(DESCONECTADO); }

    estado_t tx(comando_t& comando,
                std::function<void(resposta_t& rsp)> cb_rsp_received,
                std::chrono::milliseconds timeout) {

        // discarta todos os bytes até agora do medidor
        byte_t data;
        while (_porta->read(&data, 1))
            ;

        _comando = comando;
        _callbackDeResposta = cb_rsp_received;
        _setEstado(DESCONECTADO);
        _tasks.addTask(
            std::bind(&Leitor::_tentaSincronizarPeriodicamente, this));
        _tasks.addTask(
            [this]() {
                if (this->_getEstado() == DESCONECTADO) {
                    this->_setEstado(SINCRONIZACAO_FALHOU);
                    this->_tasks.runStop();
                }
            },
            timeout);

        _tasks.run();

        return _getEstado();
    }
};

} // namespace NBR14522