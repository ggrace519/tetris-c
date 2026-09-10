#include "core/modes.hpp"

namespace tetris {

namespace {
// Win-line targets (modes.py _win_condition). Ultra has none.
constexpr int kMarathonLines = 40;
constexpr int kSprintLines = 10;
}  // namespace

ModeController::ModeController(GameMode mode, Difficulty diff, std::uint64_t seed)
    : mode_(mode), diff_(diff), board_(seed), seed_(seed) {
    holeState_ = static_cast<std::uint32_t>(seed ^ 0x9E3779B9u);
}

void ModeController::reset() {
    board_.reset();
    timer_ = 0.0;
    garbageTimer_ = 0.0;
    won_ = false;
    holeState_ = static_cast<std::uint32_t>(seed_ ^ 0x9E3779B9u);
}

int ModeController::winLines() const {
    switch (mode_) {
        case GameMode::Marathon: return kMarathonLines;
        case GameMode::Sprint:   return kSprintLines;
        case GameMode::Ultra:    return 0;
    }
    return 0;
}

double ModeController::garbageInterval() const {
    // modes.py _garbage_intervals (seconds).
    switch (diff_) {
        case Difficulty::Easy:   return 8.0;
        case Difficulty::Normal: return 5.0;
        case Difficulty::Hard:   return 3.0;
        case Difficulty::Expert: return 2.0;
    }
    return 5.0;
}

int ModeController::nextHoleCol() {
    // xorshift32 → column in [0, kCols). Deterministic per seed.
    holeState_ ^= holeState_ << 13;
    holeState_ ^= holeState_ >> 17;
    holeState_ ^= holeState_ << 5;
    return static_cast<int>(holeState_ % static_cast<std::uint32_t>(kCols));
}

void ModeController::update(double dt) {
    if (board_.gameOver() || won_) return;
    timer_ += dt;

    if (mode_ == GameMode::Ultra) {
        garbageTimer_ += dt;
        const double interval = garbageInterval();
        // Inject one row per elapsed interval (handles large dt without drift).
        while (garbageTimer_ >= interval && !board_.gameOver()) {
            garbageTimer_ -= interval;
            board_.injectGarbage(nextHoleCol());
        }
    }

    checkWin();
}

void ModeController::checkWin() {
    const int target = winLines();
    if (target > 0 && board_.linesCleared() >= target) {
        won_ = true;
    }
}

}  // namespace tetris
