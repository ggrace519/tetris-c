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

static int filled(const Board& b) {
    int n = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++n;
    return n;
}

TEST_CASE("hardDrop starts an animation with correct start/landing and does not lock") {
    Board b(1);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    const int startY = b.current().y();
    b.hardDrop();
    CHECK(b.animating());
    CHECK(b.animStartY() == startY);
    CHECK(b.animLandingY() > startY);      // it fell some distance
    CHECK(b.animProgress() == doctest::Approx(0.0));
    CHECK(filled(b) == 0);                 // not locked yet
}

TEST_CASE("animation progress advances with dt and locks at completion") {
    Board b(2);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    b.hardDrop();
    // Halfway through the animation.
    b.step(kDropAnimDuration / 2.0);
    CHECK(b.animating());
    CHECK(b.animProgress() == doctest::Approx(0.5).epsilon(0.05));
    CHECK(filled(b) == 0);
    // Finish it.
    const bool locked = b.step(kDropAnimDuration);
    CHECK(locked);
    CHECK_FALSE(b.animating());
    CHECK(filled(b) == 4);
}

TEST_CASE("move and rotate are no-ops during the drop animation") {
    Board b(3);
    b.setActiveForTest(placed(ShapeId::T, 4, 0, 0));
    b.hardDrop();
    REQUIRE(b.animating());
    CHECK_FALSE(b.move(-1, 0));
    CHECK_FALSE(b.move(1, 0));
    CHECK_FALSE(b.rotate(1));
    // A second hardDrop is also ignored while animating.
    b.hardDrop();
    CHECK(b.animating());
}

TEST_CASE("hard-drop score is credited at drop time, before the lock") {
    Board b(4);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    const long before = b.score();
    b.hardDrop();
    // Score already reflects the drop distance while the animation is still running.
    CHECK(b.score() > before);
    const long afterDrop = b.score();
    b.step(kDropAnimDuration + 0.001);  // completes + locks (O lands in open cols, no clear)
    CHECK(b.score() == afterDrop);      // lock added nothing extra (no line clear)
}

TEST_CASE("REGRESSION: garbage injected mid-animation does not corrupt the drop") {
    // If the floor rises (Ultra garbage) while a hard drop is animating, the piece
    // must re-find its landing row at completion, not overwrite the new garbage.
    Board b(1);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    b.hardDrop();
    REQUIRE(b.animating());
    b.injectGarbage(0);  // 9 garbage cells at the bottom, floor rises one row
    while (b.animating()) b.step(0.1);
    // 9 garbage + 4 O cells, and the O rests ON TOP of the garbage (no overwrite).
    CHECK(filled(b) == 13);
}

TEST_CASE("landing row snaps exactly to the computed landingY") {
    Board b(5);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    b.hardDrop();
    const int landing = b.animLandingY();
    b.step(kDropAnimDuration + 0.001);
    // After locking, the O's cells are at rows landing+? — verify the bottom-most
    // filled row corresponds to the landing (O occupies rows y..y+1).
    int lowestFilledRow = -1;
    for (int r = kRows - 1; r >= 0; --r) {
        bool any = false;
        for (int c = 0; c < kCols; ++c) if (b.grid()[r][c].has_value()) any = true;
        if (any) { lowestFilledRow = r; break; }
    }
    CHECK(lowestFilledRow == landing + 1);  // O's bottom cells are at y+1
}
