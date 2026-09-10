// Rendering — the ONLY place (besides input/game) that touches raylib.
// Draws the core Board state. Depends on core, never the reverse. (ADR-0001)
#ifndef TETRIS_APP_RENDER_HPP
#define TETRIS_APP_RENDER_HPP

#include "core/board.hpp"

namespace tetris {

// Layout constants for the window (app-layer only; core has none of these).
inline constexpr int kCellPx = 30;
inline constexpr int kSidePanel = 220;
inline constexpr int kPlayW = kCols * kCellPx;
inline constexpr int kPlayH = kRows * kCellPx;
inline constexpr int kWinW = kPlayW + kSidePanel;
inline constexpr int kWinH = kPlayH;

enum class Screen { Playing, Paused, GameOver };

// Draw the whole frame for the given board + screen state. Call between
// BeginDrawing/EndDrawing.
void drawFrame(const Board& board, Screen screen);

}  // namespace tetris

#endif  // TETRIS_APP_RENDER_HPP
