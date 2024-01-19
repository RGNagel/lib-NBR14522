#include "doctest/doctest.h"
#include <NBR14522.h>
#include <get_comandos_from_data.h>

TEST_CASE("split") {
    std::string str = "14010203 27010203 28010203";
    std::vector<std::string> tokens = split(str, " ");

    REQUIRE(tokens.size() == 3);

    CHECK(tokens.at(0) == "14010203");
    CHECK(tokens.at(1) == "27010203");
    CHECK(tokens.at(2) == "28010203");
}

TEST_CASE("getComandosFromData") {
    std::string data = "14010203 27010203 28010203";
    std::vector<comando_t> cmds = getComandosFromData(data);
    
    REQUIRE(cmds.size() == 3);
    
    CHECK(cmds.at(0).at(0) == 0x14);
    CHECK(cmds.at(0).at(1) == 0x01);
    CHECK(cmds.at(0).at(2) == 0x02);
    CHECK(cmds.at(0).at(3) == 0x03);
    CHECK(cmds.at(0).at(4) == 0x00);

    CHECK(cmds.at(1).at(0) == 0x27);
    CHECK(cmds.at(1).at(1) == 0x01);
    CHECK(cmds.at(1).at(2) == 0x02);
    CHECK(cmds.at(1).at(3) == 0x03);
    CHECK(cmds.at(1).at(4) == 0x00);

    CHECK(cmds.at(2).at(0) == 0x28);
    CHECK(cmds.at(2).at(1) == 0x01);
    CHECK(cmds.at(2).at(2) == 0x02);
    CHECK(cmds.at(2).at(3) == 0x03);
    CHECK(cmds.at(2).at(4) == 0x00);
}