#include "core/piece.hpp"

namespace tetris {

Piece::Piece(ShapeId shape) : shape_(shape) {}

std::array<Cell, 4> Piece::cellsAt(int atX, int atY, int rot) const {
    const int r = ((rot % 4) + 4) % 4;
    const Rotation& offsets = kShapes[static_cast<int>(shape_)][r];
    std::array<Cell, 4> out{};
    for (int i = 0; i < 4; ++i) {
        out[i] = Cell{atX + offsets[i].x, atY + offsets[i].y};
    }
    return out;
}

const Cell* Piece::kicks(int& count) const {
    switch (shape_) {
        case ShapeId::I:
            count = static_cast<int>(kKicksI.size());
            return kKicksI.data();
        case ShapeId::O:
            count = static_cast<int>(kKicksO.size());
            return kKicksO.data();
        default:
            count = static_cast<int>(kKicksJLSTZ.size());
            return kKicksJLSTZ.data();
    }
}

}  // namespace tetris
