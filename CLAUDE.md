# CLAUDE.md — tetris-c

Guidance for Claude Code when working in this repository. Read this and the
planning docs before writing code.

## Project

**tetris-c** — a **C++20** Tetris for Linux rendered with **raylib**. It is a
**feature-parity port of the sibling `../python-tetris`** project. All game rules
live in a pure-logic `core` with zero rendering/OS dependency; rendering, input,
and audio live in a thin raylib `app` layer.

> Directory is `tetris-c` but the language is **C++**, not C (see DECISIONS.md
> ADR-0002).

## Read first

- `PRD.md` — scope, modes, controls, tiers, **non-goals** (multiplayer, ML AI,
  mobile, procedural music — all explicitly rejected; don't add them).
- `docs/TDD.md` — module map, core/app boundary, tick model, data representation,
  test strategy, and Prerequisites (the exact `apt install` commands).
- `DECISIONS.md` — ADRs. Consult before changing architecturally significant
  behavior; append a new ADR rather than rewriting.
- The port source of truth is `../python-tetris/tetris_game/` — port its tested
  values (shapes, kick offsets, scoring) verbatim; don't reconstruct from memory.

## Status

**Feature-complete (all 3 tiers).** The full python-tetris parity feature set is
implemented, tested headlessly (80 core test cases / 336 assertions), and the
game runs at 60 FPS:
- **Tier 1** — 10×20 board, 7-bag, SRS-like kicks, ghost, next, soft/hard drop,
  scoring, levels, pause/restart/game-over.
- **Tier 2** — Marathon/Sprint/Ultra modes + start menu, DAS/ARR + lock delay +
  SOCD, combo tracking, hard-drop animation.
- **Tier 3** — T-spin detection (ADR-0006), high-score persistence, juice
  (screen shake + particles), heuristic AI demo (toggle A).

See `docs/PROGRESS.md` for the per-feature log. Optional remaining polish:
rebindable keys / a settings screen.

## Commands

Both the game build and the tests are real and pass.

```bash
# Build raylib + the game → bin/Debug/tetris-c
cd build && ./premake5 gmake && cd ..   # regenerate makefiles after premake5.lua edits
make

# Run the game
./bin/Debug/tetris-c

# Build + run the headless core tests (doctest; NO raylib)
make -f tests/tests.mk

# Line-coverage report for our own src/ (gcov; NO raylib)
make -f tests/tests.mk coverage
```

> CI runs both on every push/PR to `develop`/`main` (`.github/workflows/ci.yml`):
> a `test` job (`make -f tests/tests.mk` + coverage, no raylib — this is the PR
> gate) and a `build` job (premake5 → make, which fetches and compiles raylib and
> so installs its X11/GL/Wayland deps). There is no coverage *gate* — the report
> is informational only.

> The core tests use a standalone `tests/tests.mk`, NOT a premake target — the
> top-level `Makefile` is premake-generated and gitignored, so a hand-added
> `make test` rule there would be clobbered on the next `premake5 gmake`.
> `make clean && make` if an incremental build complains about a stale object.

- **No `apt install` for raylib** — Debian 13 does not package raylib; the
  quickstart vendors and builds it (DECISIONS ADR-0003). Don't add `libraylib-dev`
  to any instructions; it doesn't exist in Debian.
- Verified on this box (2026-09-10): `g++ 14.2`, `make`, and the vendored raylib
  build are present; `cmake` is not used. doctest will be vendored (no apt pkg).
- Game sources are **`.cpp`** (C++). The template's `src/main.c` is C; replacing it
  with `.cpp` and a clean build switches the toolchain to g++ (per quickstart).

## Architecture (target)

Dependencies point downward only: **`app` → `core`**, never up. `core` never
includes a raylib header.

- `src/core/` — pure logic: board/grid, piece + rotation state, collision,
  wall-kick rotation, gravity, line clears, scoring, level/speed, ghost, 7-bag,
  game modes. Unit-tested headlessly. **No raylib.**
- `src/app/` — raylib: window, 60 FPS loop, delta-time gravity accumulator,
  keyboard input mapping, rendering (playfield, ghost, next, HUD), audio, screen
  states (playing/paused/game-over).
- `tests/` — doctest single-header; tests drive `core` directly (fill rows, force
  positions) without a window, mirroring `python-tetris/tests/`.

## Conventions

- Keep modules cohesive and within Greg's size limits; split "god modules".
- Write tests **concurrently** with core code; target ≥90% coverage on `core`.
- "Done" = built and run, with evidence — never assert without running the real
  game / real tests.
- Tiers are **gated**: don't start Tier 2 work until Tier 1 is playable and its
  core tests pass (see PRD §6).
- Keep these docs true to the code: when a change touches a documented command,
  invariant, or path, update the doc in the same change.

## Git (per Greg's global convention)

- Two long-lived branches: **`develop`** (integration/staging) and **`main`**
  (blessed). **No PR ever targets `main` directly** — every feature/fix branch
  merges into `develop`. Promotion `develop → main` is a separate PR, opened only
  when Greg says develop is verified, merged as a merge commit / fast-forward
  (never squash), and tagged on main.
- Never commit directly to `main` or `develop`; branch from `develop` first
  (`<type>/<kebab-summary>`). Branch and commit freely; **pause and ask before
  pushing or opening a PR.**
- Remote is **`github.com/ggrace519/tetris-c`** (public); GitHub default branch is
  `develop`. Before any push/PR, confirm the active gh account is `ggrace519`
  (`gh api user --jq .login`; `gh auth switch` if it's `ggrace-kurv`).
