# Changelog

All notable changes to this project are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/); entries accumulate under
`[Unreleased]` on `develop` and roll into a dated version at promotion.

## [Unreleased]

### Added
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
- Unified the project into a single repo: un-nested the quickstart scaffold and
  merged it with the planning docs (one `.gitignore`, one README), fresh git
  history, `develop`/`main` branch model.

### Notes
- ADR-0003 records a corrected decision: an earlier draft wrongly claimed raylib
  was apt-installable on Debian; it is not, so raylib is vendored via the
  quickstart instead.
- Documented a bug in the port source (`python-tetris` Ultra mode never drains its
  garbage queue); the C++ port implements the intended behavior (TDD §6).
