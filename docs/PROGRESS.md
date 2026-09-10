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
- [x] Hard-drop animation (`innovation/drop-animation` @ 9d9054a) ✅
      Core: hardDrop() credits score + starts a 0.08s stretch animation (no
      instant lock); step() advances it and locks on completion; move/rotate/
      hardDrop are no-ops mid-anim. App: interpolated piece + trail. +8 tests.

**Tier 2 COMPLETE** — modes, DAS/ARR+lock delay, combos, drop animation.

### Tier 3 — Polish + persistence
- [x] T-spin detection (`innovation/t-spin` @ 40c94f5) ✅ (see ADR-0006)
      Core: simplified 3-corner rule on a rotated T (spinAxis tracked per Piece);
      >=3 corners blocked = Full, 2 = Mini; bonus mini100/single200/double400/
      triple800 * level. HUD shows T-SPIN!. +6 tests. Matches python-tetris's
      simplified detector (not guideline SRS) per the parity scope.
- [x] Juice: shake/particles (`innovation/juice` @ 0cab591) ✅
      Core: Board::lastClearCount() signal. App: Juice (screen shake 0.15s scaling
      with lines + particle burst on clears), pure math unit-tested headlessly
      (juice.cpp has no raylib dep). +5 tests. Shake offsets the playfield draw;
      particles integrate with gravity and fade.
- [x] Heuristic AI (`innovation/ai` @ d285bb9) ✅
      Core TetrisAI: Seki(2004) 4-heuristic eval (aggregate height, complete lines,
      holes, bumpiness) over all rotation x column placements; difficulty tunes
      error rate + decision delay. App: toggle A for AI demo/autoplay. +9 tests.
      Verified end-to-end: expert AI placed 500 pieces w/o topping out, 198 lines.

**Tier 3 COMPLETE** — T-spin, persistence, juice, AI. (rebindable keys optional.)
- [x] High-score persistence ✅
      Core HighScores (per-mode best score + Sprint best time) serialized to
      highscores.dat; loaded on start, submitted+saved on game end. Verified by a
      CROSS-PROCESS round-trip (write in one process, read in another). HUD + menu
      show BEST. +6 tests.
- [ ] (optional) rebindable keys / settings screen

## Commands
- Build game: `cd build && ./premake5 gmake && cd .. && make` → `bin/Debug/tetris-c`
- Core tests: `make -f tests/tests.mk`
