#include "core/board.hpp"

#include <algorithm>

namespace tetris {

Board::Board() { reset(); }
Board::Board(std::uint64_t seed) : rng_(seed) { reset(); }

void Board::reset() {
    for (auto& row : grid_) row.fill(std::nullopt);
    bag_.clear();
    score_ = 0;
    linesTotal_ = 0;
    level_ = 1;
    fallSpeed_ = kStartFallSpeed;
    gameOver_ = false;
    current_ = nextFromBag();
    next_ = nextFromBag();
    if (collides(current_)) gameOver_ = true;
}

void Board::refillBag() {
    // One of each shape, shuffled — the 7-bag (board.py _refill_bag).
    std::vector<ShapeId> pieces;
    pieces.reserve(kShapeCount);
    for (int i = 0; i < kShapeCount; ++i) pieces.push_back(static_cast<ShapeId>(i));
    rng_.shuffle(pieces.begin(), pieces.end());
    // Append; we pop from the back (order within a bag is still uniform-random).
    bag_.insert(bag_.end(), pieces.begin(), pieces.end());
}

Piece Board::nextFromBag() {
    if (bag_.empty()) refillBag();
    ShapeId s = bag_.back();
    bag_.pop_back();
    return Piece(s);
}

bool Board::collides(const Piece& p, int atX, int atY, int rot) const {
    for (const Cell& c : p.cellsAt(atX, atY, rot)) {
        if (c.x < 0 || c.x >= kCols) return true;
        if (c.y >= kRows) return true;
        if (c.y >= 0 && grid_[c.y][c.x].has_value()) return true;
    }
    return false;
}

bool Board::move(int dx, int dy) {
    const int nx = current_.x() + dx;
    const int ny = current_.y() + dy;
    if (collides(current_, nx, ny, current_.rotation())) return false;
    current_.setPos(nx, ny);
    return true;
}

bool Board::rotate(int direction) {
    const int newRot = current_.rotation() + direction;
    int nKicks = 0;
    const Cell* kicks = current_.kicks(nKicks);
    for (int i = 0; i < nKicks; ++i) {
        const int testX = current_.x() + kicks[i].x;
        const int testY = current_.y() + kicks[i].y;
        if (!collides(current_, testX, testY, newRot)) {
            current_.setRotation(newRot);
            current_.setPos(testX, testY);
            return true;
        }
    }
    return false;
}

void Board::hardDrop() {
    int distance = 0;
    while (move(0, 1)) ++distance;
    score_ += static_cast<long>(distance) * kHardDropPerCell;
    lock();
}

void Board::lock() {
    for (const Cell& c : current_.cells()) {
        if (c.y < 0) {  // locked above the top → game over (board.py lock_current_piece)
            gameOver_ = true;
            return;
        }
        grid_[c.y][c.x] = current_.shape();
    }

    const int lines = clearLines();
    if (lines > 0) {
        score_ += static_cast<long>(kLineScores[lines]) * level_;
        linesTotal_ += lines;
        level_ = linesTotal_ / kLinesPerLevel + 1;
        fallSpeed_ = std::max(kMinFallSpeed,
                              kStartFallSpeed - (level_ - 1) * kFallSpeedPerLevel);
    }

    current_ = next_;
    next_ = nextFromBag();
    if (collides(current_)) gameOver_ = true;
}

int Board::clearLines() {
    // Keep only rows that have at least one empty cell; refill from the top.
    Grid out{};
    int writeRow = kRows - 1;
    for (int r = kRows - 1; r >= 0; --r) {
        bool full = true;
        for (int c = 0; c < kCols; ++c) {
            if (!grid_[r][c].has_value()) { full = false; break; }
        }
        if (!full) {
            out[writeRow] = grid_[r];
            --writeRow;
        }
    }
    // Rows above writeRow remain default-empty (already the case for `out`).
    const int cleared = writeRow + 1;
    grid_ = out;
    return cleared;
}

bool Board::injectGarbage(int holeCol) {
    if (gameOver_) return false;
    if (holeCol < 0) holeCol = 0;
    if (holeCol >= kCols) holeCol = kCols - 1;

    // If the top row holds any filled cell, shifting up tops it out.
    for (int c = 0; c < kCols; ++c) {
        if (grid_[0][c].has_value()) {
            gameOver_ = true;
            return false;
        }
    }

    // Shift every row up by one (row r takes row r+1's contents).
    for (int r = 0; r < kRows - 1; ++r) {
        grid_[r] = grid_[r + 1];
    }
    // Build the garbage row: all filled except the hole. Use the garbage marker
    // shape (rendered gray) — we reuse ShapeId but color via kColGarbage in render.
    auto& bottom = grid_[kRows - 1];
    for (int c = 0; c < kCols; ++c) {
        bottom[c] = (c == holeCol) ? std::nullopt : GridCell(ShapeId::Garbage);
    }
    // Garbage may now overlap the active piece; if so, the player is topping out.
    if (collides(current_)) gameOver_ = true;
    return true;
}

std::array<Cell, 4> Board::ghostCells() const {
    int ghostY = current_.y();
    while (!collides(current_, current_.x(), ghostY + 1, current_.rotation())) {
        ++ghostY;
    }
    return current_.cellsAt(current_.x(), ghostY, current_.rotation());
}

}  // namespace tetris
