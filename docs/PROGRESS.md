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
- [x] Modes: Marathon / Sprint / Ultra (`innovation/modes` @ c4c9676) ✅
      Core `ModeController` + `Board::injectGarbage` (fixes the dead Python Ultra
      mechanic). Start menu (mode + difficulty select) makes them reachable —
      menu render verified by screenshot. Core tests +13 (garbage + modes).
- [x] DAS/ARR + lock delay (`innovation/das-lock-delay` @ 4e91423) ✅
      Core: Board::step(dt) drives gravity + a 0.5s lock delay with move/rotate
      reset (hard drop bypasses). App: DAS 167ms / ARR 33ms horizontal auto-shift
      + SOCD (last-pressed wins), soft drop ~20 rows/s. Core tests +7 (lock delay).
- [x] Combo tracking (`innovation/combos` @ 12dd321) ✅
      Core: consecutive line-clearing locks increment combo; bonus = combo*50*level;
      a no-clear lock resets it. maxCombo tracked. HUD shows COMBO xN. +5 tests.
- [ ] Hard-drop animation (`innovation/drop-animation` @ 9d9054a)

### Tier 3 — Polish + persistence
- [ ] T-spin detection (`innovation/t-spin` @ 40c94f5)
- [ ] Juice: shake/particles (`innovation/juice` @ 0cab591)
- [ ] Heuristic AI (`innovation/ai` @ d285bb9)
- [ ] Settings + high-score persistence; rebindable keys

## Commands
- Build game: `cd build && ./premake5 gmake && cd .. && make` → `bin/Debug/tetris-c`
- Core tests: `make -f tests/tests.mk`
