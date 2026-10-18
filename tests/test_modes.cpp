#include "doctest.h"

#include "core/board.hpp"
#include "core/constants.hpp"
#include "core/modes.hpp"

using namespace tetris;

static Piece placed(ShapeId s, int x, int y, int rot) {
    Piece p(s);
    p.setPos(x, y);
    p.setRotation(rot);
    return p;
}

// Drive one guaranteed line clear on a ModeController's board.
static void clearOneLine(ModeController& m) {
    Board& b = m.board();
    for (int c = 0; c < kCols - 2; ++c) b.setCellForTest(c, kRows - 1, ShapeId::I);
    b.setActiveForTest(placed(ShapeId::O, 7, kRows - 2, 0));
    b.lock();
}

TEST_CASE("reset clears win/timer/garbage state and reproduces the seed") {
    ModeController m(GameMode::Sprint, Difficulty::Normal, 4242);
    // Advance state: clear some lines (toward a win), tick the timer.
    for (int i = 0; i < 10; ++i) clearOneLine(m);
    m.update(2.5);
    REQUIRE(m.won());                    // Sprint won at 10
    REQUIRE(m.elapsed() > 0.0);

    m.reset();
    CHECK_FALSE(m.won());                 // win cleared
    CHECK(m.elapsed() == doctest::Approx(0.0));  // timer cleared
    CHECK(m.board().linesCleared() == 0); // board reset too

    // The garbage-hole sequence is reproducible after reset (same seed → same holes).
    ModeController a(GameMode::Ultra, Difficulty::Expert, 4242);
    a.update(2.5);  // one injection (expert = 2s)
    a.reset();
    a.update(2.5);  // first injection again — same hole as a fresh run with this seed
    ModeController fresh(GameMode::Ultra, Difficulty::Expert, 4242);
    fresh.update(2.5);
    int holeA = -1, holeF = -1;
    for (int c = 0; c < kCols; ++c) {
        if (!a.board().grid()[kRows - 1][c].has_value()) holeA = c;
        if (!fresh.board().grid()[kRows - 1][c].has_value()) holeF = c;
    }
    CHECK(holeA == holeF);
}

TEST_CASE("update is a no-op after a win (won state is sticky)") {
    ModeController m(GameMode::Sprint, Difficulty::Normal, 9);
    for (int i = 0; i < 10; ++i) clearOneLine(m);
    m.update(0.016);
    REQUIRE(m.won());
    const double tAtWin = m.elapsed();
    m.update(5.0);  // should not advance the timer once won
    CHECK(m.elapsed() == doctest::Approx(tAtWin));
}

TEST_CASE("mode win targets") {
    ModeController marathon(GameMode::Marathon, Difficulty::Normal, 1);
    ModeController sprint(GameMode::Sprint, Difficulty::Normal, 1);
    ModeController ultra(GameMode::Ultra, Difficulty::Normal, 1);
    CHECK(marathon.winLines() == 40);
    CHECK(sprint.winLines() == 10);
    CHECK(ultra.winLines() == 0);
}

TEST_CASE("garbage interval by difficulty") {
    CHECK(ModeController(GameMode::Ultra, Difficulty::Easy, 1).garbageInterval() == doctest::Approx(8.0));
    CHECK(ModeController(GameMode::Ultra, Difficulty::Normal, 1).garbageInterval() == doctest::Approx(5.0));
    CHECK(ModeController(GameMode::Ultra, Difficulty::Hard, 1).garbageInterval() == doctest::Approx(3.0));
    CHECK(ModeController(GameMode::Ultra, Difficulty::Expert, 1).garbageInterval() == doctest::Approx(2.0));
}

TEST_CASE("Sprint wins at 10 lines") {
    ModeController m(GameMode::Sprint, Difficulty::Normal, 7);
    for (int i = 0; i < 9; ++i) {
        clearOneLine(m);
        m.update(0.016);
        CHECK_FALSE(m.won());
    }
    clearOneLine(m);
    m.update(0.016);
    CHECK(m.won());
    CHECK(m.board().linesCleared() == 10);
}

TEST_CASE("Marathon does not win at 10, wins at 40") {
    ModeController m(GameMode::Marathon, Difficulty::Normal, 7);
    for (int i = 0; i < 39; ++i) {
        clearOneLine(m);
        m.update(0.016);
    }
    CHECK_FALSE(m.won());
    clearOneLine(m);
    m.update(0.016);
    CHECK(m.won());
}

TEST_CASE("Ultra injects garbage on the difficulty cadence") {
    ModeController m(GameMode::Ultra, Difficulty::Normal, 42);  // 5s interval
    // No garbage before the interval elapses.
    m.update(4.0);
    int g0 = 0;
    for (const auto& row : m.board().grid())
        for (const auto& c : row)
            if (c.has_value() && c.value() == ShapeId::Garbage) ++g0;
    CHECK(g0 == 0);
    // Cross the 5s boundary → exactly one garbage row (kCols-1 cells).
    m.update(1.5);  // total 5.5s
    int g1 = 0;
    for (const auto& row : m.board().grid())
        for (const auto& c : row)
            if (c.has_value() && c.value() == ShapeId::Garbage) ++g1;
    CHECK(g1 == kCols - 1);
}

TEST_CASE("Ultra never sets a win (no line target)") {
    ModeController m(GameMode::Ultra, Difficulty::Normal, 5);
    for (int i = 0; i < 15; ++i) {
        // clearing lines shouldn't trigger a win in Ultra
        Board& b = m.board();
        for (int c = 0; c < kCols - 2; ++c) b.setCellForTest(c, kRows - 1, ShapeId::I);
        b.setActiveForTest(placed(ShapeId::O, 7, kRows - 2, 0));
        b.lock();
        m.update(0.016);
    }
    CHECK_FALSE(m.won());
}

TEST_CASE("update is a no-op after game over") {
    ModeController m(GameMode::Ultra, Difficulty::Expert, 9);
    // Force game over by topping out via garbage against a filled top row.
    m.board().setCellForTest(3, 0, ShapeId::L);
    m.update(10.0);  // would inject several rows, but top row is filled → tops out
    CHECK(m.board().gameOver());
    const long before = m.board().score();
    m.update(10.0);  // no-op now
    CHECK(m.board().score() == before);
}

TEST_CASE("deterministic garbage holes for a given seed") {
    ModeController a(GameMode::Ultra, Difficulty::Expert, 777);
    ModeController b(GameMode::Ultra, Difficulty::Expert, 777);
    a.update(2.5);  // expert = 2s interval → one injection
    b.update(2.5);
    // Same hole column in the bottom row.
    int holeA = -1, holeB = -1;
    for (int c = 0; c < kCols; ++c) {
        if (!a.board().grid()[kRows - 1][c].has_value()) holeA = c;
        if (!b.board().grid()[kRows - 1][c].has_value()) holeB = c;
    }
    CHECK(holeA == holeB);
    CHECK(holeA >= 0);
}
