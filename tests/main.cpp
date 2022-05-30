#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

// This is all that is needed to compile a test-runner executable.
// More tests can be added here, or in a new tests/*.cpp file.

// TEST_CASE("aplicação final") {

//     IPort fakeSerial;
//     IMeter meter(fakeSerial);

//     TEST_CASE("usuário comum") {

//         TEST_CASE("consumo de energia") { meter.getConsumo(); }

//         TEST_CASE("histórico do consumo de energia") {
//             meter.getHistoricoDeConsumo();
//         }
//     }

//     TEST_CASE("concessionária") {

//         TEST_CASE("leitura padronizada") {

//             leituraPadrao LeituraPadronizadaNBR14522(meter);

//             leituraPadrao.ReposicaoDeDemanda();
//             // ou
//             leituraPadrao.leituraPadronizada(REPOSICA0_DE_DEMANDA);
//             // ou
//             meter.comando(REPOSICA0_DE_DEMANDA, 3, 8);
//             // ou
//             meter.comando(REPOSICA0_DE_DEMANDA);
//         }

//         TEST_CASE("parametrização") { meter.comando("3312345611223398"); }
//     }
// }
