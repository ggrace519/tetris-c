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
    bool move(int dx, int dy);

    // Rotate active piece (+1 CW, -1 CCW) trying the kick table. Returns success.
    bool rotate(int direction);

    // Drop active piece to landing row, score the distance, and lock.
    void hardDrop();

    // Copy the active piece into the grid, clear lines, spawn next, check game-over.
    void lock();

    // Remove full rows, shift down, return count cleared.
    int clearLines();

    // Landing cells of the active piece (for the ghost).
    std::array<Cell, 4> ghostCells() const;

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
};

}  // namespace tetris

#endif  // TETRIS_CORE_BOARD_HPP
