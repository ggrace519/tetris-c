// Game: owns the raylib window, the 60 FPS loop, screen state, the gravity
// accumulator, mode selection, and input→core mapping. Ports app.py + a menu.
#ifndef TETRIS_APP_GAME_HPP
#define TETRIS_APP_GAME_HPP

#include <memory>

#include <string>

#include "app/render.hpp"
#include "core/highscores.hpp"
#include "core/modes.hpp"

namespace tetris {

class Game {
public:
    Game();
    void run();  // blocks until the window closes

private:
    void processMenuInput();
    void processPlayInput();
    void updateGravity(float dt);
    void startSelectedMode();
    void recordResult();  // submit current run to high scores + save (once per end)

    Screen screen_ = Screen::Menu;
    int modeSel_ = 0;  // 0..2  Marathon/Sprint/Ultra
    int diffSel_ = 1;  // 0..3  Easy/Normal/Hard/Expert

    std::unique_ptr<ModeController> mc_;  // created when a mode starts
    HighScores highScores_;
    std::string savePath_;
    bool resultRecorded_ = false;  // guards recordResult() to once per game end

    // DAS/ARR horizontal auto-shift state.
    int dasDir_ = 0;         // -1 left, +1 right, 0 none (last resolved direction)
    int socdLast_ = 0;       // -1/+1: most-recently-pressed horizontal key (SOCD)
    double dasTimer_ = 0.0;  // time the current direction has been held
    double arrTimer_ = 0.0;  // accumulator for repeat firing
    double softDropTimer_ = 0.0;  // soft-drop cadence accumulator
    void handleHorizontal(double dt);  // applies DAS/ARR + SOCD moves
};

// Soft drop: ~20 rows/second while Down is held (a common feel value).
inline constexpr double kSoftDropInterval = 1.0 / 20.0;

}  // namespace tetris

#endif  // TETRIS_APP_GAME_HPP
