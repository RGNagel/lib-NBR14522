#pragma once

#include <CRC.h>
#include <NBR14522.h>
#include <random>

class GeradorDeRespostas {
  private:
    byte_t _code;
    static constexpr uint16_t _ComandoCompostoTamanho = 3;
    NBR14522::medidor_num_serie_t _serie;
    uint16_t _respostaIndex;
    uint16_t _quantidadeDeRespostas;

  public:
    GeradorDeRespostas(NBR14522::medidor_num_serie_t medidor = {1, 2, 3, 4})
        : _serie(medidor), _respostaIndex(0), _quantidadeDeRespostas(0) {}

    // retorna numero de respostas geradas
    uint16_t gerar(NBR14522::comando_t& comando) {

        if (!NBR14522::isValidCodeCommand(comando.at(0)))
            return 0;

        _code = comando.at(0);

        _respostaIndex = 0;

        switch (_code) {
        // comandos compostos
        case 0x26:
        case 0x27:
        case 0x52:
            _quantidadeDeRespostas = _ComandoCompostoTamanho;
            break;
        default:
            _quantidadeDeRespostas = 1;
            break;
        }

        return _quantidadeDeRespostas;
    }

    bool getNextResposta(NBR14522::resposta_t& res) {

        if (_respostaIndex == _quantidadeDeRespostas)
            return false;

        res.at(0) = _code;
        res.at(1) = _serie.at(0);
        res.at(2) = _serie.at(1);
        res.at(3) = _serie.at(2);
        res.at(4) = _serie.at(3);
        // TODO completar o resto da resposta com dados dummy
        NBR14522::setCRC(res, CRC16(res.data(), res.size() - 2));

        _respostaIndex++;

        return true;
    }
};
