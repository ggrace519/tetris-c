// Rendering — the ONLY place (besides input/game) that touches raylib.
// Draws the core Board state. Depends on core, never the reverse. (ADR-0001)
#ifndef TETRIS_APP_RENDER_HPP
#define TETRIS_APP_RENDER_HPP

#include "app/juice.hpp"
#include "app/music.hpp"
#include "core/board.hpp"
#include "core/highscores.hpp"
#include "core/modes.hpp"

namespace tetris {

// Layout constants for the window (app-layer only; core has none of these).
inline constexpr int kCellPx = 30;
inline constexpr int kSidePanel = 220;
inline constexpr int kPlayW = kCols * kCellPx;
inline constexpr int kPlayH = kRows * kCellPx;
inline constexpr int kWinW = kPlayW + kSidePanel;
inline constexpr int kWinH = kPlayH;

enum class Screen { Menu, Playing, Paused, GameOver, Won };

// Draw the in-game frame for the given controller + screen state. `best` is the
// stored record for the current mode (for the BEST line). Call between
// BeginDrawing/EndDrawing.
void drawFrame(const ModeController& mc, Screen screen, const ModeRecord& best,
               const Juice& juice, bool aiOn);

// Draw the start menu (mode + difficulty selection) with the highlighted mode's
// stored best and the current music track. `modeSel`/`diffSel` are the currently
// highlighted indices.
void drawMenu(int modeSel, int diffSel, const ModeRecord& best, Track music);

// Draw the pause-menu overlay (Resume/Restart/Music/Quit). `sel` is the
// highlighted row; `music` is the current track (shown on the Music row). Drawn
// AFTER the scene blit so text stays crisp.
void drawPauseMenu(int sel, Track music);

// Draw the danger vignette (pulsing red edges) at intensity `danger` in [0,1].
// Drawn on top of the blitted scene.
void drawDangerVignette(float danger);

}  // namespace tetris

#endif  // TETRIS_APP_RENDER_HPP
