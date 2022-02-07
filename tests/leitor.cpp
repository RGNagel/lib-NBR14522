#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

// #include <leitor.h>
#include <NBR14522.h>
#include <iporta.h>
// #include <leitor.h>
#include <CRC.h>
#include <task_scheduler.h>

namespace NBR14522 {

template <typename T> using sptr = std::shared_ptr<T>;

class Leitor {
  public:
    typedef enum {
        DESCONECTADO,
        SINCRONIZADO,
        NAK_ENVIADO,
        NAK_TX_EXCEDIDO,
        NAK_RX_EXCEDIDO,
        TIMED_OUT,
        RESPOSTA_CODIGO_INVALIDO
    } estado_t;

  private:
    estado_t _estado;
    sptr<IPorta> _porta;
    TaskScheduler<> _tasks;
    resposta_t _resposta;
    size_t _respostaIndex;
    size_t _countTransmittedNAK;
    size_t _countReceivedNAK;

    void _setEstado(estado_t estado) { _estado = estado; }

    bool _sincronizar() {
        byte_t data;
        size_t read = _porta->read(&data, 1);
        if (read == 1 && data == ENQ) {
            _setEstado(SINCRONIZADO);
            return true;
        } else {
            return false;
        }
    }

  public:
    Leitor(sptr<IPorta> porta) : _porta(porta) { _setEstado(DESCONECTADO); }

    void tx(comando_t& comando, std::chrono::milliseconds timeout) {

        // discarta todos os bytes até agora do medidor
        byte_t data;
        while (_porta->read(&data, 1))
            ;

        using func = std::function<void()>;

        func readPieceOfResposta;
        readPieceOfResposta = [this, &readPieceOfResposta, &comando]() {
            size_t read =
                _porta->read(&_resposta[_respostaIndex], _resposta.size());

            // no data within TMAXRSP or TMAXCAR?
            if (read <= 0) {
                // response from meter has timed out
                _setEstado(TIMED_OUT);
                _tasks.runStop();
                return;
            }

            // check if NAK
            if (_respostaIndex == 0 && _resposta.at(0) == NAK) {
                _countReceivedNAK++;
                // devemos enviar novamente o comando ao medidor, caso menos de
                // 7 NAKs recebidos
                if (_countReceivedNAK >= MAX_BLOCO_NAK) {
                    _setEstado(NAK_RX_EXCEDIDO);
                    _tasks.runStop();
                    return;
                } else {
                    // discarta possiveis dados que estejam no buffer da porta
                    byte_t byte;
                    while (_porta->read(&byte, 1));

                    // retransmite comando
                    _porta->write(comando.data(), comando.size());
                    _respostaIndex = 0;
                    _tasks.addTask(readPieceOfResposta,
                                   std::chrono::milliseconds(TMAXRSP_MSEC));
                }
            }

            _respostaIndex += read;

            if (_respostaIndex >= _resposta.size()) {
                // resposta completa recebida
                // _respostaRecebida(_resposta);
                uint16_t crcReceived = getCRC(_resposta);
                uint16_t crcCalculated =
                    CRC16(_resposta.data(), _resposta.size() - 2);

                if (crcReceived != crcCalculated) {
                    // se CRC com erro, o medidor deve enviar um NAK para o
                    // leitor e aguardar novo COMANDO. O máximo de NAKs enviados
                    // para um mesmo comando é 7.

                    if (_countTransmittedNAK >= MAX_BLOCO_NAK) {
                        _setEstado(NAK_TX_EXCEDIDO);
                        _tasks.runStop();
                    } else {
                        byte_t sinalizador = NAK;
                        _porta->write(&sinalizador, 1);
                        _countTransmittedNAK++;
                        _setEstado(NAK_ENVIADO);

                        // aguarda retransmissao da resposta pelo medidor
                        _respostaIndex = 0;
                        _tasks.addTask(readPieceOfResposta,
                                       std::chrono::milliseconds(TMAXRSP_MSEC));
                    }
                } else if (!isValidCodeCommand(_resposta.at(0)) ||
                           comando.at(0) != _resposta.at(0)) {
                    // a norma não define qual deve ser o comportamento do
                    // leitor após receber uma resposta inválida mas com CRC OK.
                    // vamos quebrar a sessão.

                    _tasks.runStop();
                    _setEstado(RESPOSTA_CODIGO_INVALIDO);
                } else {
                    // resposta recebida está OK!
                    byte_t sinalizador = ACK;
                    _porta->write(&sinalizador, 1);
                    // TODO ...
                }
            } else {
                // resposta parcial recebida
                _tasks.addTask(readPieceOfResposta,
                               std::chrono::milliseconds(TMAXCAR_MSEC));
            }
        };

        func tryToSync;
        tryToSync = [this, &tryToSync, &readPieceOfResposta, &comando]() {
            // tentar sincronizar com medidor periodicamente
            // caso a sincronização não aconteça, é importante existir um
            // timeout para a operação
            if (_sincronizar()) {
                // discarta todos os bytes até agora do medidor
                byte_t data;
                while (_porta->read(&data, 1))
                    ;

                setCRC(comando, CRC16(comando.data(), comando.size() - 2));
                _porta->write(comando.data(), comando.size());
                _respostaIndex = 0;
                _countReceivedNAK = 0;
                _countTransmittedNAK = 0;
                _tasks.addTask(readPieceOfResposta,
                               std::chrono::milliseconds(TMAXRSP_MSEC));
            } else {
                _tasks.addTask(tryToSync,
                               std::chrono::milliseconds(TMINENQ_MSEC));
            }
        };
        _tasks.addTask(tryToSync, std::chrono::milliseconds(TMINENQ_MSEC) / 2);

        // timeout task
        _tasks.addTask([this]() { _tasks.runStop(); }, timeout);

        _tasks.run();
    }
};

} // namespace NBR14522

using namespace NBR14522;

class MockPortaSincronizada : public IPorta {
  public:
    MockPortaSincronizada(std::string name) {
        std::cout << "Iniciando MockPortaSincronizada" << name << std::endl;
    }

  protected:
    size_t _write(const byte_t* data, const size_t data_sz) {
        std::cout << "Writing: ";
        for (size_t i = 0; i < data_sz; i++)
            std::cout << data[i];
        std::cout << "\n";
        return 0;
    }
    size_t _read(byte_t* data, const size_t max_data_sz) {
        for (size_t i = 0; i < max_data_sz; i++)
            data[i] = ENQ;
        return max_data_sz;
    }
};

TEST_CASE("leitor") {

    // Leitor leitor;
    // REQUIRE(leitor.getEstado() == Leitor::DESCONECTADO);

    // SUBCASE("sem porta") {

    //     leitor.sincronizar();
    //     CHECK(leitor.getEstado() == Leitor::DESCONECTADO);
    // }

    // SUBCASE("com porta sincronizada") {
    //     MockPortaSincronizada porta("/path/to/port");
    //     leitor.setPorta(&porta);
    //     CHECK(leitor.getEstado() == Leitor::DESCONECTADO);
    //     leitor.sincronizar();
    //     CHECK(leitor.getEstado() == Leitor::SINCRONIZADO);
    // }
}
