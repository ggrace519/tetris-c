#include "app/render.hpp"

#include <array>
#include <cstdio>

#include "raylib.h"

#include "app/reasings.h"  // static-inline easing curves (raylib/reasings, zlib)

namespace tetris {
namespace {

// Convert a core RGBA color to a raylib Color.
inline ::Color rl(const tetris::Color& c) { return ::Color{c.r, c.g, c.b, c.a}; }

// Playfield draw offset (set by screen shake each frame).
int gOffX = 0, gOffY = 0;

void drawCell(int col, int row, const tetris::Color& color, bool ghost = false) {
    const int px = col * kCellPx + gOffX;
    const int py = row * kCellPx + gOffY;
    if (ghost) {
        // Outline only for the ghost.
        DrawRectangleLines(px, py, kCellPx, kCellPx, rl(color));
    } else {
        DrawRectangle(px, py, kCellPx, kCellPx, rl(color));
        DrawRectangleLines(px, py, kCellPx, kCellPx, rl(kColDarkBg));
    }
}

void drawPlayfield(const Board& board) {
    // Background + grid lines (with shake offset).
    DrawRectangle(gOffX, gOffY, kPlayW, kPlayH, rl(kColBlack));
    for (int x = 0; x <= kCols; ++x)
        DrawLine(x * kCellPx + gOffX, gOffY, x * kCellPx + gOffX, kPlayH + gOffY, rl(kColGridLine));
    for (int y = 0; y <= kRows; ++y)
        DrawLine(gOffX, y * kCellPx + gOffY, kPlayW + gOffX, y * kCellPx + gOffY, rl(kColGridLine));

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
    // NEXT sits at the top of the side panel. Label at y=15; piece box below it.
    const int panelX = kPlayW + 20;
    const int labelY = 15;
    const int boxY = 40;               // top of the piece box
    const int cell = 22;               // smaller cells so a piece fits cleanly
    DrawText("NEXT", panelX, labelY, 20, rl(kColGray));
    const Piece& n = board.next();
    for (const Cell& c : n.cellsAt(0, 0, 0)) {
        const int px = panelX + c.x * cell;
        const int py = boxY + c.y * cell;
        DrawRectangle(px, py, cell, cell, rl(n.color()));
        DrawRectangleLines(px, py, cell, cell, rl(kColDarkBg));
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

// Panel layout: NEXT occupies y=15..~110 (drawn by drawNextPreview). The stat
// stack starts below it. `best`, combo/T-spin, and AI-DEMO indicators are drawn
// by drawFrame at reserved y-slots to avoid overlap.
void drawHud(const ModeController& mc) {
    const Board& board = mc.board();
    const int x = kPlayW + 20;
    char buf[64];

    // Mode title + objective line (below the NEXT preview).
    DrawText(modeName(mc.mode()), x, 120, 22, rl(kColWhite));
    if (mc.mode() == GameMode::Ultra) {
        std::snprintf(buf, sizeof(buf), "Time %.1fs", mc.elapsed());
    } else {
        std::snprintf(buf, sizeof(buf), "%d / %d lines  %.1fs",
                      board.linesCleared(), mc.winLines(), mc.elapsed());
    }
    DrawText(buf, x, 146, 15, rl(kColGray));

    // SCORE (+ BEST is drawn just under it by drawFrame at y=196).
    DrawText("SCORE", x, 176, 16, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%ld", board.score());
    DrawText(buf, x, 196, 24, rl(kColWhite));

    DrawText("LEVEL", x, 260, 16, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%d", board.level());
    DrawText(buf, x, 280, 24, rl(kColWhite));

    DrawText("LINES", x, 330, 16, rl(kColGray));
    std::snprintf(buf, sizeof(buf), "%d", board.linesCleared());
    DrawText(buf, x, 350, 24, rl(kColWhite));

    // Controls at the bottom.
    DrawText("Move  <- ->", x, 468, 15, rl(kColGray));
    DrawText("Rotate  Z / X", x, 487, 15, rl(kColGray));
    DrawText("Soft drop  v", x, 506, 15, rl(kColGray));
    DrawText("Hard drop  SPACE", x, 525, 15, rl(kColGray));
    DrawText("Pause P  AI demo A", x, 544, 15, rl(kColGray));
    DrawText("Menu M  Restart R", x, 563, 15, rl(kColGray));
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

void drawParticles(const Juice& juice) {
    for (const Particle& p : juice.particles()) {
        // Eased alpha fade: particles HOLD their brightness a touch longer, then
        // fall off at the end — i.e. alpha stays ABOVE the linear line. Feeding the
        // remaining-life fraction into EaseQuadOut (0→1 over remaining life) gives
        // that: a=1.0 at birth, ~0.75 at half-life (vs 0.5 linear), 0 at death.
        const float lifeFrac = p.maxLife > 0 ? (p.life / p.maxLife) : 0.0f;  // 1→0
        const float a = EaseQuadOut(lifeFrac, 0.0f, 1.0f, 1.0f);
        Color c = p.color;
        DrawRectangle(static_cast<int>(p.x), static_cast<int>(p.y), 4, 4,
                      ::Color{c.r, c.g, c.b, static_cast<unsigned char>(a * 255)});
    }
}

void drawFrame(const ModeController& mc, Screen screen, const ModeRecord& best,
               const Juice& juice, bool aiOn) {
    const Board& board = mc.board();
    ClearBackground(rl(kColDarkBg));

    // Screen shake: offset all playfield drawing by the current shake amount.
    gOffX = static_cast<int>(juice.shakeX());
    gOffY = static_cast<int>(juice.shakeY());
    drawPlayfield(board);
    gOffX = gOffY = 0;  // reset so HUD/next panel are not shaken

    drawParticles(juice);
    drawNextPreview(board);
    drawHud(mc);

    // BEST line, tucked just under the SCORE value (which ends ~y=220).
    {
        char buf[64];
        const int x = kPlayW + 20;
        if (mc.mode() == GameMode::Sprint && best.hasTime) {
            std::snprintf(buf, sizeof(buf), "BEST %.1fs", best.bestTime);
        } else {
            std::snprintf(buf, sizeof(buf), "BEST %ld", best.bestScore);
        }
        DrawText(buf, x, 224, 15, rl(kColGray));
    }

    // Event indicators zone (between LINES at y=350 and controls at y=468):
    // combo, T-spin, and AI-DEMO share this band without overlapping each other.
    {
        const int x = kPlayW + 20;
        char buf[32];
        if (board.combo() > 1) {
            std::snprintf(buf, sizeof(buf), "COMBO x%d", board.combo());
            DrawText(buf, x, 396, 22, rl(kColYellow));
        }
        if (board.lastTSpin() == Board::TSpin::Full) {
            DrawText("T-SPIN!", x, 422, 22, rl(kColPurple));
        } else if (board.lastTSpin() == Board::TSpin::Mini) {
            DrawText("T-SPIN MINI", x, 422, 18, rl(kColPurple));
        }
        if (aiOn) {
            DrawText("AI DEMO", x, 448, 18, rl(kColGreen));  // its own line above the controls
        }
    }

    if (screen == Screen::Paused) {
        drawCenterMessage("PAUSED", "Press P to resume");
    } else if (screen == Screen::GameOver) {
        drawCenterMessage("GAME OVER", "R restart  ·  M menu");
    } else if (screen == Screen::Won) {
        drawCenterMessage("YOU WIN!", "R restart  ·  M menu");
    }
}

void drawMenu(int modeSel, int diffSel, const ModeRecord& best) {
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

    // Best for the highlighted mode.
    {
        char buf[64];
        if (modeSel == 1 && best.hasTime) {  // Sprint → best time
            std::snprintf(buf, sizeof(buf), "Best time: %.1fs", best.bestTime);
        } else {
            std::snprintf(buf, sizeof(buf), "Best score: %ld", best.bestScore);
        }
        DrawText(buf, 60, 425, 18, rl(kColCyan));
    }

    DrawText("Press ENTER or SPACE to start", 60, 470, 22, rl(kColGreen));
    DrawText("Esc to quit", 60, 505, 16, rl(kColGray));
}

}  // namespace tetris
