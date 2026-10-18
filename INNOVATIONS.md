# Innovation Proposals — tetris-c
*Generated 2026-09-10 · based on commit 49d6b44 · focus: **UI & game experience***

## How this codebase stands today

tetris-c is a **feature-complete, correctly-implemented** C++20/raylib Tetris — a
parity port of `../python-tetris` with a clean pure-logic `core` (ADR-0001: no
raylib in core) and a thin raylib `app` layer, 93 headless tests, CI green. The
*mechanics* are elite for a hobby project: 7-bag, wall kicks, lock delay with
reset cap, DAS/ARR/SOCD, T-spin detection, combo scoring, a Seki-2004 AI demo.

The **experience**, by contrast, is ordinary — and that's the whole opportunity:

- **Zero audio.** No `InitAudioDevice`, no SFX, no music anywhere (verified). The
  parent python-tetris is silent too, so this is greenfield. For a genre defined
  as much by its *sound* as its shapes, silence is the single biggest feel gap.
- **Flat rendering.** Every cell is a solid `DrawRectangle` + a 1px outline, in a
  muted palette, with raylib's default bitmap font. No gradients, bevels, glow,
  shaders, or render targets. It reads as "programmer art."
- **Minimal juice.** A 0.15 s screen shake and one upward particle burst fire on a
  line clear — and nothing else. No lock flash, no line-clear choreography (clears
  are an *instant* array delete), no combo/T-spin/level-up/danger feedback.
- **Fixed 520×600 window.** No resize, fullscreen, or DPI scaling; `kCellPx=30` is
  hardcoded.

Everything below is a *net addition* — none of it is "catch up to the parent."

## What the best in this space are doing

Synthesized from the research file `~/.claude/research/tetris-game-feel-raylib.md`
(primary-source-verified against the vendored raylib 6.1-dev tree; design sources
below are fetched unless marked *reported*):

- **Game-feel canon** — Vlambeer's *The Art of Screenshake* (impact frames,
  hit-pause, screenshake), Jonasson/Purho's *Juice It or Lose It*, GMTK's *Secrets
  of Game Feel* all say the same thing: **escalating, multi-sensory feedback on
  every action** is what separates "moves" from "feels good."
  [Screenshake](https://www.youtube.com/watch?v=AJdEqssNZ-U) ·
  [Juice](https://www.youtube.com/watch?v=Fy0aCDmgnxg)
- **Modern Tetris reference** — TETR.IO ships a **danger state** (verbatim, fetched:
  *"WARN ME WHEN I'M IN DANGER — the board will turn red when your stack is very
  high"*), plus PARTICLE COUNT / ACTION TEXT / STEREO options
  ([FAQ](https://tetrio.github.io/faq/troubleshooting.html)); Tetris 99 flashes red
  near death and escalates the combo meter
  ([HardDrop](https://harddrop.com/wiki/Tetris_99)); Jstris players praise its
  **line-clear delay** for "adding a lot of character"
  ([HardDrop](https://harddrop.com/wiki/Jstris)); Tetris Effect makes the whole
  presentation **reactive** at a 1:1:1 visual/audio/gameplay ratio
  ([Variety](https://variety.com/2019/gaming/features/tetris-effects-development-was-anything-but-zen-like-1203169014/)).
- **raylib gives all of this away** — verified on disk in 6.1-dev: `SetSoundPitch`
  (runtime pitch → combo escalation), `LoadSoundFromWave` over a hand-filled `Wave`
  struct (**zero-asset procedural SFX**), `LoadSoundAlias` (round-robin pools for
  rapid retriggers, each alias independently pitched — verified in `raudio.c:987`),
  a ready-made `bloom.fs`, `reasings.h` (30+ easing fns), and
  `core_window_letterbox.c` (integer-scaled resize).

## Proposals (ranked)

### 1. Give the game a voice — zero-asset procedural audio engine
**Category:** feature · **Impact 5 · Novelty 4 · Effort 4 · Fit 5**

**The idea.** Add a self-contained `app/audio` + `app/synth` module that
**synthesizes all SFX in memory at startup** — no `.wav` files ever ship. A lock
"thock," a soft-drop tick, a rotate blip, a line-clear "ding," a hard-drop whoosh,
and a distinct 4-line "Tetris!" arpeggio, each built as a PCM buffer with an ADSR
envelope, wrapped in a `Wave`, loaded via `LoadSoundFromWave`. Each event gets a
small **`LoadSoundAlias` pool** so machine-gun soft-drops and overlapping clears
don't cut each other off. This is the foundation proposals #2 and #6 build on.

**Inspired by.** raylib `examples/audio/audio_raw_stream.c` (synthesis math) and
`audio_sound_multi.c` (alias pools); the zero-asset constraint is original —
derived from the repo shipping no audio assets and wanting to stay self-contained.
Design: the "lock thock is the game's heartbeat" framing is from the research file.

**Implementation sketch.** New app-layer files, no core change:
- `src/app/synth.{hpp,cpp}` — pure DSP: `Wave tone(freq, ms, waveform, adsr)`,
  `Wave noiseBurst(...)`, `Wave arpeggio({f1,f2,f3}, ...)`. Fills a
  `std::vector<short>` @ 44100/16-bit/mono, returns a `Wave{ .frameCount=N,
  .sampleRate=44100, .sampleSize=16, .channels=1, .data=buf.data() }`.
  **Hazard (from research):** never `UnloadWave` a hand-built Wave — let the vector
  drop after `LoadSoundFromWave` copies the samples.
- `src/app/audio.{hpp,cpp}` — `class Audio { void init(); void play(Sfx, float
  pitch=1); }` owning the alias pools; `Sfx` enum = Lock/Rotate/SoftDrop/HardDrop/
  Clear/Tetris/LevelUp/GameOver.
- `game.cpp`: `InitAudioDevice()` in the ctor; call `audio_.play(...)` at the
  existing event sites — lock (in `updateGravity` after `board.step`), rotate/drop
  in `processPlayInput`, clear in the juice block.

```cpp
Wave w{ .frameCount = (unsigned)n, .sampleRate = 44100,
        .sampleSize = 16, .channels = 1, .data = buf.data() };
Sound thock = LoadSoundFromWave(w);           // raylib copies the samples
Sound pool[8]; pool[0] = thock;
for (int i = 1; i < 8; ++i) pool[i] = LoadSoundAlias(pool[0]);  // shared data, own pitch
```

**Effort.** ~1 day (the synth is the work; ~150–250 lines). **Risk:** tuning SFX to
not grate — mitigate by keeping move/rotate ticks at low `SetSoundVolume`. Audio
init can fail headless (no `/dev/snd`), so guard it and no-op cleanly. *PRD §8
leaves "ship audio" as an uncommitted Tier-3 decision — this proposal is also that
sign-off.*

**First step.** Write `synth.cpp::tone()` and a throwaway `main` that plays one
thock on this box (ALSA is present) — prove a synthesized Wave is audible.

---

### 2. Turn scoring into a *reward curve* — combo pitch-rise + Tetris fanfare
**Category:** feature · **Impact 4 · Novelty 3 · Effort 5 · Fit 5**

**The idea.** Layered on #1: each consecutive line clear plays the clear SFX a
**semitone higher** (`SetSoundPitch(sfx, powf(2, combo/12.f))`), so a combo *sounds*
like a rising run; a 4-line clear and a T-spin trigger a **categorically different
fanfare** instead of a louder single-clear. This is the "juice" that makes players
chase combos, for essentially one line of code plus one extra synthesized sound.

**Inspired by.** `SetSoundPitch` (raylib.h:1711, verified); equal-tempered semitone
= `2^(1/12)`; the "distinct reward tier for a Tetris" convention (classic Tetris /
Tetris Effect 1:1:1). The per-alias-pitch correctness (pitching alias N doesn't
disturb alias N-1 ringing out) was verified in `raudio.c:987`.

**Implementation sketch.** In the `game.cpp` clear-detection block that already
computes `board.lastClearCount()` and `board.combo()`:
```cpp
if (board.lastClearCount() >= 4 || board.lastTSpin() != Board::TSpin::None)
    audio_.play(Sfx::Tetris);                 // the fanfare
else
    audio_.play(Sfx::Clear, powf(2.f, std::min(board.combo(), 12) / 12.f));
```
Cap the combo used so it never goes chipmunk. Pairs visually with #6.

**Effort.** ~1–2 hours on top of #1. **Risk:** trivial; just cap the pitch.

**First step.** Add the `Sfx::Tetris` arpeggio to the synth and wire the `switch`.

---

### 3. Make the line clear an *event* — flash → collapse choreography
**Category:** feature · **Impact 5 · Novelty 4 · Effort 3 · Fit 4**

**The idea.** The most-watched moment in Tetris is currently an instant array
delete. Replace it with a two-phase animation: the cleared rows **flash white for
~2–3 frames**, hold for a short **line-clear delay** (~0.15 s), then **collapse**
as the stack falls — with particles shattering *outward* from each cleared cell.
This is the single biggest *gameplay-feel* upgrade and mirrors a pattern the repo
already uses (the hard-drop animation phase in `board.cpp`).

**Inspired by.** Jstris' line-clear delay ("adds a lot of character" —
[HardDrop](https://harddrop.com/wiki/Jstris)); the hard-drop-anim phase already in
`board.cpp` is the local precedent to copy.

**Implementation sketch.** This one *does* touch core (ADR-0001-safe — plain data
+ a timing phase, exactly like the existing drop animation):
- `core/board`: add a `clearAnim` phase. `lock()`, on a clear, records the cleared
  row indices and enters a "rows pending" state instead of compacting immediately;
  `step(dt)` holds ~0.15 s then compacts. New accessors: `pendingClearRows()`
  (which rows) and `inClearAnim()`.
- `app/render.cpp`: while `inClearAnim()`, draw the pending rows as a white overlay
  whose alpha ramps via `reasings.h EaseCubicOut`. `juice.cpp`: on clear, spawn
  particles at each cleared cell with **outward** velocity (currently upward-only).
- Log an ADR (mirrors ADR-0006-style timing decisions) and add core tests for the
  new phase (pattern already established by the drop-anim tests).

**Effort.** ~1 day (core timing + tests + render). **Risk:** touching core timing —
contained by copying the drop-anim phase design and testing it headlessly.

**First step.** Add `pendingClearRows()`/`inClearAnim()` to core with a test that a
clear now takes N steps to compact, before touching render.

---

### 4. Springy motion everywhere — adopt `reasings.h`
**Category:** DX/feel · **Impact 3 · Novelty 2 · Effort 5 · Fit 5** · *quick win*

**The idea.** Replace the linear `decay = timer/duration` curves (shake falloff,
particle fade) and every HUD/menu motion with **eased** curves from raylib's own
`reasings.h` — `EaseBackOut` (overshoot) for HUD number pops, `EaseCubicOut` for
shake decay and flash alpha, `EaseElasticOut` for a menu entrance. Non-linear
motion is the cheapest thing that makes everything feel deliberate.

**Inspired by.** `examples/shapes/reasings.h` (ships in the vendored tree). Note:
it lives under `examples/shapes/` (gitignored), so it must be **copied** into
`src/app/`.

**Implementation sketch.** Copy `reasings.h` → `src/app/reasings.h` (attribute in
NOTICE — it's raysan5/zlib). In `juice.cpp`, swap the linear shake decay for
`EaseCubicOut`. Add a tiny `Tween` helper for HUD number pops in `render.cpp`.

**Effort.** ~2–3 hours. **Risk:** none — header-only, already vendored; add the
attribution line to NOTICE.

**First step.** Copy the header, swap `juice.cpp`'s shake `decay` to `EaseCubicOut`,
eyeball the difference.

---

### 5. The gateway refactor — one scene `RenderTexture2D` (+ post-shake/zoom)
**Category:** architecture · **Impact 4 · Novelty 3 · Effort 3 · Fit 4**

**The idea.** Draw the whole scene (playfield + HUD) into a single
`RenderTexture2D`, then blit it to the screen with an offset/scale. Screen shake
becomes a **transform on one blit** instead of 30 per-object `gOffX/gOffY` offsets
(cleaner *and* correct — the HUD currently doesn't shake with the board), a board
**zoom-punch on a Tetris** becomes a scale on that blit, and — critically — this is
the **prerequisite for bloom (#7), CRT/chromatic, and pixel-perfect resize (#8)**.

**Inspired by.** raylib `examples/shaders/shaders_postprocessing.c` (the
render-target → blit flow). **Gotcha (from research):** render textures are
Y-flipped — the source rect height must be **negative** in `DrawTexturePro`.

**Implementation sketch.** In `game.cpp`'s draw path: `RenderTexture2D scene =
LoadRenderTexture(kWinW, kWinH)` once; each frame wrap all drawing in
`BeginTextureMode(scene) … EndTextureMode()`, then `DrawTexturePro(scene.texture,
{0,0,kWinW,-kWinH}, destWithShakeAndScale, {0,0}, 0, WHITE)`. Retire `gOffX/gOffY`;
drive shake by nudging `dest`.

**Effort.** ~half a day. **Risk:** the Y-flip and getting the shake/zoom transform
right; contained by following the example verbatim.

**First step.** Wrap the existing draw calls in `BeginTextureMode`/`EndTextureMode`
+ a plain blit; confirm the game looks identical, *then* add shake to the blit.

---

### 6. Escalate what's already there — combo/T-spin visual crescendo + danger state
**Category:** wow · **Impact 4 · Novelty 3 · Effort 4 · Fit 5**

**The idea.** Two-for-one, both feeding on signals the core already (or nearly)
exposes. **(a) Crescendo:** scale the *existing* shake/particles/HUD by combo and
T-spin — `shake = base * lines * (1 + 0.25*combo)`, particle count `*= (1+combo)`,
HUD text pops with `EaseBackOut`. **(b) Danger state:** as the stack nears the top,
a **pulsing red vignette** creeps in and (with #1) a heartbeat SFX rises — the
tension arc the game completely lacks today.

**Inspired by.** Tetris 99 combo meter + red-flash danger
([Nintendo UK](https://www.nintendo.com/en-gb/News/2019/March/Boost-your-block-dropping-abilities-with-our-TETRIS-99-tips--1526705.html));
TETR.IO's verbatim danger option (fetched). The crescendo is pure multipliers on
shipped effects.

**Implementation sketch.** (a) is app-only — multipliers in `juice.cpp`/`render.cpp`
on `board.combo()`/`board.lastTSpin()`. (b) needs **one new core accessor**
`Board::stackHeight()` (max filled row → int, ADR-0001-safe, headless-testable);
app maps height→danger∈[0,1] and draws a red vignette overlay pulsing with
`sinf(GetTime()*k)` (a `DrawRectangleGradientV` frame, or a cheap vignette uniform
if #5 is in).

**Effort.** ~half a day. **Risk:** low; the one core addition is a trivial int with
an obvious test.

**First step.** Add `stackHeight()` + a test; draw the red vignette driven by it.

---

### 7. Neon — bloom/glow post-process (flagship visual)
**Category:** wow · **Impact 4 · Novelty 4 · Effort 3 · Fit 3** · *depends on #5*

**The idea.** Wrap the scene blit in raylib's ready-made **bloom shader** so bright
cells, line flashes, and the "Tetris!" pop **glow** — the single biggest leap from
"hobby clone" to "art-directed." Optionally a subtle **chromatic-aberration** pass
that spikes on hard-drop/clear then decays, making impacts pop.

**Inspired by.** raylib `bloom.fs` + `shaders_postprocessing.c` (on disk). TETR.IO
is *reported* (not fetch-verified) to expose bloom/chromatic-aberration power
settings — precedent the effect belongs in a Tetris.

**Implementation sketch.** Needs #5's render target. `LoadShaderFromMemory(0,
bloomSrc)` (embed the `.fs` as a string literal → asset-free); wrap the scene blit
in `BeginShaderMode`. **Fix the stock shader (from research):** its
`const vec2 size = vec2(800,450)` must become a `uniform vec2 size` set via
`SetShaderValue`, or the glow is mis-scaled for our 520×600.

**Effort.** ~half a day *after* #5. **Risk:** GLSL 330 desktop-only (fine here);
tuning glow so it's tasteful not garish.

**First step.** Land #5, then blit through the bloom shader with `size` uniformized.

---

### 8. Look professional at any size — integer-scaled pixel-perfect resize
**Category:** DX/polish · **Impact 3 · Novelty 2 · Effort 4 · Fit 4** · *depends on #5*

**The idea.** Render at the fixed 520×600 virtual resolution (the #5 render texture)
and scale to the window by the **largest integer factor** with letterbox bars —
crisp at any size and fullscreen, no half-pixel shimmer. Add `FLAG_WINDOW_RESIZABLE`
+ fullscreen toggle. Matters especially on this box (Wayland/HiDPI).

**Inspired by.** raylib `core_window_letterbox.c` / `core_smooth_pixelperfect.c`
(on disk).

**Implementation sketch.** `SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT)`
(the flag line is already at `game.cpp:10`). Compute `scale = floor(min(
GetRenderWidth()/kWinW, GetRenderHeight()/kWinH))`; center the blit with black bars;
`SetTextureFilter(scene.texture, TEXTURE_FILTER_POINT)`. **Map menu mouse coords
back** through the scale (the menu is keyboard-driven today, so low-risk).

**Effort.** ~2–4 hours after #5. **Risk:** mouse mapping if the menu ever goes
mouse-driven; negligible now.

**First step.** Add the resizable flag + integer-scale blit; drag the window and
confirm crisp scaling.

---

### 9. Kill the programmer-art look — gradient + beveled blocks
**Category:** feature · **Impact 3 · Novelty 2 · Effort 5 · Fit 5** · *quick win*

**The idea.** Replace flat `DrawRectangle` cells with a **vertical gradient** sheen
+ a light top-left / dark bottom-right **bevel** (a 3D block for two thin rects),
and swap the flat background for a `DrawRectangleGradientV` whose hue shifts by
`level`. Changes the entire look, every frame, with **no shader and no render
target** — pure shape calls in one function.

**Inspired by.** raylib shapes API; the "flat rects read as programmer art"
observation is from the survey.

**Implementation sketch.** Rewrite `drawCell()` in `render.cpp`:
`DrawRectangleGradientV(x,y,w,h, lighter(base), base)` → 2 bevel rects → 1px inner
highlight. Background: one `DrawRectangleGradientV` tinted by `board.level()`.

**Effort.** ~3–4 hours. **Risk:** none — no new subsystems; purely local to
`render.cpp`.

**First step.** Change `drawCell` to a gradient + bevel; compare side by side.

## Killed ideas (and why)

- **Port rendering to a full ECS / retained-mode UI toolkit** — a rewrite in
  disguise; the immediate-mode raylib draw is the right size for this game.
- **Procedural *music* generation** — explicitly a PRD §2 non-goal. (Procedural
  *SFX* is in scope and is proposal #1.)
- **Online multiplayer / netcode** — PRD non-goal; enormous scope; not "experience
  polish."
- **Replace the AI with an ML/RL agent** — PRD non-goal (no-ML-AI); the Seki
  heuristic is fine and out of the UI scope of this pass.
- **Ship a bundled TTF/SDF font for the HUD** — breaks the zero-asset property for a
  medium payoff; a draw-twice **drop shadow** on the default font gets 70% of the
  legibility win for free (folded into #9's polish, not its own proposal).
- **"Add tests / add CI / add docs"** — already done this session (93 tests, CI
  green); no severe gap to justify a proposal.

## Suggested order of attack

Start with the **quick wins that need no refactor** and stack cleanly: **#4
(reasings)** → **#9 (gradient blocks)** → **#1 (audio)** → **#2 (combo pitch +
fanfare)**. That sequence alone transforms the perceived feel in ~2 days and each
piece is independently shippable. Then do the **gateway refactor #5 (render
target)**, which unlocks the flagship visuals **#7 (bloom)** and **#8 (resize)**.
**#3 (line-clear choreography)** and **#6 (crescendo + danger)** are the highest-
ceiling *feel* items and can slot in any time — #3 touches core timing (do it when
you can give it an ADR + tests), #6's danger half needs the one-line
`stackHeight()` core accessor. Build order for this pass (below): **#4, #9, #1, #2**
first — the four highest impact-per-effort items, each on its own branch.
