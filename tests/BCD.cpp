#include <BCD.h>
#include <doctest/doctest.h>

TEST_CASE("bcd2dec") {
    CHECK(0 == bcd2dec(0x00));
    CHECK(1 == bcd2dec(0x01));
    CHECK(14 == bcd2dec(0x14));
    CHECK(99 == bcd2dec(0x99));
}

TEST_CASE("dec2bcd") {
    CHECK(0x00 == dec2bcd(0));
    CHECK(0x01 == dec2bcd(1));
    CHECK(0x14 == dec2bcd(14));
    CHECK(0x99 == dec2bcd(99));
}