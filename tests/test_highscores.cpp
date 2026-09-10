#include "doctest.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

#include "core/highscores.hpp"
#include "core/modes.hpp"

using namespace tetris;

// A scratch file path in the system temp dir, unique-ish per run.
static std::string tmpPath() {
    return std::string("/tmp/tetris_hs_test_") + std::to_string(::getpid()) + ".dat";
}

TEST_CASE("fresh HighScores is all zeros") {
    HighScores hs;
    for (int i = 0; i < 3; ++i) {
        const auto& r = hs.record(static_cast<GameMode>(i));
        CHECK(r.bestScore == 0);
        CHECK_FALSE(r.hasTime);
    }
}

TEST_CASE("submit sets a new best score and reports improvement") {
    HighScores hs;
    CHECK(hs.submit(GameMode::Marathon, 5000, 120.0, false));      // new best
    CHECK(hs.record(GameMode::Marathon).bestScore == 5000);
    CHECK_FALSE(hs.submit(GameMode::Marathon, 3000, 90.0, false)); // worse → no
    CHECK(hs.record(GameMode::Marathon).bestScore == 5000);
    CHECK(hs.submit(GameMode::Marathon, 8000, 60.0, false));       // better → yes
    CHECK(hs.record(GameMode::Marathon).bestScore == 8000);
}

TEST_CASE("sprint tracks fastest CLEAR time (lower is better, completed only)") {
    HighScores hs;
    // Incomplete run: no time recorded even if fast.
    hs.submit(GameMode::Sprint, 1000, 25.0, /*cleared=*/false);
    CHECK_FALSE(hs.record(GameMode::Sprint).hasTime);
    // First completed run sets the time.
    CHECK(hs.submit(GameMode::Sprint, 1200, 40.0, true));
    CHECK(hs.record(GameMode::Sprint).hasTime);
    CHECK(hs.record(GameMode::Sprint).bestTime == doctest::Approx(40.0));
    // A faster clear improves it.
    CHECK(hs.submit(GameMode::Sprint, 900, 30.0, true));
    CHECK(hs.record(GameMode::Sprint).bestTime == doctest::Approx(30.0));
    // A slower clear does not improve the best time (score 100 also won't beat 1200).
    hs.submit(GameMode::Sprint, 100, 55.0, true);
    CHECK(hs.record(GameMode::Sprint).bestTime == doctest::Approx(30.0));
}

TEST_CASE("save then load round-trips all fields (same process)") {
    const std::string path = tmpPath();
    HighScores a;
    a.submit(GameMode::Marathon, 12345, 200.0, false);
    a.submit(GameMode::Sprint, 999, 33.5, true);
    a.submit(GameMode::Ultra, 54321, 120.0, false);
    REQUIRE(a.save(path));

    HighScores b;
    b.load(path);
    CHECK(b.record(GameMode::Marathon).bestScore == 12345);
    CHECK(b.record(GameMode::Sprint).bestScore == 999);
    CHECK(b.record(GameMode::Sprint).hasTime);
    CHECK(b.record(GameMode::Sprint).bestTime == doctest::Approx(33.5));
    CHECK(b.record(GameMode::Ultra).bestScore == 54321);

    std::remove(path.c_str());
}

TEST_CASE("loading a missing file yields zeros, no crash") {
    HighScores hs;
    hs.load("/tmp/definitely_does_not_exist_tetris_hs.dat");
    CHECK(hs.record(GameMode::Marathon).bestScore == 0);
}

TEST_CASE("loading a garbage file does not crash and ignores bad lines") {
    const std::string path = tmpPath() + ".garbage";
    { std::ofstream out(path); out << "not a valid line\n@@@@\nmarathon 777 0 0\n"; }
    HighScores hs;
    hs.load(path);
    CHECK(hs.record(GameMode::Marathon).bestScore == 777);  // the one good line loaded
    std::remove(path.c_str());
}
