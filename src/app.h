#pragma once

#include "config.h"
#include "player.h"
#include "playlist.h"
#include "scanner.h"
#include "ui.h"
#include <SDL.h>
#include <chrono>
#include <vector>

class App
{
  public:
    App();
    ~App();

    int Run();

  private:
    bool PlayIndex(int index);
    void PlayNext();
    void SyncConfig();

    void UpdateUIState(UIState &state, const std::vector<int> &visible_map) const;
    void BuildVisibleList(std::vector<TrackEntry> &out_tracks, std::vector<int> &out_map) const;

    bool HandleActions(const UIActions &actions, const std::vector<int> &visible_map,
                       std::chrono::steady_clock::time_point now);
    void HandleShortcuts(bool capture_keyboard, const SDL_Event &ev);

    Config config_;
    Player player_;
    Scanner scanner_;
    Playlist playlist_;
    UI ui_;

    std::string directory_;
    std::string search_;
    SortMode sort_ = SortMode::Name;
    bool recursive_ = false;
    bool shuffle_ = false;
    RepeatMode repeat_ = RepeatMode::Off;
    int volume_ = 100;
    bool mute_ = false;
    std::string status_;
    bool scanning_active_ = false;

    std::vector<std::string> audio_devices_;
    int audio_device_index_ = 0;

    std::vector<float> waveform_;
};
