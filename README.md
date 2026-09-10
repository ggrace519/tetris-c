# tetris-c

A native **C++** Tetris for Linux, built with [raylib](https://www.raylib.com/).
All game rules live in a pure-logic core with no rendering dependency, so the
gameplay is fully unit-tested; raylib handles only the window, input, and drawing.

> Despite the `-c` in the name, this is a **C++20** project.

## Features

- Standard 10×20 playfield: seven tetrominoes, 7-bag randomizer, ghost piece,
  next-piece preview, wall-kick rotation, soft/hard drop, scoring, and level
  progression.
- **Modes:** Marathon (clear 40 lines), Sprint (clear 10 lines, timed), and Ultra
  (survive rising garbage) — pick a mode and difficulty from the start menu.
- **Game feel:** DAS/ARR horizontal auto-shift with SOCD handling, lock delay with
  move/rotate reset, combo tracking, and a hard-drop animation.
- **Extras:** T-spin detection and bonus scoring, screen shake and line-clear
  particles, per-mode high scores saved between sessions, and a heuristic AI that
  can play for you (press **A**).

## Controls

| Action | Key(s) |
|---|---|
| Move | ← / → (hold for auto-shift) |
| Soft drop | ↓ |
| Rotate | Z (CCW) · X or ↑ (CW) |
| Hard drop | Space |
| Pause / Restart / Menu | P / R / M |
| AI demo (autoplay) | A |
| Quit | Esc |

In the menu: ↑/↓ pick mode, ←/→ pick difficulty, Enter/Space to start.

## Building (Linux)

The build uses the vendored [premake5](https://premake.github.io/) that ships in
`build/` and pulls raylib automatically — no system install or CMake required.

```bash
cd build && ./premake5 gmake && cd ..   # generate the makefiles (first build only)
make                                     # build → bin/Debug/tetris-c
./bin/Debug/tetris-c                     # run
```

Re-run `./premake5 gmake` only if you change `build/premake5.lua`. Build output
(`bin/`, `obj/`) and the fetched raylib tree are gitignored.

### Tests

The game rules are covered by a headless test suite (no window required):

```bash
make -f tests/tests.mk
```

## Project layout

```
tetris-c/
├── src/
│   ├── core/     pure game logic — grid, pieces, rules, modes, AI (no raylib)
│   └── app/      raylib layer — window, game loop, input, rendering, effects
├── tests/        headless unit tests for the core
├── build/        premake config + build scripts
└── resources/    assets
```

## Credits & license

Scaffolded from [raylib-quickstart](https://github.com/raylib-extras/raylib-quickstart)
by Jeffery Myers (CC0 1.0). raylib is licensed under zlib/libpng.
