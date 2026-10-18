// Background music — procedurally-synthesized chiptune LOOPS (not a generated
// score; see DECISIONS ADR on this). Each track is a fixed PCM loop buffer built
// in code; one AudioStream is fed from the selected track, wrapping seamlessly.
// Selectable + mutable. App-layer, zero asset files. (User request.)
#ifndef TETRIS_APP_MUSIC_HPP
#define TETRIS_APP_MUSIC_HPP

#include <array>
#include <vector>

#include "raylib.h"

namespace tetris {

enum class Track { Off, Calm, Classic, Fast };
inline constexpr int kTrackCount = 4;
const char* trackName(Track t);

class Jukebox {
public:
    Jukebox() = default;
    ~Jukebox();
    Jukebox(const Jukebox&) = delete;
    Jukebox& operator=(const Jukebox&) = delete;

    // Build the loop buffers and open the stream. Safe with no audio device.
    void init();
    // Feed the stream each frame (no-op for Off / when not ready).
    void update();
    // Select a track (Off stops playback). Restarts the loop from the top.
    void select(Track t);
    Track current() const { return track_; }
    void setVolume(float v);  // 0..1

private:
    void fill(short* out, int frames);  // pull `frames` samples from the current loop

    AudioStream stream_{};
    bool ready_ = false;
    Track track_ = Track::Off;
    float volume_ = 0.5f;
    // One synthesized loop buffer per real track (index by Track-1; Off has none).
    std::array<std::vector<short>, kTrackCount - 1> loops_{};
    std::size_t readPos_ = 0;  // sample cursor into the current loop
};

}  // namespace tetris

#endif  // TETRIS_APP_MUSIC_HPP
