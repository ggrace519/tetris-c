#include "app/render.hpp"

#include <array>
#include <cstdio>

#include "raylib.h"

namespace tetris {
namespace {

// Convert a core RGBA color to a raylib Color.
inline ::Color rl(const tetris::Color& c) { return ::Color{c.r, c.g, c.b, c.a}; }

void drawCell(int col, int row, const tetris::Color& color, bool ghost = false) {
    const int px = col * kCellPx;
    const int py = row * kCellPx;
    if (ghost) {
        // Outline only for the ghost.
        DrawRectangleLines(px, py, kCellPx, kCellPx, rl(color));
    } else {
        DrawRectangle(px, py, kCellPx, kCellPx, rl(color));
        DrawRectangleLines(px, py, kCellPx, kCellPx, rl(kColDarkBg));
    }
}

void drawPlayfield(const Board& board) {
    // Background + grid lines.
    DrawRectangle(0, 0, kPlayW, kPlayH, rl(kColBlack));
    for (int x = 0; x <= kCols; ++x)
        DrawLine(x * kCellPx, 0, x * kCellPx, kPlayH, rl(kColGridLine));
    for (int y = 0; y <= kRows; ++y)
        DrawLine(0, y * kCellPx, kPlayW, y * kCellPx, rl(kColGridLine));

    // Locked cells.
    const Grid& g = board.grid();
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (g[r][c].has_value()) {
                drawCell(c, r, kPieceColors[static_cast<int>(g[r][c].value())]);
            }
        }
    }

    if (!board.gameOver()) {
        // Ghost (outline at the landing position).
        for (const Cell& gc : board.ghostCells()) {
            if (gc.y >= 0) drawCell(gc.x, gc.y, board.current().color(), /*ghost=*/true);
        }
        // Active piece.
        for (const Cell& ac : board.current().cells()) {
            if (ac.y >= 0) drawCell(ac.x, ac.y, board.current().color());
        }
    }
}

void drawNextPreview(const Board& board) {
    const int panelX = kPlayW + 20;
    const int panelY = 60;
    DrawText("NEXT", panelX, panelY - 30, 20, rl(kColWhite));
    // Draw the next piece's rotation-0 cells in a small box.
    const Piece& n = board.next();
    for (const Cell& c : n.cellsAt(0, 0, 0)) {
        const int px = panelX + c.x * kCellPx;
        const int py = panelY + c.y * kCellPx;
        DrawRectangle(px, py, kCellPx, kCellPx, rl(n.color()));
        DrawRectangleLines(px, py, kCellPx, kCellPx, rl(kColDarkBg));
    }
}

void drawHud(const Board& board) {
    const int x = kPlayW + 20;
    char buf[64];
    DrawText("SCORE", x, 220, 20, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%ld", board.score());
    DrawText(buf, x, 244, 26, rl(kColWhite));

    DrawText("LEVEL", x, 300, 20, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%d", board.level());
    DrawText(buf, x, 324, 26, rl(kColWhite));

    DrawText("LINES", x, 380, 20, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%d", board.linesCleared());
    DrawText(buf, x, 404, 26, rl(kColWhite));

    DrawText("Move  <- ->", x, 480, 16, rl(kColGray));
    DrawText("Rotate  Z / X", x, 500, 16, rl(kColGray));
    DrawText("Soft drop  v", x, 520, 16, rl(kColGray));
    DrawText("Hard drop  SPACE", x, 540, 16, rl(kColGray));
    DrawText("Pause P  Restart R", x, 560, 16, rl(kColGray));
}

void drawCenterMessage(const char* title, const char* subtitle) {
    // Dim overlay over the playfield.
    DrawRectangle(0, 0, kPlayW, kPlayH, ::Color{0, 0, 0, 160});
    const int tw = MeasureText(title, 40);
    DrawText(title, (kPlayW - tw) / 2, kPlayH / 2 - 40, 40, rl(kColWhite));
    if (subtitle) {
        const int sw = MeasureText(subtitle, 20);
        DrawText(subtitle, (kPlayW - sw) / 2, kPlayH / 2 + 10, 20, rl(kColGray));
    }
}

}  // namespace

void drawFrame(const Board& board, Screen screen) {
    ClearBackground(rl(kColDarkBg));
    drawPlayfield(board);
    drawNextPreview(board);
    drawHud(board);

    if (screen == Screen::Paused) {
        drawCenterMessage("PAUSED", "Press P to resume");
    } else if (screen == Screen::GameOver) {
        drawCenterMessage("GAME OVER", "Press R to restart");
    }
}

}  // namespace tetris
