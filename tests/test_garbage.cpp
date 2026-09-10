#include "doctest.h"

#include "core/board.hpp"
#include "core/constants.hpp"

using namespace tetris;

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
