#include "ui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

UI::UI()
{
    for (size_t i = 0; i < kWaveformBars; i++)
    {
        waveform_peaks_[i] = 0.0f;
        waveform_smooth_[i] = 0.0f;
    }
}

void UI::RequestSearchFocus()
{
    focus_search_ = true;
}

static const char *GetRepeatLabel(RepeatMode m)
{
    switch (m)
    {
    case RepeatMode::Off:
        return "Repeat: Off";
    case RepeatMode::One:
        return "Repeat: One";
    case RepeatMode::All:
        return "Repeat: All";
    }
    return "Repeat: Off";
}

void UI::SyncTextBuffers(const UIState &state)
{
    if (state.directory != dir_cache_)
    {
        dir_cache_ = state.directory;
        snprintf(dir_buf_, sizeof(dir_buf_), "%s", dir_cache_.c_str());
    }
    if (state.search != search_cache_)
    {
        search_cache_ = state.search;
        snprintf(search_buf_, sizeof(search_buf_), "%s", search_cache_.c_str());
    }
}

void UI::Draw(const UIState &state, UIActions &actions, const float *waveform, size_t waveform_len)
{
    SyncTextBuffers(state);

    ImGuiIO &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("pmdmini-gui", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize);

    const float top_h = 48.0f;
    const float bottom_h = 84.0f;
    const float left_w = 320.0f;

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // top bar
    ImGui::BeginChild("topbar", ImVec2(avail.x, top_h), false);

    ImGui::SetNextItemWidth(avail.x - 320.0f);
    if (ImGui::InputText("##dir", dir_buf_, sizeof(dir_buf_)))
    {
        actions.directory_changed = true;
        actions.directory = dir_buf_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Browse"))
        actions.request_browse = true;

    ImGui::SameLine();
    if (ImGui::Button("Scan"))
        actions.request_scan = true;

    ImGui::SameLine();
    bool rec = state.recursive;
    if (ImGui::Checkbox("Recursive", &rec))
    {
        actions.recursive_changed = true;
        actions.recursive = rec;
    }

    ImGui::EndChild();

    avail = ImGui::GetContentRegionAvail();
    float center_h = avail.y - bottom_h;

    // left panel - playlist
    ImGui::BeginChild("left", ImVec2(left_w, center_h), true);

    ImGui::TextUnformatted("Playlist");

    if (focus_search_)
    {
        ImGui::SetKeyboardFocusHere();
        focus_search_ = false;
    }

    if (ImGui::InputTextWithHint("##search", "Search", search_buf_, sizeof(search_buf_)))
    {
        actions.search_changed = true;
        actions.search = search_buf_;
    }

    const char *sort_opts[] = {"Name", "Date", "Size"};
    int sort_idx = (int)state.sort;
    if (ImGui::Combo("Sort", &sort_idx, sort_opts, 3))
    {
        actions.sort_changed = true;
        actions.sort = (SortMode)sort_idx;
    }

    ImGui::Separator();

    ImGui::BeginChild("tracks", ImVec2(0, 0), true);
    for (int i = 0; i < (int)state.tracks.size(); i++)
    {
        const auto &track = state.tracks[i];
        bool selected = (i == state.selected_index);
        bool is_current = (i == state.current_index);

        if (is_current)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.4f, 1.0f));

        if (ImGui::Selectable(track.display_name.c_str(), selected))
        {
            actions.select_index = i;
            if (ImGui::IsMouseDoubleClicked(0))
                actions.play_selected = true;
        }

        if (is_current)
            ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::EndChild();

    // right panel - now playing + waveform
    ImGui::SameLine();
    ImGui::BeginChild("right", ImVec2(0, center_h), true);

    const char *track_name = "None";
    if (state.current_index >= 0 && state.current_index < (int)state.tracks.size())
        track_name = state.tracks[state.current_index].display_name.c_str();

    ImGui::Text("Now Playing: %s", track_name);

    const char *state_str = "Stopped";
    if (state.player_state == PlayerState::Playing)
        state_str = "Playing";
    else if (state.player_state == PlayerState::Paused)
        state_str = "Paused";
    ImGui::Text("State: %s", state_str);

    // waveform
    ImVec2 region = ImGui::GetContentRegionAvail();
    float wave_h = std::max(80.0f, region.y - 40.0f);
    float wave_w = std::max(1.0f, region.x);

    DrawWaveform(waveform, waveform_len, wave_w, wave_h);

    ImGui::EndChild();

    // bottom bar - transport
    ImGui::BeginChild("transport", ImVec2(avail.x, bottom_h), true);

    if (ImGui::Button("Prev"))
        actions.prev = true;
    ImGui::SameLine();

    const char *play_btn = (state.player_state == PlayerState::Playing) ? "Pause" : "Play";
    if (ImGui::Button(play_btn))
        actions.toggle_play_pause = true;

    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        actions.stop = true;

    ImGui::SameLine();
    if (ImGui::Button("Next"))
        actions.next = true;

    ImGui::SameLine();
    ImGui::Text("  ");

    ImGui::SameLine();
    bool mute = state.mute;
    if (ImGui::Checkbox("Mute", &mute))
    {
        actions.mute_toggled = true;
        actions.mute = mute;
    }

    ImGui::SameLine();
    int vol = state.volume;
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::SliderInt("Volume", &vol, 0, 100))
    {
        actions.volume_changed = true;
        actions.volume = vol;
    }

    if (!state.audio_devices.empty())
    {
        ImGui::SameLine();

        std::vector<const char *> dev_names;
        for (auto &d : state.audio_devices)
            dev_names.push_back(d.c_str());

        int dev_idx = state.audio_device_index;
        ImGui::SetNextItemWidth(220.0f);

        if (ImGui::Combo("Output", &dev_idx, dev_names.data(), (int)dev_names.size()))
        {
            actions.audio_device_changed = true;
            actions.audio_device_index = dev_idx;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(state.shuffle ? "Shuffle: On" : "Shuffle: Off"))
        actions.shuffle_toggled = true;

    ImGui::SameLine();
    if (ImGui::Button(GetRepeatLabel(state.repeat)))
        actions.repeat_cycle = true;

    char dur_str[32];
    if (state.duration_known)
        snprintf(dur_str, sizeof(dur_str), "%.1fs", state.duration_sec);
    else
        snprintf(dur_str, sizeof(dur_str), "???");

    ImGui::Text("Position: %.1fs / %s", state.position_sec, dur_str);

    if (!state.status.empty())
        ImGui::TextWrapped("Status: %s", state.status.c_str());

    ImGui::EndChild();

    ImGui::End();
}

void UI::DrawWaveform(const float *waveform, size_t waveform_len, float width, float height)
{
    ImGui::InvisibleButton("waveform", ImVec2(width, height));

    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();

    // background
    dl->AddRectFilled(p0, p1, IM_COL32(18, 20, 28, 255));

    // grid lines
    float mid_y = (p0.y + p1.y) * 0.5f;
    dl->AddLine(ImVec2(p0.x, mid_y), ImVec2(p1.x, mid_y), IM_COL32(40, 45, 55, 255));

    // calculate peak levels for each bar from stereo waveform
    if (waveform && waveform_len > 1)
    {
        size_t samples_per_bar = std::max<size_t>(1, waveform_len / kWaveformBars);

        for (size_t bar = 0; bar < kWaveformBars; bar++)
        {
            // find peak in this segment (use absolute value for amplitude)
            float peak = 0.0f;
            size_t start = bar * samples_per_bar;
            size_t end = std::min(start + samples_per_bar, waveform_len);

            for (size_t i = start; i < end; i++)
            {
                float v = std::fabs(waveform[i]);
                if (v > peak)
                    peak = v;
            }

            // smooth transition (fast attack, slow decay)
            float target = std::min(1.0f, peak * 1.5f);
            if (target > waveform_smooth_[bar])
                waveform_smooth_[bar] = waveform_smooth_[bar] * 0.3f + target * 0.7f;
            else
                waveform_smooth_[bar] = waveform_smooth_[bar] * 0.92f + target * 0.08f;

            // peak hold with decay
            if (target > waveform_peaks_[bar])
                waveform_peaks_[bar] = target;
            else
                waveform_peaks_[bar] *= 0.97f;
        }
    }
    else
    {
        // decay when no audio
        for (size_t bar = 0; bar < kWaveformBars; bar++)
        {
            waveform_smooth_[bar] *= 0.9f;
            waveform_peaks_[bar] *= 0.95f;
        }
    }

    // draw bars
    float bar_width = width / (float)kWaveformBars;
    float gap = 2.0f;
    float usable_bar_w = std::max(1.0f, bar_width - gap);
    float max_bar_h = height * 0.45f;

    for (size_t bar = 0; bar < kWaveformBars; bar++)
    {
        float level = waveform_smooth_[bar];
        float peak_level = waveform_peaks_[bar];
        float bar_h = level * max_bar_h;
        float peak_h = peak_level * max_bar_h;

        float x = p0.x + bar * bar_width + gap * 0.5f;

        // color gradient based on level (blue to cyan to green)
        uint8_t r = (uint8_t)(40 + level * 60);
        uint8_t g = (uint8_t)(120 + level * 100);
        uint8_t b = (uint8_t)(200 - level * 80);

        // draw bar above center line
        if (bar_h > 0.5f)
        {
            ImVec2 bar_p0(x, mid_y - bar_h);
            ImVec2 bar_p1(x + usable_bar_w, mid_y);
            dl->AddRectFilled(bar_p0, bar_p1, IM_COL32(r, g, b, 200));
        }

        // draw bar below center line (mirrored)
        if (bar_h > 0.5f)
        {
            ImVec2 bar_p0(x, mid_y);
            ImVec2 bar_p1(x + usable_bar_w, mid_y + bar_h);
            dl->AddRectFilled(bar_p0, bar_p1, IM_COL32(r, g, b, 180));
        }

        // draw peak indicator
        if (peak_h > bar_h + 2.0f)
        {
            float peak_y_top = mid_y - peak_h;
            float peak_y_bot = mid_y + peak_h;
            dl->AddLine(ImVec2(x, peak_y_top), ImVec2(x + usable_bar_w, peak_y_top),
                        IM_COL32(200, 230, 255, 150));
            dl->AddLine(ImVec2(x, peak_y_bot), ImVec2(x + usable_bar_w, peak_y_bot),
                        IM_COL32(200, 230, 255, 150));
        }
    }

    // draw subtle glow on center line when active
    float total_level = 0.0f;
    for (size_t i = 0; i < kWaveformBars; i++)
        total_level += waveform_smooth_[i];
    total_level /= kWaveformBars;

    if (total_level > 0.01f)
    {
        uint8_t glow_alpha = (uint8_t)(total_level * 60);
        dl->AddLine(ImVec2(p0.x, mid_y), ImVec2(p1.x, mid_y),
                    IM_COL32(100, 180, 255, glow_alpha), 2.0f);
    }
}
