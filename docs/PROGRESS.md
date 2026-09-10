# Build progress — tetris-c

Working log for the "complete the game" effort. One feature unit at a time, each
gated by: **core tests green + a real run**. Newest at bottom.

## Tiers (from PRD §6)

### Tier 1 — Playable base ✅ DONE (2026-09-10)
Pure core (grid, 7-bag, piece, collision, move, rotate+kicks, hard drop, line
clear, scoring, level/speed, ghost, game-over) + raylib app (window, 60fps loop,
gravity, input, render playfield/ghost/next/HUD, pause/restart/game-over).
- Core tests: 21 cases / 127 assertions passing.
- Verified: game runs 60 FPS, 520×600 window, renders correctly (screenshot).

### Tier 2 — Modes + core feel (IN PROGRESS)
- [ ] Modes: Marathon / Sprint / Ultra (`innovation/modes` @ c4c9676)
      + a way to select them (start screen / key) — modes must be reachable.
      NOTE: Ultra garbage is broken in the Python source; port fixes it. Needs a
      new Board API to push a garbage row from the bottom (test headless first).
- [ ] DAS/ARR + lock delay (`innovation/das-lock-delay` @ 4e91423)
- [ ] Combo tracking (`innovation/combos` @ 12dd321)
- [ ] Hard-drop animation (`innovation/drop-animation` @ 9d9054a)

### Tier 3 — Polish + persistence
- [ ] T-spin detection (`innovation/t-spin` @ 40c94f5)
- [ ] Juice: shake/particles (`innovation/juice` @ 0cab591)
- [ ] Heuristic AI (`innovation/ai` @ d285bb9)
- [ ] Settings + high-score persistence; rebindable keys

## Commands
- Build game: `cd build && ./premake5 gmake && cd .. && make` → `bin/Debug/tetris-c`
- Core tests: `make -f tests/tests.mk`
