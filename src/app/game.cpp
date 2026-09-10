#include "app/game.hpp"

#include <random>

#include "raylib.h"

namespace tetris {

Game::Game() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kWinW, kWinH, "tetris-c");
    SetTargetFPS(60);

    // High-score file next to the executable's working dir.
    savePath_ = "highscores.dat";
    highScores_.load(savePath_);
}

void Game::recordResult() {
    if (resultRecorded_ || !mc_) return;
    resultRecorded_ = true;
    const bool cleared = mc_->won();
    highScores_.submit(mc_->mode(), mc_->board().score(), mc_->elapsed(), cleared);
    highScores_.save(savePath_);  // persist immediately
}

void Game::startSelectedMode() {
    const GameMode mode = static_cast<GameMode>(modeSel_);
    const Difficulty diff = static_cast<Difficulty>(diffSel_);
    // Fresh nondeterministic seed each run.
    const std::uint64_t seed = std::random_device{}();
    mc_ = std::make_unique<ModeController>(mode, diff, seed);
    screen_ = Screen::Playing;
    dasDir_ = 0;
    dasTimer_ = 0.0;
    arrTimer_ = 0.0;
    softDropTimer_ = 0.0;
    resultRecorded_ = false;
    aiEnabled_ = false;
    aiTimer_ = 0.0;
    ai_.setDifficulty(diff);  // AI skill follows the chosen difficulty
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

    // Toggle AI autoplay / demo.
    if (IsKeyPressed(KEY_A)) {
        aiEnabled_ = !aiEnabled_;
        aiTimer_ = 0.0;
    }
    if (aiEnabled_) return;  // AI drives; ignore manual piece input while on

    Board& board = mc_->board();
    // Rotations — edge-triggered.
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_X)) board.rotate(1);
    if (IsKeyPressed(KEY_Z)) board.rotate(-1);
    // Hard drop — edge-triggered.
    if (IsKeyPressed(KEY_SPACE)) board.hardDrop();

    if (board.gameOver()) {
        screen_ = Screen::GameOver;
        recordResult();
    }
}

void Game::handleHorizontal(double dt) {
    Board& board = mc_->board();
    const bool left = IsKeyDown(KEY_LEFT);
    const bool right = IsKeyDown(KEY_RIGHT);

    // Track the most-recently-pressed horizontal key for SOCD resolution
    // (last-pressed wins), matching python-tetris resolve_direction.
    if (IsKeyPressed(KEY_LEFT)) socdLast_ = -1;
    if (IsKeyPressed(KEY_RIGHT)) socdLast_ = 1;

    // Resolve the active direction.
    int dir = 0;
    if (left && !right) dir = -1;
    else if (right && !left) dir = 1;
    else if (left && right) {
        // Both held → last-pressed wins; default left if unknown.
        dir = (socdLast_ != 0) ? socdLast_ : -1;
    }

    if (dir == 0) {
        dasDir_ = 0;
        dasTimer_ = 0.0;
        arrTimer_ = 0.0;
        return;
    }

    if (dir != dasDir_) {
        // New direction (or first press): move once immediately, then start DAS.
        dasDir_ = dir;
        dasTimer_ = 0.0;
        arrTimer_ = 0.0;
        board.move(dir, 0);
        return;
    }

    // Same direction held: after DAS, repeat every ARR.
    dasTimer_ += dt;
    if (dasTimer_ >= kDasSeconds) {
        arrTimer_ += dt;
        while (arrTimer_ >= kArrSeconds) {
            arrTimer_ -= kArrSeconds;
            board.move(dir, 0);
        }
    }
}

void Game::updateAi(double dt) {
    Board& board = mc_->board();
    if (board.animating()) return;  // wait out the hard-drop animation

    aiTimer_ += dt;
    if (aiTimer_ < ai_.decisionDelay()) return;
    aiTimer_ = 0.0;

    // Compute the target placement for the current piece and take ONE action
    // toward it per decision tick (so the demo is watchable): align rotation,
    // then column, then hard drop.
    const AiMove m = ai_.bestMove(board.grid(), board.current());
    if (!m.valid) return;

    if (board.current().rotation() != m.rotation) {
        board.rotate(1);
        return;
    }
    const int dx = m.x - board.current().x();
    if (dx != 0) {
        board.move(dx > 0 ? 1 : -1, 0);
        return;
    }
    board.hardDrop();  // aligned → drop
}

void Game::updateGravity(float dt) {
    if (screen_ != Screen::Playing) return;

    Board& board = mc_->board();
    const int linesBefore = board.linesCleared();
    if (aiEnabled_) updateAi(dt);
    handleHorizontal(dt);

    // Soft drop: while Down is held, step at a faster cadence (kSoftDropRate rows/s)
    // in addition to gravity. Each extra move resets the gravity timer inside step
    // via move()'s lock-delay reset; the piece still respects lock delay at the bottom.
    if (IsKeyDown(KEY_DOWN)) {
        softDropTimer_ += dt;
        while (softDropTimer_ >= kSoftDropInterval) {
            softDropTimer_ -= kSoftDropInterval;
            board.move(0, 1);  // no-op if resting; lock delay still applies via step()
        }
    } else {
        softDropTimer_ = 0.0;
    }

    // Gravity + lock delay handled by the core step().
    board.step(dt);

    // Juice: if this step cleared lines, fire shake + a particle burst.
    if (board.linesCleared() > linesBefore) {
        const int lines = board.lastClearCount();
        juice_.onLineClear(lines);
        // Spread a burst across the playfield near the bottom third.
        for (int c = 0; c < kCols; ++c) {
            const float px = c * kCellPx + kCellPx / 2.0f;
            const float py = (kRows - 2) * kCellPx;
            juice_.spawnBurst(px, py, kPieceColors[c % kShapeCount], 3);
        }
    }
    juice_.update(dt);

    // Advance mode timers (Ultra garbage, win checks).
    mc_->update(dt);

    if (board.gameOver()) {
        screen_ = Screen::GameOver;
        recordResult();
    } else if (mc_->won()) {
        screen_ = Screen::Won;
        recordResult();
    }
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
            const GameMode m = static_cast<GameMode>(modeSel_);
            drawMenu(modeSel_, diffSel_, highScores_.record(m));
        } else {
            drawFrame(*mc_, screen_, highScores_.record(mc_->mode()), juice_, aiEnabled_);
        }
        EndDrawing();
    }
    CloseWindow();
}

}  // namespace tetris
