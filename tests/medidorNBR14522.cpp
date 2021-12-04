#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <iport.h>
// This is all that is needed to compile a test-runner executable.
// More tests can be added here, or in a new tests/*.cpp file.

TEST_CASE("medidor síncrono com enqueue,transmit e dequeue") {
    
    IPort serial("/path/to/port");

    IMeter meter(serial);

    CommandNBR14522 cmd1("141234560001");
    CommandNBR14522 cmd2("21123456");

    meter.enqueue(cmd1);
    meter.enqueue(cmd2);

    meter.transmit();

    // faz sentido ter a opcao de remover da fila um comando que ja foi
    // enfileirado, antes dele ser transmitido? qualquer coisa o meter pode
    // retornar a lista da fila pro usuário remover algum elemento (commando)
    // caso queira
    // queue cmds = meter.getQueue();

    CommandNBR14522 cmd;
    while (meter.hasCommandResponse()) {
        cmd = meter.dequeue();

        cmd.printHexa();
    }
}

