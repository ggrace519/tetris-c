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
    combo_ = 0;
    maxCombo_ = 0;
    lastTSpin_ = TSpin::None;
    lastClearCount_ = 0;
    fallSpeed_ = kStartFallSpeed;
    gameOver_ = false;
    gravityTimer_ = 0.0;
    lockTimer_ = 0.0;
    landed_ = false;
    dropAnimActive_ = false;
    animProgress_ = 0.0;
    current_ = nextFromBag();
    next_ = nextFromBag();
    if (collides(current_)) gameOver_ = true;
}

void Board::resetLockDelay() {
    // Move/rotate reset: while resting, any successful move or rotate refreshes
    // the lock timer and un-lands the piece (it may now be able to fall again).
    lockTimer_ = 0.0;
    landed_ = false;
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
    if (dropAnimActive_) return false;  // locked out during hard-drop animation
    const int nx = current_.x() + dx;
    const int ny = current_.y() + dy;
    if (collides(current_, nx, ny, current_.rotation())) return false;
    current_.setPos(nx, ny);
    // NOTE: spin flag is intentionally NOT cleared on translation, to match
    // python-tetris's t-spin branch (its move_current_piece leaves spin_axis set;
    // a fresh piece clears it on spawn). This is looser than the guideline "last
    // maneuver must be a rotation" but is the parity target (ADR-0006).
    resetLockDelay();  // move reset
    return true;
}

bool Board::rotate(int direction) {
    if (dropAnimActive_) return false;  // locked out during hard-drop animation
    const int newRot = current_.rotation() + direction;
    int nKicks = 0;
    const Cell* kicks = current_.kicks(nKicks);
    for (int i = 0; i < nKicks; ++i) {
        const int testX = current_.x() + kicks[i].x;
        const int testY = current_.y() + kicks[i].y;
        if (!collides(current_, testX, testY, newRot)) {
            current_.setRotation(newRot);
            current_.setPos(testX, testY);
            current_.setSpin(kicks[i]);  // record the kick used (T-spin: last move = rotation)
            resetLockDelay();  // rotate reset
            return true;
        }
    }
    return false;
}

void Board::hardDrop() {
    if (dropAnimActive_ || gameOver_) return;
    // Find the landing row without moving the piece yet.
    int landingY = current_.y();
    while (!collides(current_, current_.x(), landingY + 1, current_.rotation())) {
        ++landingY;
    }
    const int distance = landingY - current_.y();
    score_ += static_cast<long>(distance) * kHardDropPerCell;  // credit score now

    if (distance == 0) {
        // Already resting — lock immediately (no animation needed).
        lock();
        return;
    }
    // Start the stretch animation; the piece locks when it completes (in step()).
    dropAnimActive_ = true;
    animStartY_ = current_.y();
    animLandingY_ = landingY;
    animProgress_ = 0.0;
}

void Board::lock() {
    // T-spin must be judged from the piece's position BEFORE it is written to the
    // grid (its own cells must not count as blocked corners).
    const TSpin tspin = detectTSpin();
    lastTSpin_ = tspin;

    for (const Cell& c : current_.cells()) {
        if (c.y < 0) {  // locked above the top → game over (board.py lock_current_piece)
            gameOver_ = true;
            return;
        }
        grid_[c.y][c.x] = current_.shape();
    }

    // All scoring for THIS lock multiplies by the level BEFORE the clear (the
    // guideline convention "level is the level before the line clear"). Capture it
    // up front so line score, combo bonus, and T-spin bonus are all consistent —
    // previously the T-spin bonus read the recomputed level and could use a
    // different multiplier than the line score within the same lock.
    const int scoreLevel = level_;

    const int lines = clearLines();
    lastClearCount_ = lines;  // signal for app-side juice
    if (lines > 0) {
        score_ += static_cast<long>(kLineScores[lines]) * scoreLevel;
        linesTotal_ += lines;
        // Combo: consecutive line-clearing locks. First clear = combo 1.
        ++combo_;
        if (combo_ > maxCombo_) maxCombo_ = combo_;
        score_ += static_cast<long>(combo_) * kComboBonusPerLevel * scoreLevel;
        level_ = linesTotal_ / kLinesPerLevel + 1;
        fallSpeed_ = std::max(kMinFallSpeed,
                              kStartFallSpeed - (level_ - 1) * kFallSpeedPerLevel);
    } else {
        combo_ = 0;  // a lock with no clear breaks the combo
    }

    // T-spin bonus (× the pre-clear level), added on top of any line score (ADR-0006).
    if (tspin != TSpin::None) {
        int bonus = 0;
        if (tspin == TSpin::Mini) {
            bonus = kTSpinMini;  // mini: fixed bonus regardless of lines here
        } else {  // Full T-spin: bonus scales with lines cleared
            switch (lines) {
                case 0: bonus = kTSpinMini; break;  // spin with no lines → small bonus
                case 1: bonus = kTSpinSingle; break;
                case 2: bonus = kTSpinDouble; break;
                default: bonus = kTSpinTriple; break;  // 3
            }
        }
        score_ += static_cast<long>(bonus) * scoreLevel;
    }

    current_ = next_;
    next_ = nextFromBag();
    // Fresh piece: clear lock/gravity timing.
    gravityTimer_ = 0.0;
    lockTimer_ = 0.0;
    landed_ = false;
    if (collides(current_)) gameOver_ = true;
}

bool Board::step(double dt) {
    if (gameOver_) return false;

    // Hard-drop animation takes over the step: no gravity/lock-delay while it runs.
    if (dropAnimActive_) {
        animProgress_ += dt / kDropAnimDuration;
        if (animProgress_ >= 1.0) {
            // Animation done: RECOMPUTE the landing row before locking. animLandingY_
            // was captured at hardDrop() time, but the floor can rise underneath the
            // animation (e.g. Ultra garbage injected mid-drop), which would make the
            // stale value overwrite locked cells. Re-scan from the current position.
            int landingY = current_.y();
            while (!collides(current_, current_.x(), landingY + 1, current_.rotation())) {
                ++landingY;
            }
            current_.setPos(current_.x(), landingY);
            dropAnimActive_ = false;
            animProgress_ = 0.0;
            lock();  // lock() clears timing state and spawns the next piece
            return true;
        }
        return false;
    }

    // Is the piece resting on the stack (can't move down)?
    const bool resting = collides(current_, current_.x(), current_.y() + 1,
                                  current_.rotation());

    if (!resting) {
        // Not landed: normal gravity. Any prior landed state is cleared.
        landed_ = false;
        lockTimer_ = 0.0;
        gravityTimer_ += dt;
        if (gravityTimer_ >= fallSpeed_) {
            gravityTimer_ = 0.0;
            move(0, 1);  // guaranteed to succeed (not resting)
            // Gravity descent is not a spin: clear the flag so a T that merely
            // FELL into a blocked slot doesn't score a free T-spin (a player
            // rotation right before lock still counts). Amends ADR-0006.
            current_.clearSpin();
        }
        return false;
    }

    // Resting: run the lock-delay timer. move()/rotate() reset it via
    // resetLockDelay() (which also clears landed_, so we re-enter here next frame).
    landed_ = true;
    gravityTimer_ = 0.0;
    lockTimer_ += dt;
    if (lockTimer_ >= kLockDelay) {
        lock();
        return true;
    }
    return false;
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

Board::TSpin Board::detectTSpin() const {
    // Simplified 3-corner rule (ADR-0006 / python-tetris innovation/t-spin):
    // only a T piece whose last successful move was a rotation can be a T-spin.
    if (current_.shape() != ShapeId::T) return TSpin::None;
    if (!current_.hasSpin()) return TSpin::None;

    // Four corners of the T's 3x3 bounding box (piece origin is its top-left).
    const int x = current_.x();
    const int y = current_.y();
    const Cell corners[4] = {
        {x, y}, {x + 2, y}, {x, y + 2}, {x + 2, y + 2}};

    int blocked = 0;
    for (const Cell& c : corners) {
        if (c.x < 0 || c.x >= kCols || c.y >= kRows) {
            ++blocked;  // wall/floor counts as blocked
        } else if (c.y >= 0 && grid_[c.y][c.x].has_value()) {
            ++blocked;  // filled cell
        }
    }

    if (blocked >= 3) return TSpin::Full;
    if (blocked == 2) return TSpin::Mini;  // spin_axis is non-null (hasSpin true)
    return TSpin::None;
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
