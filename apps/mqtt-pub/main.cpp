#include "NBR14522.h"
#include "serial/serial_policy_unix.h"
#include <cstring>
#include <get_comandos_from_data.h>
#include <hexstr2bytes.h>
#include <leitor.h>
#include <log_policy.h>
#include <queue>
#include <timer/timer_policy_generic_os.h>
#include <vector>

#include <unistd.h> // usleep(), close(), getopt(), Include before MQTT-C includes.

#include "MQTT-C/examples/templates/posix_sockets.h"
#include "MQTT-C/include/mqtt.h"
#include "MQTT-C/include/mqtt_pal.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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

struct reconnect_state_t {
    const char* hostname;
    const char* port;
    uint8_t* sendbuf;
    size_t sendbufsz;
    uint8_t* recvbuf;
    size_t recvbufsz;
    int keep_alive;
};

void reconnect_callback(struct mqtt_client* client, void** state) {
    struct reconnect_state_t* reconnect_state =
        *((struct reconnect_state_t**)state);

    /* Close the clients socket if this isn't the initial reconnect call */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        close(client->socketfd);
    }

    /* Perform error handling here. */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        printf("reconnect_callback: called while client was in error state "
               "\"%s\"\n",
               mqtt_error_str(client->error));
    }

    /* Open a new socket. */
    int sockfd =
        open_nb_socket(reconnect_state->hostname, reconnect_state->port);
    if (sockfd == -1) {
        fprintf(stderr, "Failed to open socket: %s\n", strerror(errno));
        sleep(5);
        return;
    }

    /* Reinitialize the client. */
    mqtt_reinit(client, sockfd, reconnect_state->sendbuf,
                reconnect_state->sendbufsz, reconnect_state->recvbuf,
                reconnect_state->recvbufsz);

    /* Create an anonymous session */
    const char* client_id = NULL;
    /* Ensure we have a clean session */
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    /* Send connection request to the broker. */
    mqtt_connect(client, client_id, NULL, NULL, 0, NULL, NULL, connect_flags,
                 reconnect_state->keep_alive);

    /* Subscribe to the topic. */
    mqtt_subscribe(client, "cmds", 1);
}

std::queue<resposta_t> fifoRespostas;
std::queue<std::string> fifoErrors;
const char* serial_port_name;

void publish_callback(void** unused, struct mqtt_response_publish* published) {
    /* note that published->topic_name is NOT null-terminated (here we'll change
     * it to a c-string) */
    char* topic_name = (char*)malloc(published->topic_name_size + 1);
    memcpy(topic_name, published->topic_name, published->topic_name_size);
    topic_name[published->topic_name_size] = '\0';

    char* message = (char*)malloc(published->application_message_size + 1);
    memcpy(message, published->application_message,
           published->application_message_size);
    message[published->application_message_size] = '\0';

    LogPolicy::log("Received publish('%s'): %s\n", topic_name,
                   (const char*)message);

    if (strcmp(topic_name, "cmds") == 0) {
        std::string data_str((const char*)message);
        std::vector<NBR14522::comando_t> comandos =
            getComandosFromData(data_str);
        if (comandos.size() > 0) {
            if (!readRespostasFromMeter<LogPolicyStdout>(
                    comandos, serial_port_name,
                    [&](const resposta_t& rsp) { fifoRespostas.push(rsp); })) {
                LogPolicy::log("Erro ao ler respostas do medidor\n");
            }
        } else {
            LogPolicy::log("Nenhum comando válido recebido\n");
        }
    }

    free(topic_name);
    free(message);
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Invalid number of arguments." << std::endl;
        std::cerr
            << "Usage: " << argv[0]
            << " <server_host> <server_port> <serial_port> [<tx_period_sec>]"
            << std::endl;
        return EXIT_FAILURE;
    }

    serial_port_name = argv[3];

    uint8_t sendbuf[2048]; /* sendbuf should be large enough to hold multiple
                              whole mqtt messages */
    uint8_t recvbuf[1024]; /* recvbuf should be large enough any whole mqtt
                              message expected to be received */

    struct reconnect_state_t reconnect_state;
    reconnect_state.hostname = argv[1];
    reconnect_state.port = argv[2];
    reconnect_state.sendbuf = sendbuf;
    reconnect_state.sendbufsz = sizeof(sendbuf);
    reconnect_state.recvbuf = recvbuf;
    reconnect_state.recvbufsz = sizeof(recvbuf);
    reconnect_state.keep_alive = 300;

    /* setup a client */
    struct mqtt_client client;

    mqtt_init_reconnect(&client, reconnect_callback, &reconnect_state,
                        publish_callback);

    std::chrono::time_point<std::chrono::system_clock> last_tx =
        std::chrono::system_clock::now();

    int tx_period_sec = (argc > 4) ? atoi(argv[4]) : 10;

    enum { IDLE, WAITING_FOR_PUBLISH_ACK } state = IDLE;

    int last_number_of_pubacks = 0;

    while (true) {

        std::chrono::duration<double> elapsed_seconds_since_last_tx =
            std::chrono::system_clock::now() - last_tx;

        switch (state) {
        case IDLE:
            if (!fifoRespostas.empty() &&
                elapsed_seconds_since_last_tx >
                    std::chrono::seconds(tx_period_sec)) {
                resposta_t rsp = fifoRespostas.front();
                fifoRespostas.pop();
                // log
                LogPolicy::log("Publishing rsp: %02x%02x%02x%02x%02x%02x%02x...\n", rsp[0], rsp[1], rsp[2], rsp[3], rsp[4], rsp[5], rsp[6]);
                mqtt_publish(&client, "rsps", rsp.data(), rsp.size(),
                             MQTT_PUBLISH_QOS_1);
                state = WAITING_FOR_PUBLISH_ACK;
                last_number_of_pubacks = client.number_of_pubacks;
                last_tx = std::chrono::system_clock::now();
            }
            break;
        case WAITING_FOR_PUBLISH_ACK:
            if (client.number_of_pubacks > last_number_of_pubacks)
                state = IDLE;
            break;
        }

        mqtt_sync(&client);
        usleep(100000U);
    }

    return EXIT_SUCCESS;
}