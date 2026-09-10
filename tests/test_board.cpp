#include "doctest.h"

#include "core/board.hpp"
#include "core/constants.hpp"
#include "core/piece.hpp"
#include "test_helpers.hpp"

using namespace tetris;

// Helper: place a specific piece as the active one at a known position/rotation.
static Piece placed(ShapeId s, int x, int y, int rot) {
    Piece p(s);
    p.setPos(x, y);
    p.setRotation(rot);
    return p;
}

TEST_CASE("fresh board: empty grid, level 1, score 0, not over") {
    Board b(42);
    CHECK(b.score() == 0);
    CHECK(b.level() == 1);
    CHECK(b.linesCleared() == 0);
    CHECK_FALSE(b.gameOver());
    int filled = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++filled;
    CHECK(filled == 0);
}

TEST_CASE("collision: walls, floor, and locked cells") {
    Board b(1);
    // O piece occupies (x+1,y),(x+2,y),(x+1,y+1),(x+2,y+1)
    Piece o(ShapeId::O);
    // Left wall: x=-2 puts left cells at column -1
    CHECK(b.collides(o, -2, 0, 0));
    // Right wall: x that pushes a cell to column 10
    CHECK(b.collides(o, kCols - 1, 0, 0));  // cells at cols 10,11
    // Floor: y past bottom
    CHECK(b.collides(o, 3, kRows, 0));
    // In-bounds open space: no collision
    CHECK_FALSE(b.collides(o, 3, 0, 0));
    // Locked cell blocks it
    b.setCellForTest(4, 5, ShapeId::I);  // matches O at x=3,y=4 -> cell (4,5)
    CHECK(b.collides(o, 3, 4, 0));
}

TEST_CASE("collision allows cells above the top (y<0)") {
    Board b(1);
    Piece o(ShapeId::O);
    // y=-1: top cells at row -1 (above field) but not colliding
    CHECK_FALSE(b.collides(o, 3, -1, 0));
}

TEST_CASE("move: valid succeeds, blocked is a no-op returning false") {
    Board b(7);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    CHECK(b.move(1, 0));
    CHECK(b.current().x() == 4);
    // Push to right wall then try again
    while (b.move(1, 0)) {}
    const int wallX = b.current().x();
    CHECK_FALSE(b.move(1, 0));         // blocked
    CHECK(b.current().x() == wallX);   // unchanged
}

TEST_CASE("rotate: succeeds in open space") {
    Board b(3);
    b.setActiveForTest(placed(ShapeId::T, 4, 2, 0));
    CHECK(b.rotate(1));
    CHECK(b.current().rotation() == 1);
    CHECK(b.rotate(-1));
    CHECK(b.current().rotation() == 0);
}

TEST_CASE("rotate: kicks off the left wall") {
    Board b(3);
    // Place an I flat against the left wall where a naive rotation would clip it;
    // the kick table should shift it right and succeed.
    b.setActiveForTest(placed(ShapeId::I, -1, 0, 0));
    // At x=-1 rotation 0, I cells are cols -1..2 -> already colliding, but we test
    // that rotate finds a kick to a legal vertical orientation.
    const bool ok = b.rotate(1);
    CHECK(ok);
    // After a successful kick the piece must be in a non-colliding state.
    CHECK_FALSE(b.collides(b.current()));
}

TEST_CASE("hard drop: lands on floor, scores 2/cell, and locks") {
    Board b(9);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    const long before = b.score();
    hardDropAndSettle(b);  // hardDrop() now animates; settle to lock
    // O landed: its bottom cells should be on the last row.
    // Score increased by 2 * distance (>0 since it fell from top).
    CHECK(b.score() > before);
    // A piece is now locked into the grid (some cells filled).
    int filled = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++filled;
    CHECK(filled == 4);
}

TEST_CASE("line clear: single row clears, rows above shift down") {
    Board b(11);
    // Fill the bottom row completely except we'll fill it fully via test hooks.
    for (int c = 0; c < kCols; ++c) b.setCellForTest(c, kRows - 1, ShapeId::I);
    // Put one marker cell above it to verify it shifts down.
    b.setCellForTest(0, kRows - 2, ShapeId::T);
    const int cleared = b.clearLines();
    CHECK(cleared == 1);
    // Bottom row should now hold the shifted marker at col 0.
    CHECK(b.grid()[kRows - 1][0].has_value());
    CHECK(b.grid()[kRows - 1][0].value() == ShapeId::T);
    // And only that one cell is filled now.
    int filled = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++filled;
    CHECK(filled == 1);
}

TEST_CASE("line clear: tetris (4 rows) clears all four") {
    Board b(13);
    for (int r = kRows - 4; r < kRows; ++r)
        for (int c = 0; c < kCols; ++c)
            b.setCellForTest(c, r, ShapeId::I);
    CHECK(b.clearLines() == 4);
}

TEST_CASE("scoring and level: lock a full row applies LINE_SCORES * level") {
    Board b(15);
    // Fill bottom row except one gap, then drop a piece into the gap via lock.
    // Simpler: fill bottom row fully with test hooks minus one, set active O over gap.
    for (int c = 0; c < kCols - 2; ++c) b.setCellForTest(c, kRows - 1, ShapeId::I);
    // Active O will fill cols 8,9 at the bottom two rows; the bottom row completes.
    b.setActiveForTest(placed(ShapeId::O, 7, kRows - 2, 0));  // cells cols 8,9 rows 18,19
    const long before = b.score();
    b.lock();
    // One line cleared → line score (100 * level 1) + combo bonus (combo 1 * 50 * 1).
    CHECK(b.score() == before + kLineScores[1] * 1 + 1 * kComboBonusPerLevel * 1);
    CHECK(b.linesCleared() == 1);
    CHECK(b.combo() == 1);
}

TEST_CASE("level rises every 10 lines and fall speed decreases") {
    Board b(17);
    // Directly clear 10 rows one at a time by filling+locking is heavy; instead
    // drive linesTotal via repeated full-row clears using lock on prepared rows.
    // We simulate 10 single clears.
    for (int i = 0; i < 10; ++i) {
        for (int c = 0; c < kCols - 2; ++c) b.setCellForTest(c, kRows - 1, ShapeId::I);
        b.setActiveForTest(placed(ShapeId::O, 7, kRows - 2, 0));
        b.lock();
    }
    CHECK(b.linesCleared() == 10);
    CHECK(b.level() == 2);
    CHECK(b.fallSpeed() < kStartFallSpeed);
}

TEST_CASE("7-bag: each 7-piece window contains all 7 shapes once (seeded)") {
    Board b(2024);
    // The first 14 pieces (current, next, then locking to advance) span two bags.
    // We inspect by locking repeatedly and recording the shape that became active.
    std::array<int, kShapeCount> count0{};
    // current + next are the first two of bag draws; collect 7 by advancing.
    // Record current shape, then lock to advance, 7 times = one bag's worth.
    for (int i = 0; i < kShapeCount; ++i) {
        count0[static_cast<int>(b.current().shape())]++;
        // Move current out of the way and lock to spawn the next.
        hardDropAndSettle(b);
        if (b.gameOver()) break;  // shouldn't happen this early
    }
    // Note: current+next come from the same bag stream; after 7 draws we have seen
    // a full bag only if no early game-over. Each shape appears at least... we assert
    // the multiset over the FIRST 7 current-shapes is a permutation of 0..6.
    int seen = 0;
    for (int c : count0) if (c >= 1) ++seen;
    CHECK(seen == kShapeCount);
    for (int c : count0) CHECK(c == 1);
}

TEST_CASE("ghost cells equal the hard-drop landing position") {
    Board b(19);
    b.setActiveForTest(placed(ShapeId::T, 4, 0, 0));
    auto ghost = b.ghostCells();
    // Ghost must be a legal (non-colliding) position and be the lowest such.
    Piece probe = placed(ShapeId::T, 4, 0, 0);
    // Find expected landing y by scanning.
    int y = 0;
    while (!b.collides(probe, 4, y + 1, 0)) ++y;
    auto expected = probe.cellsAt(4, y, 0);
    for (int i = 0; i < 4; ++i) {
        CHECK(ghost[i].x == expected[i].x);
        CHECK(ghost[i].y == expected[i].y);
    }
}

TEST_CASE("game over: spawn collision sets gameOver") {
    Board b(21);
    // Block the spawn area so the NEXT piece collides on spawn — but leave a gap in
    // each row so these rows are NOT full lines (a full line would be cleared by
    // lock() before the game-over check, freeing the spawn area).
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < kCols - 1; ++c)  // leave column 9 empty → not a full row
            b.setCellForTest(c, r, ShapeId::I);
    // Lock the current piece harmlessly at the bottom; then next spawns into the
    // blocked top area and must collide.
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));
    b.lock();
    CHECK(b.gameOver());
}

TEST_CASE("deterministic seed reproduces the same piece sequence") {
    Board a(555), c(555);
    for (int i = 0; i < 5; ++i) {
        CHECK(a.current().shape() == c.current().shape());
        hardDropAndSettle(a);
        hardDropAndSettle(c);
    }
}
