#include "app/game.hpp"

#include "raylib.h"

namespace tetris {

Game::Game() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kWinW, kWinH, "tetris-c");
    SetTargetFPS(60);
}

void Game::processInput() {
    // Global keys (work in any screen state).
    if (IsKeyPressed(KEY_R)) {
        board_.reset();
        screen_ = Screen::Playing;
        fallTimer_ = 0.0;
        return;
    }
    if (IsKeyPressed(KEY_P) && screen_ != Screen::GameOver) {
        screen_ = (screen_ == Screen::Paused) ? Screen::Playing : Screen::Paused;
        return;
    }

    if (screen_ != Screen::Playing) return;

    // Movement / rotation / drop — discrete key presses (Tier 1).
    if (IsKeyPressed(KEY_LEFT)) board_.move(-1, 0);
    if (IsKeyPressed(KEY_RIGHT)) board_.move(1, 0);
    if (IsKeyPressed(KEY_DOWN)) {
        // Soft drop: one row; reset gravity timer so it feels responsive.
        if (board_.move(0, 1)) fallTimer_ = 0.0;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_X)) board_.rotate(1);
    if (IsKeyPressed(KEY_Z)) board_.rotate(-1);
    if (IsKeyPressed(KEY_SPACE)) {
        board_.hardDrop();
        fallTimer_ = 0.0;
    }

    if (board_.gameOver()) screen_ = Screen::GameOver;
}

void Game::updateGravity(float dt) {
    if (screen_ != Screen::Playing) return;
    fallTimer_ += dt;
    if (fallTimer_ >= board_.fallSpeed()) {
        fallTimer_ = 0.0;
        if (!board_.move(0, 1)) {
            board_.lock();
            if (board_.gameOver()) screen_ = Screen::GameOver;
        }
    }
}

void Game::run() {
    while (!WindowShouldClose()) {  // Esc or window close
        const float dt = GetFrameTime();
        processInput();
        updateGravity(dt);

        BeginDrawing();
        drawFrame(board_, screen_);
        EndDrawing();
    }
    CloseWindow();
}

}  // namespace tetris
