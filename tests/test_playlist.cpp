#include "playlist.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Playlist next/prev wrap")
{
    Playlist pl;
    pl.Add({"a", "a", 0, {}});
    pl.Add({"b", "b", 0, {}});
    pl.SetCurrent(0);

    REQUIRE(pl.NextIndex(RepeatMode::All) == 1);
    REQUIRE(pl.PrevIndex(RepeatMode::All) == 1); // wraps around
}
