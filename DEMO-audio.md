# DEMO — innovation #1 + #2: procedural audio engine (+ combo pitch / fanfare)

Gives the game a voice — a full SFX layer synthesized **in memory at startup**, so
the repo ships with **zero audio asset files**. Line clears escalate in pitch with
the combo, and a Tetris / T-spin gets a distinct fanfare. From `INNOVATIONS.md`
#1 (audio) and #2 (combo escalation), built together since #2 needs #1.

## What changed
- `src/app/synth.{hpp,cpp}` — a small DSP module. Builds 16-bit mono `Wave`
  buffers (sine/square/triangle/noise + ADSR envelope) for lock, move, rotate,
  soft-drop, hard-drop whoosh, clear ding, and 3-note arpeggio fanfares. No files,
  no `rand()` (deterministic xorshift noise). **Hazard handled:** the hand-built
  `Wave`s are freed via `synth::freeWave()`, never raylib's `UnloadWave`.
- `src/app/audio.{hpp,cpp}` — the engine. On `init()` synthesizes every SFX,
  loads each via `LoadSoundFromWave`, and builds a round-robin `LoadSoundAlias`
  pool per event so rapid retriggers (soft-drop machine-gun, overlapping clears)
  don't cut each other off. `playClear()` pitches the clear SFX up a semitone per
  combo step (`SetSoundPitch`, `2^(combo/12)`, capped) and routes a 4-line / T-spin
  clear to the fanfare. Cleans up aliases-before-sources in the dtor.
- `src/app/game.{hpp,cpp}` — owns an `Audio` member; `init()` in the ctor; plays
  SFX at the existing event sites (rotate, hard-drop, lock, clear, level-up,
  game-over, win). Lock detection reuses the `Board::piecesLocked()` counter.

Safe when no audio device exists (headless): `init()` no-ops and `play()` is silent.

## How to run
```bash
cd build && ./premake5 gmake && cd .. && make
./bin/Debug/tetris-c
```
Play with sound on: pieces click when they rotate, thock when they lock, a whoosh
on hard drop; clearing lines dings, and **consecutive clears rise in pitch**; a
4-line clear or a T-spin plays a bright fanfare; level-ups and game-over have their
own cues.

## What works — verified with evidence
- Full game builds (premake5 + make) and **launches with the audio device
  initialized** (`AUDIO: Device initialized successfully | miniaudio | PulseAudio`).
- **Synthesis verified numerically** via a throwaway harness: every SFX Wave is
  non-empty and non-silent — e.g. lock `peak=17807 rms=5464`, clear `peak=19113`,
  tetris fanfare `frames=11907 peak=15996`, whoosh `peak=11987` (all OK, ~100%
  non-zero samples).
- **Engine verified**: `Audio.ready() == true` on this box; playing Lock + a
  combo-3 pitched clear + the Tetris fanfare runs without crashing.
- Headless suite unaffected: **93/428 green**.

## What's stubbed
- Nothing external is stubbed. No music (out of PRD scope §2). SFX tuning
  (frequencies/envelopes) is functional but hand-tuned by ear-of-the-math; a pass
  on Greg's speakers may refine a couple of envelopes.

## Next increment
- Danger-responsive audio (a heartbeat that rises with `stackHeight()`) — proposal
  #6; needs the one-line `stackHeight()` core accessor.
- A settings toggle for master volume / mute (pairs with a settings screen).
