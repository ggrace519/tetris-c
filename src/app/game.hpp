// Game: owns the raylib window, the 60 FPS loop, screen state, the gravity
// accumulator, mode selection, and input→core mapping. Ports app.py + a menu.
#ifndef TETRIS_APP_GAME_HPP
#define TETRIS_APP_GAME_HPP

#include <memory>

#include <string>

#include "app/audio.hpp"
#include "app/juice.hpp"
#include "app/music.hpp"
#include "app/postfx.hpp"
#include "app/render.hpp"
#include "core/ai.hpp"
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
    void processPauseInput();  // pause-menu navigation
    void updateGravity(float dt, int linesBefore);
    void startSelectedMode();
    void recordResult();  // submit current run to high scores + save (once per end)
    void openPause();         // enter the pause menu
    void cycleMusic(int dir); // change the selected music track (Options / menu)
    void updateDanger(double dt);  // ease the danger level + heartbeat

    Screen screen_ = Screen::Menu;
    int modeSel_ = 0;  // 0..2  Marathon/Sprint/Ultra
    int diffSel_ = 1;  // 0..3  Easy/Normal/Hard/Expert

    std::unique_ptr<ModeController> mc_;  // created when a mode starts
    HighScores highScores_;
    std::string savePath_;
    bool resultRecorded_ = false;  // guards recordResult() to once per game end
    Juice juice_;
    Audio audio_;
    PostFx postfx_;
    Jukebox music_;

    // Pause menu: Enter (or P) opens it during play; arrows navigate, Enter picks.
    int pauseSel_ = 0;  // 0 Resume, 1 Restart, 2 Music, 3 Quit to menu

    // Danger state: smoothed [0,1] driven by Board::stackHeight(), for the red
    // vignette + heartbeat. Kept on Game so it eases across frames.
    float danger_ = 0.0f;
    double heartbeatTimer_ = 0.0;

    // DAS/ARR horizontal auto-shift state.
    int dasDir_ = 0;         // -1 left, +1 right, 0 none (last resolved direction)
    int socdLast_ = 0;       // -1/+1: most-recently-pressed horizontal key (SOCD)
    double dasTimer_ = 0.0;  // time the current direction has been held
    double arrTimer_ = 0.0;  // accumulator for repeat firing
    double softDropTimer_ = 0.0;  // soft-drop cadence accumulator
    void handleHorizontal(double dt);  // applies DAS/ARR + SOCD moves

    // AI autoplay / demo (toggle with A during play).
    bool aiEnabled_ = false;
    TetrisAI ai_;
    double aiTimer_ = 0.0;
    void updateAi(double dt);
    // Cached target placement for the CURRENT piece. bestMove() re-rolls its error
    // and re-searches on every call, so calling it each tick made Easy/Normal
    // incoherent (a fresh column/rotation choice per action, #4). Compute it ONCE
    // per piece and step toward that fixed target. Keyed on Board::piecesLocked()
    // so it recomputes whenever the piece changes via ANY lock path (gravity,
    // lock-delay, or hard drop) — not only the hard drops the AI itself initiates.
    AiMove aiPlan_{0, 0, false};
    int aiPlanPiece_ = -1;  // piecesLocked() the cached plan was computed for
};

// Soft drop: ~20 rows/second while Down is held (a common feel value).
inline constexpr double kSoftDropInterval = 1.0 / 20.0;

}  // namespace tetris

#endif  // TETRIS_APP_GAME_HPP
