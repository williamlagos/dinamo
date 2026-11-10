#include "SoundEngine.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace dinamo {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

SoundEngine::~SoundEngine() {
    shutdown();
}

bool SoundEngine::init() {
    if (deviceId_ != 0) return true;

    SDL_AudioSpec desired{};
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = &SoundEngine::audioCallback;
    desired.userdata = this;

    SDL_AudioSpec obtained{};
    deviceId_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (deviceId_ == 0) {
        return false;
    }

    sampleRate_ = obtained.freq > 0 ? obtained.freq : 44100;
    SDL_PauseAudioDevice(deviceId_, 0);
    return true;
}

void SoundEngine::shutdown() {
    if (deviceId_ != 0) {
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    voices_.clear();
}

void SoundEngine::setMuted(bool muted) {
    muted_ = muted;
}

void SoundEngine::enqueueTone(double freqStart, double freqEnd, double durationSec, bool isNoise, float amplitude) {
    if (muted_ || deviceId_ == 0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (voices_.size() >= 8) {
        voices_.erase(voices_.begin());
    }
    Voice v;
    v.freqStart = freqStart;
    v.freqEnd = freqEnd;
    v.durationSec = durationSec;
    v.isNoise = isNoise;
    v.amplitude = amplitude;
    voices_.push_back(v);
}

void SoundEngine::playLaser() {
    enqueueTone(880.0, 220.0, 0.10, false, 0.20f);
}

void SoundEngine::playDive() {
    enqueueTone(520.0, 260.0, 0.35, false, 0.16f);
}

void SoundEngine::playHit() {
    enqueueTone(660.0, 440.0, 0.08, false, 0.18f);
}

void SoundEngine::playExplosion() {
    enqueueTone(0.0, 0.0, 0.22, true, 0.24f);
}

void SoundEngine::playPlayerDeath() {
    enqueueTone(0.0, 0.0, 0.45, true, 0.28f);
}

void SoundEngine::audioCallback(void* userdata, Uint8* stream, int len) {
    auto* self = static_cast<SoundEngine*>(userdata);
    auto* out = reinterpret_cast<int16_t*>(stream);
    int sampleCount = len / static_cast<int>(sizeof(int16_t));
    self->mix(out, sampleCount);
}

void SoundEngine::mix(int16_t* out, int sampleCount) {
    std::memset(out, 0, static_cast<size_t>(sampleCount) * sizeof(int16_t));
    if (muted_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (voices_.empty()) return;

    const double dt = 1.0 / static_cast<double>(sampleRate_);
    for (int i = 0; i < sampleCount; ++i) {
        double mixed = 0.0;
        for (auto it = voices_.begin(); it != voices_.end();) {
            double progress = it->elapsedSec / std::max(0.001, it->durationSec);
            if (progress >= 1.0) {
                it = voices_.erase(it);
                continue;
            }

            double env = std::max(0.0, 1.0 - progress * 1.1);
            double sample = 0.0;
            if (it->isNoise) {
                // Game Boy Channel 4 LFSR pseudo-random noise
                uint16_t bit = static_cast<uint16_t>((it->lfsr & 1u) ^ ((it->lfsr >> 1) & 1u));
                it->lfsr = static_cast<uint16_t>((it->lfsr >> 1) | (bit << 14));
                it->lfsr = static_cast<uint16_t>((it->lfsr & ~0x0040u) | (bit << 6));
                sample = (it->lfsr & 1u) ? 1.0 : -1.0;
            } else {
                double freq = it->freqStart + (it->freqEnd - it->freqStart) * progress;
                it->phase += 2.0 * kPi * freq * dt;
                if (it->phase >= 2.0 * kPi) it->phase -= 2.0 * kPi;
                sample = (std::sin(it->phase) >= 0.0) ? 1.0 : -1.0;
            }

            mixed += sample * env * it->amplitude;
            it->elapsedSec += dt;
            ++it;
        }
        mixed = std::max(-1.0, std::min(1.0, mixed));
        out[i] = static_cast<int16_t>(mixed * 24000.0);
    }
}

} // namespace dinamo
