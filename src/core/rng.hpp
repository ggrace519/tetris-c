// Seedable RNG wrapper so 7-bag shuffles are deterministic in tests.
// Pure — no raylib. (ADR-0001)
#ifndef TETRIS_CORE_RNG_HPP
#define TETRIS_CORE_RNG_HPP

#include <algorithm>
#include <cstdint>
#include <random>

namespace tetris {

class Rng {
public:
    // Default: nondeterministic seed. Tests pass an explicit seed for repeatability.
    Rng() : engine_(std::random_device{}()) {}
    explicit Rng(std::uint64_t seed) : engine_(seed) {}

    void seed(std::uint64_t s) { engine_.seed(s); }

    // Fisher-Yates shuffle over [first, last) using this engine.
    template <typename It>
    void shuffle(It first, It last) {
        std::shuffle(first, last, engine_);
    }

private:
    std::mt19937_64 engine_;
};

}  // namespace tetris

#endif  // TETRIS_CORE_RNG_HPP
