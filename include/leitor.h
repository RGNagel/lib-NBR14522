#pragma once

#include <functional>
#include <leitor_fsm.h>
#include <log_policy.h>

template <class TimerPolicy, class SerialPolicy,
          class LogPolicy = LogPolicyStdout>
class Leitor {

    using FSM = LeitorFSM<TimerPolicy, SerialPolicy>;

  public:
    Leitor(sptr<SerialPolicy> porta) : _leitor(porta) {}

    bool leitura(NBR14522::comando_t& comando,
                 std::function<void(const NBR14522::resposta_t& rsp)> callback,
                 uint32_t timeout_resposta_ms = 0) {

        _leitor.setComando(comando);
        _leitor.setCallback(callback);

        TimerPolicy leituraDeadline;
        if (timeout_resposta_ms)
            leituraDeadline.setTimeout(timeout_resposta_ms);

        _leitor.setCallback([&](const NBR14522::resposta_t& rsp) {
            // reatualiza o timeout informado pelo usuário toda vez que uma
            // resposta é recebida pelo leitor
            if (timeout_resposta_ms)
                leituraDeadline.setTimeout(timeout_resposta_ms);

            callback(rsp);
        });

        while (true) {
            typename FSM::estado_t estado = _leitor.processaEstado();

            switch (estado) {
            case FSM::estado_t::AguardaNovoComando:
                switch (_leitor.status()) {
                case FSM::status_t::Sucesso:
                    return true;
                case FSM::status_t::Processando:
                    break;
                default:
                    LogPolicy::log("Processo falhou. Status: %s\n",
                                   _status2verbose(_leitor.status()));
                    return false;
                }
                break;
            default:
                break;
            }

            if (timeout_resposta_ms && leituraDeadline.timedOut()) {
                LogPolicy::log(
                    "O processo de leitura excedeu o tempo informado pelo "
                    "usuário (%i ms) sem receber nenhuma resposta completa com "
                    "sucesso. Estado: "
                    "%s\n, Status: %s",
                    timeout_resposta_ms, _estado2string(estado),
                    _status2verbose(_leitor.status()));
                return false;
            }
        }
    }

  private:
    const char* _estado2string(const typename FSM::estado_t estado) {
        switch (estado) {
        case FSM::estado_t::Dessincronizado:
            return "Dessincronizado";
        case FSM::estado_t::Sincronizado:
            return "Sincronizado";
        case FSM::estado_t::ComandoTransmitido:
            return "Comando Transmitido";
        case FSM::estado_t::AtrasoDeSequenciaRecebido:
            return "Atraso de Sequência Recebido";
        case FSM::estado_t::CodigoRecebido:
            return "Código Recebido";
        case FSM::estado_t::AguardaNovoComando:
            return "Aguarda Novo Comando";
        }
        return "";
    }
    const char* _status2verbose(const typename FSM::status_t status) {
        switch (status) {
        case FSM::status_t::Sucesso:
            return "Leitura realizada com sucesso";
        case FSM::status_t::Processando:
            return "Leitura em processamento";
        case FSM::status_t::ErroLimiteDeNAKsRecebidos:
            return "Erro: limite excedido de NAKs recebidos pelo leitor";
        case FSM::status_t::ErroLimiteDeNAKsTransmitidos:
            return "Erro: limite excedido de NAKs transmitidos pelo leitor";
        case FSM::status_t::ErroLimiteDeTransmissoesSemRespostas:
            return "Erro: limite excedido de transmissões sem resposta do "
                   "medidor";
        case FSM::status_t::ErroTempoSemWaitEsgotado:
            return "Erro: limite de tempo excedido sem receber WAIT";
        case FSM::status_t::ErroLimiteDeWaitsRecebidos:
            return "Erro: limite excedido de WAITs recebidos";
        case FSM::status_t::ErroQuebraDeSequencia:
            return "Erro: quebra de sequência (byte diferente de sinalizador "
                   "ou resposta) pelo medidor";
        case FSM::status_t::ErroAposRespostaRecebeNAK:
            return "Erro: NAK recebido mesmo após medidor já ter recebido "
                   "comando íntegro";
        case FSM::status_t::ErroSemRespostaAoAguardarProximaResposta:
            return "Erro: Ao aguardar próxima resposta de um comando composto, "
                   "leitor não recebe nada do medidor";
        case FSM::status_t::ExcecaoOcorrenciaNoMedidor:
            return "Exceção: informação de ocorrência no medidor recebida no "
                   "lugar da resposta ao comando solicitado";
        case FSM::status_t::ExcecaoComandoNaoImplementado:
            return "Exceção: comando solicitado ao medidor não foi "
                   "implementado";
        }

        return "";
    }

    FSM _leitor;
};
