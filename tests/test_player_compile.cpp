#include "player.h"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

TEST_CASE("Player basic construction") {
    Player player;
    REQUIRE(player.GetState() == PlayerState::Stopped);
}

TEST_CASE("Player loading state default") {
    Player player;
    REQUIRE(player.IsLoading() == false);
}

TEST_CASE("Frame duration helper returns positive") {
    REQUIRE(Player::FrameDurationMs(1024, 48000) > 0);
}

TEST_CASE("Normalize device list adds default") {
    std::vector<std::string> empty;
    auto list = Player::NormalizeDeviceList(empty);

    REQUIRE(list.size() == 1);
    REQUIRE(list[0] == "Default");
}
