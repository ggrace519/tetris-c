// Game: owns the raylib window, the 60 FPS loop, screen state, the gravity
// accumulator, and input→core mapping. Ports python-tetris/tetris_game/app.py.
#ifndef TETRIS_APP_GAME_HPP
#define TETRIS_APP_GAME_HPP

#include "app/render.hpp"
#include "core/board.hpp"

namespace tetris {

class Game {
public:
    Game();
    void run();  // blocks until the window closes

private:
    void processInput();
    void updateGravity(float dt);

    Board board_;
    Screen screen_ = Screen::Playing;
    double fallTimer_ = 0.0;
};

}  // namespace tetris

#endif  // TETRIS_APP_GAME_HPP
