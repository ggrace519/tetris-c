# Technical Design — tetris-c

**Status:** Draft · **Date:** 2026-09-10 · **Companion to:** [../PRD.md](../PRD.md),
[../DECISIONS.md](../DECISIONS.md)

This document specifies *how* the game is built. The *what* is in the PRD. The
authoritative source for ported values (shapes, kick offsets, scoring, mode
rules) is `../../python-tetris/tetris_game/` — port those verbatim.

---

## 1. Guiding principle: a testable core

The single load-bearing decision (ADR-0001) is a **pure-logic `core` with zero
dependency on raylib or any OS/graphics/audio API.** Everything else — build
system, render backend, test framework — is a swappable leaf. This is the same
separation `python-tetris` uses (`board.py`/`piece.py`/`settings.py` have no
Pygame import; only `app.py` does), and it is why its tests can drive `Board`
directly without a window.

```
        ┌──────────────────────────────────────────┐
        │  app/ (raylib)                             │
        │  window · 60fps loop · input · render · sfx│
        └───────────────────┬──────────────────────┘
                            │  depends on ↓ (one way only)
        ┌───────────────────▼──────────────────────┐
        │  core/ (pure C++20, no external deps)      │
        │  grid · piece · rules · scoring · modes     │
        └───────────────────▲──────────────────────┘
                            │  drives directly
        ┌───────────────────┴──────────────────────┐
        │  tests/ (doctest, headless)                │
        └──────────────────────────────────────────┘
```

**Invariant:** no file under `src/core/` includes a raylib header or any
platform header. Enforced by review (and trivially greppable).

---

## 2. Module map

Ported 1:1 from `python-tetris`, C++-idiomatic. Sizes are targets (Greg's
600-line module limit; these are all far under).

| C++ file | Ports from | Responsibility |
|---|---|---|
| `src/core/constants.hpp` | `settings.py` | Board dims, colors (as plain RGBA structs, no raylib), tetromino shape tables, kick tables, scoring tables, timing constants. Pure data. **Done.** |
| `src/core/piece.{hpp,cpp}` | `piece.py` | `Piece`: shape id, rotation (0–3), x/y; `cells()` returns occupied board cells; `kicks()` returns the kick list for its shape class. **Done.** |
| `src/core/board.{hpp,cpp}` | `board.py` | `Board`: the 10×20 grid, active + next piece, score/level/lines/fall-speed, game-over. Owns all rules: `collides`, `move`, `rotate`, `hardDrop`, `lock`, `clearLines`, `ghostCells`, 7-bag refill. **Done.** |
| `src/core/rng.hpp` | (Python `random`) | Seedable RNG wrapper (`std::mt19937_64`) so 7-bag shuffles are deterministic in tests. **Done.** |
| `src/core/modes.{hpp,cpp}` | `modes.py` | `GameMode` enum + `ModeController` wrapping `Board`: Marathon (40), Sprint (10, timed), Ultra (garbage injection by difficulty). *Tier 2.* |
| `src/app/game.{hpp,cpp}` | `app.py` | Owns the raylib window, the loop, screen state (playing/paused/over), the gravity accumulator, and input→core mapping (input lives here in Tier 1, not a separate file). **Done.** |
| `src/app/render.{hpp,cpp}` | `app.py` draw_* | Draw playfield, locked blocks, active piece, ghost, next preview, HUD (score/level/lines), center messages. **Done.** |
| `src/main.cpp` | `tetris.py` | Thin entry point: construct `Game`, run. **Done.** |

> Input is handled inside `game.cpp` (Tier 1, discrete keys). If Tier 2's DAS/ARR
> state grows the file past readability, split it into `src/app/input.{hpp,cpp}`
> then — premake's `src/**` glob picks new files up automatically.

### Colors without raylib
`core` defines `struct Color { unsigned char r,g,b,a; };` and the piece colors as
those. The `app` layer converts to raylib's `Color` at draw time. This keeps the
palette (ported from `settings.py`) in `core` while `core` stays raylib-free.

---

## 3. Data representation

### 3.1 Grid
`std::array<std::array<Cell, COLS>, ROWS>` where `Cell` is an occupancy: an
`enum class ShapeId` (0–6) plus an "empty" sentinel, or a small `std::optional<Color>`
for garbage/locked color. (python-tetris stores the color or `None`; we store the
shape id so rendering can pick the canonical color and tests can assert cleanly.)
Rows are indexed top(0)→bottom(ROWS-1), columns left(0)→right(COLS-1), matching
the Python code.

### 3.2 Tetromino shapes
Ported **exactly** from `settings.py::SHAPES` — each of the 7 pieces has 4
rotation states, each a list of 4 `(dx, dy)` cell offsets within a 4×4 box.
Represented as `constexpr` tables:

```
// shape → rotation(0..3) → 4 cells of {dx,dy}
struct Cell4 { int dx, dy; };
using Rotation = std::array<Cell4, 4>;
using ShapeTable = std::array<Rotation, 4>;
```

Piece spawn is `x=3, y=0` (as in python-tetris `Piece.__init__`).

### 3.3 Kick tables (see ADR-0004)
Ported verbatim from `settings.py`:

```
KICKS_JLSTZ = {(0,0),(-1,0),(1,0),(0,-1),(-2,0),(2,0)}
KICKS_I     = {(0,0),(-2,0),(1,0),(-1,0),(2,0),(0,-1)}
KICKS_O     = {(0,0)}
```

Rotation tries each offset in order against the target rotation and takes the
first that doesn't collide (python-tetris `Board.rotate_current_piece`). This is
**not** full SRS — see §9 for the optional SRS upgrade (data captured in the
research file).

### 3.4 Scoring
Ported from `settings.py::LINE_SCORES` and `board.py`:
- Line clears: `{1:100, 2:300, 3:500, 4:800}` × `level`.
- Hard drop: `+2 × cells_dropped`.
- Soft drop: not scored in the base (Tier-2 DAS work may add a soft-drop point).
- Level = `total_lines / 10 + 1`.
- Fall speed = `max(MIN_FALL_SPEED, START_FALL_SPEED − (level−1)·0.06)` with
  `START_FALL_SPEED=0.8s`, `MIN_FALL_SPEED=0.08s`.

---

## 4. Game loop & timing (app layer)

Frame-rate-independent gravity via a delta-time accumulator, exactly as
`python-tetris/app.py` does (`fall_timer += dt; if fall_timer >= fall_speed: drop`).

```
InitWindow(); SetTargetFPS(60);
while (!WindowShouldClose() && !quit) {
    float dt = GetFrameTime();
    processInput();                 // discrete key events → core calls
    if (state == Playing) {
        fallTimer += dt;
        if (fallTimer >= board.fallSpeed()) {
            fallTimer = 0;
            if (!board.move(0, +1)) board.lock();   // gravity step
        }
        modeController.update(dt);  // timers, garbage (Tier 2)
    }
    BeginDrawing(); render(); EndDrawing();
}
CloseWindow();
```

- **60 FPS** target via `SetTargetFPS(60)`; `GetFrameTime()` gives `dt`.
- Gravity is time-based, not frame-count-based, so it's stable if a frame is late.
- Locking: a downward move that collides triggers `lock()` (spawn next, check
  game-over). Tier 2 inserts lock-delay here.

---

## 5. Input

### Tier 1 — discrete keydown (parity with python-tetris)
Map single key presses to core calls (raylib `IsKeyPressed`): ←/→ move, ↓ soft
drop (one row), ↑/X rotate CW, Z rotate CCW, Space hard drop, P pause, R restart,
Esc quit.

### Tier 2 — DAS / ARR / SOCD (from python-tetris `innovation/das-lock-delay`)
Held movement keys use **DAS** (delayed auto-shift): first move on press, then
after a `DAS` delay begin repeating every `ARR` interval (both use `IsKeyDown` +
timers in `input.cpp`). **SOCD** resolves left+right held simultaneously (last-
input-wins or neutral — port python-tetris's choice). **Lock delay** with move/
rotate reset lives in the loop, not input, but is driven by the same timers.

Concrete DAS/ARR default values: port from python-tetris's branch; guideline
reference values are in the research file for comparison.

---

## 6. Modes (port of `modes.py`)

`ModeController` wraps a `Board`, delegating rules and layering mode state:
- **Marathon:** win at `lines_total >= 40`.
- **Sprint:** win at `lines_total >= 10`; track elapsed time for the result.
- **Ultra:** no win; inject a garbage row every `interval` seconds where
  `interval` depends on difficulty (`easy 8 / normal 5 / hard 3 / expert 2` s,
  from `modes.py`). Garbage = a full row with one hole, pushed from the bottom.

C++ note: python-tetris uses `__getattr__` delegation; in C++ we make
`ModeController` hold a `Board` and expose the needed methods explicitly (no magic
forwarding). Cleaner and type-safe.

> **Port note — a real bug in the source `modes.py` (Ultra garbage never lands).**
> In `python-tetris/tetris_game/modes.py` (branch `innovation/modes`),
> `_inject_garbage()` only appends a hole-column to `_garbage_queue`; the actual
> drain, `inject_garbage()`, is **never called anywhere** (verified: no reference
> in `modes.py` or `app.py`). So in the Python version Ultra's garbage is queued
> but never pushed onto the board — the mode's core mechanic doesn't work.
> **The C++ port implements the *intended* behavior** (spec above: inject on the
> difficulty cadence), i.e. it fixes the bug. A future maintainer diffing against
> the Python source should expect this deliberate divergence. (Also fixable
> upstream on a branch in `../python-tetris` — flagged to Greg.)

---

## 7. Test strategy

- **Framework:** doctest, **vendored** as `tests/doctest.h` (single header — no
  apt install). See ADR-0005.
- **Scope:** the **`core` library only**, headless. The app layer (raylib window,
  rendering) is verified by running the real game (`/verify`-style), not unit-
  tested — matching how python-tetris tests only `board`, not `app`.
- **Style:** drive `Board`/`Piece`/`ModeController` directly — fill rows, force
  piece positions, seed the RNG — exactly like `python-tetris/tests/test_board.py`.
- **Determinism:** seed `rng` so 7-bag order is reproducible in tests.
- **Coverage target:** ≥90% on `core` (Greg's standard), reported via
  `g++ --coverage` + `gcov`/`lcov` (no extra apt package beyond gcov, which ships
  with g++).

### Test checklist (Tier 1, one `TEST_CASE` group each)
- Piece: `cells()` offsets per rotation; spawn position; kick list per shape.
- Collision: walls (x<0, x≥COLS), floor (y≥ROWS), locked blocks; above-top allowed.
- Move: valid move succeeds, blocked move is a no-op returning false.
- Rotate: succeeds in open space; kicks off a wall; O-piece is a no-op-shape; fails
  when no kick fits.
- Hard drop: lands on floor/stack; adds `2×distance`; locks.
- Line clear: single/double/triple/tetris detected; rows above shift down; count
  and score (×level) correct.
- Level/speed: level rises every 10 lines; fall speed decreases to the floor.
- 7-bag: each 7-piece window contains all 7 exactly once (seeded).
- Ghost: landing cells equal a hard-drop position.
- Game over: spawn-collision sets `game_over`.
- Modes: Marathon win at 40, Sprint win at 10, Ultra garbage injection cadence.

Later tiers add: DAS/ARR timing, lock-delay reset, SOCD, combo counting, T-spin
detection (matching the test counts python-tetris's innovation branches carry).

---

## 8. Build

The build comes from the **raylib-quickstart** scaffold (ADR-0002/0003): raylib is
**vendored** and built locally; **premake5** (bundled in `build/`) generates the
makefiles. There is **no `apt` step for raylib** (Debian doesn't package it) and
**no CMake**.

### 8.1 Prerequisites (verified on this box 2026-09-10)
`g++ 14.2` and `make` are present. raylib does **not** need installing — it is
already vendored under `build/external/raylib-master/` and built into
`bin/Debug/libraylib.a` (7.5 MB `ar` archive, all 7 objects, symbols verified).
premake5 ships in the repo. doctest will be **vendored** as a single header, so no
test package is needed either.

If the vendored raylib tree is ever missing (fresh clone), re-run the quickstart
bootstrap (`build/premake5.lua` fetches it), or build raylib from source per
`~/.claude/research/raylib-cpp-linux.md`. Its build-time deps (`libasound2-dev`,
the `libx*-dev` set, `libgl1-mesa-dev`, `libwayland-dev`, `libxkbcommon-dev`) are
apt-installable — Greg runs those.

### 8.2 Build commands
```bash
cd build && ./premake5 gmake && cd ..   # (re)generate makefiles after premake5.lua edits
make                                     # build the game → bin/Debug/tetris-c
./bin/Debug/tetris-c                     # run
```

Two artifacts share the `core` sources:
- **game** — the quickstart's premake project builds `src/**` (core + app + main)
  and links the vendored raylib. `src/core/*.cpp` and `src/app/*.cpp` are picked up
  automatically by premake's recursive `src/**` glob — no premake edit needed to add
  core/app files.
- **tests** — a **standalone `tests/tests.mk`** (hand-written, invoked as
  `make -f tests/tests.mk`) compiles only `src/core/*.cpp` + `tests/*.cpp` (doctest)
  with g++, **no raylib link** (core is raylib-free, so tests build and run
  headlessly). It is a separate file rather than a premake/`make test` target
  because the top-level `Makefile` is premake-generated and gitignored, so a rule
  added there would be lost on regeneration.

Standard flags: `-std=c++20 -Wall -Wextra`; coverage via `--coverage` for the
`coverage` target.

**raylib link libraries** (the quickstart's generated makefiles already supply
these on Linux; documented here for the test/tooling side): the standard desktop
set is `-lGL -lm -lpthread -ldl -lrt -lX11` alongside the vendored `libraylib.a`.
`-lX11` stays even on Wayland (raylib/GLFW abstract the windowing system). Full API
details (signatures, `KeyboardKey` values, `Color`/`Rectangle` structs,
default-font text) are in `~/.claude/research/raylib-cpp-linux.md`.

### 8.3 Text & color
Base UI text uses raylib's **default font** via `DrawText`/`MeasureText` — no TTF
dependency. `core` colors are plain `struct Color {unsigned char r,g,b,a;}`,
converted at draw time to raylib's `(Color){r,g,b,a}`.

---

## 9. Optional upgrade path: full SRS (Tier 3, not committed)

python-tetris's simplified kicks (§3.3) are the Tier-1 baseline. If T-spins (Tier
3) need guideline-correct behavior, upgrade rotation to **true SRS** — eight
per-transition offset rows for JLSTZ and a separate I-piece table. The exact
tables, the guideline scoring, and the 3-corner T-spin rule are captured verbatim
in **`~/.claude/research/tetris-guideline.md`** so nothing is re-derived from
memory. This would be a new ADR superseding ADR-0004.

Three gotchas that file flags (repeated here because they bite at implementation
time):
- **Negate `y`.** The wiki tables are **y-up**; this engine is **y-down** (§3.1),
  so every kick `y` must be negated when ported.
- **Rotation-state mapping:** our `rotation` 0/1/2/3 (CW) maps to the wiki's
  `0/R/2/L`.
- **T-spin mini→full upgrade:** a mini T-spin is promoted to a full T-spin when
  the successful rotation used the `(1,2)` kick (the last SRS offset) — this is
  the subtle rule most implementations miss, and it's what enables the T-Spin
  Triple. The **Arika I-table on the same wiki page is a decoy** — use the
  Guideline I-table only.

T-spin/combo/B2B scoring values (also in the research file) layer onto Tier-1's
plain 100/300/500/800×level table when those features land.

---

## 10. Traceability to python-tetris

**Port source of truth.** The sibling repo (`../python-tetris`) keeps its base game
on `main` and its polished features on **separate `innovation/*` branches** — the
base `board.py`/`app.py` do **not** contain DAS, T-spin, combos, juice, or AI. So
"feature parity" = base (Tier 1) **plus the union of the innovation branches**
(Tier 2–3). Port each feature from its branch; read the branch diff before porting
(don't reconstruct from memory). Branch refs captured 2026-09-10 (re-`git fetch`
and re-check before porting, as they may advance):

| Feature | python-tetris branch | ref (2026-09-10) | Tier |
|---|---|---|---|
| base game | `main` | `6224714` | 1 |
| challenge modes (marathon/ultra/sprint) | `innovation/modes` | `c4c9676` | 2 |
| DAS/ARR + lock delay | `innovation/das-lock-delay` | `4e91423` | 2 |
| combo tracking | `innovation/combos` | `12dd321` | 2 |
| hard-drop animation | `innovation/drop-animation` | `9d9054a` | 2 |
| T-spin detection | `innovation/t-spin` | `40c94f5` | 3 |
| juice (shake/particles) | `innovation/juice` | `0cab591` | 3 |
| heuristic AI | `innovation/ai` | `d285bb9` | 3 |

> **AI design boundary (not a bug):** `TetrisAI::bestMove` searches rotation ×
> column with a straight vertical drop. Tuck/slide placements (moving a piece
> sideways *under* an overhang after it has dropped) are therefore unreachable by
> the AI — the same limitation as python-tetris's AI. This is an intentional scope
> boundary, not a defect; a fuller search (BFS over reachable resting states)
> would be a future enhancement, not parity work.

> The port source was read on `innovation/modes` (which contains `modes.py`). Note
> local `main` (`6224714`) was **ahead of** `origin/main` (`1e2c204`) and not
> fetched — treat remote state as unverified until a `git fetch`.

### File mapping

| python-tetris | tetris-c |
|---|---|
| `settings.py` (constants, shapes, kicks, scores) | `src/core/constants.h` |
| `piece.py` | `src/core/piece.{h,cpp}` |
| `board.py` (all rules) | `src/core/board.{h,cpp}` |
| `modes.py` | `src/core/modes.{h,cpp}` |
| `app.py` (loop/input/render) | `src/app/game.{h,cpp}`, `src/app/render.{h,cpp}`, `src/app/input.{h,cpp}` |
| `tetris.py` (entry) | `src/main.cpp` (replaces the quickstart template `main.c`) |
| `tests/test_board.py` | `tests/*.cpp` (doctest) |
