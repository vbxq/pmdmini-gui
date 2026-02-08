#pragma once

#include "ring_buffer.h"
#include <SDL.h>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

enum class PlayerState
{
    Stopped,
    Playing,
    Paused
};

struct TrackInfo
{
    std::filesystem::path path;
    std::string display_name;
    int sample_rate = 44100;
    int channels = 2;
    bool duration_known = false;
    int64_t duration_samples = 0;
};

class Player
{
  public:
    Player();
    ~Player();

    static int FrameDurationMs(int frames, int sample_rate);
    static std::vector<std::string> NormalizeDeviceList(const std::vector<std::string> &devices);
    static std::vector<std::string> ListOutputDevices();

    bool Load(const std::filesystem::path &path);
    void Play();
    void Pause();
    void Stop();

    void SetOutputDevice(const std::string &name);
    void SetVolume(int vol);
    void SetMute(bool m);

    void StartFadeOut(int duration_ms);
    void SetPendingFadeIn(int duration_ms);
    void ResetFade();
    bool IsFadeOutComplete();
    bool IsFadingIn() const;

    PlayerState GetState() const;
    bool IsLoading() const;
    std::string GetOutputDevice() const;
    TrackInfo GetTrackInfo() const;
    int64_t GetPositionSamples() const;

    // track end notification
    bool HasTrackEnded();
    void SetOnTrackEnd(std::function<void()> callback);

    size_t ReadWaveform(float *out, size_t count);

  private:
    void DecodeThread();
    bool DoLoad(const std::filesystem::path &path);
    bool InitAudio(int sample_rate, int channels);
    void ShutdownAudio();

    static void SDLAudioCallback(void *userdata, Uint8 *stream, int len);

    std::atomic<PlayerState> state_{PlayerState::Stopped};
    std::atomic<bool> stop_decode_{false};
    std::atomic<bool> loading_{false};
    std::atomic<bool> loaded_{false};
    std::atomic<bool> track_ended_{false};

    std::thread decode_thread_;
    std::mutex request_mutex_;
    std::condition_variable request_cv_;
    bool request_pending_ = false;
    std::filesystem::path pending_path_;

    static constexpr size_t ring_capacity_ = 262144;
    RingBuffer audio_ring_{ring_capacity_};
    RingBuffer viz_ring_{ring_capacity_};

    SDL_AudioDeviceID device_ = 0;
    int sample_rate_ = 44100;
    int channels_ = 2;
    std::string output_device_;

    std::atomic<int> volume_{100};
    std::atomic<bool> mute_{false};
    std::atomic<int64_t> position_samples_{0};
    std::atomic<uint64_t> underrun_count_{0};

    // fade state (manipulated from audio callback)
    float fade_gain_ = 1.0f;
    float fade_target_ = 1.0f;
    float fade_delta_ = 0.0f;
    std::atomic<bool> fade_out_complete_{false};
    std::atomic<int> pending_fade_in_ms_{0}; // >0 means apply fade in after next load

    std::function<void()> on_track_end_;
    std::mutex callback_mutex_;

    TrackInfo track_;
};
