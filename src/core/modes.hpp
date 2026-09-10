// Game modes wrapping a Board. Ports python-tetris/tetris_game/modes.py
// (fixing the dead Ultra garbage mechanic). Pure — no raylib. (ADR-0001)
#ifndef TETRIS_CORE_MODES_HPP
#define TETRIS_CORE_MODES_HPP

#include <cstdint>

#include "core/board.hpp"

namespace tetris {

enum class GameMode { Marathon, Sprint, Ultra };
enum class Difficulty { Easy, Normal, Hard, Expert };

class ModeController {
public:
    ModeController(GameMode mode, Difficulty diff, std::uint64_t seed);

    void reset();

    // Advance mode-specific timers by dt seconds (call each frame while playing).
    // Injects Ultra garbage on the difficulty cadence and checks win conditions.
    void update(double dt);

    Board& board() { return board_; }
    const Board& board() const { return board_; }

    GameMode mode() const { return mode_; }
    Difficulty difficulty() const { return diff_; }
    bool won() const { return won_; }
    double elapsed() const { return timer_; }

    // Win-line target for the current mode (0 = no line target, e.g. Ultra).
    int winLines() const;

    // Garbage interval (seconds) for the current difficulty.
    double garbageInterval() const;

private:
    void checkWin();

    GameMode mode_;
    Difficulty diff_;
    Board board_;
    double timer_ = 0.0;
    double garbageTimer_ = 0.0;
    bool won_ = false;
    std::uint64_t seed_;
    // Simple deterministic sequence for garbage hole columns (independent of the
    // board RNG so it doesn't disturb 7-bag reproducibility).
    std::uint32_t holeState_ = 0;
    int nextHoleCol();
};

}  // namespace tetris

#endif  // TETRIS_CORE_MODES_HPP
