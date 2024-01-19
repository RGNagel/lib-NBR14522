#include "NBR14522.h"
#include "client_tcp.h"
#include "serial/serial_policy_unix.h"
#include <cstring>
#include <hexstr2bytes.h>
#include <leitor.h>
#include <log_policy.h>
#include <timer/timer_policy_generic_os.h>
#include <vector>
#include <get_comandos_from_data.h>

using namespace NBR14522;


template <class LogPolicy = LogPolicyStdout>
bool readRespostasFromMeter(
    std::vector<comando_t>& cmds, const char* serial_port,
    std::function<void(const NBR14522::resposta_t& rsp)> callback) {

    std::shared_ptr<SerialPolicyUnix> porta =
        std::make_shared<SerialPolicyUnix>();

    if (!porta->openSerial(serial_port)) {
        LogPolicy::log("Não foi possível abrir a porta serial\n\n");
        return false;
    }

    Leitor<TimerPolicyWinUnix, SerialPolicyUnix, LogPolicy> leitor(porta);

    bool retval = true;

    for (auto& cmd : cmds) {
        retval = leitor.leitura(cmd, callback);
        if (!retval)
            break;
    }

    porta->closeSerial();
    return retval;
}

using LogPolicy = LogPolicyStdout;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Invalid number of arguments." << std::endl;
        std::cerr << "Usage: " << argv[0]
                  << " <server_host> <server_port> <serial_port>" << std::endl;
        return EXIT_FAILURE;
    }

    ClientTCP<LogPolicyStdout> client;

    std::shared_ptr<SerialPolicyUnix> porta =
        std::make_shared<SerialPolicyUnix>();

    while (true) {

        if (!client.isConnect()) {
            if (client.connect(argv[1], atoi(argv[2]))) {
                LogPolicy::log("Connected.\n");
                uint8_t data[3] = {0x05, 0x05, 0x05};
                client.tx(data, 3);
            } else {
                sleep(5);
                continue;
            }
        }

        std::uint8_t rx_data[1024];
        size_t rx_sz = client.rx(rx_data, sizeof(rx_data));

        if (rx_sz > 0) {
            // convert the buffer rx_data to a string
            std::string data_str(reinterpret_cast<char*>(rx_data), rx_sz);

            LogPolicy::log("Received data: %s\n", data_str.c_str());

            std::vector<NBR14522::comando_t> comandos =
                getComandosFromData(data_str);

            readRespostasFromMeter<LogPolicyStdout>(comandos, argv[3], [&](const resposta_t& rsp) {
                client.tx(rsp.data(), rsp.size());
            });
        }

        sleep(1);
    }

    client.disconnect();

    return EXIT_SUCCESS;
}