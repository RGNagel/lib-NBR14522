#include <NBR14522.h>
#include <functional>
#include <leitor_interface.h>
#include <serial/serial_policy_win_unix.h>
#include <timer/timer_policy_win_unix.h>

using namespace NBR14522;

template <typename T, size_t N>
void print_arr_hex(const std::array<T, N>& arr) {
    for (const auto i : arr)
        printf("%02X", (int)i);
}

// TODO
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
        print_usage();
        return EXIT_FAILURE;
    }

    std::shared_ptr<SerialPolicyWinUnix> porta =
        std::make_shared<SerialPolicyWinUnix>();

    if (!porta->open(argv[1], 9600)) {
        printf("Não foi possível abrir a porta serial\n");
        return EXIT_FAILURE;
    }

    LeitorInterface<TimerPolicyWinUnix, SerialPolicyWinUnix> leitor(porta);

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
        leitor.leitura(comando, [&](const resposta_t& rsp) {
            print_arr_hex(rsp);
            printf("\n");
        });
    }

    printf("\n");

    porta->close();
    return EXIT_SUCCESS;
}
