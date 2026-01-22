#include "ring_buffer.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Ring buffer basic push/pop")
{
    RingBuffer rb(8);

    float in[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    REQUIRE(rb.Write(in, 4) == 4);

    float out[4] = {0};
    REQUIRE(rb.Read(out, 4) == 4);
    REQUIRE(out[2] == Catch::Approx(0.3f));
}

TEST_CASE("Ring buffer drops new samples on overflow")
{
    RingBuffer rb(4);

    float in[6] = {1, 2, 3, 4, 5, 6};
    // capacity is 4+1=5, so we can store 4 samples
    // samples 5 and 6 should be dropped
    REQUIRE(rb.Write(in, 6) == 4);

    float out[4] = {0};
    REQUIRE(rb.Read(out, 4) == 4);
    REQUIRE(out[0] == Catch::Approx(1));
    REQUIRE(out[3] == Catch::Approx(4));
}

TEST_CASE("Ring buffer available count")
{
    RingBuffer rb(8);

    REQUIRE(rb.Available() == 0);

    float in[3] = {1, 2, 3};
    rb.Write(in, 3);
    REQUIRE(rb.Available() == 3);

    float out[2];
    rb.Read(out, 2);
    REQUIRE(rb.Available() == 1);
}

TEST_CASE("Ring buffer clear")
{
    RingBuffer rb(8);

    float in[4] = {1, 2, 3, 4};
    rb.Write(in, 4);
    REQUIRE(rb.Available() == 4);

    rb.Clear();
    REQUIRE(rb.Available() == 0);
}
