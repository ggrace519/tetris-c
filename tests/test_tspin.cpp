#include "doctest.h"

#include "core/board.hpp"
#include "core/constants.hpp"

using namespace tetris;

static Piece placed(ShapeId s, int x, int y, int rot) {
    Piece p(s);
    p.setPos(x, y);
    p.setRotation(rot);
    return p;
}

TEST_CASE("no T-spin for a non-T piece") {
    Board b(1);
    Piece o = placed(ShapeId::O, 3, kRows - 2, 0);
    o.setSpin(Cell{-1, 0});  // pretend a rotation happened
    b.setActiveForTest(o);
    b.lock();
    CHECK(b.lastTSpin() == Board::TSpin::None);
}

TEST_CASE("no T-spin if last move was not a rotation (no spin flag)") {
    Board b(2);
    // T in a fully-blocked slot but with NO spin flag → not a T-spin.
    // Fill 3 corners around a T at (x,y).
    const int x = 3, y = kRows - 3;
    b.setCellForTest(x, y, ShapeId::I);          // top-left
    b.setCellForTest(x + 2, y, ShapeId::I);      // top-right
    b.setCellForTest(x, y + 2, ShapeId::I);      // bottom-left
    Piece t = placed(ShapeId::T, x, y, 2);       // T pointing up (rot 2)
    // no setSpin
    b.setActiveForTest(t);
    b.lock();
    CHECK(b.lastTSpin() == Board::TSpin::None);
}

TEST_CASE("full T-spin: 3+ corners blocked with a spin") {
    Board b(3);
    const int x = 3, y = kRows - 3;
    // Block 3 of the 4 corners.
    b.setCellForTest(x, y, ShapeId::I);
    b.setCellForTest(x + 2, y, ShapeId::I);
    b.setCellForTest(x, y + 2, ShapeId::I);
    Piece t = placed(ShapeId::T, x, y, 2);
    t.setSpin(Cell{-1, 0});  // last move was a rotation
    b.setActiveForTest(t);
    b.lock();
    CHECK(b.lastTSpin() == Board::TSpin::Full);
}

TEST_CASE("mini T-spin: exactly 2 corners blocked with a spin") {
    Board b(4);
    const int x = 3, y = kRows - 3;
    // Block exactly 2 corners.
    b.setCellForTest(x, y, ShapeId::I);
    b.setCellForTest(x + 2, y, ShapeId::I);
    Piece t = placed(ShapeId::T, x, y, 2);
    t.setSpin(Cell{1, 0});
    b.setActiveForTest(t);
    b.lock();
    CHECK(b.lastTSpin() == Board::TSpin::Mini);
}

TEST_CASE("floor counts as a blocked corner") {
    Board b(5);
    // T rotation 0 cells: (x+1,y),(x,y+1),(x+1,y+1),(x+2,y+1) — flat bottom at y+1.
    // Place at y=kRows-2 so the piece rests on the floor (y+1 == last row). The two
    // bottom corners {x,y+2},{x+2,y+2} are then at row kRows → off-field (blocked).
    const int x = 3, y = kRows - 2;
    b.setCellForTest(x, y, ShapeId::I);  // top-left corner filled → 3rd blocked corner
    Piece t = placed(ShapeId::T, x, y, 0);
    t.setSpin(Cell{-1, 0});
    b.setActiveForTest(t);         // make it the active piece (was missing!)
    REQUIRE_FALSE(b.collides(b.current()));  // legal placement (asserts the test is real)
    REQUIRE(b.current().hasSpin());
    b.lock();
    CHECK(b.lastTSpin() == Board::TSpin::Full);  // 2 floor corners + 1 filled = 3
}

TEST_CASE("REGRESSION: all scoring terms in one lock use the pre-clear level") {
    // Preload 9 cleared lines so the NEXT single clear tips level 1 -> 2. A T-spin
    // single that IS that 10th line must score ALL its terms (line + combo + T-spin
    // bonus) at level 1 (pre-clear), not a mix of level 1 and level 2.
    Board b(77);
    // Clear 9 lines the direct way (fill bottom row, lock a harmless piece).
    for (int i = 0; i < 9; ++i) {
        for (int c = 0; c < kCols - 2; ++c) b.setCellForTest(c, kRows - 1, ShapeId::J);
        b.setActiveForTest(placed(ShapeId::O, 7, kRows - 2, 0));  // fills cols 8,9 → clears row
        b.lock();
    }
    REQUIRE(b.linesCleared() == 9);
    REQUIRE(b.level() == 1);  // still level 1 (9 < 10)

    // Now a T-spin single as the 10th line → tips to level 2 AFTER scoring.
    const int x = 3, y = kRows - 3;
    for (int c = 0; c < kCols; ++c) if (c != x + 1) b.setCellForTest(c, kRows - 1, ShapeId::I);
    b.setCellForTest(x, y, ShapeId::I);
    b.setCellForTest(x + 2, y, ShapeId::I);
    b.setCellForTest(x, y + 2, ShapeId::I);
    Piece t = placed(ShapeId::T, x, y, 2);
    t.setSpin(Cell{-1, 0});
    b.setActiveForTest(t);
    const long before = b.score();
    b.lock();
    REQUIRE(b.lastTSpin() == Board::TSpin::Full);
    REQUIRE(b.linesCleared() == 10);
    REQUIRE(b.level() == 2);  // level bumped after this lock
    // But all terms scored at level 1: line(100) + combo(10*50) + tspin(200), ×1.
    const long expected = 100 * 1 + b.combo() * kComboBonusPerLevel * 1 + kTSpinSingle * 1;
    CHECK(b.score() - before == expected);
}

TEST_CASE("player hard drop after a spin into a blocked slot DOES score a T-spin (ADR-0006)") {
    // Intended rule: player-initiated descent preserves the spin flag. Build a
    // 3-corner blocked slot with the T one row above its resting spot, rotate
    // (sets spin), then HARD DROP — it should still classify as a T-spin.
    Board b(88);
    const int x = 3;
    const int slotY = kRows - 3;  // where the T will rest (rot 2)
    b.setCellForTest(x, slotY, ShapeId::I);
    b.setCellForTest(x + 2, slotY, ShapeId::I);
    b.setCellForTest(x, slotY + 2, ShapeId::I);
    // Place the T one row above the slot so hard drop moves it down by 1.
    Piece t = placed(ShapeId::T, x, slotY - 1, 2);
    b.setActiveForTest(t);
    b.rotate(1); b.rotate(-1);  // player rotation → spin flag set (still at slotY-1)
    REQUIRE(b.current().hasSpin());
    b.hardDrop();
    while (b.animating()) b.step(0.1);  // complete the drop → lock
    CHECK(b.lastTSpin() != Board::TSpin::None);  // hard drop preserved the spin
}

TEST_CASE("REGRESSION: a T that merely FELL (gravity) into a blocked slot is not a T-spin") {
    Board b(99);
    const int x = 3;
    const int landY = kRows - 3;
    // Blocked slot: 3 corners around the resting position.
    b.setCellForTest(x, landY, ShapeId::I);
    b.setCellForTest(x + 2, landY, ShapeId::I);
    b.setCellForTest(x, landY + 2, ShapeId::I);
    // Start a T high, rotate it (sets the spin flag), then let gravity carry it down.
    Piece t = placed(ShapeId::T, x, 0, 2);
    b.setActiveForTest(t);
    b.rotate(1); b.rotate(-1);              // player rotation → spin flag set
    REQUIRE(b.current().hasSpin());
    // Gravity descent must clear the spin so this is NOT a free T-spin.
    bool locked = false;
    for (int i = 0; i < 60 && !locked && !b.gameOver(); ++i) {
        locked = b.step(b.fallSpeed());
    }
    REQUIRE(locked);
    CHECK(b.lastTSpin() == Board::TSpin::None);
}

TEST_CASE("T-spin single awards the T-spin bonus on top of the line score") {
    Board b(6);
    // Build a row that completes when the T locks, with 3 corners blocked so it's
    // a full T-spin single.
    const int x = 3, y = kRows - 3;
    // Fill the bottom row except the T's slot columns, so locking the T clears it.
    // T rot 2 (pointing up) cells: (x,y+1),(x+1,y+1),(x+2,y+1),(x+1,y+2).
    // Fill bottom row (y+2 == kRows-1) except column x+1 (the T's nub fills it).
    for (int c = 0; c < kCols; ++c) {
        if (c != x + 1) b.setCellForTest(c, kRows - 1, ShapeId::I);
    }
    // Block 3 corners of the T box for the spin.
    b.setCellForTest(x, y, ShapeId::I);
    b.setCellForTest(x + 2, y, ShapeId::I);
    b.setCellForTest(x, y + 2, ShapeId::I);  // bottom-left corner (already part of filled row)
    Piece t = placed(ShapeId::T, x, y, 2);
    t.setSpin(Cell{-1, 0});
    b.setActiveForTest(t);
    const long before = b.score();
    b.lock();
    // Exactly one line should clear, and it should be classified a full T-spin.
    CHECK(b.lastTSpin() == Board::TSpin::Full);
    CHECK(b.linesCleared() == 1);
    // Score gained includes: line(100) + combo(1*50) + t-spin single(200), all * level 1.
    const long gained = b.score() - before;
    CHECK(gained == (100 + 50 + kTSpinSingle) * 1);
}
