#include "app/game.hpp"

#include <random>

#include "raylib.h"

namespace tetris {

Game::Game() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kWinW, kWinH, "tetris-c");
    SetTargetFPS(60);
}

void Game::startSelectedMode() {
    const GameMode mode = static_cast<GameMode>(modeSel_);
    const Difficulty diff = static_cast<Difficulty>(diffSel_);
    // Fresh nondeterministic seed each run.
    const std::uint64_t seed = std::random_device{}();
    mc_ = std::make_unique<ModeController>(mode, diff, seed);
    screen_ = Screen::Playing;
    fallTimer_ = 0.0;
}

void Game::processMenuInput() {
    if (IsKeyPressed(KEY_DOWN)) modeSel_ = (modeSel_ + 1) % 3;
    if (IsKeyPressed(KEY_UP)) modeSel_ = (modeSel_ + 2) % 3;
    if (IsKeyPressed(KEY_RIGHT)) diffSel_ = (diffSel_ + 1) % 4;
    if (IsKeyPressed(KEY_LEFT)) diffSel_ = (diffSel_ + 3) % 4;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) startSelectedMode();
}

void Game::processPlayInput() {
    // Return to menu from any in-game state.
    if (IsKeyPressed(KEY_M)) {
        screen_ = Screen::Menu;
        return;
    }
    // Restart the same mode.
    if (IsKeyPressed(KEY_R)) {
        startSelectedMode();
        return;
    }
    if (IsKeyPressed(KEY_P) && (screen_ == Screen::Playing || screen_ == Screen::Paused)) {
        screen_ = (screen_ == Screen::Paused) ? Screen::Playing : Screen::Paused;
        return;
    }

    if (screen_ != Screen::Playing) return;

    Board& board = mc_->board();
    if (IsKeyPressed(KEY_LEFT)) board.move(-1, 0);
    if (IsKeyPressed(KEY_RIGHT)) board.move(1, 0);
    if (IsKeyPressed(KEY_DOWN)) {
        if (board.move(0, 1)) fallTimer_ = 0.0;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_X)) board.rotate(1);
    if (IsKeyPressed(KEY_Z)) board.rotate(-1);
    if (IsKeyPressed(KEY_SPACE)) {
        board.hardDrop();
        fallTimer_ = 0.0;
    }

    if (board.gameOver()) screen_ = Screen::GameOver;
}

void Game::updateGravity(float dt) {
    if (screen_ != Screen::Playing) return;

    Board& board = mc_->board();
    fallTimer_ += dt;
    if (fallTimer_ >= board.fallSpeed()) {
        fallTimer_ = 0.0;
        if (!board.move(0, 1)) board.lock();
    }
    // Advance mode timers (Ultra garbage, win checks).
    mc_->update(dt);

    if (board.gameOver()) screen_ = Screen::GameOver;
    else if (mc_->won()) screen_ = Screen::Won;
}

void Game::run() {
    while (!WindowShouldClose()) {  // Esc or window close
        const float dt = GetFrameTime();

        if (screen_ == Screen::Menu) {
            processMenuInput();
        } else {
            processPlayInput();
            updateGravity(dt);
        }

        BeginDrawing();
        if (screen_ == Screen::Menu || !mc_) {
            drawMenu(modeSel_, diffSel_);
        } else {
            drawFrame(*mc_, screen_);
        }
        EndDrawing();
    }
    CloseWindow();
}

}  // namespace tetris
