# tetris-c

A native **C++** Tetris for Linux, built on [raylib](https://www.raylib.com/)
via the [raylib-quickstart](https://github.com/raylib-extras/raylib-quickstart)
template (raylib is vendored and built locally — no system install). A
feature-parity port of the sibling [`python-tetris`](../python-tetris) project,
with all game rules in a pure-logic core that is unit-tested headlessly.

> **Language note:** the directory is named `tetris-c`, but this is a **C++20**
> project, not C (see [DECISIONS.md](DECISIONS.md), ADR-0002).

## Status

**Playable and feature-complete.** All three tiers are implemented, unit-tested
headlessly (80 test cases / 336 assertions), and the game runs at 60 FPS.

Docs:
- [docs/PROGRESS.md](docs/PROGRESS.md) — per-feature build log (what's done).
- [PRD.md](PRD.md) — scope, modes, tiers, non-goals.
- [docs/TDD.md](docs/TDD.md) — technical design: module map, core/render split,
  tick model, data representation, test strategy, build.
- [DECISIONS.md](DECISIONS.md) — architecture decisions (ADRs).
- [CHANGELOG.md](CHANGELOG.md) — changes, newest under `[Unreleased]`.

## Features

- 10×20 Tetris: seven tetrominoes, 7-bag randomizer, ghost piece, next-piece
  preview, wall-kick rotation, soft/hard drop, scoring, level progression.
- **Modes:** Marathon (40 lines), Sprint (10 lines, timed), Ultra (survive
  garbage) — with a start menu to pick mode + difficulty.
- **Feel:** DAS/ARR + SOCD input, lock delay (move/rotate reset), combo tracking,
  hard-drop animation.
- **Polish:** T-spin detection & scoring, screen shake + line-clear particles,
  per-mode high-score persistence, and a heuristic AI demo (press **A**).

## Controls

| Action | Key(s) |
|---|---|
| Move | ← / → (hold for DAS/ARR auto-shift) |
| Soft drop | ↓ |
| Rotate | Z (CCW) · X or ↑ (CW) |
| Hard drop | Space |
| Pause / Restart / Menu | P / R / M |
| AI demo (autoplay) | A |
| Quit | Esc |

Menu: ↑/↓ pick mode, ←/→ pick difficulty, Enter/Space start.

## Build (Linux)

raylib is **vendored** under `build/external/raylib-master/` (fetched by the
quickstart bootstrap — not committed) and built into `bin/Debug/libraylib.a`.
The build uses **premake5 → make** (no CMake, no `sudo apt install` for raylib).

```bash
cd build
./premake5 gmake      # generate the makefiles (already done once; re-run after
                      #   editing build/premake5.lua)
cd ..
make                  # build; output goes to bin/Debug/
./bin/Debug/tetris-c  # run
```

- **C++:** the game is C++ — source files are `.cpp`. (The template ships a C
  `src/main.c`; our code replaces it. Per the quickstart, renaming to `.cpp` and
  doing a clean build switches the toolchain to g++.)
- **Resources** live in `resources/`; the template's `resource_dir.h` helper sets
  the working directory to it at startup.
- Build output (`bin/`, `obj/`) and the vendored raylib tree are gitignored.

See [docs/TDD.md](docs/TDD.md) §8 for how the pure-logic `core` (no raylib) and
the raylib `app` layer are compiled and how the headless tests build.

## Layout

```
tetris-c/
├── PRD.md · docs/TDD.md · DECISIONS.md · CHANGELOG.md · CLAUDE.md
├── build/                    # premake5 + build scripts (raylib fetched into external/)
├── src/                      # game source (currently the quickstart template main.c)
│   ├── core/   (planned)     # pure logic: grid, piece, rules, modes — NO raylib
│   └── app/    (planned)     # raylib: window, loop, input, rendering
├── tests/      (planned)     # doctest-based headless core tests
├── resources/                # assets (png, sounds, …)
└── bin/, obj/                # build output (gitignored)
```

## Credits / license

Scaffolded from **raylib-quickstart** by Jeffery Myers (CC0 1.0). raylib itself is
zlib/libpng-licensed. Project code license: TBD.
