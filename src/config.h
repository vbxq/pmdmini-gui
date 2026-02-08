#pragma once

#include <chrono>
#include <filesystem>
#include <string>

enum class RepeatMode
{
    Off,
    One,
    All
};

struct Config
{
    std::string last_directory;
    int volume = 100;
    bool mute = false;
    bool recursive_scan = false;
    bool shuffle = false;
    RepeatMode repeat = RepeatMode::Off;

    int window_x = 100;
    int window_y = 100;
    int window_w = 1200;
    int window_h = 800;

    std::string audio_device;

    bool crossfade_enabled = false;
    int crossfade_duration_ms = 1000;

    bool Load(const std::filesystem::path &path);
    bool Save(const std::filesystem::path &path) const;

    void MarkDirty(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    bool ShouldSave(std::chrono::steady_clock::time_point now,
                    std::chrono::milliseconds debounce) const;
    void Saved();

  private:
    bool dirty_ = false;
    std::chrono::steady_clock::time_point dirty_since_{};
};
