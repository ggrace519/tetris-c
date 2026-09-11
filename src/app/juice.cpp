#include "app/juice.hpp"

#include <algorithm>
#include <cmath>

#include "app/reasings.h"  // static-inline easing curves (raylib/reasings, zlib)

namespace tetris {

namespace {
constexpr double kShakeDuration = 0.15;   // seconds (settings.py SHAKE_DURATION)
constexpr float kShakeBase = 3.0f;        // px per line (SHAKE_INTENSITY)
constexpr float kParticleLife = 0.5f;     // seconds
}  // namespace

float Juice::frand(float lo, float hi) {
    // xorshift32 → [lo, hi]
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 17;
    rngState_ ^= rngState_ << 5;
    const float t = (rngState_ & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
    return lo + t * (hi - lo);
}

void Juice::onLineClear(int lines) {
    if (lines <= 0) return;
    // Shake scales with the number of lines cleared.
    shakeDuration_ = kShakeDuration;
    shakeTimer_ = kShakeDuration;
    shakeIntensity_ = kShakeBase * static_cast<float>(lines);
}

void Juice::spawnBurst(float px, float py, Color color, int count) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = px;
        p.y = py;
        p.vx = frand(-120.0f, 120.0f);
        p.vy = frand(-180.0f, -20.0f);  // upward-ish
        p.maxLife = kParticleLife * frand(0.6f, 1.0f);
        p.life = p.maxLife;
        p.color = color;
        particles_.push_back(p);
    }
}

void Juice::update(float dt) {
    // Shake: decaying random offset while the timer runs.
    if (shakeTimer_ > 0.0) {
        shakeTimer_ -= dt;
        if (shakeTimer_ <= 0.0) {
            shakeTimer_ = 0.0;
            shakeX_ = shakeY_ = 0.0f;
        } else {
            // Eased shake falloff: the amplitude HOLDS high early then drops off at
            // the end ("punchy start, smooth settle") — i.e. a curve that stays
            // ABOVE the linear ramp. reasings ease(t,b,c,d): t=time, b=start,
            // c=change, d=duration. Feeding the *remaining* time into EaseCubicOut
            // (0→1 over remaining) yields exactly that: decay = 1.0 at full
            // remaining, ~0.88 at the midpoint (vs 0.5 linear), 0 at timeout.
            const float remaining = static_cast<float>(shakeTimer_);
            const float duration = static_cast<float>(shakeDuration_);
            const float decay = EaseCubicOut(remaining, 0.0f, 1.0f, duration);
            const float amp = shakeIntensity_ * decay;
            shakeX_ = frand(-amp, amp);
            shakeY_ = frand(-amp, amp);
        }
    }

    // Particles: integrate with a little gravity, cull the dead.
    const float gravity = 320.0f;
    for (auto& p : particles_) {
        p.life -= dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += gravity * dt;
    }
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
                       [](const Particle& p) { return p.life <= 0.0f; }),
        particles_.end());
}

}  // namespace tetris
