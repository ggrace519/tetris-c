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
                drawCell(c, r, colorForCell(g[r][c].value()));
            }
        }
    }

    if (!board.gameOver()) {
        if (board.animating()) {
            // Hard-drop animation: draw the piece interpolated between its start
            // and landing rows, with a bright trail behind it for feedback.
            const double t = board.animProgress();
            const double interpY = board.animStartY() +
                                   (board.animLandingY() - board.animStartY()) * t;
            const Piece& p = board.current();
            // Trail: faint copies from start to current interpolated position.
            for (const Cell& ac : p.cellsAt(p.x(), board.animStartY(), p.rotation())) {
                if (ac.y < 0) continue;
                const int px = ac.x * kCellPx;
                const int topY = static_cast<int>(ac.y * kCellPx);
                const int botY = static_cast<int>((ac.y + (interpY - board.animStartY())) * kCellPx);
                Color c = p.color();
                DrawRectangle(px, topY, kCellPx, botY - topY + kCellPx,
                              ::Color{c.r, c.g, c.b, 90});
            }
            // The piece itself at the interpolated row.
            for (const Cell& ac : p.cellsAt(p.x(), static_cast<int>(interpY), p.rotation())) {
                if (ac.y >= 0) drawCell(ac.x, ac.y, p.color());
            }
        } else {
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

const char* modeName(GameMode m) {
    switch (m) {
        case GameMode::Marathon: return "MARATHON";
        case GameMode::Sprint:   return "SPRINT";
        case GameMode::Ultra:    return "ULTRA";
    }
    return "";
}

void drawHud(const ModeController& mc) {
    const Board& board = mc.board();
    const int x = kPlayW + 20;
    char buf[64];

    DrawText(modeName(mc.mode()), x, 20, 22, rl(kColWhite));
    // Mode-specific objective line.
    if (mc.mode() == GameMode::Ultra) {
        std::snprintf(buf, sizeof(buf), "Time %.1fs", mc.elapsed());
    } else {
        std::snprintf(buf, sizeof(buf), "%d / %d lines  %.1fs",
                      board.linesCleared(), mc.winLines(), mc.elapsed());
    }
    DrawText(buf, x, 46, 16, rl(kColGray));

    DrawText("SCORE", x, 100, 18, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%ld", board.score());
    DrawText(buf, x, 122, 26, rl(kColWhite));

    DrawText("LEVEL", x, 180, 18, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%d", board.level());
    DrawText(buf, x, 202, 26, rl(kColWhite));

    DrawText("LINES", x, 260, 18, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%d", board.linesCleared());
    DrawText(buf, x, 282, 26, rl(kColWhite));

    // Combo indicator (only while an active combo run is going).
    if (board.combo() > 1) {
        std::snprintf(buf, sizeof(buf), "COMBO x%d", board.combo());
        DrawText(buf, x, 340, 24, rl(kColYellow));
    }

    DrawText("Move  <- ->", x, 470, 16, rl(kColGray));
    DrawText("Rotate  Z / X", x, 490, 16, rl(kColGray));
    DrawText("Soft drop  v", x, 510, 16, rl(kColGray));
    DrawText("Hard drop  SPACE", x, 530, 16, rl(kColGray));
    DrawText("Pause P", x, 550, 16, rl(kColGray));
    DrawText("Menu M  Restart R", x, 570, 16, rl(kColGray));
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

void drawFrame(const ModeController& mc, Screen screen) {
    const Board& board = mc.board();
    ClearBackground(rl(kColDarkBg));
    drawPlayfield(board);
    drawNextPreview(board);
    drawHud(mc);

    if (screen == Screen::Paused) {
        drawCenterMessage("PAUSED", "Press P to resume");
    } else if (screen == Screen::GameOver) {
        drawCenterMessage("GAME OVER", "R restart  ·  M menu");
    } else if (screen == Screen::Won) {
        drawCenterMessage("YOU WIN!", "R restart  ·  M menu");
    }
}

void drawMenu(int modeSel, int diffSel) {
    static const char* kModes[] = {"MARATHON", "SPRINT", "ULTRA"};
    static const char* kDiffs[] = {"EASY", "NORMAL", "HARD", "EXPERT"};

    ClearBackground(rl(kColDarkBg));

    const char* title = "TETRIS-C";
    const int tw = MeasureText(title, 48);
    DrawText(title, (kWinW - tw) / 2, 50, 48, rl(kColCyan));

    DrawText("MODE   (up/down)", 60, 150, 20, rl(kColGray));
    for (int i = 0; i < 3; ++i) {
        const bool sel = (i == modeSel);
        DrawText(kModes[i], 90, 185 + i * 34, 26,
                 rl(sel ? kColYellow : kColWhite));
        if (sel) DrawText(">", 60, 185 + i * 34, 26, rl(kColYellow));
    }

    DrawText("DIFFICULTY   (left/right)", 60, 320, 20, rl(kColGray));
    for (int i = 0; i < 4; ++i) {
        const bool sel = (i == diffSel);
        DrawText(kDiffs[i], 90 + i * 110, 355, 22,
                 rl(sel ? kColYellow : kColWhite));
    }
    DrawText("(difficulty affects Ultra garbage speed)", 60, 390, 14, rl(kColGray));

    DrawText("Press ENTER or SPACE to start", 60, 470, 22, rl(kColGreen));
    DrawText("Esc to quit", 60, 505, 16, rl(kColGray));
}

}  // namespace tetris
