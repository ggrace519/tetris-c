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

// Count filled cells in the grid.
static int filledCells(const Board& b) {
    int n = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++n;
    return n;
}

TEST_CASE("step applies gravity when the piece is not resting") {
    Board b(1);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    const int y0 = b.current().y();
    // One full fallSpeed worth of dt → drops one row.
    const bool locked = b.step(b.fallSpeed());
    CHECK_FALSE(locked);
    CHECK(b.current().y() == y0 + 1);
    CHECK_FALSE(b.landed());
}

TEST_CASE("piece resting on the floor enters lock delay, not instant lock") {
    Board b(2);
    // Drop an O to the floor first.
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));  // bottom cells on last row
    // A tiny step: piece is resting but lock delay has not elapsed.
    const bool locked = b.step(0.05);
    CHECK_FALSE(locked);
    CHECK(b.landed());
    CHECK(filledCells(b) == 0);  // not locked yet
}

TEST_CASE("lock delay expires -> piece locks") {
    Board b(3);
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));
    // Advance past the full lock delay.
    const bool locked = b.step(kLockDelay + 0.001);
    CHECK(locked);
    CHECK(filledCells(b) == 4);
}

TEST_CASE("move reset: moving while landed refreshes the lock timer") {
    Board b(4);
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));
    // Land it and run most of the lock delay.
    b.step(kLockDelay - 0.05);
    CHECK(b.landed());
    CHECK(filledCells(b) == 0);
    // Move sideways (still resting) — resets the timer.
    CHECK(b.move(1, 0));
    // Another near-full delay should NOT lock yet (timer was reset).
    b.step(kLockDelay - 0.05);
    CHECK(filledCells(b) == 0);
    // Now let it fully expire.
    b.step(0.1);
    CHECK(filledCells(b) == 4);
}

TEST_CASE("rotate reset: rotating while landed refreshes the lock timer") {
    Board b(5);
    b.setActiveForTest(placed(ShapeId::T, 4, kRows - 2, 0));  // T resting near floor
    // Land it.
    b.step(kLockDelay - 0.05);
    const int before = filledCells(b);
    CHECK(before == 0);
    // Rotate (open space above) resets the timer.
    CHECK(b.rotate(1));
    b.step(kLockDelay - 0.05);
    CHECK(filledCells(b) == 0);  // still not locked
}

TEST_CASE("hard drop bypasses the 0.5s lock delay (locks after the drop animation)") {
    Board b(6);
    b.setActiveForTest(placed(ShapeId::O, 3, 0, 0));
    b.hardDrop();
    // hardDrop starts a stretch animation, not a 0.5s lock-delay wait.
    CHECK(b.animating());
    CHECK(filledCells(b) == 0);  // not locked mid-animation
    // The animation is far shorter than the lock delay: it completes (and locks)
    // well before 0.5s.
    CHECK(kDropAnimDuration < kLockDelay);
    const bool locked = b.step(kDropAnimDuration + 0.001);
    CHECK(locked);
    CHECK(filledCells(b) == 4);
}

TEST_CASE("hard drop onto the floor with zero distance locks immediately (no anim)") {
    Board b(66);
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));  // already resting
    b.hardDrop();
    CHECK_FALSE(b.animating());       // distance 0 → no animation
    CHECK(filledCells(b) == 4);       // locked right away
}

TEST_CASE("soft drop while resting still locks (move fails, timer keeps running)") {
    // Simulates the app soft-drop loop: move(0,1) each frame while Down is held.
    // Once the piece rests, move(0,1) fails (no lock-delay reset), so step()'s
    // timer must still elapse and lock the piece — not stall forever.
    Board b(8);
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));  // already on the floor
    bool locked = false;
    for (int frame = 0; frame < 60 && !locked; ++frame) {  // ~1s at 60fps
        b.move(0, 1);            // soft-drop attempt (fails while resting → no reset)
        locked = b.step(1.0 / 60.0);
    }
    CHECK(locked);
    int filled = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++filled;
    CHECK(filled == 4);
}

TEST_CASE("lock-delay reset is capped: alternating taps cannot stall forever (#3)") {
    // Without a reset cap, alternating left/right moves on a flat floor would
    // refresh the lock timer every frame and the piece would never lock.
    //
    // This must step with a SMALL per-frame dt so the reset COUNTER actually
    // accumulates toward kMaxLockResets. A previous version of this test stepped
    // kLockDelay (a full lock-delay) each frame, so the piece locked on iteration 1
    // and the cap was never exercised — the test passed even with the cap removed.
    const double frame = 0.016;  // ~60 FPS
    Board b(8);
    b.setActiveForTest(placed(ShapeId::O, 3, kRows - 2, 0));  // resting on the floor
    b.step(frame);  // land it
    REQUIRE(b.landed());

    // Each successful move resets the lock timer. While under the cap, the piece
    // must NOT lock — even though many frames pass — because every wiggle refreshes
    // the timer. The number of resets before the cap forces a lock is kMaxLockResets.
    bool locked = false;
    int iters = 0;
    for (; iters < 500 && !locked; ++iters) {
        REQUIRE(b.move((iters % 2 == 0) ? -1 : 1, 0));  // wiggle always succeeds (flat floor)
        locked = b.step(frame);
    }
    CHECK(locked);
    // With the cap at kMaxLockResets, locking cannot happen on the first frame; it
    // must take strictly more than one wiggle (proves the timer was being reset and
    // the cap — not a single lock-delay — is what finally forces the lock).
    CHECK(iters > 1);
    CHECK(iters >= kMaxLockResets);  // the cap gated the lock, not a lone timeout

    int filled = 0;
    for (const auto& row : b.grid())
        for (const auto& c : row)
            if (c.has_value()) ++filled;
    CHECK(filled == 4);
}

TEST_CASE("moving off a ledge un-lands the piece (gravity resumes)") {
    Board b(7);
    // Build a one-column ledge so the piece rests, then can move off it.
    // Column 0 filled at the bottom row; O resting on it at x=0.
    b.setCellForTest(0, kRows - 1, ShapeId::I);
    b.setCellForTest(1, kRows - 1, ShapeId::I);
    b.setActiveForTest(placed(ShapeId::O, 0, kRows - 3, 0));  // cells cols1,2 rows(kRows-3..-2)
    // Rest on the ledge.
    b.step(0.05);
    CHECK(b.landed());
    // Move right to where column 2 is open below → should un-land.
    CHECK(b.move(1, 0));
    CHECK_FALSE(b.landed());
}
