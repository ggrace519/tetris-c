// Procedural SFX synthesis — builds raylib Wave buffers in memory, so the game
// ships with ZERO audio asset files (INNOVATIONS.md #1). App-layer only (uses
// raylib's Wave type); no core dependency.
//
// A Wave is a plain struct { frameCount, sampleRate, sampleSize, channels, data }
// we fill ourselves and hand to LoadSoundFromWave (which copies the samples).
// IMPORTANT: never UnloadWave() a Wave built here — its `data` is a heap buffer we
// own via freeWave(), not memory raylib allocated. See synth.cpp.
#ifndef TETRIS_APP_SYNTH_HPP
#define TETRIS_APP_SYNTH_HPP

#include "raylib.h"

namespace tetris::synth {

inline constexpr int kSampleRate = 44100;

enum class Waveform { Sine, Square, Triangle, Noise };

// A simple amplitude envelope (seconds), applied over the tone's duration.
struct Env {
    float attack = 0.004f;
    float decay = 0.0f;
    float sustain = 1.0f;   // sustain level [0,1]
    float release = 0.05f;
};

// Build a single-tone Wave (mono, 16-bit). `ms` is total duration.
Wave tone(float freqHz, float ms, Waveform wf, Env env, float amp = 0.6f);

// A short filtered-noise burst (for thock/impact/whoosh). `sweep` bends pitch
// of an optional tonal component from freqHz*startMul → freqHz*endMul over time.
Wave noiseBurst(float ms, float amp = 0.5f);

// A quick descending "whoosh" for hard drop (noise + falling tone).
Wave whoosh(float ms);

// A 3-note ascending arpeggio (for the Tetris / T-spin fanfare).
Wave arpeggio(float f0, float f1, float f2, float noteMs);

// Free the heap buffer inside a Wave built by this module. Call AFTER
// LoadSoundFromWave has copied the samples. Do NOT use raylib's UnloadWave.
void freeWave(Wave& w);

}  // namespace tetris::synth

#endif  // TETRIS_APP_SYNTH_HPP
