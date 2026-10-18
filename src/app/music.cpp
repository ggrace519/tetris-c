#include "app/music.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tetris {

namespace {
constexpr int kSR = 44100;         // sample rate
constexpr int kStreamFrames = 4096; // per-refill chunk

// Note frequencies (equal temperament), a small scale to compose loops from.
constexpr float A3 = 220.00f, C4 = 261.63f, D4 = 293.66f, E4 = 329.63f,
                F4 = 349.23f, G4 = 392.00f, A4 = 440.00f, C5 = 523.25f,
                E5 = 659.25f, G5 = 783.99f;

float square(float ph, float duty = 0.5f) { return (ph - std::floor(ph)) < duty ? 1.f : -1.f; }
float tri(float ph) { float p = ph - std::floor(ph); return 4.f * std::fabs(p - 0.5f) - 1.f; }

// Append `beats` of a note (melody square + bass triangle) into buf at sample rate.
// A short amplitude envelope per note avoids clicks and gives a plucky chiptune feel.
void addNote(std::vector<short>& buf, float melodyHz, float bassHz, float beatSec,
             float melAmp, float bassAmp) {
    const int n = static_cast<int>(kSR * beatSec);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / kSR;
        const float env = std::min(1.0f, (n - i) / (0.08f * kSR));  // decay tail
        const float atk = std::min(1.0f, i / (0.005f * kSR));
        const float a = env * atk;
        float s = 0.0f;
        if (melodyHz > 0) s += square(melodyHz * t, 0.5f) * melAmp;
        if (bassHz > 0)   s += tri(bassHz * t) * bassAmp;
        s *= a;
        if (s > 1.f) s = 1.f; if (s < -1.f) s = -1.f;
        buf.push_back(static_cast<short>(s * 9000.0f));  // headroom under SFX
    }
}
}  // namespace

const char* trackName(Track t) {
    switch (t) {
        case Track::Off:     return "OFF";
        case Track::Calm:    return "CALM";
        case Track::Classic: return "CLASSIC";
        case Track::Fast:    return "FAST";
    }
    return "OFF";
}

Jukebox::~Jukebox() {
    if (ready_) { StopAudioStream(stream_); UnloadAudioStream(stream_); }
}

void Jukebox::init() {
    // --- CALM: slow gentle arpeggio over a soft bass (loops/[0]). ---
    {
        auto& b = loops_[0];
        const float beat = 0.42f;
        const float mel[] = {C5, E5, G5, E5, A4, C5, E5, C5};
        const float bass[] = {C4, C4, A3, A3, F4, F4, G4, G4};
        for (int rep = 0; rep < 2; ++rep)
            for (int i = 0; i < 8; ++i) addNote(b, mel[i], bass[i], beat, 0.28f, 0.22f);
    }
    // --- CLASSIC: brisk Korobeiniki-flavoured melody (loops/[1]). ---
    {
        auto& b = loops_[1];
        const float beat = 0.20f;
        const float mel[]  = {E5, A4, C5, E5, D4, C5, A4, A4,
                              C5, E5, D4, C5, A4, A4, C5, E5};
        const float bass[] = {A3, A3, A3, A3, F4, F4, F4, F4,
                              C4, C4, C4, C4, A3, A3, A3, A3};
        for (int i = 0; i < 16; ++i) addNote(b, mel[i], bass[i], beat, 0.30f, 0.20f);
    }
    // --- FAST: driving high-tempo loop for tense play (loops/[2]). ---
    {
        auto& b = loops_[2];
        const float beat = 0.12f;
        const float mel[]  = {E5, G5, E5, C5, E5, G5, A4, C5,
                              D4, F4, A4, C5, G4, E5, C5, G4};
        const float bass[] = {A3, A3, C4, C4, F4, F4, G4, G4,
                              A3, A3, C4, C4, D4, D4, E4, E4};
        for (int rep = 0; rep < 2; ++rep)
            for (int i = 0; i < 16; ++i) addNote(b, mel[i], bass[i], beat, 0.30f, 0.22f);
    }

    SetAudioStreamBufferSizeDefault(kStreamFrames);
    stream_ = LoadAudioStream(kSR, 16, 1);
    if (stream_.buffer == nullptr) return;  // no device
    SetAudioStreamVolume(stream_, volume_);
    ready_ = true;
}

void Jukebox::fill(short* out, int frames) {
    const int idx = static_cast<int>(track_) - 1;  // Track::Calm==1 → loops_[0]
    if (idx < 0 || idx >= static_cast<int>(loops_.size()) || loops_[idx].empty()) {
        for (int i = 0; i < frames; ++i) out[i] = 0;
        return;
    }
    const auto& loop = loops_[idx];
    for (int i = 0; i < frames; ++i) {
        out[i] = loop[readPos_];
        if (++readPos_ >= loop.size()) readPos_ = 0;  // seamless wrap
    }
}

void Jukebox::update() {
    if (!ready_ || track_ == Track::Off) return;
    static short chunk[kStreamFrames];
    while (IsAudioStreamProcessed(stream_)) {
        fill(chunk, kStreamFrames);
        UpdateAudioStream(stream_, chunk, kStreamFrames);
    }
}

void Jukebox::select(Track t) {
    if (!ready_) { track_ = t; return; }
    track_ = t;
    readPos_ = 0;
    if (t == Track::Off) {
        if (IsAudioStreamPlaying(stream_)) StopAudioStream(stream_);
    } else {
        if (!IsAudioStreamPlaying(stream_)) PlayAudioStream(stream_);
    }
}

void Jukebox::setVolume(float v) {
    volume_ = v < 0 ? 0 : (v > 1 ? 1 : v);
    if (ready_) SetAudioStreamVolume(stream_, volume_);
}

}  // namespace tetris
