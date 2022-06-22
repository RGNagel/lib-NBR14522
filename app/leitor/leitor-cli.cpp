#include <NBR14522.h>
#include <functional>
#include <leitor.h>
#include <serial/serial_policy_win_unix.h>
#include <timer/timer_policy_win_unix.h>

using namespace NBR14522;

template <typename T, size_t N>
void print_arr_hex(const std::array<T, N>& arr) {
    for (const auto i : arr)
        printf("%02X", (int)i);
}

// TODO
void print_usage() {
    printf(
        "Este programa recebe dois argumentos:\n\n"
        "1 - porta serial que o medidor está conectado. Em sistemas unix será\n" 
        "um pseudo-arquivo localizado em /dev/, por exemplo: /dev/ttyUSB0,\n"
        "/dev/ttyUSB1, etc. No Windows: COM1, COM2, etc\n\n"

        "2 - comando da comunicação convencional leitor-medidor (NBR14522). \n"
        "Este programa interpreta este parâmetro na codificação BCD. Caso o \n"
        "comando não seja passado por parâmetro contendo todos os 64 bytes \n"
        "referentes ao código + payload, o programa preenche o restante dos \n"
        "bytes com zeros. Os dois últimos bytes referentes ao CRC são sempre \n"
        "adicionados ou sobrescritos pela biblioteca NBR14522.\n\n"

        "Exemplos de invocação do programa:\n"
        "./leitor-cli /dev/ttyUSB0 14\n"
        "./leitor-cli /dev/ttyUSB0 14123456\n"
        "./leitor-cli /dev/ttyUSB0 20\n"
        "./leitor-cli /dev/ttyUSB0 204455660101\n\n"
    );
}

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
        printf("Número de argumentos inválido.\n\n");
        print_usage();
        return EXIT_FAILURE;
    }

    std::shared_ptr<SerialPolicyWinUnix> porta =
        std::make_shared<SerialPolicyWinUnix>();

    if (!porta->open(argv[1], 9600)) {
        printf("Não foi possível abrir a porta serial\n\n");
        return EXIT_FAILURE;
    }

    Leitor<TimerPolicyWinUnix, SerialPolicyWinUnix> leitor(porta);

    std::vector<comando_t> comandos;
    for (int i = 2; i < argc; i++) {
        auto bytes = get_hex_bytes(argv[i]);
        comando_t cmd;
        cmd.fill(0x00);
        std::copy(bytes.begin(), bytes.end(), cmd.begin());
        comandos.push_back(cmd);
    }

    for (auto& comando : comandos) {
        setCRC(comando, CRC16(comando.data(), comando.size()-2));
        printf("\nComando:\n");
        print_arr_hex(comando);

        printf("\nResposta(s):\n");
        leitor.leitura(comando, [&](const resposta_t& rsp) {
            print_arr_hex(rsp);
            printf("\n");
        });
    }

    printf("\n");

    porta->close();
    return EXIT_SUCCESS;
}
