// Game: owns the raylib window, the 60 FPS loop, screen state, the gravity
// accumulator, mode selection, and input→core mapping. Ports app.py + a menu.
#ifndef TETRIS_APP_GAME_HPP
#define TETRIS_APP_GAME_HPP

#include <memory>

#include "app/render.hpp"
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

    Screen screen_ = Screen::Menu;
    int modeSel_ = 0;  // 0..2  Marathon/Sprint/Ultra
    int diffSel_ = 1;  // 0..3  Easy/Normal/Hard/Expert

    std::unique_ptr<ModeController> mc_;  // created when a mode starts
    double fallTimer_ = 0.0;
};

}  // namespace tetris

#endif  // TETRIS_APP_GAME_HPP
