#include "player.h"
#include "logger.h"
#include "pmdmini.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>

Player::Player()
{
    stop_decode_.store(false);
    decode_thread_ = std::thread(&Player::DecodeThread, this);
}

Player::~Player()
{
    stop_decode_.store(true);
    request_cv_.notify_one();

    if (decode_thread_.joinable())
        decode_thread_.join();

    if (loaded_.load())
        pmd_stop();

    ShutdownAudio();
}

int Player::FrameDurationMs(int frames, int sample_rate)
{
    if (frames <= 0 || sample_rate <= 0)
        return 0;
    return std::max(1, (int)((frames * 1000LL) / sample_rate));
}

std::vector<std::string> Player::NormalizeDeviceList(const std::vector<std::string> &devices)
{
    std::vector<std::string> out;
    out.push_back("Default");
    for (auto &name : devices)
    {
        if (!name.empty() && name != "Default")
            out.push_back(name);
    }
    return out;
}

std::vector<std::string> Player::ListOutputDevices()
{
    std::vector<std::string> devices;
    int count = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < count; i++)
    {
        auto name = SDL_GetAudioDeviceName(i, 0);
        if (name)
            devices.emplace_back(name);
    }
    return NormalizeDeviceList(devices);
}

bool Player::Load(const std::filesystem::path &path)
{
    {
        std::lock_guard lock(request_mutex_);
        pending_path_ = path;
        request_pending_ = true;
    }
    loading_.store(true);
    track_ended_.store(false);
    request_cv_.notify_one();
    return true;
}

void Player::Play()
{
    if (device_)
        SDL_PauseAudioDevice(device_, 0);
    state_.store(PlayerState::Playing);
}

void Player::Pause()
{
    if (device_)
        SDL_PauseAudioDevice(device_, 1);
    state_.store(PlayerState::Paused);
}

void Player::Stop()
{
    if (device_)
        SDL_PauseAudioDevice(device_, 1);
    audio_ring_.Clear();
    viz_ring_.Clear();
    position_samples_.store(0);
    track_ended_.store(false);
    state_.store(PlayerState::Stopped);
}

void Player::SetVolume(int volume)
{
    volume_.store(volume);
}
void Player::SetMute(bool mute)
{
    mute_.store(mute);
}

void Player::StartFadeOut(int duration_ms)
{
    float total_samples = (duration_ms / 1000.0f) * sample_rate_ * channels_;
    if (total_samples < 1.0f)
        total_samples = 1.0f;
    fade_target_ = 0.0f;
    fade_delta_ = -fade_gain_ / total_samples;
    fade_out_complete_.store(false);
}

void Player::SetPendingFadeIn(int duration_ms)
{
    pending_fade_in_ms_.store(duration_ms);
}

void Player::ResetFade()
{
    fade_gain_ = 1.0f;
    fade_target_ = 1.0f;
    fade_delta_ = 0.0f;
    fade_out_complete_.store(false);
}

bool Player::IsFadeOutComplete()
{
    return fade_out_complete_.exchange(false);
}

bool Player::IsFadingIn() const
{
    return fade_target_ == 1.0f && fade_delta_ > 0.0f;
}

void Player::SetOutputDevice(const std::string &name)
{
    auto next = (name == "Default") ? "" : name;
    if (next == output_device_)
        return;

    {
        std::lock_guard lock(request_mutex_);
        output_device_ = next;
    }

    if (device_)
    {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }

    if (InitAudio(sample_rate_, channels_))
    {
        SDL_PauseAudioDevice(device_, state_.load() != PlayerState::Playing);
    }
}

PlayerState Player::GetState() const
{
    return state_.load();
}
bool Player::IsLoading() const
{
    return loading_.load();
}
std::string Player::GetOutputDevice() const
{
    return output_device_;
}
TrackInfo Player::GetTrackInfo() const
{
    return track_;
}
int64_t Player::GetPositionSamples() const
{
    return position_samples_.load();
}

bool Player::HasTrackEnded()
{
    return track_ended_.exchange(false);
}

void Player::SetOnTrackEnd(std::function<void()> callback)
{
    std::lock_guard lock(callback_mutex_);
    on_track_end_ = std::move(callback);
}

size_t Player::ReadWaveform(float *out, size_t count)
{
    return viz_ring_.Read(out, count);
}

void Player::DecodeThread()
{
    const int frames = 1024;
    std::vector<int16_t> pcm(frames * channels_);
    std::vector<float> float_pcm(frames * channels_);

    uint64_t last_underrun = 0;
    auto last_log = std::chrono::steady_clock::now();

    while (!stop_decode_.load())
    {
        std::filesystem::path request_path;
        {
            std::unique_lock lock(request_mutex_);
            request_cv_.wait_for(lock, std::chrono::milliseconds(5),
                                 [this] { return request_pending_ || stop_decode_.load(); });

            if (request_pending_)
            {
                request_path = pending_path_;
                request_pending_ = false;
            }
        }

        if (!request_path.empty())
        {
            loading_.store(true);
            bool ok = DoLoad(request_path);
            loaded_.store(ok);
            loading_.store(false);

            if (ok)
            {
                int fade_ms = pending_fade_in_ms_.exchange(0);
                if (fade_ms > 0)
                {
                    float total = (fade_ms / 1000.0f) * sample_rate_ * channels_;
                    if (total < 1.0f)
                        total = 1.0f;
                    fade_gain_ = 0.0f;
                    fade_target_ = 1.0f;
                    fade_delta_ = 1.0f / total;
                    fade_out_complete_.store(false);
                }
                else
                {
                    ResetFade();
                }

                if (state_.load() == PlayerState::Playing)
                    SDL_PauseAudioDevice(device_, 0);
            }
        }

        if (state_.load() != PlayerState::Playing || !loaded_.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // check if track ended (position >= duration)
        if (track_.duration_known)
        {
            auto pos = position_samples_.load();
            if (pos >= track_.duration_samples)
            {
                track_ended_.store(true);
                state_.store(PlayerState::Stopped);

                std::lock_guard lock(callback_mutex_);
                if (on_track_end_)
                    on_track_end_();
                continue;
            }
        }

        auto available = audio_ring_.Available();
        if (available > ring_capacity_ * 3 / 4)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(FrameDurationMs(frames, sample_rate_)));
            continue;
        }

        pmd_renderer(pcm.data(), frames);

        for (size_t i = 0; i < float_pcm.size(); i++)
            float_pcm[i] = (float)pcm[i] / 32768.0f;

        audio_ring_.Write(float_pcm.data(), float_pcm.size());
        viz_ring_.Write(float_pcm.data(), float_pcm.size());
        position_samples_.fetch_add(frames);

        auto underruns = underrun_count_.load();
        auto now = std::chrono::steady_clock::now();
        if (underruns > last_underrun &&
            std::chrono::duration_cast<std::chrono::seconds>(now - last_log).count() >= 1)
        {
            Logger::Warn("Audio underrun detected");
            last_underrun = underruns;
            last_log = now;
        }
    }
}

bool Player::DoLoad(const std::filesystem::path &path)
{
    if (loaded_.load())
    {
        pmd_stop();
        loaded_.store(false);
    }

    audio_ring_.Clear();
    viz_ring_.Clear();
    position_samples_.store(0);
    track_ended_.store(false);

    auto pcm_dir = path.parent_path().string();
    if (pcm_dir.empty())
        pcm_dir = ".";

    if (!InitAudio(sample_rate_, channels_))
        return false;

    pmd_init(pcm_dir.data());
    pmd_setrate(sample_rate_);

    auto path_str = path.string();
    char *argv[4] = {(char *)"pmdmini-gui", path_str.data(), nullptr, nullptr};

    if (pmd_play(argv, pcm_dir.data()) != 0)
    {
        pmd_stop();
        return false;
    }

    track_.path = path;
    track_.display_name = path.filename().string();
    track_.sample_rate = sample_rate_;
    track_.channels = channels_;

    int len_sec = pmd_length_sec();
    track_.duration_known = len_sec > 0;
    track_.duration_samples = track_.duration_known ? (int64_t)len_sec * sample_rate_ : 0;

    SDL_PauseAudioDevice(device_, 1);
    return true;
}

bool Player::InitAudio(int sample_rate, int channels)
{
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0)
    {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
            return false;
    }

    // close existing device so multiple devices can't consume the same buffer
    // without it they can consume from same ring buffer causing x3 speedup
    if (device_)
    {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }

    SDL_AudioSpec want{}, got{};
    want.freq = sample_rate;
    want.format = AUDIO_F32SYS;
    want.channels = (Uint8)channels;
    want.samples = 1024;
    want.callback = &Player::SDLAudioCallback;
    want.userdata = this;

    std::string dev_name;
    {
        std::lock_guard lock(request_mutex_);
        dev_name = output_device_;
    }

    auto dev_ptr = dev_name.empty() ? nullptr : dev_name.c_str();

    // exact format
    device_ = SDL_OpenAudioDevice(dev_ptr, 0, &want, &got, 0);

    // retry with format negotiation
    if (device_ == 0)
    {
        device_ =
            SDL_OpenAudioDevice(dev_ptr, 0, &want, &got,
                                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }

    // last resort: default device
    if (device_ == 0 && dev_ptr != nullptr)
    {
        Logger::Warn("Failed to open audio device, trying default");
        device_ =
            SDL_OpenAudioDevice(nullptr, 0, &want, &got,
                                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }

    if (device_ == 0)
    {
        Logger::Error("SDL audio init failed: " + std::string(SDL_GetError()));
        return false;
    }

    if (got.freq != want.freq || got.channels != want.channels)
    {
        Logger::Warn("Audio device negotiated different format: " + std::to_string(got.freq) +
                     "Hz " + std::to_string(got.channels) + "ch");
    }

    sample_rate_ = got.freq;
    channels_ = got.channels;
    return true;
}

void Player::ShutdownAudio()
{
    if (device_)
    {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
}

void Player::SDLAudioCallback(void *userdata, Uint8 *stream, int len)
{
    auto player = (Player *)userdata;
    auto out = (float *)stream;
    size_t samples = len / sizeof(float);

    size_t got = player->audio_ring_.Read(out, samples);
    if (got < samples)
    {
        memset(out + got, 0, (samples - got) * sizeof(float));
        player->underrun_count_.fetch_add(1);
    }

    if (player->mute_.load())
    {
        memset(out, 0, samples * sizeof(float));
        return;
    }

    float vol = std::clamp(player->volume_.load() / 100.0f, 0.0f, 1.0f);

    // apply fade + volume in one pass
    float gain = player->fade_gain_;
    float delta = player->fade_delta_;
    float target = player->fade_target_;

    for (size_t i = 0; i < samples; i++)
    {
        out[i] *= vol * gain;

        if (delta != 0.0f)
        {
            gain += delta;
            if ((delta < 0.0f && gain <= target) || (delta > 0.0f && gain >= target))
            {
                gain = target;
                delta = 0.0f;

                if (target == 0.0f)
                    player->fade_out_complete_.store(true);
            }
        }
    }

    player->fade_gain_ = gain;
    player->fade_delta_ = delta;
}
