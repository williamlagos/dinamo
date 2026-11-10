#pragma once

#include <SDL.h>
#include <cstdint>
#include <mutex>
#include <vector>

namespace dinamo {

class SoundEngine {
public:
    SoundEngine() = default;
    ~SoundEngine();

    bool init();
    void shutdown();

    void setMuted(bool muted);
    bool isMuted() const { return muted_; }
    void toggleMute() { muted_ = !muted_; }

    void playLaser();
    void playDive();
    void playHit();
    void playExplosion();
    void playPlayerDeath();

private:
    struct Voice {
        double freqStart = 440.0;
        double freqEnd = 440.0;
        double durationSec = 0.1;
        double elapsedSec = 0.0;
        double phase = 0.0;
        bool isNoise = false;
        uint16_t lfsr = 0x7FFF;
        float amplitude = 0.2f;
    };

    static void audioCallback(void* userdata, Uint8* stream, int len);
    void mix(int16_t* out, int sampleCount);
    void enqueueTone(double freqStart, double freqEnd, double durationSec, bool isNoise, float amplitude);

    SDL_AudioDeviceID deviceId_ = 0;
    int sampleRate_ = 44100;
    bool muted_ = false;
    std::mutex mutex_;
    std::vector<Voice> voices_;
};

} // namespace dinamo
