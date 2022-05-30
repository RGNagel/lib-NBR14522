#include <CRC.h>
#include <NBR14522.h>
#include <log_policy.h>
#include <memory>
#include <porta_serial/porta_serial.h>
#include <timer/timer.h>

template <typename T> using sptr = std::shared_ptr<T>;

template <class LogPolicy = LogPolicyStdout> class Leitor {
  public:
    typedef enum {
        Desconectado,
        Sincronizado,
        ComandoTransmitido,
        AtrasoDeSequenciaRecebido,
        CodigoRecebido,
        RespostaCompostaParcialRecebida,
        AguardaNovoComando,
    } estado_t;

    typedef enum {
        Sucesso,
        Processando,
        RespostaCompostaIncompleta,
        ErroLimiteDeNAKsRecebidos,
        ErroLimiteDeNAKsTransmitidos,
        ErroLimiteDeTransmissoesSemRespostas,
        ErroTempoSemWaitEsgotado,
        ErroLimiteDeWaitsRecebidos,
    } status_t;

    void setComando(const NBR14522::comando_t& comando) {
        _comando = comando;
        _estado = Desconectado;
        _status = Processando;
        _esvaziaPortaSerial();
    }
    estado_t processaEstado() {

        byte_t byte;
        size_t bytesLidosSz;

        switch (_estado) {
        case AguardaNovoComando:
            // nao faz nada neste estado, aguardando comando ser setado em
            // setComando()
            break;
        case Desconectado:
            if (_porta->read(&byte, 1) && byte == NBR14522::ENQ) {
                _estado = Sincronizado;
                _timer.setTimeout(NBR14522::TMAXENQ_MSEC);
            }
            break;
        case Sincronizado:
            if (_timer.timedOut()) {
                _estado = Desconectado;
                _esvaziaPortaSerial();
            } else if (_porta->read(&byte, 1) && byte == NBR14522::ENQ) {
                _transmiteComando();
                _counterNakRecebido = 0;
                _counterNakTransmitido = 0;
                _counterSemResposta = 0;
                _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                _estado = ComandoTransmitido;
            }
            break;
        case ComandoTransmitido:
            if (_timer.timedOut()) {
                _counterSemResposta++;
                if (_counterSemResposta < NBR14522::MAX_COMANDO_SEM_RESPOSTA) {
                    _transmiteComando();
                    _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                } else {
                    // falhou
                    _estado = AguardaNovoComando;
                    _status = ErroLimiteDeTransmissoesSemRespostas;
                    _esvaziaPortaSerial();
                }
            } else if (_porta->read(&byte, 1)) {
                // byte recebido
                if (byte == NBR14522::NAK) {
                    _counterNakRecebido++;
                    if (_counterNakRecebido == NBR14522::MAX_BLOCO_NAK) {
                        _status = ErroLimiteDeNAKsRecebidos;
                        _estado = AguardaNovoComando;
                    } else {
                        _transmiteComando();
                        _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                    }
                } else if (byte == NBR14522::WAIT) {
                    _estado = AtrasoDeSequenciaRecebido;
                    _timer.setTimeout(NBR14522::TSEMWAIT_SEC * 1000);
                } else if (byte == _comando.at(0)) {
                    // código do comando
                    _resposta.at(0) = byte;
                    _respostaBytesLidos = 1;
                    _timer.setTimeout(NBR14522::TMAXCAR_MSEC);
                    _estado = CodigoRecebido;
                }
            }
            break;
        case AtrasoDeSequenciaRecebido:
            if (_timer.timedOut()) {
                // falhou
                _estado = AguardaNovoComando;
                _status = ErroTempoSemWaitEsgotado;
                _esvaziaPortaSerial();
            } else if (_porta->read(&byte, 1)) {
                // byte recebido
                if (byte == NBR14522::ENQ) {
                    _estado = ComandoTransmitido;
                    _transmiteComando();
                    _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                } else if (byte == NBR14522::WAIT) {
                    _counterWaitRecebido++;
                    if (_counterWaitRecebido == NBR14522::MAX_BLOCO_WAIT) {
                        // falhou
                        _estado = AguardaNovoComando;
                        _status = ErroLimiteDeWaitsRecebidos;
                        _esvaziaPortaSerial();
                    } else {
                        _timer.setTimeout(NBR14522::TSEMWAIT_SEC * 1000);
                    }
                }
            }
            break;
        case CodigoRecebido:
            bytesLidosSz =
                _porta->read(&_resposta[_respostaBytesLidos],
                             NBR14522::RESPOSTA_SZ - _respostaBytesLidos);
            _respostaBytesLidos += bytesLidosSz;

            if (bytesLidosSz)
                _timer.setTimeout(NBR14522::TMAXCAR_MSEC);

            if (_timer.timedOut()) {
                _counterSemResposta++;
                if (_counterSemResposta == NBR14522::MAX_COMANDO_SEM_RESPOSTA) {
                    // falhou
                    _estado = AguardaNovoComando;
                    _status = ErroLimiteDeTransmissoesSemRespostas;
                    _esvaziaPortaSerial();
                } else {
                    _transmiteComando();
                    _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                }
            } else if (_respostaBytesLidos >= NBR14522::RESPOSTA_SZ) {
                // resposta completa recebida, verifica CRC
                if (NBR14522::getCRC(_resposta) ==
                    CRC16(_resposta.data(), _resposta.size() - 2)) {
                    // CRC correto
                    // transmite ACK
                    byte = NBR14522::ACK;
                    _porta->write(&byte, 1);
                    // resetar contadores, pois são referentes a cada resposta
                    _counterNakRecebido = 0;
                    _counterNakTransmitido = 0;
                    _counterSemResposta = 0;
                    _counterWaitRecebido = 0;
                    if (_isComposto(_resposta.at(0))) {
                        if (_isRespostaFinalDeComandoComposto(_resposta)) {
                            // resposta composta recebida por completo, sucesso
                            _estado = estado_t::AguardaNovoComando;
                            _status = Sucesso;
                        } else {
                            _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                            _estado = RespostaCompostaParcialRecebida;
                        }
                    } else {
                        // resposta simples recebido, sucesso
                        _estado = estado_t::AguardaNovoComando;
                        _status = Sucesso;
                    }
                } else {
                    // CRC incorreto
                    // transmite NAK
                    byte = NBR14522::NAK;
                    _porta->write(&byte, 1);
                    _counterNakTransmitido++;
                    if (_counterNakTransmitido == NBR14522::MAX_BLOCO_NAK) {
                        // falhou
                        _estado = estado_t::AguardaNovoComando;
                        _status = status_t::ErroLimiteDeNAKsTransmitidos;
                        _esvaziaPortaSerial();
                    } else {
                        _estado = estado_t::ComandoTransmitido;
                        _timer.setTimeout(NBR14522::TMAXRSP_MSEC);
                    }
                }
            }
            break;
        case RespostaCompostaParcialRecebida:
            if (_timer.timedOut()) {
                // resposta composta parcialmente recebida
                _estado = AguardaNovoComando;
                _status = RespostaCompostaIncompleta;
            } else if (_porta->read(&byte, 1)) {
                // byte recebido
                if (byte == NBR14522::ENQ) {
                    // resposta composta parcialmente recebida
                    _estado = AguardaNovoComando;
                    _status = RespostaCompostaIncompleta;
                } else if (byte == _comando.at(0)) {
                    // vai para a leitura da próxima resposta (composta)
                    _estado = CodigoRecebido;
                    _respostaBytesLidos = 1;
                    _timer.setTimeout(NBR14522::TMAXCAR_MSEC);
                }
            }

            break;
        }

        return _estado;
    }

    Leitor(sptr<PortaSerial> porta) : _porta(porta) {}

    uint32_t counterNakRecebido() { return _counterNakRecebido; }
    uint32_t counterNakTransmitido() { return _counterNakTransmitido; }
    uint32_t counterSemResposta() { return _counterSemResposta; }
    uint32_t counterWaitRecebido() { return _counterWaitRecebido; }
    status_t status() { return _status; }

    NBR14522::resposta_t resposta() { return _resposta; }

  private:
    estado_t _estado = Desconectado;
    status_t _status = Processando;
    sptr<PortaSerial> _porta;
    Timer _timer;
    NBR14522::comando_t _comando;
    NBR14522::resposta_t _resposta;
    size_t _respostaBytesLidos;
    uint32_t _counterNakRecebido = 0;
    uint32_t _counterNakTransmitido = 0;
    uint32_t _counterSemResposta = 0;
    uint32_t _counterWaitRecebido = 0;

    void _esvaziaPortaSerial() {
        byte_t byte;
        while (_porta->read(&byte, 1))
            ;
    }

    void _transmiteComando() {
        // nao incluir os dois ultimos bytes de CRC no calculo do CRC
        NBR14522::setCRC(_comando, CRC16(_comando.data(), _comando.size() - 2));
        _porta->write(_comando.data(), _comando.size());
    }

    bool _isComposto(const byte_t codigo) {
        return codigo == 0x26 || codigo == 0x27 || codigo == 0x52;
    }

    bool
    _isRespostaFinalDeComandoComposto(const NBR14522::resposta_t& resposta) {
        return resposta.at(5) & 0x10;
    }
};
