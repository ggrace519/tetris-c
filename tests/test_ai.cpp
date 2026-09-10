#include "doctest.h"

#include "core/ai.hpp"
#include "core/board.hpp"
#include "core/constants.hpp"
#include "core/modes.hpp"
#include "core/piece.hpp"

using namespace tetris;

// Build an empty grid.
static Grid emptyGrid() { Grid g{}; for (auto& r : g) r.fill(std::nullopt); return g; }

TEST_CASE("aggregate height: empty grid is 0, one bottom cell is 1") {
    Grid g = emptyGrid();
    CHECK(TetrisAI::aggregateHeight(g) == 0);
    g[kRows - 1][0] = ShapeId::I;  // one cell at the bottom → column height 1
    CHECK(TetrisAI::aggregateHeight(g) == 1);
    g[0][0] = ShapeId::I;          // topmost fill in col 0 → height becomes kRows
    CHECK(TetrisAI::aggregateHeight(g) == kRows);
}

TEST_CASE("complete lines counts full rows") {
    Grid g = emptyGrid();
    CHECK(TetrisAI::completeLines(g) == 0);
    for (int c = 0; c < kCols; ++c) g[kRows - 1][c] = ShapeId::I;
    CHECK(TetrisAI::completeLines(g) == 1);
}

TEST_CASE("holes counts empty cells under filled ones") {
    Grid g = emptyGrid();
    CHECK(TetrisAI::holes(g) == 0);
    g[kRows - 2][0] = ShapeId::I;  // filled cell with an empty one below it → 1 hole
    CHECK(TetrisAI::holes(g) == 1);
    g[kRows - 1][0] = ShapeId::I;  // fill the hole → 0 holes
    CHECK(TetrisAI::holes(g) == 0);
}

TEST_CASE("bumpiness sums adjacent column height differences") {
    Grid g = emptyGrid();
    CHECK(TetrisAI::bumpiness(g) == 0);  // flat empty board
    g[kRows - 1][0] = ShapeId::I;        // col 0 height 1, col 1 height 0 → diff 1
    CHECK(TetrisAI::bumpiness(g) == 1);
}

TEST_CASE("evaluate uses the Seki weights (holes hurt the score)") {
    Grid flat = emptyGrid();
    Grid withHole = emptyGrid();
    withHole[kRows - 2][0] = ShapeId::I;  // creates a hole + height + bumpiness
    // A board with a hole should score strictly worse than the empty board.
    CHECK(TetrisAI::evaluate(withHole) < TetrisAI::evaluate(flat));
}

TEST_CASE("difficulty sets error rate and decision delay (Seki values)") {
    CHECK(TetrisAI(Difficulty::Easy).errorRate() == doctest::Approx(0.30));
    CHECK(TetrisAI(Difficulty::Normal).errorRate() == doctest::Approx(0.10));
    CHECK(TetrisAI(Difficulty::Hard).errorRate() == doctest::Approx(0.02));
    CHECK(TetrisAI(Difficulty::Expert).errorRate() == doctest::Approx(0.00));
    CHECK(TetrisAI(Difficulty::Expert).decisionDelay() == doctest::Approx(0.05));
}

TEST_CASE("bestMove returns a legal placement on an empty board") {
    TetrisAI ai(Difficulty::Expert, 1);  // expert = no error, deterministic best
    Grid g = emptyGrid();
    Piece t(ShapeId::T);
    AiMove m = ai.bestMove(g, t);
    REQUIRE(m.valid);
    CHECK(m.rotation >= 0);
    CHECK(m.rotation < 4);
    // The chosen x must keep the piece in-bounds at that rotation.
    for (const Cell& c : t.cellsAt(m.x, 0, m.rotation)) {
        CHECK(c.x >= 0);
        CHECK(c.x < kCols);
    }
}

TEST_CASE("expert AI prefers completing a line when one is available") {
    // Fill the bottom row except a single gap in column 0; an I piece placed there
    // vertically... use a simpler setup: leave a 1-wide notch that an O or I can't
    // trivially fill. Instead, verify the AI picks a move that clears the line when
    // a piece can complete it. Fill bottom row cols 1..9, leave col 0 empty, and
    // give the AI an I piece: the vertical I in col 0 would fill 4 rows incl the gap.
    Grid g = emptyGrid();
    for (int c = 1; c < kCols; ++c) g[kRows - 1][c] = ShapeId::J;
    TetrisAI ai(Difficulty::Expert, 5);
    Piece i(ShapeId::I);
    AiMove m = ai.bestMove(g, i);
    REQUIRE(m.valid);
    // Simulate the AI's chosen move and check it cleared the line (0 complete lines
    // remain because the full row was cleared, and holes shouldn't increase).
    // We reconstruct the resulting grid via the same public evaluate signal: the
    // chosen move should score at least as well as any line-completing option.
    // Direct check: the AI should target column 0 to fill the gap.
    // (An I in col 0, some rotation, occupies col 0.)
    bool touchesCol0 = false;
    for (const Cell& c : i.cellsAt(m.x, 0, m.rotation)) if (c.x == 0) touchesCol0 = true;
    CHECK(touchesCol0);
}

TEST_CASE("deterministic: same seed + same board yields the same move") {
    Grid g = emptyGrid();
    Piece s(ShapeId::S);
    TetrisAI a(Difficulty::Normal, 999);
    TetrisAI b(Difficulty::Normal, 999);
    AiMove ma = a.bestMove(g, s);
    AiMove mb = b.bestMove(g, s);
    CHECK(ma.x == mb.x);
    CHECK(ma.rotation == mb.rotation);
}
