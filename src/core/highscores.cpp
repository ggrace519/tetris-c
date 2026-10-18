#include "core/highscores.hpp"

#include <fstream>
#include <sstream>

namespace tetris {

namespace {
const char* modeKey(GameMode m) {
    switch (m) {
        case GameMode::Marathon: return "marathon";
        case GameMode::Sprint:   return "sprint";
        case GameMode::Ultra:    return "ultra";
    }
    return "?";
}
}  // namespace

void HighScores::load(const std::string& path) {
    records_ = {};  // reset to zeros first
    std::ifstream in(path);
    if (!in) return;  // missing file → all zeros, no error

    // Simple line format:  <mode> <bestScore> <bestTime> <hasTime>
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string key;
        long score = 0;
        double time = 0.0;
        int hasTime = 0;
        if (!(ss >> key >> score >> time >> hasTime)) continue;  // skip malformed
        for (int i = 0; i < 3; ++i) {
            if (key == modeKey(static_cast<GameMode>(i))) {
                records_[i].bestScore = score;
                records_[i].bestTime = time;
                records_[i].hasTime = (hasTime != 0);
                break;
            }
        }
    }
}

bool HighScores::save(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    for (int i = 0; i < 3; ++i) {
        const ModeRecord& r = records_[i];
        out << modeKey(static_cast<GameMode>(i)) << ' ' << r.bestScore << ' '
            << r.bestTime << ' ' << (r.hasTime ? 1 : 0) << '\n';
    }
    return static_cast<bool>(out);
}

bool HighScores::submit(GameMode m, long score, double timeSec, bool cleared) {
    ModeRecord& r = records_[idx(m)];
    bool improved = false;

    if (score > r.bestScore) {
        r.bestScore = score;
        improved = true;
    }
    // Sprint's headline metric is fastest clear time; only count completed runs.
    if (m == GameMode::Sprint && cleared) {
        if (!r.hasTime || timeSec < r.bestTime) {
            r.bestTime = timeSec;
            r.hasTime = true;
            improved = true;
        }
    }
    return improved;
}

}  // namespace tetris
