#include "app/synth.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace tetris::synth {
namespace {

constexpr float kTwoPi = 6.28318530718f;

// Deterministic xorshift noise so builds are reproducible (no rand()).
struct Noise {
    std::uint32_t s = 0x2545F491u;
    float next() {  // [-1, 1)
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (static_cast<float>(s & 0xFFFFFF) / static_cast<float>(0x800000)) - 1.0f;
    }
};

float oscillate(Waveform wf, float phase, Noise& noise) {
    // phase in turns [0,1)
    switch (wf) {
        case Waveform::Sine:     return std::sin(kTwoPi * phase);
        case Waveform::Square:   return (phase < 0.5f) ? 1.0f : -1.0f;
        case Waveform::Triangle: return 4.0f * std::fabs(phase - 0.5f) - 1.0f;
        case Waveform::Noise:    return noise.next();
    }
    return 0.0f;
}

// ADSR-ish amplitude at time t (seconds) over total duration d.
float envelope(const Env& e, float t, float d) {
    if (t < e.attack)                 return e.attack > 0 ? t / e.attack : 1.0f;
    const float ad = e.attack + e.decay;
    if (t < ad && e.decay > 0)        return 1.0f - (1.0f - e.sustain) * (t - e.attack) / e.decay;
    const float relStart = d - e.release;
    if (t >= relStart && e.release > 0) return e.sustain * (1.0f - (t - relStart) / e.release);
    return e.sustain;
}

// Pack a float sample buffer [-1,1] into a freshly malloc'd 16-bit PCM Wave.
Wave packWave(const std::vector<float>& buf) {
    const int n = static_cast<int>(buf.size());
    auto* pcm = static_cast<std::int16_t*>(std::malloc(sizeof(std::int16_t) * (n > 0 ? n : 1)));
    for (int i = 0; i < n; ++i) {
        float v = buf[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        pcm[i] = static_cast<std::int16_t>(v * 32000.0f);
    }
    Wave w{};
    w.frameCount = static_cast<unsigned>(n);
    w.sampleRate = static_cast<unsigned>(kSampleRate);
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm;
    return w;
}

}  // namespace

Wave tone(float freqHz, float ms, Waveform wf, Env env, float amp) {
    const int n = static_cast<int>(kSampleRate * ms / 1000.0f);
    const float d = ms / 1000.0f;
    std::vector<float> buf(n);
    Noise noise;
    float phase = 0.0f;
    const float inc = freqHz / kSampleRate;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        buf[i] = oscillate(wf, phase, noise) * envelope(env, t, d) * amp;
        phase += inc;
        if (phase >= 1.0f) phase -= 1.0f;
    }
    return packWave(buf);
}

Wave noiseBurst(float ms, float amp) {
    const int n = static_cast<int>(kSampleRate * ms / 1000.0f);
    const float d = ms / 1000.0f;
    std::vector<float> buf(n);
    Noise noise;
    // A little low-frequency body under the noise so it reads as a "thock", not hiss.
    float phase = 0.0f;
    const float bodyInc = 110.0f / kSampleRate;
    float lp = 0.0f;  // one-pole low-pass on the noise for a softer texture
    const Env env{0.002f, 0.0f, 1.0f, d * 0.9f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        lp += 0.35f * (noise.next() - lp);
        const float body = std::sin(kTwoPi * phase) * 0.6f;
        buf[i] = (lp * 0.7f + body) * envelope(env, t, d) * amp;
        phase += bodyInc;
        if (phase >= 1.0f) phase -= 1.0f;
    }
    return packWave(buf);
}

Wave whoosh(float ms) {
    const int n = static_cast<int>(kSampleRate * ms / 1000.0f);
    const float d = ms / 1000.0f;
    std::vector<float> buf(n);
    Noise noise;
    float phase = 0.0f;
    const Env env{0.002f, 0.0f, 1.0f, d * 0.8f};
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kSampleRate;
        const float prog = t / d;
        const float freq = 520.0f * (1.0f - 0.8f * prog);  // falling pitch
        buf[i] = (std::sin(kTwoPi * phase) * 0.5f + noise.next() * 0.25f) *
                 envelope(env, t, d) * 0.5f;
        phase += freq / kSampleRate;
        if (phase >= 1.0f) phase -= 1.0f;
    }
    return packWave(buf);
}

Wave arpeggio(float f0, float f1, float f2, float noteMs) {
    const float freqs[3] = {f0, f1, f2};
    const int per = static_cast<int>(kSampleRate * noteMs / 1000.0f);
    const int n = per * 3;
    std::vector<float> buf(n);
    Noise noise;
    const float noteD = noteMs / 1000.0f;
    const Env env{0.004f, 0.02f, 0.7f, noteD * 0.5f};
    for (int note = 0; note < 3; ++note) {
        float phase = 0.0f;
        const float inc = freqs[note] / kSampleRate;
        for (int i = 0; i < per; ++i) {
            const float t = static_cast<float>(i) / kSampleRate;
            // Square gives it a bright chiptune fanfare character.
            buf[note * per + i] = oscillate(Waveform::Square, phase, noise) *
                                  envelope(env, t, noteD) * 0.5f;
            phase += inc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }
    return packWave(buf);
}

void freeWave(Wave& w) {
    std::free(w.data);
    w.data = nullptr;
    w.frameCount = 0;
}

}  // namespace tetris::synth
