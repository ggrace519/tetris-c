# DEMO — innovation #9: gradient + beveled blocks

Replaces the flat `DrawRectangle` cells with 3D-looking beveled, gradient blocks,
and gives the playfield a subtle level-reactive background gradient. No shader, no
render target — pure shape calls in `render.cpp`. From `INNOVATIONS.md` #9.

## What changed
- `src/app/render.cpp`:
  - New `drawBlock()` — a vertical gradient sheen (`DrawRectangleGradientV`) plus a
    beveled edge (light top-left, dark bottom-right via `ColorBrightness`) so each
    tile reads as a raised 3D block. Used for both the field cells and the NEXT
    preview so they match.
  - Playfield background is now a vertical gradient whose top edge warms (toward
    red) as `board.level()` climbs — a quiet reactive-background nod — instead of a
    flat fill.

## How to run
```bash
cd build && ./premake5 gmake && cd .. && make
./bin/Debug/tetris-c
```
Start any mode: the locked blocks and the falling/next pieces now have a beveled,
glossy look instead of flat squares; the background darkens toward the bottom and
warms as you level up.

## What works — verified
- Full game builds (premake5 + make) and launches on the real GPU (raylib 6.1-dev,
  AMD, 520×600) without error.
- **Visual verification:** rendered a real in-game frame through the actual
  `drawFrame()` path into a `RenderTexture2D` and exported it to PNG (throwaway
  harness, since headless CI can't drive the interactive window). The beveled
  gradient blocks render correctly across all 7 piece colors; the background
  gradient is confirmed alive (top `(41,41,45)` at L1 → `(74,41,45)` at L10 vs the
  black bottom).
- Headless suite unaffected: **93/428 green**.

## What's stubbed
- Nothing. Complete, self-contained change.

## Next increment
- The bevel/gradient pairs naturally with proposal #7 (bloom): bright block
  highlights bloom nicely once the scene render target (#5) lands. A per-piece
  inner-highlight animation on lock would layer on the lock-flash (#3/#6).
