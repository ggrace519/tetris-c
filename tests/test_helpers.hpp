// Shared helpers for the core test suite.
#ifndef TETRIS_TEST_HELPERS_HPP
#define TETRIS_TEST_HELPERS_HPP

#include "core/board.hpp"
#include "core/constants.hpp"

namespace tetris {

// Hard drop and run the drop animation to completion so the piece actually
// locks. Use wherever a test just wants the piece down-and-locked (the hard-drop
// animation, innovation/drop-animation, means hardDrop() alone no longer locks
// immediately unless the piece was already resting).
inline void hardDropAndSettle(Board& b) {
    b.hardDrop();
    // One step past the animation duration completes it (or is a no-op if the
    // piece was already resting and locked immediately).
    if (b.animating()) b.step(kDropAnimDuration + 0.001);
}

}  // namespace tetris

#endif  // TETRIS_TEST_HELPERS_HPP
