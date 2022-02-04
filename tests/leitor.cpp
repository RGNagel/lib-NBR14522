#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

// #include <leitor.h>
#include <NBR14522.h>
#include <iporta.h>

namespace NBR14522 {

class Leitor {
  public:
    typedef enum { DESCONECTADO, SINCRONIZADO } estado_t;

  private:
    estado_t _estado;
    IPorta* _porta = nullptr;
    void _setEstado(estado_t estado) { _estado = estado; }

  public:
    Leitor() {
        _setEstado(DESCONECTADO);
        _porta = nullptr;
    }
    Leitor(IPorta* porta) {
        _setEstado(DESCONECTADO);
        _porta = porta;
    }

    estado_t getEstado() { return _estado; }

    void setPorta(IPorta* porta) { _porta = porta; }

    void sincronizar() {
        if (_porta == nullptr)
            return;

        byte_t data;
        size_t read = _porta->read(&data, 1);
        if (read == 1 && data == ENQ)
            _setEstado(SINCRONIZADO);
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

    Leitor leitor;
    REQUIRE(leitor.getEstado() == Leitor::DESCONECTADO);

    SUBCASE("sem porta") {

        leitor.sincronizar();
        CHECK(leitor.getEstado() == Leitor::DESCONECTADO);
    }

    SUBCASE("com porta sincronizada") {
        MockPortaSincronizada porta("/path/to/port");
        leitor.setPorta(&porta);
        CHECK(leitor.getEstado() == Leitor::DESCONECTADO);
        leitor.sincronizar();
        CHECK(leitor.getEstado() == Leitor::SINCRONIZADO);
    }
}
