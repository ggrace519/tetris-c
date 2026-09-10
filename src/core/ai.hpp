// Heuristic Tetris AI (Seki 2004 four-heuristic evaluation). Pure core, no raylib.
// Ports python-tetris innovation/ai. (ADR-0001)
#ifndef TETRIS_CORE_AI_HPP
#define TETRIS_CORE_AI_HPP

#include "core/board.hpp"
#include "core/constants.hpp"
#include "core/modes.hpp"  // Difficulty
#include "core/piece.hpp"
#include "core/rng.hpp"

namespace tetris {

struct AiMove {
    int x;         // target column (piece origin x)
    int rotation;  // target rotation state
    bool valid;    // false if no legal placement was found
};

class TetrisAI {
public:
    explicit TetrisAI(Difficulty diff = Difficulty::Normal, std::uint64_t seed = 0);

    void setDifficulty(Difficulty d);
    double decisionDelay() const { return decisionDelay_; }
    double errorRate() const { return errorRate_; }

    // Evaluate a grid with the four weighted heuristics (higher = better).
    static double evaluate(const Grid& grid);

    // Individual heuristics (exposed for testing).
    static int aggregateHeight(const Grid& grid);
    static int completeLines(const Grid& grid);
    static int holes(const Grid& grid);
    static int bumpiness(const Grid& grid);

    // Choose the best placement for `piece` on `grid`. With the difficulty's error
    // rate, occasionally returns a random legal move instead. Uses the AI's RNG.
    AiMove bestMove(const Grid& grid, const Piece& piece);

private:
    // Simulate dropping the piece at (x, rotation) onto a copy of grid, clear full
    // rows, and return the resulting grid. Assumes the placement column is legal.
    static Grid simulateDrop(const Grid& grid, const Piece& piece, int x, int rotation);
    static bool columnFits(const Grid& grid, const Piece& piece, int x, int rotation);

    Difficulty diff_;
    double errorRate_ = 0.10;
    double decisionDelay_ = 0.3;
    Rng rng_;
};

}  // namespace tetris

#endif  // TETRIS_CORE_AI_HPP
