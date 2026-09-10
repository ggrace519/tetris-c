// Board state and all gameplay rules. Ports python-tetris/tetris_game/board.py.
// Pure — no raylib. All rules are here and unit-tested headlessly. (ADR-0001)
#ifndef TETRIS_CORE_BOARD_HPP
#define TETRIS_CORE_BOARD_HPP

#include <array>
#include <optional>
#include <vector>

#include "core/constants.hpp"
#include "core/piece.hpp"
#include "core/rng.hpp"

namespace tetris {

// A locked cell stores the shape that filled it (for color) or is empty.
using GridCell = std::optional<ShapeId>;
using Grid = std::array<std::array<GridCell, kCols>, kRows>;

class Board {
public:
    Board();
    explicit Board(std::uint64_t seed);

    void reset();

    // --- accessors ---
    const Grid& grid() const { return grid_; }
    const Piece& current() const { return current_; }
    const Piece& next() const { return next_; }
    long score() const { return score_; }
    int level() const { return level_; }
    int linesCleared() const { return linesTotal_; }
    double fallSpeed() const { return fallSpeed_; }
    bool gameOver() const { return gameOver_; }

    // --- rules ---
    // True if the piece at (atX, atY, rot) overlaps walls, floor, or locked cells.
    bool collides(const Piece& p, int atX, int atY, int rot) const;
    bool collides(const Piece& p) const { return collides(p, p.x(), p.y(), p.rotation()); }

    // Move active piece by (dx, dy) if the destination is valid. Returns success.
    // A successful move resets the lock-delay timer (move reset).
    bool move(int dx, int dy);

    // Rotate active piece (+1 CW, -1 CCW) trying the kick table. Returns success.
    // A successful rotation resets the lock-delay timer (rotate reset).
    bool rotate(int direction);

    // Advance gravity + lock delay by dt seconds. Applies gravity every fallSpeed
    // seconds; when the piece rests on the stack, runs a lock-delay timer that
    // move()/rotate() reset. Locks the piece when the timer expires. Returns true
    // if a piece locked this call. This is the single time-stepped entry point the
    // app calls each frame (replaces manual move(0,1)/lock()).
    bool step(double dt);

    // Is the active piece currently resting (lock-delay running)?
    bool landed() const { return landed_; }

    // Drop active piece to landing row, score the distance, and lock.
    void hardDrop();

    // Copy the active piece into the grid, clear lines, spawn next, check game-over.
    void lock();

    // Remove full rows, shift down, return count cleared.
    int clearLines();

    // Landing cells of the active piece (for the ghost).
    std::array<Cell, 4> ghostCells() const;

    // Push one garbage row onto the bottom: the locked stack shifts up one row and
    // a full row with a single empty column (`holeCol`) is inserted at the bottom.
    // If shifting would carry any filled cell off the top of the field, the board
    // tops out (game over). Used by Ultra mode. Returns true if a row was injected
    // (false if already game over). NOTE: the Python source's Ultra garbage was
    // dead code (never drained); this is the intended, working behavior.
    bool injectGarbage(int holeCol);

    // Testing hook: force a specific active piece (position/rotation preserved from spawn).
    void setActiveForTest(const Piece& p) { current_ = p; }
    // Testing hook: set a locked cell.
    void setCellForTest(int x, int y, ShapeId s) { grid_[y][x] = s; }

private:
    Piece nextFromBag();
    void refillBag();

    Grid grid_{};
    std::vector<ShapeId> bag_;
    Rng rng_;
    long score_ = 0;
    int linesTotal_ = 0;
    int level_ = 1;
    double fallSpeed_ = kStartFallSpeed;
    bool gameOver_ = false;
    Piece current_{ShapeId::I};
    Piece next_{ShapeId::I};

    // Timing state for step()/lock delay.
    double gravityTimer_ = 0.0;
    double lockTimer_ = 0.0;
    bool landed_ = false;

    void resetLockDelay();
};

}  // namespace tetris

#endif  // TETRIS_CORE_BOARD_HPP
