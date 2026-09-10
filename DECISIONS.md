# Architecture Decision Log — tetris-c

This file records significant architectural and design decisions. Each entry is
an ADR: context, decision, consequences. Append new entries; don't rewrite old
ones (supersede instead).

---

## ADR-0001 — Pure-logic core separated from rendering/input/audio

**Date:** 2026-09-10 · **Status:** Accepted

**Context.** A game is hard to test if its rules are entangled with a window and
an event loop. The sibling `python-tetris` already solved this with a three-layer
split (pure `settings`/`piece`/`board`, then a Pygame `app`), and its tests
manipulate `Board` state directly with no Pygame running.

**Decision.** Build a **`core` static library with zero dependency on raylib or
any OS/graphics/audio API.** All Tetris rules — grid, pieces, collision,
rotation+kicks, gravity, line clears, scoring, modes — live in `core`. Rendering,
input, and audio live in an `app` layer that depends on `core` and on raylib.
`core` never includes a raylib header.

**Consequences.**
- Core is unit-testable **headlessly** — no display needed, so coverage is real
  and CI-friendly.
- The render backend is a **swappable leaf**: choosing raylib is cheap to reverse.
- The app layer stays thin (window, loop, draw, input mapping) — the shape
  `python-tetris`'s `app.py` already has.
- One rule: dependencies only point downward (app → core), never up.

---

## ADR-0002 — Language C++17; build via the raylib-quickstart scaffold (premake5 → make)

**Date:** 2026-09-10 · **Status:** Accepted (supersedes the original
"hand-written Makefile" decision)

**Context.** The request is a C++ remake. `g++ 14.2` and `make` are present;
`cmake` is **not** installed (verified 2026-09-10). Greg then set the project up
from the **[raylib-quickstart](https://github.com/raylib-extras/raylib-quickstart)**
template (raylib-extras, CC0), which vendors raylib and drives the build with a
bundled **premake5** that generates makefiles. A working `libraylib.a` is already
built under `bin/Debug/`.

**Decision.** Use **C++17**, and adopt the **quickstart's premake5 → make build**
rather than hand-rolling a Makefile or introducing CMake. premake5 ships in the
repo (`build/premake5`), so there is no build-tool install step. Game sources are
`.cpp` (the template's `src/main.c` is replaced by our code).

**Consequences.**
- Build is `cd build && ./premake5 gmake && cd .. && make` → output in `bin/`.
- No CMake, no `apt` build-tooling; premake is vendored.
- The `core`/`app` split (ADR-0001) is realized inside `src/` and wired through
  `build/premake5.lua`; the headless test target is added there too.
- Trade-off: the scaffold carries some Windows/macOS cruft (`.bat` files,
  `premake5.exe`/`.osx`, a large VisualStudio `.gitignore`). Kept for now — it is
  harmless and preserves cross-platform build scripts; prune later if desired.

---

## ADR-0003 — Render / input / audio backend: raylib (vendored, not apt)

**Date:** 2026-09-10 · **Status:** Accepted · **Amended 2026-09-10**

**Context.** Options weighed: **raylib** (bundles window, input, audio, text,
shapes), **SDL2 + ttf/mixer/image**, **ncurses** (a different product). Greg chose
raylib.

> **Correction (important, kept for the record).** The first draft of this ADR
> claimed raylib was "one apt package (`libraylib-dev`, candidate 5.5)". That was
> **wrong and unverified** — a fabricated environment fact. Verified 2026-09-10:
> **Debian 13 does NOT package raylib** (`apt-cache policy libraylib-dev` returns
> nothing; only the unrelated `xraylib` exists). SDL2's four dev packages *do*
> exist in apt. So the "one apt install vs four" argument that originally favored
> raylib was **inverted**. The decision was re-examined on the true facts.

**Decision.** Keep **raylib**, obtained by **vendoring + building from source via
the quickstart** (not apt). The quickstart fetches raylib into
`build/external/raylib-master/` and builds it into `bin/Debug/libraylib.a`, so the
Debian packaging gap is a non-issue — raylib installs itself into the project.
This preserves raylib's small, game-focused API and built-in text/audio while
matching Greg's explicit choice.

**Consequences.**
- No system raylib install; the library is local to the repo (build output and the
  vendored source tree are gitignored).
- raylib stays confined to the `app` layer (ADR-0001), so a switch to SDL2 (which
  *is* apt-installable) remains cheap if ever wanted.
- Vendored raylib is currently **`6.1-dev`** (quickstart pulls raylib master). If a
  reproducible stable build matters, pin a release tag (5.5 / 6.0) in
  `build/premake5.lua` — a small follow-up, not a blocker.
- Text uses raylib's built-in default font (no TTF dependency).

---

## ADR-0004 — Wall-kick system: port python-tetris's simplified table first

**Date:** 2026-09-10 · **Status:** Accepted

**Context.** `python-tetris` does **not** implement full SRS. It uses a single
symmetric 6-offset kick list per piece class (`KICKS_JLSTZ`, `KICKS_I`,
`KICKS_O`) tried in order on any rotation. True SRS uses eight per-transition
offset tables and is what the modern Guideline specifies. The decided scope is
**feature parity with python-tetris**, not guideline-faithful SRS.

**Decision.** Port `python-tetris`'s **simplified kick table verbatim** as the
Tier-1 rotation system. Treat **full SRS as a documented, optional Tier-3
upgrade**, not a Tier-1 requirement. The exact SRS tables are captured in the
research file so the upgrade needs no re-derivation.

**Consequences.**
- Parity is exact and low-risk; the tested values come straight from a working
  game.
- T-spin detection (Tier 3) is cleaner under true SRS; if T-spins feel wrong on
  the simplified kicks, upgrading to SRS becomes the natural prerequisite — decide
  then, in a new ADR.

---

## ADR-0005 — Test framework: doctest (single-header)

**Date:** 2026-09-10 · **Status:** Proposed (pending first test run)

**Context.** doctest, Catch2, and gtest are all in Debian apt (doctest 2.4.11,
Catch2 3.7.1, gtest 1.16). The core is pure C++ with simple assertions; compile
speed and zero setup matter for a solo project.

**Decision.** Use **doctest** as a **vendored single header** (`doctest.h` in
`tests/`), so tests need no apt package and compile fast. Test the `core` library
only; the app layer is verified by running the real game.

**Consequences.**
- No test-framework install step; `make test` builds and runs core tests.
- Status stays **Proposed** until the first `make test` actually runs and passes
  (Greg's rule: a documented command that hasn't run is not verified).
