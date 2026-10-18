// Audio engine — loads procedurally-synthesized SFX (no asset files) and plays
// them via round-robin LoadSoundAlias pools so rapid retriggers don't cut each
// other off. App-layer only. (INNOVATIONS.md #1 + #2.)
//
// Combo escalation (#2): playClear() pitches the clear SFX up a semitone per combo
// step (SetSoundPitch), and a 4-line / T-spin clear plays a distinct fanfare.
#ifndef TETRIS_APP_AUDIO_HPP
#define TETRIS_APP_AUDIO_HPP

#include <array>

#include "raylib.h"

namespace tetris {

enum class Sfx { Move, Rotate, SoftDrop, HardDrop, Lock, Clear, Tetris, LevelUp, GameOver };

class Audio {
public:
    Audio() = default;
    ~Audio();
    Audio(const Audio&) = delete;
    Audio& operator=(const Audio&) = delete;

    // Init the audio device and synthesize all SFX. Safe to call when no audio
    // device is available (e.g. headless) — it no-ops and play() becomes silent.
    void init();

    // Play a one-shot SFX (round-robin alias so overlaps don't cut off).
    void play(Sfx s, float pitch = 1.0f);

    // Line clear with combo-escalating pitch; routes 4-line / T-spin to the fanfare.
    void playClear(int lines, int combo, bool tSpin);

    bool ready() const { return ready_; }

private:
    static constexpr int kPoolSize = 6;  // aliases per SFX for overlap
    struct Pool {
        Sound source{};                        // owns the sample data
        std::array<Sound, kPoolSize> alias{};  // share data, own pitch/volume
        int cursor = 0;
        bool loaded = false;
    };
    Pool& poolFor(Sfx s);
    void loadPool(Sfx s, Sound source, float volume);

    std::array<Pool, 9> pools_{};  // indexed by Sfx
    bool ready_ = false;
};

}  // namespace tetris

#endif  // TETRIS_APP_AUDIO_HPP
