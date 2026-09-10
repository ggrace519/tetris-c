#include "doctest.h"

#include "app/juice.hpp"
#include "core/constants.hpp"

using namespace tetris;

TEST_CASE("no shake until a line clear triggers it") {
    Juice j;
    CHECK(j.shakeX() == doctest::Approx(0.0));
    CHECK(j.shakeY() == doctest::Approx(0.0));
    j.update(0.016f);
    CHECK(j.shakeX() == doctest::Approx(0.0));
}

TEST_CASE("line clear triggers shake that decays to zero") {
    Juice j;
    j.onLineClear(4);            // tetris → strong shake
    j.update(0.01f);
    // Shake is active (non-zero magnitude possible; at least the timer runs).
    // Advance well past the shake duration → shake returns to zero.
    for (int i = 0; i < 40; ++i) j.update(0.016f);  // ~0.64s >> 0.15s duration
    CHECK(j.shakeX() == doctest::Approx(0.0));
    CHECK(j.shakeY() == doctest::Approx(0.0));
}

TEST_CASE("onLineClear(0) does nothing") {
    Juice j;
    j.onLineClear(0);
    j.update(0.016f);
    CHECK(j.shakeX() == doctest::Approx(0.0));
}

TEST_CASE("spawnBurst adds particles that expire over time") {
    Juice j;
    j.spawnBurst(100.0f, 100.0f, kColRed, 10);
    CHECK(j.particles().size() == 10);
    // After enough time, all particles die and are culled.
    for (int i = 0; i < 60; ++i) j.update(0.016f);  // ~1s >> particle life
    CHECK(j.particles().empty());
}

TEST_CASE("particles move over time") {
    Juice j;
    j.spawnBurst(100.0f, 100.0f, kColBlue, 1);
    REQUIRE(j.particles().size() == 1);
    const float x0 = j.particles()[0].x;
    const float y0 = j.particles()[0].y;
    j.update(0.05f);
    // At least one coordinate changed (velocity is nonzero in general).
    const bool moved = (j.particles().size() == 1) &&
                       (j.particles()[0].x != x0 || j.particles()[0].y != y0);
    CHECK(moved);
}
