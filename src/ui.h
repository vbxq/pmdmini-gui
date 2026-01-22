#pragma once

#include "config.h"
#include "player.h"
#include "scanner.h"
#include <string>
#include <vector>

struct UIState
{
    std::string directory;
    bool recursive = false;
    std::string search;
    SortMode sort = SortMode::Name;

    std::vector<TrackEntry> tracks;
    int selected_index = -1;
    int current_index = -1;

    bool shuffle = false;
    RepeatMode repeat = RepeatMode::Off;
    int volume = 100;
    bool mute = false;

    std::vector<std::string> audio_devices;
    int audio_device_index = 0;

    PlayerState player_state = PlayerState::Stopped;
    bool duration_known = false;
    float position_sec = 0;
    float duration_sec = 0;

    std::string status;
    bool scanning = false;
};

struct UIActions
{
    bool request_browse = false;
    bool request_scan = false;
    bool play_selected = false;
    bool toggle_play_pause = false;
    bool stop = false;
    bool next = false;
    bool prev = false;

    bool shuffle_toggled = false;
    bool repeat_cycle = false;
    bool mute_toggled = false;
    bool mute = false;

    bool volume_changed = false;
    int volume = 100;

    bool audio_device_changed = false;
    int audio_device_index = 0;

    bool directory_changed = false;
    std::string directory;
    bool recursive_changed = false;
    bool recursive = false;
    bool search_changed = false;
    std::string search;

    bool sort_changed = false;
    SortMode sort = SortMode::Name;
    int select_index = -1;
};

class UI
{
  public:
    UI();
    void Draw(const UIState &state, UIActions &actions, const float *waveform, size_t waveform_len);
    void RequestSearchFocus();

  private:
    void SyncTextBuffers(const UIState &state);

    std::string dir_cache_;
    std::string search_cache_;
    char dir_buf_[512] = {};
    char search_buf_[256] = {};
    bool focus_search_ = false;
};
