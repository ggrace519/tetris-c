# PRD — Tetris (C++ / Linux)

**Status:** Draft · **Owner:** Greg · **Date:** 2026-09-10
**Related:** [docs/TDD.md](docs/TDD.md) · [DECISIONS.md](DECISIONS.md) · sibling project `../python-tetris`

---

## 1. Summary

A native C++ Tetris for Linux (Debian 13), rendered with **raylib**. It is a
**feature-parity port of the existing `python-tetris` project** — the same game,
the same rules and feel, reimplemented idiomatically in C++ with a
rendering-agnostic core so the logic can be unit-tested headlessly.

This is a **solo-maintainer polish project**, not a product launch. The goal is a
crisp, correct, good-feeling single-player Tetris that is a pleasure to run and
easy to keep working on.

> **Naming note:** the directory is `tetris-c` but the implementation language is
> **C++** (per decision on 2026-09-10). It is not a C project. See README.

---

## 2. Goals & non-goals

### Goals
- Reproduce `python-tetris`'s gameplay in C++ with equivalent or better feel.
- Keep **all game rules in a pure-logic core** with zero rendering/audio/OS
  dependency, so the rules are unit-testable without a window (mirrors the
  three-layer separation `python-tetris` already uses).
- Ship the base game first (Tier 1), then port the polished "innovation"
  features that `python-tetris` proved out on branches (Tiers 2–3), each gated.
- Single `apt` dependency for the graphics stack (`libraylib-dev`).

### Non-goals (explicitly out of scope)
These are carried over from `python-tetris`'s **killed-ideas** list — already
considered and rejected for a solo polish project. Do not add without a new
decision:
- **Multiplayer / online / netcode** — needs a server and infra.
- **Machine-learning AI** — heuristic AI is sufficient and more transparent.
- **Mobile / touch port** — different UI and build pipeline.
- **Procedural music generation** — no clear user value here.
- **A second render backend** — raylib only unless the core/render split is ever
  cashed in for a real reason.

---

## 3. Target platform & environment

| Item | Value (verified 2026-09-10) |
|---|---|
| OS | Debian 13, KDE Plasma |
| Session | **Wayland** (`XDG_SESSION_TYPE=wayland`) — raylib abstracts the windowing system; no raw X11 assumptions |
| Compiler | `g++ 14.2` (present) |
| Build | **premake5 → make** via the raylib-quickstart scaffold (premake5 is vendored in `build/`); `make` present; CMake not used |
| Render/input/audio | **raylib**, **vendored + built locally** by the quickstart (`bin/Debug/libraylib.a` already built, `6.1-dev`). Debian does **not** package raylib — so it is built from source, not apt-installed. See DECISIONS ADR-0003. |
| Test framework | **doctest** (single-header, vendored) — see TDD |

---

## 4. Player-facing requirements

### 4.1 Core gameplay (must match python-tetris)
- **Playfield:** 10 columns × 20 rows.
- **Seven tetrominoes** (I, O, T, S, Z, J, L) with the exact shapes/rotation
  states and colors used by `python-tetris` (ported from its `settings.py`).
- **7-bag randomizer** — each bag of 7 is shuffled; fair distribution.
- **Rotation with wall kicks** — clockwise and counter-clockwise, using the
  forgiving kick offsets from `python-tetris` (simplified symmetric table, *not*
  full per-transition SRS — see TDD; true SRS is a documented upgrade path).
- **Ghost piece** showing the landing position.
- **Next-piece preview.**
- **Soft drop** (move down faster while held), **hard drop** (instant, scored).
- **Line clears** with the classic scoring table (single/double/triple/tetris =
  100/300/500/800 × level); hard-drop bonus = 2 × cells dropped.
- **Level progression** — level = total lines / 10 + 1; fall speed increases per
  level down to a floor.
- **Game states:** playing, paused, game-over, restart.

### 4.2 Controls (default; rebindable is a Tier-3 item)
| Action | Key(s) |
|---|---|
| Move left / right | ← / → |
| Soft drop | ↓ |
| Rotate CW | ↑ or X |
| Rotate CCW | Z |
| Hard drop | Space |
| Hold (Tier 2) | C or Shift |
| Pause | P |
| Restart | R |
| Quit | Esc |

### 4.3 Game modes (port of `python-tetris/modes.py`)
- **Marathon** — clear 40 lines to win.
- **Sprint** — clear 10 lines as fast as possible (timed).
- **Ultra** — survive rising garbage; difficulty (easy/normal/hard/expert) tunes
  garbage injection interval.

> **Note on the win targets.** These match `python-tetris`'s `modes.py`, which is
> the parity target. They deliberately differ from the *guideline convention*
> (Sprint is usually 40 lines; Ultra is usually a 2-minute score attack). Parity
> wins here — the numbers above are intentional, not a mistake. If we ever want
> guideline conventions, that's a new decision. Also: Ultra's garbage mechanic is
> **broken in the Python source** and the port fixes it — see TDD §6.

### 4.4 Feel / juice (Tier 2–3, from python-tetris innovation branches)
- **DAS/ARR** delayed-auto-shift with configurable delay and repeat rate.
- **Lock delay** with move/rotate reset.
- **SOCD** resolution (simultaneous opposite directions).
- **T-spin detection** and bonus scoring.
- **Combo** tracking and combo bonus.
- **Hard-drop animation** and **juice** (screen shake, particle burst on clear).
- **Heuristic AI** opponent / demo mode (transparent evaluation, no ML).

### 4.5 Persistence (Tier 3)
- **Settings persistence** — key bindings, chosen mode/difficulty, display prefs.
- **Local high scores** per mode.

---

## 5. Feel targets (acceptance feel, not just correctness)
- Runs at a stable **60 FPS**; gravity is frame-rate-independent (delta-time
  accumulator, same model as python-tetris).
- Input is responsive: a tap moves one cell; a hold auto-repeats after DAS.
- Rotations near walls/floor "just work" via kicks; no visual stutter on lock.
- Hard drop is instant and satisfying; line clears read clearly.

---

## 6. Milestones (tiered — each tier is gated; do not start the next until the
prior passes)

### Tier 1 — Playable base (parity with python-tetris base `board.py` + `app.py`)
Pure core: grid, 7-bag, piece, collision, move, rotate+kicks, hard drop, line
clear, scoring, level/speed, ghost, game-over. raylib app: window, 60 FPS loop,
gravity accumulator, keydown input, playfield + next + ghost + HUD rendering,
pause/restart/game-over. **Headless unit tests for the core.** *Definition of
done: game is playable end-to-end and core tests pass.*

### Tier 2 — Modes + core feel
Marathon / Sprint / Ultra (port `modes.py`). DAS/ARR, lock delay, SOCD, hold
piece, combo tracking, hard-drop animation. Tests for each mechanic (matching
the test counts python-tetris's branches carry).

### Tier 3 — Polish + persistence
T-spin detection & scoring, juice (shake/particles), heuristic AI/demo, settings
& high-score persistence, rebindable keys.

---

## 7. Success criteria
- **Correctness:** core rules unit-tested headlessly, ≥90% coverage on core
  modules (Greg's standard).
- **Parity:** every python-tetris base feature and each ported innovation behaves
  equivalently.
- **Feel:** stable 60 FPS, responsive input, no lock/clear stutter (verified by
  running the real game, not asserted).
- **Maintainability:** core has zero backend dependency; one apt dependency to
  build; modules within Greg's size limits.

---

## 8. Open questions (deferred, do not block the plan)
- **Remote host at push time:** GitHub (`ggrace519`) vs `git.skynet.home` (where
  the sibling `python-tetris` lives). Decide when we first push — does not affect
  these docs.
- **Audio assets:** raylib supports sound; whether to add SFX/music (and from
  where) is a Tier-3 decision, not committed here.
