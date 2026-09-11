#include "app/game.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include "raylib.h"

namespace tetris {

Game::Game() {
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kWinW, kWinH, "tetris-c");
    SetTargetFPS(60);

    // Procedural SFX + music (no asset files). Safe if no audio device is present.
    audio_.init();
    music_.init();

    // Post-processing pipeline (scene render target + bloom). Needs the GL context.
    postfx_.init(kWinW, kWinH);

    // High-score file next to the executable's working dir.
    savePath_ = "highscores.dat";
    highScores_.load(savePath_);
}

void Game::openPause() {
    screen_ = Screen::Paused;
    pauseSel_ = 0;
    music_.setVolume(0.25f);  // duck music while paused
}

void Game::cycleMusic(int dir) {
    const int next = ((static_cast<int>(music_.current()) + dir) % kTrackCount + kTrackCount) % kTrackCount;
    music_.select(static_cast<Track>(next));
    audio_.play(Sfx::Rotate);  // a small confirmation blip
}

void Game::updateDanger(double dt) {
    // Target danger ramps in over the top ~40% of the well: 0 below 60% height,
    // 1 at the ceiling. Only while actively playing; eases to 0 otherwise.
    float target = 0.0f;
    if (screen_ == Screen::Playing && mc_) {
        const float frac = static_cast<float>(mc_->board().stackHeight()) / static_cast<float>(kRows);
        target = std::clamp((frac - 0.6f) / 0.4f, 0.0f, 1.0f);
    }
    // Exponential smoothing so it swells/fades rather than snapping.
    const float rate = 4.0f;
    danger_ += (target - danger_) * std::min(1.0f, static_cast<float>(dt) * rate);

    // Heartbeat: a low thump whose period shortens as danger rises.
    if (danger_ > 0.25f && screen_ == Screen::Playing) {
        heartbeatTimer_ += dt;
        const double period = 1.0 - 0.55 * danger_;  // ~1.0s → ~0.45s
        if (heartbeatTimer_ >= period) {
            heartbeatTimer_ = 0.0;
            audio_.play(Sfx::SoftDrop, 0.5f);  // reuse a low tick as a heartbeat pulse
        }
    } else {
        heartbeatTimer_ = 0.0;
    }
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
    aiPlanPiece_ = -1;  // force a fresh plan on first AI tick
    ai_.setDifficulty(diff);  // AI skill follows the chosen difficulty
}

void Game::processMenuInput() {
    if (IsKeyPressed(KEY_DOWN)) modeSel_ = (modeSel_ + 1) % 3;
    if (IsKeyPressed(KEY_UP)) modeSel_ = (modeSel_ + 2) % 3;
    if (IsKeyPressed(KEY_RIGHT)) diffSel_ = (diffSel_ + 1) % 4;
    if (IsKeyPressed(KEY_LEFT)) diffSel_ = (diffSel_ + 3) % 4;
    if (IsKeyPressed(KEY_M)) cycleMusic(1);  // cycle the background music track
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
    // Open the pause menu — Enter (primary) or P. Return immediately so the same
    // Enter press isn't also seen by the pause-menu handler this frame (it would
    // instantly pick "Resume" and the pause would flicker).
    if (screen_ == Screen::Playing &&
        (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_P))) {
        openPause();
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
    // Rotations — edge-triggered, mutually exclusive. CW (Up/X) takes precedence so
    // that pressing CW and CCW on the SAME frame applies a single rotation rather
    // than rotate(1) then rotate(-1) (a net-zero double-rotate with spurious lock-
    // delay resets / spin-flag churn).
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_X)) {
        if (board.rotate(1)) audio_.play(Sfx::Rotate);
    } else if (IsKeyPressed(KEY_Z)) {
        if (board.rotate(-1)) audio_.play(Sfx::Rotate);
    }
    // Hard drop — edge-triggered.
    if (IsKeyPressed(KEY_SPACE)) {
        board.hardDrop();
        audio_.play(Sfx::HardDrop);
    }

    if (board.gameOver()) {
        screen_ = Screen::GameOver;
        recordResult();
    }
}

void Game::processPauseInput() {
    // Pause menu: 0 Resume · 1 Restart · 2 Music (left/right cycles) · 3 Quit.
    // Enter or P also resumes directly (quick unpause).
    if (IsKeyPressed(KEY_P)) {
        screen_ = Screen::Playing;
        music_.setVolume(0.5f);
        return;
    }
    if (IsKeyPressed(KEY_DOWN)) pauseSel_ = (pauseSel_ + 1) % 4;
    if (IsKeyPressed(KEY_UP)) pauseSel_ = (pauseSel_ + 3) % 4;

    // Music row: left/right cycles the track live (audible immediately).
    if (pauseSel_ == 2) {
        if (IsKeyPressed(KEY_RIGHT)) cycleMusic(1);
        if (IsKeyPressed(KEY_LEFT)) cycleMusic(-1);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        switch (pauseSel_) {
            case 0:  // Resume
                screen_ = Screen::Playing;
                music_.setVolume(0.5f);
                break;
            case 1:  // Restart
                startSelectedMode();
                break;
            case 2:  // Music: Enter also advances the track
                cycleMusic(1);
                break;
            case 3:  // Quit to menu
                screen_ = Screen::Menu;
                music_.setVolume(0.5f);
                break;
        }
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

    // Compute the target placement ONCE per piece (bestMove re-rolls error + re-
    // searches on every call — calling it each tick made low difficulties pick a
    // fresh target per action, #4). Recompute whenever the active piece changes,
    // detected via piecesLocked() so it covers gravity/lock-delay locks too — not
    // just the hard drops this function initiates. Then step toward the fixed
    // cached target: align rotation, then column, then hard drop.
    if (board.piecesLocked() != aiPlanPiece_) {
        aiPlan_ = ai_.bestMove(board.grid(), board.current());
        aiPlanPiece_ = board.piecesLocked();
    }
    const AiMove m = aiPlan_;
    if (!m.valid) return;  // no legal placement (board is topping out) — nothing to do

    if (board.current().rotation() != m.rotation) {
        // Take one rotation step toward the target. If it succeeds, that's this
        // tick's action. If the rotation is blocked (kick fails), DON'T keep
        // retrying it every tick (#7) — fall through to column alignment / drop at
        // the current rotation and let gravity/lock-delay resolve the piece.
        if (board.rotate(1)) return;
    }
    const int dx = m.x - board.current().x();
    if (dx != 0) {
        // If the horizontal move is blocked, stop trying to reach an unreachable
        // column (#7) and just drop from here rather than stalling every tick.
        if (board.move(dx > 0 ? 1 : -1, 0)) return;
    }
    board.hardDrop();  // aligned (or blocked) → drop; piecesLocked() will advance,
                       // so the next tick recomputes the plan for the next piece.
}

void Game::updateGravity(float dt, int linesBefore) {
    if (screen_ != Screen::Playing) return;

    Board& board = mc_->board();
    if (aiEnabled_) {
        // AI drives the piece — ignore ALL manual piece input, including the held
        // horizontal (DAS/ARR) and soft-drop paths. Previously only the edge-
        // triggered rotate/hard-drop in processPlayInput were suppressed, so held
        // arrow keys still moved the piece while the demo played (#6).
        updateAi(dt);
    } else {
        handleHorizontal(dt);

        // Soft drop: while Down is held, step at a faster cadence in addition to
        // gravity. Each extra move resets the gravity timer inside step via move()'s
        // lock-delay reset; the piece still respects lock delay at the bottom.
        if (IsKeyDown(KEY_DOWN)) {
            softDropTimer_ += dt;
            while (softDropTimer_ >= kSoftDropInterval) {
                softDropTimer_ -= kSoftDropInterval;
                board.move(0, 1);  // no-op if resting; lock delay still applies via step()
            }
        } else {
            softDropTimer_ = 0.0;
        }
    }

    // Snapshot lock count + level so we can detect a lock / level-up caused by
    // this step (the core advances piecesLocked() on any lock path).
    const int locksBefore = board.piecesLocked();
    const int levelBefore = board.level();

    // Gravity + lock delay handled by the core step().
    board.step(dt);

    // A piece locked this step: play the "thock" (unless it cleared lines — the
    // clear SFX below is the louder, more informative event for that case).
    const bool locked = board.piecesLocked() > locksBefore;
    const bool clearedLines = board.linesCleared() > linesBefore;
    if (locked && !clearedLines && !board.gameOver()) audio_.play(Sfx::Lock);

    // Juice + audio: if this step cleared lines, fire shake + a particle burst and
    // the combo-escalating clear SFX (fanfare for a Tetris / T-spin).
    if (clearedLines) {
        const int lines = board.lastClearCount();
        juice_.onLineClear(lines);
        audio_.playClear(lines, board.combo(), board.lastTSpin() != Board::TSpin::None);
        // Spread a burst across the playfield near the bottom third.
        for (int c = 0; c < kCols; ++c) {
            const float px = c * kCellPx + kCellPx / 2.0f;
            const float py = (kRows - 2) * kCellPx;
            juice_.spawnBurst(px, py, kPieceColors[c % kShapeCount], 3);
        }
    }
    if (board.level() > levelBefore) audio_.play(Sfx::LevelUp);
    juice_.update(dt);

    // Advance mode timers (Ultra garbage, win checks).
    mc_->update(dt);

    if (board.gameOver()) {
        screen_ = Screen::GameOver;
        if (!resultRecorded_) audio_.play(Sfx::GameOver);  // before recordResult sets the flag
        recordResult();
    } else if (mc_->won()) {
        screen_ = Screen::Won;
        if (!resultRecorded_) audio_.play(Sfx::Tetris);    // win fanfare
        recordResult();
    }
}

void Game::run() {
    while (!WindowShouldClose()) {  // Esc or window close
        const float dt = GetFrameTime();

        music_.update();  // keep the stream fed regardless of screen

        if (screen_ == Screen::Menu) {
            processMenuInput();
        } else if (screen_ == Screen::Paused) {
            processPauseInput();  // game logic frozen; only the menu takes input
        } else {
            // Sample the line count BEFORE input so a Space-to-lock clear inside
            // processPlayInput (instant lock when the piece is already resting)
            // still triggers juice — updateGravity compares against this (#5).
            const int linesBefore = (screen_ == Screen::Playing && mc_)
                                        ? mc_->board().linesCleared()
                                        : 0;
            processPlayInput();
            updateGravity(dt, linesBefore);
        }

        // Danger state: ease `danger_` toward the current stack-height fraction and
        // rise a heartbeat as it climbs (only while actively playing).
        updateDanger(dt);

        // Draw the scene into the post-fx render target, then blit (with bloom
        // during play). The crisp pause overlay + danger vignette are drawn AFTER
        // the blit so text stays sharp and the vignette isn't blurred by bloom.
        BeginDrawing();
        ClearBackground(::Color{kColBlack.r, kColBlack.g, kColBlack.b, kColBlack.a});
        postfx_.begin();
        if (screen_ == Screen::Menu || !mc_) {
            const GameMode m = static_cast<GameMode>(modeSel_);
            drawMenu(modeSel_, diffSel_, highScores_.record(m), music_.current());
        } else {
            drawFrame(*mc_, screen_, highScores_.record(mc_->mode()), juice_, aiEnabled_);
        }
        postfx_.end(/*bloom=*/screen_ != Screen::Menu);

        // Overlays on top of the blitted scene.
        if (screen_ != Screen::Menu && mc_) drawDangerVignette(danger_);
        if (screen_ == Screen::Paused) drawPauseMenu(pauseSel_, music_.current());
        EndDrawing();
    }
    CloseWindow();
}

}  // namespace tetris
