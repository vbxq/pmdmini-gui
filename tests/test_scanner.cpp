#include "scanner.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Scanner filters extensions") {
    REQUIRE(IsPmdFile("song.M"));
    REQUIRE(IsPmdFile("song.m2"));
    REQUIRE(IsPmdFile("TRACK.M2"));
    REQUIRE_FALSE(IsPmdFile("song.txt"));
    REQUIRE_FALSE(IsPmdFile("readme.md"));
}
