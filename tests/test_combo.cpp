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

// Set up and lock a piece that completes exactly one bottom line.
static void clearOne(Board& b) {
    for (int c = 0; c < kCols - 2; ++c) b.setCellForTest(c, kRows - 1, ShapeId::I);
    b.setActiveForTest(placed(ShapeId::O, 7, kRows - 2, 0));
    b.lock();
}

// Lock a piece that clears nothing (drops into open space at the top).
static void lockNoClear(Board& b) {
    b.setActiveForTest(placed(ShapeId::O, 0, 0, 0));
    b.lock();
}

TEST_CASE("combo starts at 0 on a fresh board") {
    Board b(1);
    CHECK(b.combo() == 0);
    CHECK(b.maxCombo() == 0);
}

TEST_CASE("consecutive clears increment the combo") {
    Board b(2);
    clearOne(b);
    CHECK(b.combo() == 1);
    clearOne(b);
    CHECK(b.combo() == 2);
    clearOne(b);
    CHECK(b.combo() == 3);
    CHECK(b.maxCombo() == 3);
}

TEST_CASE("a lock with no clear resets the combo") {
    Board b(3);
    clearOne(b);
    clearOne(b);
    CHECK(b.combo() == 2);
    lockNoClear(b);
    CHECK(b.combo() == 0);
    CHECK(b.maxCombo() == 2);  // max is remembered
}

TEST_CASE("combo bonus is combo * 50 * level, added on top of line score") {
    Board b(4);
    // First clear: combo becomes 1 → line 100 + combo bonus 50 = 150 (level 1).
    long before = b.score();
    clearOne(b);
    CHECK(b.score() == before + 100 * 1 + 1 * kComboBonusPerLevel * 1);
    // Second consecutive clear: combo 2 → line 100 + combo bonus 100 = 200.
    before = b.score();
    clearOne(b);
    CHECK(b.score() == before + 100 * 1 + 2 * kComboBonusPerLevel * 1);
}

TEST_CASE("maxCombo tracks the best run across resets") {
    Board b(5);
    clearOne(b); clearOne(b); clearOne(b);  // run of 3
    lockNoClear(b);                          // reset
    clearOne(b);                             // run of 1
    CHECK(b.combo() == 1);
    CHECK(b.maxCombo() == 3);
}
