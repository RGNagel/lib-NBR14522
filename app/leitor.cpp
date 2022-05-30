#include <NBR14522.h>
#include <iporta.h>
#include <leitor_old.h>
#include <memory>
#include <porta_serial/porta_serial.h>

using namespace NBR14522;

using namespace std::literals;

template <typename T, size_t N>
void print_arr_hex(const std::array<T, N>& arr) {
    for (const auto i : arr)
        printf("%02X", (int)i);
}

void print_usage() {}

std::vector<byte_t> get_hex_bytes(const std::string& hexbytes) {
    std::vector<byte_t> retval;

    // should not be odd length
    if (hexbytes.size() & 1)
        return retval;

    for (size_t i = 0; i < hexbytes.size(); i += 2) {
        std::string hexbyte = hexbytes.substr(i, 2);
        auto byte = strtol(hexbyte.c_str(), nullptr, 16);
        retval.push_back(static_cast<byte_t>(byte));
    }

    return retval;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        printf("Invalid number of args\n");
        print_usage(); // TODO
        return EXIT_FAILURE;
    }

    std::shared_ptr<PortaSerial> porta = std::make_shared<PortaSerial>();

    if (!porta->open(argv[1], 9600)) {
        printf("Couldn't open serial port\n");
        porta->close();
        return EXIT_FAILURE;
    }

    Leitor<> leitor(porta);

    std::vector<comando_t> comandos;
    for (int i = 2; i < argc; i++) {
        auto bytes = get_hex_bytes(argv[i]);
        comando_t cmd;
        cmd.fill(0x00);
        std::copy(bytes.begin(), bytes.end(), cmd.begin());
        comandos.push_back(cmd);
    }

    for (auto& comando : comandos) {
        printf("\nComando:\n");
        print_arr_hex(comando);

        printf("\nResposta(s):\n");
        Leitor<>::estado_t est = leitor.read(
            comando,
            [&](const resposta_t& rsp) {
                print_arr_hex(rsp);
                printf("\n");
            },
            1000ms);
    }

    printf("\n");

    porta->close();
    return EXIT_SUCCESS;
}