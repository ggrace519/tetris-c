# DEMO — innovation #4: easing curves (`reasings.h`)

Adopts raylib's `reasings.h` easing library so motion is springy/non-linear
instead of a flat linear ramp. First, foundational step from `INNOVATIONS.md`.

## What changed
- Vendored `src/app/reasings.h` (raylib/raysan5, zlib — attributed in NOTICE).
  Header-only, `static inline`, safe to include in multiple TUs.
- `src/app/juice.cpp` — screen-shake falloff now uses `EaseCubicOut` (punchy start,
  smooth settle) instead of a linear `timer/duration` decay.
- `src/app/render.cpp` — line-clear particle alpha fade now uses `EaseQuadOut`
  (holds brightness slightly longer, then falls off) instead of a linear fade.

## How to run
```bash
cd build && ./premake5 gmake && cd .. && make
./bin/Debug/tetris-c
```
Start any mode and clear a line: the screen shake settles with an eased curve and
the particle burst fades more livelily. (The change is felt, not structural — this
branch is the easing *foundation* other proposals build HUD/menu tweens on.)

## What works — verified
- Full game builds (premake5 + make), binary runs.
- **Curve behavior verified numerically** (not just "it builds"): both eased curves
  start at 1.0, end at 0.0, and sit ABOVE the linear line in between — shake decay
  `0.984 / 0.875 / 0.578` and particle alpha `0.938 / 0.750 / 0.438` at 25/50/75%
  of life, vs linear `0.750 / 0.500 / 0.250`. That is the "hold high, then settle"
  shape claimed. (An earlier draft had the curve inverted — fed elapsed instead of
  remaining time — which produced the *opposite* feel; caught and corrected.)
- Headless suite still green: **93 tests / 428 assertions** (juice.cpp compiles into
  it, so the reasings include is verified not to break the headless build).

## What's stubbed
- Nothing. This is a complete, self-contained change.

## Next increment
- Use `EaseBackOut` for HUD number pops (SCORE/LEVEL) and `EaseElasticOut` for a
  menu-panel entrance — see proposal #6 (crescendo) and the menu-polish note in
  `INNOVATIONS.md`. Those build directly on this header.
