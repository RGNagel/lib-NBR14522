#pragma once

#include <BCD.h>
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

    inline bool _comandoComposto() { return _quantidadeDeRespostas > 1; }

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

        if (_comandoComposto()) {

            uint16_t numeroDoBloco = _respostaIndex + 1;
            // e.g. bloco nº 234:
            // octeto006 <- 0x02
            // octeto007 <- 0x34
            uint8_t octeto006 = static_cast<uint8_t>(numeroDoBloco / 100);
            uint8_t octeto007 =
                static_cast<uint8_t>(numeroDoBloco - 100 * octeto006);

            res.at(5) = dec2bcd(octeto006);
            res.at(6) = dec2bcd(octeto007);

            // ultimo bloco/resposta do comando composto?
            if ((_respostaIndex + 1) == _quantidadeDeRespostas)
                res.at(5) |= 0x10;
        }

        // TODO completar o resto da resposta com dados dummy

        NBR14522::setCRC(res, CRC16(res.data(), res.size() - 2));
        _respostaIndex++;
        return true;
    }
};
