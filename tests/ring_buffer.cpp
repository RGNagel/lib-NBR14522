#include "doctest/doctest.h"

#include <NBR14522.h>
#include <ring_buffer.h>

TEST_CASE("Ring Buffer") {
    RingBuffer<byte_t, 3> rb;
    CHECK(rb.toread() == 0);
    rb.write(0xFF);
    CHECK(rb.toread() == 1);
    CHECK(rb.read() == 0xFF);
    CHECK(rb.toread() == 0);

    rb.write(0x11);
    rb.write(0x22);
    rb.write(0x33);
    CHECK(rb.toread() == 3);
    rb.write(0x44);
    CHECK(rb.toread() == 3);
    rb.write(0x55);
    CHECK(rb.toread() == 3);
    rb.write(0x66);
    CHECK(rb.toread() == 3);

    // only the last 3 elements must be available in the ringbuffer
    CHECK(0x44 == rb.read());
    CHECK(rb.toread() == 2);
    CHECK(0x55 == rb.read());
    CHECK(rb.toread() == 1);
    CHECK(0x66 == rb.read());
    CHECK(rb.toread() == 0);

    // if ringbuffer is empty, a read operation returns the first element
    // amongst the last 3 ones.

    CHECK(0x44 == rb.read());
    CHECK(0x44 == rb.read());
    CHECK(0x44 == rb.read());
    CHECK(0x44 == rb.read());
    CHECK(rb.toread() == 0);

    // writing after all elements were read
    rb.write(0x77);
    CHECK(1 == rb.toread());
    CHECK(0x77 == rb.read());
    CHECK(0 == rb.toread());

    rb.write(0x88);
    rb.write(0x99);
    rb.write(0xAA);
    CHECK(3 == rb.toread());
    CHECK(0x88 == rb.read());
    CHECK(0x99 == rb.read());
    CHECK(0xAA == rb.read());
}
