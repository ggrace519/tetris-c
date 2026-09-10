#include "core/ai.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace tetris {

namespace {
// Seki (2004) weights (python-tetris innovation/ai).
constexpr double kWAggregate = -0.51;
constexpr double kWComplete = 0.76;
constexpr double kWHoles = -0.35;
constexpr double kWBumpiness = -0.18;

// Column height = kRows - (row of topmost filled cell); 0 if empty.
int columnHeight(const Grid& grid, int col) {
    for (int r = 0; r < kRows; ++r) {
        if (grid[r][col].has_value()) return kRows - r;
    }
    return 0;
}
}  // namespace

TetrisAI::TetrisAI(Difficulty diff, std::uint64_t seed) : rng_(seed) {
    setDifficulty(diff);
}

void TetrisAI::setDifficulty(Difficulty d) {
    diff_ = d;
    switch (d) {
        case Difficulty::Easy:   errorRate_ = 0.30; decisionDelay_ = 0.5; break;
        case Difficulty::Normal: errorRate_ = 0.10; decisionDelay_ = 0.3; break;
        case Difficulty::Hard:   errorRate_ = 0.02; decisionDelay_ = 0.1; break;
        case Difficulty::Expert: errorRate_ = 0.00; decisionDelay_ = 0.05; break;
    }
}

int TetrisAI::aggregateHeight(const Grid& grid) {
    int total = 0;
    for (int c = 0; c < kCols; ++c) total += columnHeight(grid, c);
    return total;
}

int TetrisAI::completeLines(const Grid& grid) {
    int lines = 0;
    for (int r = 0; r < kRows; ++r) {
        bool full = true;
        for (int c = 0; c < kCols; ++c) {
            if (!grid[r][c].has_value()) { full = false; break; }
        }
        if (full) ++lines;
    }
    return lines;
}

int TetrisAI::holes(const Grid& grid) {
    int h = 0;
    for (int c = 0; c < kCols; ++c) {
        bool filledAbove = false;
        for (int r = 0; r < kRows; ++r) {
            if (grid[r][c].has_value()) filledAbove = true;
            else if (filledAbove) ++h;  // empty cell under a filled one
        }
    }
    return h;
}

int TetrisAI::bumpiness(const Grid& grid) {
    int b = 0;
    for (int c = 0; c < kCols - 1; ++c) {
        b += std::abs(columnHeight(grid, c) - columnHeight(grid, c + 1));
    }
    return b;
}

double TetrisAI::evaluate(const Grid& postClearGrid, int linesCleared) {
    // The grid is post-clear, so completeLines(postClearGrid) is ~0; the reward for
    // clearing comes from the explicit linesCleared argument (Seki's intent).
    return kWAggregate * aggregateHeight(postClearGrid) +
           kWComplete * linesCleared + kWHoles * holes(postClearGrid) +
           kWBumpiness * bumpiness(postClearGrid);
}

bool TetrisAI::columnFits(const Grid& grid, const Piece& piece, int x, int rotation) {
    // A placement column is usable if the piece has at least one legal resting
    // position in it. We check that at spawn height (y such that cells are on the
    // field) the horizontal position is in-bounds and not overlapping at the top.
    for (const Cell& c : piece.cellsAt(x, 0, rotation)) {
        if (c.x < 0 || c.x >= kCols) return false;
    }
    // Must not be blocked at the very top row (else it can't enter the column).
    for (const Cell& c : piece.cellsAt(x, 0, rotation)) {
        if (c.y >= 0 && c.y < kRows && grid[c.y][c.x].has_value()) return false;
    }
    return true;
}

TetrisAI::DropResult TetrisAI::simulateDrop(const Grid& grid, const Piece& piece,
                                            int x, int rotation) {
    // Drop the piece straight down until it would collide, then lock it.
    auto collidesAt = [&](int py) {
        for (const Cell& c : piece.cellsAt(x, py, rotation)) {
            if (c.x < 0 || c.x >= kCols) return true;
            if (c.y >= kRows) return true;
            if (c.y >= 0 && grid[c.y][c.x].has_value()) return true;
        }
        return false;
    };

    int y = 0;
    while (!collidesAt(y + 1)) ++y;

    Grid out = grid;
    for (const Cell& c : piece.cellsAt(x, y, rotation)) {
        if (c.y >= 0 && c.y < kRows && c.x >= 0 && c.x < kCols) {
            out[c.y][c.x] = piece.shape();
        }
    }

    // Clear full rows (compact downward), mirroring Board::clearLines.
    Grid cleared{};
    int writeRow = kRows - 1;
    for (int r = kRows - 1; r >= 0; --r) {
        bool full = true;
        for (int c = 0; c < kCols; ++c) {
            if (!out[r][c].has_value()) { full = false; break; }
        }
        if (!full) { cleared[writeRow] = out[r]; --writeRow; }
    }
    const int clearedCount = writeRow + 1;  // rows removed
    return DropResult{cleared, clearedCount};
}

AiMove TetrisAI::bestMove(const Grid& grid, const Piece& piece) {
    double bestScore = -std::numeric_limits<double>::infinity();
    AiMove best{0, 0, false};
    std::vector<AiMove> legal;

    for (int rot = 0; rot < 4; ++rot) {
        // Determine the valid x range from the rotation's cell bounding box.
        Piece probe = piece;
        probe.setRotation(rot);
        int minDx = 3, maxDx = 0;
        for (const Cell& c : probe.cellsAt(0, 0, rot)) {
            minDx = std::min(minDx, c.x);
            maxDx = std::max(maxDx, c.x);
        }
        for (int x = -minDx; x <= kCols - 1 - maxDx; ++x) {
            if (!columnFits(grid, probe, x, rot)) continue;
            legal.push_back(AiMove{x, rot, true});
            const DropResult sim = simulateDrop(grid, probe, x, rot);
            const double score = evaluate(sim.grid, sim.cleared);
            if (score > bestScore) {
                bestScore = score;
                best = AiMove{x, rot, true};
            }
        }
    }

    if (legal.empty()) return AiMove{0, 0, false};

    // Error rate: occasionally pick a random legal move instead of the best.
    if (errorRate_ > 0.0 && rng_.nextDouble() < errorRate_) {
        best = legal[static_cast<size_t>(rng_.nextInt(static_cast<int>(legal.size())))];
    }

    return best;
}

}  // namespace tetris
