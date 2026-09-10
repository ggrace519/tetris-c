// A falling tetromino: shape id, rotation state, board position.
// Ports python-tetris/tetris_game/piece.py. Pure — no raylib. (ADR-0001)
#ifndef TETRIS_CORE_PIECE_HPP
#define TETRIS_CORE_PIECE_HPP

#include <array>

#include "core/constants.hpp"

namespace tetris {

class Piece {
public:
    explicit Piece(ShapeId shape);

    ShapeId shape() const { return shape_; }
    int rotation() const { return rotation_; }
    int x() const { return px_; }
    int y() const { return py_; }
    Color color() const { return kPieceColors[static_cast<int>(shape_)]; }

    void setRotation(int r) { rotation_ = ((r % 4) + 4) % 4; }
    void setPos(int nx, int ny) { px_ = nx; py_ = ny; }

    // Board cells occupied at a given (x, y, rotation); defaults use current state.
    // Returns the 4 absolute cells (board coordinates).
    std::array<Cell, 4> cells() const { return cellsAt(px_, py_, rotation_); }
    std::array<Cell, 4> cellsAt(int atX, int atY, int rot) const;

    // Kick offsets for this shape class (settings.py get_kicks).
    // Returns pointer+count to avoid heap allocation.
    const Cell* kicks(int& count) const;

private:
    ShapeId shape_;
    int rotation_ = 0;
    int px_ = kSpawnX;
    int py_ = kSpawnY;
};

}  // namespace tetris

#endif  // TETRIS_CORE_PIECE_HPP
