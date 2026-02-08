#include "app.h"
#include "icon_data.h"
#include "logger.h"
#include "utils.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <algorithm>
#include <cstdlib>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <tinyfiledialogs.h>

namespace
{

int MapIndexToVisible(int idx, const std::vector<int> &map)
{
    for (size_t i = 0; i < map.size(); i++)
    {
        if (map[i] == idx)
            return (int)i;
    }
    return -1;
}

std::filesystem::path GetConfigPath()
{
#ifdef _WIN32
    auto appdata = std::getenv("APPDATA");
    if (!appdata)
        return "config.json";
    return std::filesystem::path(appdata) / "pmdmini-gui" / "config.json";
#else
    auto home = std::getenv("HOME");
    if (!home)
        return "config.json";
    return std::filesystem::path(home) / ".config" / "pmdmini-gui" / "config.json";
#endif
}

void EnsureParentDir(const std::filesystem::path &p)
{
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
}

} // namespace

void App::SyncConfig()
{
    config_.last_directory = directory_;
    config_.recursive_scan = recursive_;
    config_.shuffle = shuffle_;
    config_.repeat = repeat_;
    config_.volume = volume_;
    config_.mute = mute_;
    config_.crossfade_enabled = crossfade_enabled_;
    config_.crossfade_duration_ms = crossfade_duration_ms_;

    if (!audio_devices_.empty())
    {
        auto &dev = audio_devices_[audio_device_index_];
        config_.audio_device = (dev == "Default") ? "" : dev;
    }
}

App::App()
{
    Logger::Init();
}

App::~App()
{
    Logger::Shutdown();
}

bool App::PlayIndex(int index, bool fade_in)
{
    auto &items = playlist_.Items();
    if (index < 0 || index >= (int)items.size())
        return false;

    auto &entry = items[index];
    if (!player_.Load(entry.path))
    {
        status_ = "Failed to load track";
        return false;
    }

    playlist_.SetCurrent(index);
    player_.SetVolume(volume_);
    player_.SetMute(mute_);

    if (fade_in)
    {
        player_.SetPendingFadeIn(crossfade_duration_ms_);
        status_ = "Fading in...";
    }

    player_.Play();
    if (!fade_in)
        status_ = "Playing";
    return true;
}

void App::PlayNext()
{
    int next = playlist_.NextIndex(repeat_, shuffle_);
    if (next < 0)
    {
        player_.Stop();
        status_ = "Playlist ended";
        return;
    }

    if (crossfade_enabled_ && player_.GetState() == PlayerState::Playing)
    {
        fading_to_next_ = true;
        pending_next_index_ = next;
        player_.StartFadeOut(crossfade_duration_ms_);
        status_ = "Fading out...";
    }
    else
    {
        PlayIndex(next);
    }
}

void App::BuildVisibleList(std::vector<TrackEntry> &out_tracks, std::vector<int> &out_map) const
{
    out_tracks.clear();
    out_map.clear();

    auto &items = playlist_.Items();
    for (int i = 0; i < (int)items.size(); i++)
    {
        if (utils::contains_ignore_case(items[i].display_name, search_))
        {
            out_tracks.push_back(items[i]);
            out_map.push_back(i);
        }
    }
}

void App::UpdateUIState(UIState &state, const std::vector<int> &visible_map) const
{
    state.directory = directory_;
    state.recursive = recursive_;
    state.search = search_;
    state.sort = sort_;
    state.shuffle = shuffle_;
    state.repeat = repeat_;
    state.volume = volume_;
    state.mute = mute_;
    state.audio_devices = audio_devices_;
    state.audio_device_index = audio_device_index_;
    state.player_state = player_.GetState();
    state.status = status_;
    state.scanning = scanning_active_;

    auto info = player_.GetTrackInfo();
    float sr = info.sample_rate > 0 ? (float)info.sample_rate : 44100.0f;

    state.position_sec = player_.GetPositionSamples() / sr;
    state.duration_known = info.duration_known;
    state.duration_sec = info.duration_known ? info.duration_samples / sr : 0.0f;

    state.selected_index = MapIndexToVisible(playlist_.SelectedIndex(), visible_map);
    state.current_index = MapIndexToVisible(playlist_.CurrentIndex(), visible_map);
    state.crossfade_enabled = crossfade_enabled_;
    state.crossfade_duration_ms = crossfade_duration_ms_;
}

bool App::HandleActions(const UIActions &actions, const std::vector<int> &visible_map,
                        std::chrono::steady_clock::time_point now)
{
    bool changed = false;

    if (actions.directory_changed)
    {
        directory_ = actions.directory;
        changed = true;
    }

    if (actions.recursive_changed)
    {
        recursive_ = actions.recursive;
        changed = true;
    }

    if (actions.search_changed)
        search_ = actions.search;

    if (actions.sort_changed)
    {
        sort_ = actions.sort;
        playlist_.Sort(sort_);
    }

    if (actions.select_index >= 0 && actions.select_index < (int)visible_map.size())
        playlist_.SetSelected(visible_map[actions.select_index]);

    if (actions.play_selected)
        PlayIndex(playlist_.SelectedIndex());

    if (actions.toggle_play_pause)
    {
        auto st = player_.GetState();
        if (st == PlayerState::Playing)
            player_.Pause();
        else if (st == PlayerState::Paused)
            player_.Play();
        else
            PlayIndex(playlist_.SelectedIndex());
    }

    if (actions.stop)
        player_.Stop();

    if (actions.next)
        PlayNext();

    if (actions.prev)
    {
        int prev = playlist_.PrevIndex(repeat_);
        if (prev >= 0)
        {
            if (crossfade_enabled_ && player_.GetState() == PlayerState::Playing)
            {
                fading_to_next_ = true;
                pending_next_index_ = prev;
                player_.StartFadeOut(crossfade_duration_ms_);
                status_ = "Fading out...";
            }
            else
            {
                PlayIndex(prev);
            }
        }
    }

    if (actions.shuffle_toggled)
    {
        shuffle_ = !shuffle_;
        changed = true;
    }

    if (actions.repeat_cycle)
    {
        if (repeat_ == RepeatMode::Off)
            repeat_ = RepeatMode::One;
        else if (repeat_ == RepeatMode::One)
            repeat_ = RepeatMode::All;
        else
            repeat_ = RepeatMode::Off;
        changed = true;
    }

    if (actions.volume_changed)
    {
        volume_ = actions.volume;
        player_.SetVolume(volume_);
        changed = true;
    }

    if (actions.mute_toggled)
    {
        mute_ = actions.mute;
        player_.SetMute(mute_);
        changed = true;
    }

    if (actions.audio_device_changed)
    {
        if (actions.audio_device_index >= 0 &&
            actions.audio_device_index < (int)audio_devices_.size())
        {
            audio_device_index_ = actions.audio_device_index;
            player_.SetOutputDevice(audio_devices_[audio_device_index_]);
            changed = true;
        }
    }

    if (actions.crossfade_toggled)
    {
        crossfade_enabled_ = actions.crossfade_enabled;
        changed = true;
    }

    if (actions.crossfade_duration_changed)
    {
        crossfade_duration_ms_ = actions.crossfade_duration_ms;
        changed = true;
    }

    if (actions.request_scan)
    {
        if (!directory_.empty() && std::filesystem::exists(directory_))
        {
            playlist_.Clear();
            scanner_.Start(directory_, recursive_, sort_);
            scanning_active_ = true;
            status_ = "Scanning...";
        }
        else
        {
            status_ = "Directory not found";
        }
    }

    if (changed)
        config_.MarkDirty(now);

    return changed;
}

void App::HandleShortcuts(bool capture_keyboard, const SDL_Event &ev)
{
    if (capture_keyboard)
        return;
    if (ev.type != SDL_KEYDOWN)
        return;

    auto key = ev.key.keysym.sym;
    auto mod = SDL_GetModState();

    if (key == SDLK_SPACE)
    {
        auto st = player_.GetState();
        if (st == PlayerState::Playing)
            player_.Pause();
        else if (st == PlayerState::Paused)
            player_.Play();
        else
            PlayIndex(playlist_.SelectedIndex());
        return;
    }

    if (key == SDLK_RETURN)
    {
        PlayIndex(playlist_.SelectedIndex());
        return;
    }

    if ((mod & KMOD_CTRL) && key == SDLK_f)
    {
        ui_.RequestSearchFocus();
        return;
    }

    if (key == SDLK_UP)
    {
        int sel = playlist_.SelectedIndex();
        if (sel > 0)
            playlist_.SetSelected(sel - 1);
        return;
    }

    if (key == SDLK_DOWN)
    {
        int sel = playlist_.SelectedIndex();
        int count = (int)playlist_.Items().size();
        if (sel < count - 1)
            playlist_.SetSelected(sel + 1);
        return;
    }
}

int App::Run()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
    {
        Logger::Error("SDL init failed");
        return 1;
    }

    auto config_path = GetConfigPath();
    config_.Load(config_path);

    directory_ = config_.last_directory;
    recursive_ = config_.recursive_scan;
    shuffle_ = config_.shuffle;
    repeat_ = config_.repeat;
    volume_ = config_.volume;
    mute_ = config_.mute;
    crossfade_enabled_ = config_.crossfade_enabled;
    crossfade_duration_ms_ = config_.crossfade_duration_ms;

    audio_devices_ = Player::ListOutputDevices();
    audio_device_index_ = 0;

    if (!config_.audio_device.empty())
    {
        auto it = std::find(audio_devices_.begin(), audio_devices_.end(), config_.audio_device);
        if (it != audio_devices_.end())
            audio_device_index_ = (int)std::distance(audio_devices_.begin(), it);
    }

    if (!audio_devices_.empty())
        player_.SetOutputDevice(audio_devices_[audio_device_index_]);

    // opengl
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    auto window =
        SDL_CreateWindow("pmdmini-gui", config_.window_x, config_.window_y, config_.window_w,
                         config_.window_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        Logger::Error("SDL window creation failed");
        return 1;
    }

    // set window icon
    auto icon_rw = SDL_RWFromConstMem(icon_bmp_data, icon_bmp_data_len);
    if (icon_rw)
    {
        auto icon_surface = SDL_LoadBMP_RW(icon_rw, 1);
        if (icon_surface)
        {
            SDL_SetWindowIcon(window, icon_surface);
            SDL_FreeSurface(icon_surface);
        }
    }

    auto gl_ctx = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_ctx);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 150");

    status_ = "Ready";
    waveform_.resize(2048);

    bool running = true;
    while (running)
    {
        auto now = std::chrono::steady_clock::now();

        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            ImGui_ImplSDL2_ProcessEvent(&ev);

            if (ev.type == SDL_QUIT)
                running = false;

            if (ev.type == SDL_DROPFILE)
            {
                std::filesystem::path p = ev.drop.file;
                SDL_free(ev.drop.file);

                if (std::filesystem::is_directory(p))
                {
                    directory_ = p.string();
                    config_.MarkDirty(now);
                    playlist_.Clear();
                    scanner_.Start(directory_, recursive_, sort_);
                    scanning_active_ = true;
                    status_ = "Scanning...";
                }
                else if (IsPmdFile(p.filename().string()))
                {
                    TrackEntry entry;
                    entry.display_name = p.filename().string();
                    entry.path = std::filesystem::absolute(p);
                    playlist_.Add(entry);
                    playlist_.SetSelected((int)playlist_.Items().size() - 1);
                    PlayIndex(playlist_.SelectedIndex());
                }
            }

            HandleShortcuts(ImGui::GetIO().WantCaptureKeyboard, ev);
        }

        // scanner batches
        std::vector<TrackEntry> batch;
        if (scanner_.ConsumeBatch(batch))
        {
            for (auto &e : batch)
                playlist_.Add(e);
            status_ = "Scanning (" + std::to_string(playlist_.Items().size()) + ")";
        }

        if (!scanner_.IsRunning() && scanning_active_)
        {
            scanning_active_ = false;
            playlist_.Sort(sort_);
            status_ = "Scan complete (" + std::to_string(playlist_.Items().size()) + ")";
        }

        // update status when fade in finishes
        if (status_ == "Fading in..." && !player_.IsFadingIn())
            status_ = "Playing";

        // crossfade: anticipate track end and start fading out early
        if (crossfade_enabled_ && !fading_to_next_ &&
            player_.GetState() == PlayerState::Playing)
        {
            auto info = player_.GetTrackInfo();
            if (info.duration_known)
            {
                float sr = info.sample_rate > 0 ? (float)info.sample_rate : 44100.0f;
                float pos_sec = player_.GetPositionSamples() / sr;
                float dur_sec = info.duration_samples / sr;
                float remaining_ms = (dur_sec - pos_sec) * 1000.0f;

                if (remaining_ms <= crossfade_duration_ms_ && remaining_ms > 0)
                {
                    int next = playlist_.NextIndex(repeat_, shuffle_);
                    if (next >= 0)
                    {
                        fading_to_next_ = true;
                        pending_next_index_ = next;
                        player_.StartFadeOut((int)remaining_ms);
                        status_ = "Fading out...";
                    }
                }
            }
        }

        // fade-out completed: load pending next track with fade in
        if (fading_to_next_ && player_.IsFadeOutComplete())
        {
            fading_to_next_ = false;
            PlayIndex(pending_next_index_, true);
            pending_next_index_ = -1;
        }

        // auto-next on track end (no crossfade, or no next track during fade)
        if (player_.HasTrackEnded())
        {
            if (fading_to_next_)
            {
                // fade was in progress but track ended, load the pending track now
                fading_to_next_ = false;
                PlayIndex(pending_next_index_, true);
                pending_next_index_ = -1;
            }
            else
            {
                int next = playlist_.NextIndex(repeat_, shuffle_);
                if (next >= 0)
                    PlayIndex(next);
                else
                {
                    player_.Stop();
                    status_ = "Playlist ended";
                }
            }
        }

        auto waveform_count = player_.ReadWaveform(waveform_.data(), waveform_.size());

        std::vector<TrackEntry> visible_tracks;
        std::vector<int> visible_map;
        BuildVisibleList(visible_tracks, visible_map);

        UIState ui_state;
        ui_state.tracks = std::move(visible_tracks);
        UpdateUIState(ui_state, visible_map);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        UIActions actions{};
        ui_.Draw(ui_state, actions, waveform_.data(), waveform_count);

        if (actions.request_browse)
        {
            auto folder = tinyfd_selectFolderDialog(
                "Select PMD folder", directory_.empty() ? nullptr : directory_.c_str());

            if (folder)
            {
                directory_ = folder;
                config_.MarkDirty(now);
                playlist_.Clear();
                scanner_.Start(directory_, recursive_, sort_);
                scanning_active_ = true;
                status_ = "Scanning...";
            }
        }

        HandleActions(actions, visible_map, now);

        if (config_.ShouldSave(now, std::chrono::milliseconds(750)))
        {
            SyncConfig();
            EnsureParentDir(config_path);
            config_.Save(config_path);
            config_.Saved();
        }

        ImGui::Render();

        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        // frame limiter - cap at ~60fps if vsync fails
        auto frame_end = std::chrono::steady_clock::now();
        auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - now);
        if (frame_time.count() < 16)
            SDL_Delay((Uint32)(16 - frame_time.count()));
    }

    int w, h, x, y;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowPosition(window, &x, &y);
    config_.window_w = w;
    config_.window_h = h;
    config_.window_x = x;
    config_.window_y = y;

    SyncConfig();
    EnsureParentDir(config_path);
    config_.Save(config_path);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
