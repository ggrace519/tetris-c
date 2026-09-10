// Pure game data — dimensions, colors, tetromino shapes, kicks, scoring.
// Ported verbatim from python-tetris/tetris_game/settings.py (the parity source).
// NO raylib / OS headers here — this is part of the headless core (ADR-0001).
#ifndef TETRIS_CORE_CONSTANTS_HPP
#define TETRIS_CORE_CONSTANTS_HPP

#include <array>
#include <cstdint>
#include <string_view>

namespace tetris {

// --- Board dimensions ---
inline constexpr int kCols = 10;
inline constexpr int kRows = 20;

// --- Timing (seconds) — settings.py START_FALL_SPEED / MIN_FALL_SPEED ---
inline constexpr double kStartFallSpeed = 0.8;
inline constexpr double kMinFallSpeed = 0.08;
inline constexpr double kFallSpeedPerLevel = 0.06;  // subtracted per level above 1

// --- Colors: plain RGBA, no raylib. The app layer converts at draw time. ---
struct Color {
    std::uint8_t r, g, b, a;
};
inline constexpr Color kColBlack{18, 18, 22, 255};
inline constexpr Color kColDarkBg{28, 30, 36, 255};
inline constexpr Color kColGridLine{55, 60, 70, 255};
inline constexpr Color kColWhite{240, 240, 240, 255};
inline constexpr Color kColGray{160, 160, 170, 255};
inline constexpr Color kColRed{220, 70, 70, 255};
inline constexpr Color kColGreen{70, 200, 120, 255};
inline constexpr Color kColBlue{70, 120, 220, 255};
inline constexpr Color kColCyan{70, 210, 230, 255};
inline constexpr Color kColYellow{240, 215, 70, 255};
inline constexpr Color kColOrange{235, 145, 60, 255};
inline constexpr Color kColPurple{170, 90, 220, 255};
inline constexpr Color kColGarbage{100, 100, 100, 255};

// --- The 7 tetrominoes ---
// Order matches settings.py SHAPES dict order (I,O,T,S,Z,J,L) so the 7-bag draws
// the same set. ShapeId indexes into kShapes / kPieceColors.
// Garbage is a grid-only marker (Ultra mode): never drawn from the bag, never
// used to index kShapes/kPieceColors — it sits AFTER Count so kShapeCount stays 7.
enum class ShapeId : int { I = 0, O, T, S, Z, J, L, Count, Garbage };
inline constexpr int kShapeCount = static_cast<int>(ShapeId::Count);  // 7

inline constexpr std::array<char, kShapeCount> kShapeNames{'I', 'O', 'T', 'S', 'Z', 'J', 'L'};

inline constexpr std::array<Color, kShapeCount> kPieceColors{
    kColCyan,    // I
    kColYellow,  // O
    kColPurple,  // T
    kColGreen,   // S
    kColRed,     // Z
    kColBlue,    // J
    kColOrange,  // L
};

// Color for any cell stored in the grid (handles the Garbage marker).
inline constexpr Color colorForCell(ShapeId s) {
    return (s == ShapeId::Garbage) ? kColGarbage
                                   : kPieceColors[static_cast<int>(s)];
}

// A single cell offset within a piece's 4x4 box.
struct Cell {
    int x, y;
};

// shape -> rotation(0..3) -> 4 occupied cells. Verbatim from settings.py SHAPES.
using Rotation = std::array<Cell, 4>;
using ShapeTable = std::array<Rotation, 4>;

inline constexpr std::array<ShapeTable, kShapeCount> kShapes{{
    // I
    {{{{ {0,1},{1,1},{2,1},{3,1} }},
      {{ {2,0},{2,1},{2,2},{2,3} }},
      {{ {0,2},{1,2},{2,2},{3,2} }},
      {{ {1,0},{1,1},{1,2},{1,3} }}}},
    // O
    {{{{ {1,0},{2,0},{1,1},{2,1} }},
      {{ {1,0},{2,0},{1,1},{2,1} }},
      {{ {1,0},{2,0},{1,1},{2,1} }},
      {{ {1,0},{2,0},{1,1},{2,1} }}}},
    // T
    {{{{ {1,0},{0,1},{1,1},{2,1} }},
      {{ {1,0},{1,1},{2,1},{1,2} }},
      {{ {0,1},{1,1},{2,1},{1,2} }},
      {{ {1,0},{0,1},{1,1},{1,2} }}}},
    // S
    {{{{ {1,0},{2,0},{0,1},{1,1} }},
      {{ {1,0},{1,1},{2,1},{2,2} }},
      {{ {1,1},{2,1},{0,2},{1,2} }},
      {{ {0,0},{0,1},{1,1},{1,2} }}}},
    // Z
    {{{{ {0,0},{1,0},{1,1},{2,1} }},
      {{ {2,0},{1,1},{2,1},{1,2} }},
      {{ {0,1},{1,1},{1,2},{2,2} }},
      {{ {1,0},{0,1},{1,1},{0,2} }}}},
    // J
    {{{{ {0,0},{0,1},{1,1},{2,1} }},
      {{ {1,0},{2,0},{1,1},{1,2} }},
      {{ {0,1},{1,1},{2,1},{2,2} }},
      {{ {1,0},{1,1},{0,2},{1,2} }}}},
    // L
    {{{{ {2,0},{0,1},{1,1},{2,1} }},
      {{ {1,0},{1,1},{1,2},{2,2} }},
      {{ {0,1},{1,1},{2,1},{0,2} }},
      {{ {0,0},{1,0},{1,1},{1,2} }}}},
}};

// --- Wall-kick offsets (settings.py KICKS_*). Simplified symmetric table, tried
//     in order on any rotation. NOT full SRS (see DECISIONS ADR-0004). ---
inline constexpr std::array<Cell, 6> kKicksJLSTZ{{
    {0, 0}, {-1, 0}, {1, 0}, {0, -1}, {-2, 0}, {2, 0}}};
inline constexpr std::array<Cell, 6> kKicksI{{
    {0, 0}, {-2, 0}, {1, 0}, {-1, 0}, {2, 0}, {0, -1}}};
inline constexpr std::array<Cell, 1> kKicksO{{{0, 0}}};

// --- Scoring (settings.py LINE_SCORES), multiplied by level in board.cpp ---
// index by cleared-line count 1..4.
inline constexpr std::array<int, 5> kLineScores{0, 100, 300, 500, 800};

inline constexpr int kLinesPerLevel = 10;
inline constexpr int kHardDropPerCell = 2;  // hard_drop: distance * 2
inline constexpr int kComboBonusPerLevel = 50;  // combo bonus = combo * 50 * level

// T-spin scoring (python-tetris innovation/t-spin values; × level). ADR-0006.
inline constexpr int kTSpinMini = 100;
inline constexpr int kTSpinSingle = 200;
inline constexpr int kTSpinDouble = 400;
inline constexpr int kTSpinTriple = 800;

// Feel timings (settings.py on innovation/das-lock-delay + drop-animation).
inline constexpr double kLockDelay = 0.5;   // seconds a landed piece waits before locking
inline constexpr double kDasSeconds = 0.167;  // Delayed Auto Shift
inline constexpr double kArrSeconds = 0.033;  // Auto Repeat Rate
inline constexpr double kDropAnimDuration = 0.08;  // hard-drop stretch animation

// Spawn position (settings.py Piece.__init__: x=3, y=0)
inline constexpr int kSpawnX = 3;
inline constexpr int kSpawnY = 0;

}  // namespace tetris

#endif  // TETRIS_CORE_CONSTANTS_HPP
