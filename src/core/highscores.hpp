// Persistent high scores per game mode. Pure core (no raylib); uses only the
// standard library for file I/O so it stays headless-testable. (ADR-0001)
#ifndef TETRIS_CORE_HIGHSCORES_HPP
#define TETRIS_CORE_HIGHSCORES_HPP

#include <array>
#include <string>

#include "core/modes.hpp"

namespace tetris {

// Best result per mode. For Marathon/Ultra the metric is score (higher better);
// for Sprint it's time to clear the target (lower better). We store both a best
// score and a best time so the app can show whichever fits the mode.
struct ModeRecord {
    long bestScore = 0;
    double bestTime = 0.0;   // seconds; meaningful for Sprint (0 = none yet)
    bool hasTime = false;    // whether bestTime is set
};

class HighScores {
public:
    // Load from `path`; a missing/garbage file yields all-zero records (no throw).
    void load(const std::string& path);
    // Write to `path`. Returns false on I/O failure.
    bool save(const std::string& path) const;

    const ModeRecord& record(GameMode m) const { return records_[idx(m)]; }

    // Submit a finished game. Returns true if it set a new record for that mode.
    // score: final score. timeSec: elapsed time. cleared: did the player win
    // (relevant for Sprint's best-time).
    bool submit(GameMode m, long score, double timeSec, bool cleared);

private:
    static int idx(GameMode m) { return static_cast<int>(m); }
    std::array<ModeRecord, 3> records_{};
};

}  // namespace tetris

#endif  // TETRIS_CORE_HIGHSCORES_HPP
