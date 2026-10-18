#include "doctest.h"

#include "core/board.hpp"
#include "core/constants.hpp"
#include "core/piece.hpp"

using namespace tetris;

static Piece placed(ShapeId s, int x, int y, int rot) {
    Piece p(s);
    p.setPos(x, y);
    p.setRotation(rot);
    return p;
}

TEST_CASE("injectGarbage adds a bottom row with exactly one hole") {
    Board b(100);
    const int hole = 4;
    REQUIRE(b.injectGarbage(hole));
    const Grid& g = b.grid();
    int filled = 0;
    for (int c = 0; c < kCols; ++c) {
        if (c == hole) {
            CHECK_FALSE(g[kRows - 1][c].has_value());
        } else {
            CHECK(g[kRows - 1][c].has_value());
            CHECK(g[kRows - 1][c].value() == ShapeId::Garbage);
            ++filled;
        }
    }
    CHECK(filled == kCols - 1);
}

TEST_CASE("injectGarbage shifts the existing stack up by one row") {
    Board b(101);
    // Put a marker on the bottom row; after one injection it should move up 1 row.
    b.setCellForTest(0, kRows - 1, ShapeId::T);
    REQUIRE(b.injectGarbage(9));
    // The marker is now on the second-from-bottom row.
    CHECK(b.grid()[kRows - 2][0].has_value());
    CHECK(b.grid()[kRows - 2][0].value() == ShapeId::T);
    // Bottom row is now garbage (col 0 filled since hole is col 9).
    CHECK(b.grid()[kRows - 1][0].value() == ShapeId::Garbage);
}

TEST_CASE("injectGarbage tops out when a filled cell would leave the top") {
    Board b(102);
    // Fill the top row → shifting up would push it off the field → game over.
    b.setCellForTest(3, 0, ShapeId::L);
    CHECK_FALSE(b.injectGarbage(5));  // returns false
    CHECK(b.gameOver());
}

TEST_CASE("injectGarbage clamps the hole column into range") {
    Board b(103);
    REQUIRE(b.injectGarbage(-5));   // clamps to 0
    CHECK_FALSE(b.grid()[kRows - 1][0].has_value());  // hole at col 0
    Board b2(104);
    REQUIRE(b2.injectGarbage(99));  // clamps to kCols-1
    CHECK_FALSE(b2.grid()[kRows - 1][kCols - 1].has_value());
}

TEST_CASE("garbage does not top out a piece resting on an empty well (#2)") {
    // Regression: the falling piece must rise WITH the world when garbage shifts
    // the stack up. Previously a piece resting on the floor of an otherwise-empty
    // board was left embedded in the risen garbage and false-topped-out.
    Board b(200);
    // O resting on the floor of an empty board: cells at rows kRows-2, kRows-1.
    b.setActiveForTest(placed(ShapeId::O, 4, kRows - 2, 0));
    REQUIRE_FALSE(b.gameOver());
    const int yBefore = b.current().y();
    REQUIRE(b.injectGarbage(0));   // one garbage row (hole at col 0)
    CHECK_FALSE(b.gameOver());      // must survive
    CHECK(b.current().y() == yBefore - 1);  // piece rose one row with the stack
}

TEST_CASE("garbage tops out via the top-row guard when a filled cell would leave the field (#2)") {
    // A filled cell in the TOP row means shifting up pushes it off the field →
    // injectGarbage returns false and tops out (the pre-existing guard). The lift
    // fix must not mask this legitimate loss.
    Board b(201);
    b.setCellForTest(3, 0, ShapeId::L);  // occupied top row
    CHECK_FALSE(b.injectGarbage(0));
    CHECK(b.gameOver());
}

TEST_CASE("garbage tops out when the lifted piece is trapped by cells directly above it (#2)") {
    // Reachable, verified by exhaustive probe: a piece that cannot rise because its
    // cells are blocked directly above, while garbage rises into it from below, is a
    // genuine top-out. This exercises the post-lift collision branch (not the
    // top-row guard). Top row stays empty so the guard does NOT fire first.
    Board b(202);
    // Vertical I at x=0 (rotation 1): cells at (2,0),(2,1),(2,2),(2,3) relative →
    // occupies column 2, rows 0..3 when placed at (0,2)? Use the probe-found case:
    b.setActiveForTest(placed(ShapeId::I, 0, 2, 1));
    // Block directly above each of the piece's cells (rows >=1 so top row stays free).
    for (const Cell& c : b.current().cells()) {
        const int ay = c.y - 1;
        if (ay >= 1 && ay < kRows) b.setCellForTest(c.x, ay, ShapeId::I);
    }
    REQUIRE_FALSE(b.gameOver());
    const int yBefore = b.current().y();
    CHECK(b.injectGarbage(9));          // shift succeeds (top row empty) → returns true
    CHECK(b.current().y() == yBefore);   // piece could NOT rise (trapped above)
    CHECK(b.gameOver());                 // …and garbage overlaps it → real top-out
}

TEST_CASE("garbage lifts a piece resting at the top off-field without a false top-out (#2)") {
    // A piece at the very top rises to y<0 (partly above the field), which is legal;
    // it only tops out if it later LOCKS above the field, not from the lift itself.
    Board b(203);
    b.setActiveForTest(placed(ShapeId::O, 4, 0, 0));  // O at the top, rows 0..1
    const int yBefore = b.current().y();
    REQUIRE(b.injectGarbage(0));
    CHECK(b.current().y() == yBefore - 1);  // rose one row (now partly off-field)
    CHECK_FALSE(b.gameOver());               // rising off-screen is not a loss
}

TEST_CASE("multiple injections stack garbage upward") {
    Board b(105);
    REQUIRE(b.injectGarbage(0));
    REQUIRE(b.injectGarbage(0));
    REQUIRE(b.injectGarbage(0));
    // Bottom three rows are garbage (col 1..9 filled, col 0 hole).
    for (int r = kRows - 3; r < kRows; ++r) {
        CHECK_FALSE(b.grid()[r][0].has_value());
        CHECK(b.grid()[r][1].value() == ShapeId::Garbage);
    }
}
