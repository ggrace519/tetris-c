#include "app/audio.hpp"

#include <algorithm>
#include <cmath>

#include "app/synth.hpp"

namespace tetris {

namespace {
int idx(Sfx s) { return static_cast<int>(s); }

// Per-SFX playback volume so ticks (move/rotate/softdrop) sit quietly under the
// louder events (lock/clear/fanfare) and don't fatigue.
float volumeFor(Sfx s) {
    switch (s) {
        case Sfx::Move:     return 0.18f;
        case Sfx::Rotate:   return 0.25f;
        case Sfx::SoftDrop: return 0.15f;
        case Sfx::HardDrop: return 0.55f;
        case Sfx::Lock:     return 0.6f;
        case Sfx::Clear:    return 0.7f;
        case Sfx::Tetris:   return 0.85f;
        case Sfx::LevelUp:  return 0.7f;
        case Sfx::GameOver: return 0.7f;
    }
    return 0.5f;
}

// Synthesize the Wave for one SFX. Kept in one place so init() reads cleanly.
Wave synthFor(Sfx s) {
    using namespace tetris::synth;
    switch (s) {
        case Sfx::Move:     return tone(220.0f, 22.0f, Waveform::Square, {0.001f, 0, 1, 0.02f}, 0.4f);
        case Sfx::Rotate:   return tone(330.0f, 28.0f, Waveform::Triangle, {0.001f, 0, 1, 0.025f}, 0.5f);
        case Sfx::SoftDrop: return tone(160.0f, 18.0f, Waveform::Square, {0.001f, 0, 1, 0.016f}, 0.35f);
        case Sfx::HardDrop: return whoosh(120.0f);
        case Sfx::Lock:     return noiseBurst(85.0f, 0.6f);          // the "thock"
        case Sfx::Clear:    return tone(523.25f, 160.0f, Waveform::Sine, {0.004f, 0.03f, 0.6f, 0.09f}, 0.6f);
        case Sfx::Tetris:   return arpeggio(523.25f, 659.25f, 783.99f, 90.0f);  // C-E-G fanfare
        case Sfx::LevelUp:  return arpeggio(392.0f, 523.25f, 659.25f, 70.0f);
        case Sfx::GameOver: return arpeggio(392.0f, 311.13f, 233.08f, 160.0f);  // descending
    }
    return tone(440.0f, 40.0f, Waveform::Sine, {}, 0.4f);
}
}  // namespace

Audio::~Audio() {
    if (!ready_) return;
    // Unload aliases FIRST, then the sources (aliases share the source's data).
    for (Pool& p : pools_) {
        if (!p.loaded) continue;
        for (Sound& a : p.alias) UnloadSoundAlias(a);
        UnloadSound(p.source);
    }
    CloseAudioDevice();
}

void Audio::loadPool(Sfx s, Sound source, float volume) {
    Pool& p = pools_[idx(s)];
    p.source = source;
    SetSoundVolume(p.source, volume);
    for (int i = 0; i < kPoolSize; ++i) {
        p.alias[i] = LoadSoundAlias(p.source);
        SetSoundVolume(p.alias[i], volume);
    }
    p.loaded = true;
}

void Audio::init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        // Headless / no device — leave ready_ false; play() no-ops.
        return;
    }
    for (int i = 0; i < 9; ++i) {
        const Sfx s = static_cast<Sfx>(i);
        Wave w = synthFor(s);
        Sound src = LoadSoundFromWave(w);  // copies samples into raylib's buffer
        synth::freeWave(w);                 // free OUR heap buffer (not UnloadWave!)
        loadPool(s, src, volumeFor(s));
    }
    ready_ = true;
}

Audio::Pool& Audio::poolFor(Sfx s) { return pools_[idx(s)]; }

void Audio::play(Sfx s, float pitch) {
    if (!ready_) return;
    Pool& p = poolFor(s);
    Sound& snd = p.alias[p.cursor];
    p.cursor = (p.cursor + 1) % kPoolSize;
    SetSoundPitch(snd, pitch);  // per-alias — doesn't disturb others still ringing
    PlaySound(snd);
}

void Audio::playClear(int lines, int combo, bool tSpin) {
    if (!ready_) return;
    // A Tetris (4 lines) or any T-spin gets the categorically-different fanfare.
    if (lines >= 4 || tSpin) {
        play(Sfx::Tetris);
        return;
    }
    // Otherwise: clear ding, pitched up a semitone per combo step (2^(1/12)),
    // capped so it never goes chipmunk.
    const int c = std::clamp(combo, 0, 12);
    const float pitch = std::pow(2.0f, static_cast<float>(c) / 12.0f);
    play(Sfx::Clear, pitch);
}

}  // namespace tetris
