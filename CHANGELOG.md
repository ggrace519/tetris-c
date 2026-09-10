# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/); entries accumulate under
`[Unreleased]` on `develop` and roll into a dated version at promotion.

## [Unreleased]

### Added
- **Complete playable game (all three tiers).** A C++/raylib Tetris with feature
  parity to `python-tetris`, on a raylib-free pure-logic core (88 headless test
  cases / 362 assertions) plus a thin raylib app layer, running at 60 FPS:
  - **Tier 1:** 10×20 board, 7-bag randomizer, simplified-SRS wall kicks, ghost
    piece, next preview, soft/hard drop, classic scoring, level/speed progression,
    pause/restart/game-over.
  - **Tier 2:** Marathon/Sprint/Ultra modes with a start menu (mode + difficulty
    select); DAS/ARR horizontal auto-shift with SOCD; lock delay with move/rotate
    reset; combo tracking with bonus; hard-drop stretch animation.
  - **Tier 3:** simplified 3-corner T-spin detection & scoring; per-mode
    high-score persistence (verified across a real process boundary); juice
    (line-count-scaled screen shake + line-clear particles); a heuristic AI demo
    (Seki-2004 four-heuristic, toggle **A**) that plays competently (500 pieces /
    198 lines without topping out in a headless run).
- Fixed a real bug carried from the python-tetris source: Ultra mode's garbage
  injection was dead code there; the port implements the intended behavior.
- Fixed a scoring bug found during the port: a T that merely *fell* (gravity) into
  a blocked slot scored a free T-spin; the gravity move now clears the spin flag.

### Added (earlier)
- Initial planning documents: PRD, technical design (docs/TDD.md), architecture
  decision log (DECISIONS.md), and this changelog. Establishes tetris-c as a
  C++/raylib feature-parity port of the `python-tetris` project, with a
  rendering-agnostic pure-logic core and tiered milestones.
- raylib build scaffold from the **raylib-quickstart** template (premake5 → make,
  raylib vendored and built locally into `bin/Debug/libraylib.a`). No system
  raylib install is needed — Debian does not package raylib.
- Research references saved under `~/.claude/research/`: raylib C API + Linux
  build flags, and the Tetris Guideline (SRS kick tables, scoring, T-spin rules)
  for cross-checking and the optional future SRS upgrade.

### Infrastructure
- **Continuous integration** (`.github/workflows/ci.yml`): every push/PR to
  `develop`/`main` runs a `test` job (headless doctest suite + a gcov coverage
  report, no raylib — the PR gate) and a `build` job that compiles the full game
  via premake5, installing raylib's Linux deps and caching the raylib source/build
  so the fetch+compile only reruns when `premake5.lua` changes.
- **Coverage tooling:** `make -f tests/tests.mk coverage` reports gcov line
  coverage for our own `src/` (core `.cpp` files at 95–100%). Informational, not a
  gate — no baseline history to threshold against yet.
- Filled the largest coverage gap: added tests for `ModeController::reset()` and
  post-win/post-game-over update no-ops (`modes.cpp` 79% → 95%).
- Unified the project into a single repo: un-nested the quickstart scaffold and
  merged it with the planning docs (one `.gitignore`, one README), fresh git
  history, `develop`/`main` branch model.

### Notes
- ADR-0003 records a corrected decision: an earlier draft wrongly claimed raylib
  was apt-installable on Debian; it is not, so raylib is vendored via the
  quickstart instead.
- Documented a bug in the port source (`python-tetris` Ultra mode never drains its
  garbage queue); the C++ port implements the intended behavior (TDD §6).
