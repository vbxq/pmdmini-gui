#include "config.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static int RepeatToInt(RepeatMode m)
{
    switch (m)
    {
    case RepeatMode::Off:
        return 0;
    case RepeatMode::One:
        return 1;
    case RepeatMode::All:
        return 2;
    }
    return 0;
}

static RepeatMode IntToRepeat(int v)
{
    switch (v)
    {
    case 1:
        return RepeatMode::One;
    case 2:
        return RepeatMode::All;
    default:
        return RepeatMode::Off;
    }
}

bool Config::Load(const std::filesystem::path &path)
{
    std::ifstream f(path);
    if (!f)
        return false;

    json j;
    try
    {
        f >> j;
    }
    catch (...)
    {
        return false;
    }

    last_directory = j.value("last_directory", "");
    volume = j.value("volume", 100);
    mute = j.value("mute", false);
    recursive_scan = j.value("recursive_scan", false);
    shuffle = j.value("shuffle", false);
    repeat = IntToRepeat(j.value("repeat", 0));
    window_x = j.value("window_x", 100);
    window_y = j.value("window_y", 100);
    window_w = j.value("window_w", 1200);
    window_h = j.value("window_h", 800);
    audio_device = j.value("audio_device", "");
    crossfade_enabled = j.value("crossfade_enabled", false);
    crossfade_duration_ms = j.value("crossfade_duration_ms", 1000);

    return true;
}

bool Config::Save(const std::filesystem::path &path) const
{
    json j;
    j["last_directory"] = last_directory;
    j["volume"] = volume;
    j["mute"] = mute;
    j["recursive_scan"] = recursive_scan;
    j["shuffle"] = shuffle;
    j["repeat"] = RepeatToInt(repeat);
    j["window_x"] = window_x;
    j["window_y"] = window_y;
    j["window_w"] = window_w;
    j["window_h"] = window_h;
    j["audio_device"] = audio_device;
    j["crossfade_enabled"] = crossfade_enabled;
    j["crossfade_duration_ms"] = crossfade_duration_ms;

    std::ofstream f(path);
    if (!f)
        return false;

    f << j.dump(2);
    return true;
}

void Config::MarkDirty(std::chrono::steady_clock::time_point now)
{
    if (!dirty_)
        dirty_since_ = now;
    dirty_ = true;
}

bool Config::ShouldSave(std::chrono::steady_clock::time_point now,
                        std::chrono::milliseconds debounce) const
{
    if (!dirty_)
        return false;
    return (now - dirty_since_) >= debounce;
}

void Config::Saved()
{
    dirty_ = false;
}
