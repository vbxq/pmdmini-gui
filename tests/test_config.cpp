#include "config.h"
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>

TEST_CASE("Config defaults are valid")
{
    Config cfg;
    REQUIRE(cfg.volume == 100);
    REQUIRE(cfg.recursive_scan == false);
}

TEST_CASE("Config round trip")
{
    Config cfg;
    cfg.last_directory = "/tmp";
    cfg.volume = 42;
    cfg.shuffle = true;

    std::filesystem::path path = "/tmp/pmdmini-gui-config-test.json";
    REQUIRE(cfg.Save(path));

    Config loaded;
    REQUIRE(loaded.Load(path));
    REQUIRE(loaded.last_directory == "/tmp");
    REQUIRE(loaded.volume == 42);
    REQUIRE(loaded.shuffle == true);
}

TEST_CASE("Config save debounce")
{
    Config cfg;
    auto t = std::chrono::steady_clock::time_point{};

    cfg.MarkDirty(t);
    REQUIRE(cfg.ShouldSave(t, std::chrono::milliseconds(0)));
}
