// Visual juice: screen shake + line-clear particles. App-layer only (raylib).
// Ports the intent of python-tetris innovation/juice (shake 0.15s / intensity 3px
// scaling with lines; particle burst on clears). No core dependency beyond colors.
#ifndef TETRIS_APP_JUICE_HPP
#define TETRIS_APP_JUICE_HPP

#include <vector>

#include "core/constants.hpp"

namespace tetris {

struct Particle {
    float x, y;      // pixels
    float vx, vy;    // pixels/sec
    float life;      // seconds remaining
    float maxLife;
    Color color;
};

class Juice {
public:
    // Trigger effects for a line clear of `lines` rows at grid row `clearRowMid`
    // (used to seed particles roughly where the clear happened).
    void onLineClear(int lines);

    // Spawn a burst of particles at a pixel position (used per cleared cell).
    void spawnBurst(float px, float py, Color color, int count);

    void update(float dt);

    // Current screen-shake offset in pixels (apply to the playfield draw origin).
    float shakeX() const { return shakeX_; }
    float shakeY() const { return shakeY_; }

    const std::vector<Particle>& particles() const { return particles_; }

private:
    double shakeTimer_ = 0.0;
    double shakeDuration_ = 0.0;
    float shakeIntensity_ = 0.0f;
    float shakeX_ = 0.0f, shakeY_ = 0.0f;
    std::vector<Particle> particles_;
    unsigned rngState_ = 0x1234567u;
    float frand(float lo, float hi);
};

}  // namespace tetris

#endif  // TETRIS_APP_JUICE_HPP
