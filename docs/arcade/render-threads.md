# Render threads (software renderer)

**Read this before touching** `r_threads.c`/`.h`, the `R_TLS` macro in `doomdef.h`, any
file-scope variable in `r_main.c`, `r_bsp.c`, `r_segs.c`, `r_plane.c`, `r_things.c` or
`r_draw.c`, the view loop in `D_Display`, or `R_Use_Render_BSP` / `R_Use_Play_BSP` -- and
before trying to make the software renderer faster: the drawers' inner loops in `r_draw8.c`,
`R_Init_color12_translate`, and the ranked list of what is still untried are under
"Single-thread speed".

The cabinet draws one viewport per panel — up to four a frame — and each writes into its own
cell of the screen. They share nothing but read-only level data, so they can be drawn at the
same time on different cores. Cvar **`render_threads`**, values `Auto`, `1`, `2`, `3`, `4`.

## STATUS: still opt-in. Do not raise `render_threads` above 1 yet.

**Default `1`, the stock serial renderer, and it must stay there for now.**

Where it stands after a ThreadSanitizer pass:

| | |
| --- | --- |
| Crashes | **none.** Every map that used to crash is clean, coronas included. |
| doom2 MAP01, MAP07 | threaded output is **bit identical** to serial, repeatedly |
| doom2 MAP11, MAP15, MAP29 | no crash, but the picture still differs from serial, and differs run to run |
| ThreadSanitizer | **0 races involving a render worker** on MAP01 and on MAP11 |
| Serial path | unchanged, and measures the same as before the feature existed |
| `make smoke` | 5/5 with the default |

**The Pi ran with it above 1 anyway and found the lump-lifetime crash** — see
"6. A lump let go while another thread is still drawing from it". That one is
fixed. It is worth reading even if you never touch this file, because the
mistake was reasoning about a lock instead of about how long a pointer is held.

The remaining fault is on maps with sky and open space, it is timing-dependent
(two threaded runs of MAP11 differ from each other while two serial runs are
identical), and **ThreadSanitizer does not see it**. **MAP01 shows it too** with
four column bands, which the table above does not say: three runs of one binary
gave two distinct pictures, differing in ~240 pixels (0.03%) scattered over the
upper half of the view. One serial run repeats exactly. Measured with
`-nomonsters` and `framerate_cap 35`, so it is not the two noise sources in
"A screenshot of a live level is not reproducible"; and it is identical in the
stock binary, so it predates the lump fix. That is not a
contradiction: TSan only reports interleavings it actually observes, and a
sanitised run covers a small fraction of the frames an optimised one does. The
next step is more TSan time on MAP11/15/29 specifically -- repeated runs, and
longer ones -- rather than more reasoning.

**`./tools/build.sh --tsan`** builds the sanitizer tree (needs `libtsan`; on
Fedora `sudo dnf install libtsan`). It is deliberately *not* a build
dependency -- the cabinet and the Pi never need it. It writes its own
make_options, builds into `svn1749/tsan/bin`, leaves the real binary alone, and
runs `make depend` serially first so the parallel-dep trap cannot bite.

## Why the software renderer only

Measured on the development laptop (Ryzen 5 2500U, Vega 8) at 1366x768, MAP07, four players.
The budget at 60fps is 16.7 ms.

| renderer | CPU time rendering, per frame | worst frame |
| --- | --- | --- |
| OpenGL | 0.28 ms | 1.0 ms |
| Software | 9.9 ms | 14.5 ms |

**The OpenGL renderer spends about 1.5% of the frame budget on the CPU.** It is bounded by the
GPU and the buffer swap, and threading cannot touch either. It would also be much harder:
`HWR_Subsector` calls `HWD.pfnDrawPolygon` from inside the BSP walk, and a GL context belongs
to one thread, so threading it means rebuilding `hw_main.c` around a deferred draw-command
buffer. There is no measurable reason to.

The software renderer is the one that matters, and on the **Raspberry Pi 3 Model B** —
quad-core Cortex-A53 at 1.2 GHz, roughly 5-8x slower per core — it matters a great deal. The Pi is also VideoCore IV,
so `r_opengl`'s fixed-function `glBegin`/`gluBuild2DMipmaps` path only runs through Mesa's slow
compatibility layer: on a Pi the software renderer is effectively the only renderer.

Measured gain, same scene, four views: **9.6 ms serial → 2.8 ms with four threads, 3.4x.**
Worst frame 14.3 ms → 4-8 ms. The serial path is unchanged (see `R_TLS` below), and measures
the same as it did before the feature existed.

## Column bands: one view across every core

Per-view threading does nothing for a single player, which on a four-panel
cabinet is the common case: `D_NumViews()` may be 4, but the dispatch skips
panels nobody joined on, so one player is one view is one thread. Bands are the
answer — cut the one view into vertical slices, one per core.

**Measured, single player, 1024x768 software:**

| map | serial | 4 bands |
| --- | --- | --- |
| MAP07 | 10.56 ms | **2.79 ms** |
| MAP11 | 9.66 ms | **2.45 ms** |

About 3.8x, slightly better than per-view's 3.4x, because the duplicated BSP
walk is cheap next to the drawing. Per-view is still used whenever there is
more than one view: each thread then walks the tree once for its own view
instead of every thread walking all of it.

It rests on `R_Clear_ClipSegs`, which already marks everything outside the view
as solid so the BSP walk clips itself to the screen; a band thread marks
everything outside its *band* instead, and walls, floors and ceilings clip
themselves with no other change. Only the sprites needed telling, because they
clamp their x range against the view width directly — `rdraw_band_x1/x2`
(`r_draw.c`), reset to the whole view by `R_Set_View_Window` so nothing can
inherit a stale band, narrowed after it by `R_Set_Render_Band`.

### Bands are NOT bit-identical to serial, and cannot be

This is the important thing to know before testing them, and it cost a long
detour to work out.

The span and wall drawers step incrementally — `ds_xfrac += ds_xstep` along a
span, `rw_scale += rw_scalestep` along a wall. A span split at a band edge
recomputes its texture coordinate *exactly* at the split, where an unsplit span
would have accumulated hundreds of fixed-point steps to reach the same column.
The two differ by the accumulated truncation, so about **1% of pixels sample an
adjacent texel**. GZDoom's banded software renderer has the same property.

Measured, MAP01, against the serial frame:

| | pixels differing | at a band edge |
| --- | --- | --- |
| 2 bands | 1.34% | 0.6% of the differences |
| 4 bands | 2.41% | 1.8% of the differences |

**The differences are diffuse, not at the seams** — which is exactly what
distinguishes rounding from a coverage bug. A real coverage bug would pile up
at the band edges or delete whole objects.

So the acceptance test for bands is **not** the bit-for-bit comparison used for
per-view threading. It is:

1. no crash,
2. ThreadSanitizer clean of worker-involved races,
3. the pixel difference small *and diffuse* — if it concentrates at the band
   edges, that is a seam and a real bug.

The way to check 3 is to dump the frame and diff it, not to checksum it. A
checksum only says "different", which is what sent this down a blind alley:
the bands were working the whole time and the test was wrong.

### Two races bands introduced that per-view never had

Both were caught by ThreadSanitizer, and both come from the same thing: with
bands **every thread is drawing view 0**, so anything keyed on the view index
is suddenly shared.

- `last_viewmobj[pind]` (`R_SetupFrame`). Indexed by `pind`, which made it safe
  per view — a comment in the code even said so — but with bands every thread
  has `pind == 0`. The whole check is main-thread-only now.
- `player->mo->flags &= ~MF_NOSECTOR` at the end of `R_RenderPlayerView`: a
  read-modify-write on a shared mobj, and `R_DrawPSprite` *reads* those same
  flags for the invisibility check. Guarding it to the main thread was not
  enough, because the main thread runs it while the workers are still drawing.
  It has moved to `D_Display`, after the join.

## How it works

`R_TLS` (`doomdef.h`) is `__thread` when `RENDER_THREADS` is defined and **nothing at all** when
it is not, so a build without the feature is not merely equivalent to the old one, it is the old
one — the storage class of every renderer global goes back to what it was.

Every mutable global the software renderer *writes while drawing a frame* carries `R_TLS`, so
each thread gets its own copy and the renderer itself needed almost no changes. `D_Display`
hands views 1..N-1 to workers **before** drawing view 0 on the main thread, so all of them are
in flight together, then joins them before anything else touches the screen buffer.

The per-frame pools (`drawsegs`, `vissprites`, `openings`, the visplane pool, the drawseg
memory pools) all use plain `malloc`/`realloc`/`calloc`, which are thread-safe, and they all
grow on demand from NULL — so a worker allocates its own on first use with no extra code.

## How it ended on the Pi: read `present` as a wait, not a cost

The Pi 3 finished at 640x350, software drawmode, `draw8bpp` on, four column
bands:

```
FRAME 640x350 8bpp 17.37ms (58 fps) = tics 0.64 (4%) views 5.39 (31%) present 10.81 (62%) hud/other 0.53 (3%)
```

`present` at 62% looks like the bottleneck. **It is not.** Watch it against
`views` across samples:

| views | present | sum |
| --- | --- | --- |
| 5.14 | 11.06 | 16.20 |
| 5.39 | 10.81 | 16.20 |
| 5.93 | 10.50 | 16.43 |
| 6.52 | 10.39 | 16.91 |

They are **anti-correlated and sum to a constant**. As the drawing gets
slower the present gets shorter by the same amount. That is a fixed frame
budget, not a cost: `SDL_RenderPresent` is blocking for the 60Hz refresh, and
`present` is absorbing whatever slack is left.

**Real work per frame is `tics + views + hud` — about 6.5ms, roughly 150fps of
capability**, displayed at 57 because the panel is 60Hz.

> **Corrected 2026-09-10: the conclusion above is only half right.** The
> anti-correlation is real, and with vsync on `present` does absorb the slack.
> But it is **not only a wait**. perfchart times the present with vsync off
> (`-timedemo` clears `cv_vidwait`), and on the Pi 3 it is still the largest
> part of every frame: 7.5 ms of 13.8 at 640x350 against 5.0 for the views,
> and 28 ms of 46 at 1280x960. That covers the `draw8bpp` expansion, the texture
> upload and the scale to the panel. So the "150fps of capability" above was
> never there. The Pi draws 640x350 at about 72 fps with nothing waiting at
> all. **Cutting the present is the biggest untried speed-up on the Pi**; see
> "Not done yet". The mistake was reading one measurement, taken with vsync on,
> as if it described the machine with vsync off.

Two wrong turns were taken before seeing this, both worth remembering:

- *"The SDL renderer must be software, that's why the scale is expensive."*
  It was `opengl (accelerated)` all along. Fixed by making it say so.
- *"SDL_LockTexture writes into uncached GPU memory, that's the cost."*
  Plausible, and testable — the cached staging buffer measured **identical**
  (10.5-11.1ms either way). Both paths kept, `DL_DRAW8_LOCK=1` selects the old
  one.

**The tell was arithmetic, not instrumentation.** 17.3-18.5ms totals against a
60Hz panel is 54-58fps, and a frame rate that sits just under the refresh rate
deserves suspicion before anything is optimised. Check whether the phases sum
to a constant before believing the biggest one is a bottleneck.

## The present, and the software SDL renderer

On the Pi, after threading and `draw8bpp` had done their work, the frame looked
like this at 640x350:

```
FRAME 640x350 8bpp 17.52ms (57 fps) = tics 0.61 (3%) views 5.47 (31%) present 10.93 (62%) hud/other 0.50 (3%)
```

**The present had become 62% of the frame** — and it cost *more* at 640x350
than it had at full resolution. A present that gets dearer as the picture gets
*smaller* is not copying, it is **scaling**: `SDL_RenderCopy` stretching the
small texture up to the display, every frame.

`SDL_CreateRenderer` was asking for `SDL_RENDERER_TARGETTEXTURE` alone, with
driver `-1`. That lets SDL return the **software** renderer — and with
`SDL_HINT_FRAMEBUFFER_ACCELERATION` disabled a few lines above it, quite
likely did. A software renderer does that scale on the CPU.

It now asks for `SDL_RENDERER_ACCELERATED` first and falls back to exactly the
old request, so a machine with only the software renderer behaves as before.
And it says which it got, because the two are indistinguishable from outside
until someone reads a frame profile carefully:

```
SDL renderer: opengles2 (accelerated, vsync)
SDL renderer: software (SOFTWARE)
```

**The lesson worth keeping**: `views` was never the whole story. Threading
made the drawing ~4x faster and the frame rate did not move, because drawing
was a third of the frame and the present was two thirds. Read the profile
before optimising anything.

## `draw8bpp`: draw at 8bpp, expand at present time

**The biggest single win found, and it is not threading.**

Doom renders palettized. DoomLegacy grew 15/16/24/32-bit drawers, and
`vid.bitpp` is taken **straight from the SDL texture format**
(`sdl/i_video.c`) — so on any modern display the software renderer writes
**four bytes per pixel even in the "Software 8bit" drawmode**. There are no
8bpp display modes any more; an 8bpp request deliberately takes the native
depth for the *mode* (`i_video.c:574`), and the draw depth silently followed
it. The drawmode's name has been a lie on modern hardware for years.

`draw8bpp` keeps `vid.display` at 8bpp — which the engine already supports
completely, those drawers are the original ones — and expands once through the
palette into the texture, via `SDL_LockTexture` so there is no intermediate
buffer and no second copy.

Measured, one player, 1024x768, MAP07:

| threads | draw8bpp | total | views | present |
| --- | --- | --- | --- | --- |
| 1 | Off (32bpp) | 12.18 ms (82 fps) | 10.54 | 1.36 |
| 1 | **On (8bpp)** | **5.93 ms (169 fps)** | **4.48** | 1.32 |
| 4 | Off | 4.87 ms (205 fps) | 3.20 | 1.39 |
| 4 | **On** | **3.15 ms (317 fps)** | **1.60** | 1.42 |

**2.1x on its own**, and it stacks with threading: 82 → 317 fps together.

Note `present` did **not** get worse (1.36 → 1.32). The expansion reads a
quarter as much as the memcpy it replaces, which pays for the palette lookup.
That is the whole point: it cuts memory traffic at both ends, which is what a
machine short of bandwidth actually needs.

The palette table is built in `I_SetPalette`, which the engine already calls
on every palette change while the draw depth is 8 — the damage and bonus
flashes included, so they keep working with no extra plumbing. It is written
as arithmetic on a `uint32_t` rather than through `pixel32_t`, which makes it
correct on both endiannesses.

**Verified the picture is unchanged**: dumping the same frame at 32bpp and at
8bpp, every one of 262144 sampled pixels maps its palette index to exactly one
32bpp colour — a clean one-to-one mapping, 0.00% disagreement. The two paths
draw the same picture.

**What does differ** is translucency: the truecolor drawers blend outside the
palette, the 8bpp ones use the translucency tables. Fog and translucent
surfaces will look slightly different — the classic 8bpp-versus-truecolor
difference, not a bug.

## When threading does not help: `-frameprofile`

Threading the renderer only helps if rendering is what the frame is made of.
It is not, everywhere. Run with **`-frameprofile`** and every three seconds it
prints where the frame actually went:

```
FRAME 12.66ms (79 fps) =  tics 0.02 (0%)  views 10.63 (84%)  present 1.71 (13%)  hud/other 0.30 (2%)
```

- **views** is the part `render_threads` speeds up. If it dominates, threading
  will help and the numbers should move when you change the setting.
- **present** is `I_FinishUpdate` -- handing the finished frame to SDL. At
  1366x768x4 that is a ~4MB copy per frame, and on a machine with slow memory
  it can be most of the frame. **No amount of render threading touches it.**
- **tics** is the simulation. Threading never touches this either.
- **hud/other** is the remainder: status bar, HUD, menus, console, and
  anything not separately timed.

Measured on the development laptop, one player, software, MAP07:

| | views | present |
| --- | --- | --- |
| `render_threads 1` | 10.6 ms (84%) | 1.7 ms (13%) |
| `render_threads 4` | 5.1 ms (68%) | 2.1 ms (27%) |

Rendering dominates there, which is why threading shows up. Where it does not
show up, this is the first thing to run -- before changing any more renderer
code.

There is also a running statement of what the renderer is doing, printed once
and again whenever it changes, so "is it even on?" never has to be inferred:

```
Render: single threaded
Render: 1 view split into 4 column bands
Render: 4 views on worker threads
```

**In a hardware drawmode it says `single threaded` whatever `render_threads`
is set to** -- which is exactly the symptom that otherwise looks identical to
the feature not working.

## Single-thread speed: what a profile says

`-frameprofile` splits a frame into four buckets. A function-level profile says
what is inside them. Taken 2026-09-10 with gprof: `-timedemo` of
`doom2_MAP03_sk0_speed.lmp` (MAP01 to MAP03), 1024x768, software, `draw8bpp`
on, `render_threads 1`. These are shares of the time spent **inside the game
binary** -- gprof cannot see into libSDL or the GL driver:

| function | share | what it is |
| --- | --- | --- |
| `R_DrawColumn_8` | 36% | walls and sprites |
| `I_FinishUpdate` | 23% | the `draw8bpp` palette expansion loop alone, 0.86 ms a frame |
| `R_DrawSpan_8` | 17% | floors and ceilings |
| `R_RenderSegLoop` | 7% | per-column wall setup |
| `V_BlitScalePic` | 3% | attract pages |
| `R_Clear_Planes` | 2% | see "Not done yet" |

### The drawers' inner loops

`R_DrawSpan_8` re-read **seven** globals on every pixel -- `ds_source`,
`ds_colormap`, `ds_xstep`, `ds_ystep`, `flat_imask`, `flat_ymask`,
`flatfracbits` -- where two loads and a store are all a pixel needs.
`R_DrawColumn_8` re-read `vid.ybytes`. Nothing in the loop changes them; the
compiler simply cannot prove it:

- the store is `*dest = ...` through a `byte *`, and a character type may
  alias anything, so each pixel written might have changed any global;
- the build has `-fno-strict-aliasing` anyway, which makes that true of every
  store;
- and with `RENDER_THREADS` the drawer state is `R_TLS`, so every one of those
  reloads is a thread-local load.

Copying them into locals before the loop fixes it: the span loop went from 20
instructions with 7 `%fs:` loads to 14 with none. The locals have **exactly the
declared types of the globals**, so every expression computes what it did --
that is what makes the picture identical rather than "the same up to
rounding".

Measured, same timedemo, three runs each, in-level frames only:

| 8bpp | views | whole frame |
| --- | --- | --- |
| before | 3.38 ms | 5.90 ms |
| after | 2.97 ms | 5.61 ms |

**12% off the 3D view.** This was measured on an out-of-order x86. The Pi's
Cortex-A53 is in order and usually pays more for redundant loads, but that has
not been measured.

**Deliberately 8bpp only.** The identical change to `R_DrawColumn_32` and
`R_DrawSpan_32` measured **2.5% slower** over five runs (views 8.02 -> 8.22 ms):
at four bytes a pixel those loops are bound by the writes, not by these reads,
and the compiler laid the new loop out slightly worse. The 16 and 24 bpp
drawers were not changed either; this machine cannot run them, since a 16 or
24 bit drawmode request comes back at 32. Do not "finish the job" on them
without measuring.

The same pattern is still in the translucent and translated drawers
(`R_DrawTranslucentSpan_8` reloads six globals per pixel). They are a small
share of any frame, so they were left alone.

### Palette flashes at 8bpp

At 8bpp `V_SetPalette` calls `R_Init_color12_translate` on every palette
change, and that means every step of every damage and bonus flash. It fills
`color12_to_8`, the 4096-entry table the alpha drawer (coronas) uses, with 4096
nearest-colour searches over 256 entries. That is **3.6 ms per call** measured
on the laptop and 85 calls in the three-level timedemo: extra time on the frame
where the player is hurt or picks something up. On a Pi core it should be
several times longer, but that was never measured, and Mark had not noticed a
stutter on the Pi before the fix or after it. Call it wasted work on the busiest
frames, not a visible bug.

**It never needed doing.** `NearestColor` searches `pLocalPalette[0..255]`,
palette 0, whatever palette is passed in, so the table depends on palette 0
alone and every flash rebuilt the identical table. It now keeps a copy of the
256 colours it was built from and returns if they have not changed. Comparing
the colours rather than tracking the callers catches every way palette 0 really
changes -- `LoadPalette` on a gamma change, a new `PLAYPAL`, the Heretic
finale's palette lump -- without having to trust that each one is accounted for.

Verified both ways, with a hash of the table printed after every palette change
in an instrumented copy of the old and new code:

| run | rebuilds before | rebuilds after | table after every change |
| --- | --- | --- | --- |
| timedemo, 85 palette changes | 85 | **1** | identical |
| `gamma 3`, then `gamma 0` | 7 | **3** | identical, including the new gamma-3 table |

The second row is the one that matters: it is the check that a real change of
palette 0 still rebuilds.

### How these were verified

The software renderer is integer throughout, so both changes must leave every
pixel exactly as it was. An instrumented copy of the old and of the new source
hashed the whole of `screens[0]` (FNV-1a) at every 35th gametic of the same
`-timedemo`. A timedemo draws exactly one frame per tic, so the two builds draw
the same frames:

| depth | frames compared | identical | distinct hashes |
| --- | --- | --- | --- |
| 8bpp | 209 | 209 | 209 |
| 32bpp | 209 | 209 | 209 |

"Distinct hashes" is the control: all 209 frames differ from each other, so the
check was hashing real, changing pictures and not a blank or frozen screen.
`make demotest` 102/102 with no desync, `make smoke` 5/5.

### Profiling recipe

There is no `perf` or `valgrind` on the development laptop, and the Makefile's
own `PROFILEMODE=1` does not build on modern GCC (`-pg and -fomit-frame-pointer
are incompatible`, and it drops `-O3`, so it would profile unoptimised code).
What works:

1. A copy of `svn1749/make_options` in a scratch directory, with
   `ENV_CFLAGS=-std=gnu17 -g -pg` and `CC_EXPLICIT_CMD=<wrapper>`, where the
   wrapper is a bash script that drops `-fomit-frame-pointer` from its
   arguments and execs `gcc`. A second one for `g++`, passed as `CXX=` on the
   make line, covers ZDBSP.
2. `make MAIN_BUILD_DIR=/abs/scratch/dir/ depend`, then `-j8`. The path must be
   absolute with a trailing slash. Nothing lands in the tree.
3. Run it as any headless test (a *copy* of `legacyhome`, `offscreen`,
   `SDL_NO_SIGNAL_HANDLERS=1`), software drawmode, `render_threads "1"` since
   gprof samples one thread, with `-timedemo <record demo> -frameprofile`.
4. `gprof -b -p` for the flat profile. Distrust the call graph's caller
   attribution for inlined code: it blamed `D_PageDrawer` for 39 million
   `I_GetTime` calls that came from the main loop.

### On the Pi

Mark measured the Pi 3 Model B after these two changes (2026-09-10): software,
Render Threads on, 8bpp Draw on, Framerate Cap uncapped. The full table is in
the README under Performance. Against the previous README table:

| resolution | before | after |
| --- | --- | --- |
| 512x384 | ~60 | 68-72 |
| 640x350 | ~55 | 61-64 |
| 720x400 | ~49-51 | 51-53 |
| 640x480 | ~45-47 | 51-53 |
| 864x486 | ~38-40 | 38-39 |
| 800x600 | 35+ | 37-38 |
| 928x580 | ~33-36 | 35-36 |
| 1024x576 | ~22-27 | 28-29 |

**Read it with care.** The earlier table was probably taken with Framerate Cap
at 60, so the 512x384 "~60" was the cap, not the board. Rows well below 60 were
never limited by it and are the fair comparison. They moved by 0-15%, which is
the size you would expect from a 12% cut in view drawing when drawing is only
part of the frame. How the old figures were read is not known exactly, though,
so no single row proves anything. That is what the next section is for.

640x360 (53-55) against 640x350 (61-64) is a bigger drop than 3% more pixels
explains. The counter was hard to read at that size, so treat it as unconfirmed.

### Measuring: `tools/perfchart.py`

The table above was read off the ticrate counter by eye. `tools/perfchart.py`
does the same job repeatably: it plays one fixed demo with `-timedemo` at each
resolution and writes the README table, plus a `.csv` that `--compare` reads
back to make a before/after. **Use it for any speed change**:
`--quick --compare <before.csv>` on the Pi, before and after.

Design points, each forced by a way the measurement could lie:

- **The demo lives in the repo** (`tools/bench/doomu_E1M1_sk3_speed.lmp`, 446
  tics, UV speed E1M1). The cabinet rewrites its record demos whenever a
  record is beaten, so pointing at `legacyhome/demos` would change the workload
  under the benchmark.
- **`-timedemo`, not play.** It draws one frame per tic as fast as it can, and
  the engine turns vsync off for it (`cv_vidwait`), so a 60 Hz panel cannot cap
  the result. It is not identical to `Framerate Cap Uncapped` in play: there the
  simulation runs 35 tics a second whatever the frame rate, and here it runs
  once per frame. It is also a different scene from wherever a hand reading was
  taken, so **compare perfchart with perfchart, never with a hand-read
  table**.
- **The screen wipe is switched off (`screenlink "None"`).** The first Pi chart
  was taken with it on, and every number in it was low. The level load restarts
  the timedemo clock (`G_DoneLevelLoad`), and the crossfade or melt that follows
  runs on its own clock inside that same loop pass. That is about a second
  counted in the fps figure and drawn in no frame. On the laptop it halved the
  result: 640x480 read 241 fps with the crossfade and 505 without. On the Pi a
  size takes 8-28 s, so if the wipe lasts about a second there too (measured on
  the laptop only), it cost roughly 20% at 320x200 and a few percent at
  1280x960. That would also explain why the first Pi chart read lower than the
  hand-read table at the small sizes. **Any use of `-timedemo` as a clock has to
  switch the wipe off**, or subtract a constant nobody measured.
- **Where each frame's time went: Views, Present, Other.** With
  `-frameprofile`, the engine totals its profile buckets over exactly the
  frames the timedemo counts (`FP_Run_Reset` in `G_DoneLevelLoad`,
  `FP_Run_Report` in `G_CheckDemoStatus`) and prints `timedemo profile: ...`
  next to the result. The periodic 3-second line could not do this: its first
  window includes the level load, and a fast machine finishes the demo before
  the first window closes. The pass in which the reset happens is skipped,
  because it still holds the load.
- **The time must add up.** The three columns cover the counted frames, so
  their sum must be 1000/fps. More than 10% apart means wall time went
  somewhere no frame was drawn, and the size is flagged. This check is what
  found the wipe, and it was proven by putting the crossfade back
  (`--set screenlink=Crossfade`): every size flagged, "frames account for 2.0 ms
  each, the fps figure says 4.2".
- **The engine's result line says what it drew** (`timedemo: 446 gametics in
  64 realtics, 244.45 avg fps, 640x480 8bpp software`), and a size that came out
  different is reported, not credited. Asking for 200x150 draws 320x200, and
  the tool says so.
- **Every size must play the same number of tics.** Drawing cannot change the
  simulation, so a different count means a run did not play the whole demo.
- **On a Pi it records the temperature per size and `vcgencmd get_throttled`
  at the end.** Its "has occurred" flags stick from boot, so the tool compares
  against the value it read before starting and reports only what happened
  during the chart.
- **Wads come from where the engine would find them, the binary's own
  directory first.** The first version looked only in `$DOOMWADDIR` or
  `~/games/doom`, which is where the laptop keeps them, and the Pi keeps
  `legacy.wad` next to the binary. The scratch copy is a different directory,
  so on the Pi the engine stopped with "No legacy.wad file". The tool now
  searches the binary's directory and then `tools/smoke.sh`'s list, and checks
  for `legacy.wad` itself before starting, naming the directories it tried.
  **Any tool that runs the engine from a scratch copy has to carry the wads
  across from wherever the real install keeps them.**
- **If the first run gives no result, it stops** and prints the end of the
  engine's output. A broken demo does not end on its own (the random-bytes one
  ran until the timeout), so otherwise every size would sit out the whole
  timeout.

Each check was shown to fail before it was trusted: 200x150 for the size check,
a stand-in engine that played 300 tics at one size for the tic check, and a
random-bytes demo for the early stop.

**The timedemo result line did not reach the terminal until this tool
needed it.** An `[Arcade]` comment beside it said it went through `GenPrintf`
"so a headless timedemo can be measured", but it used `EMSG_info`, which takes
the default `EOUT_flags`. Once graphics are up those are log and console only,
and the log is not compiled in. It is `EMSG_errlog` now, the category meant for
terminal and log. The lesson generalises: **a message category is not a
promise about where it ends up**. `EOUT_flags` changes during startup, so check
the routing in `GenPrintf_va` before relying on an `EMSG_info` line from a
script.

On a fast machine each size is over in 1-2 seconds, and `realtics` are whole
35ths of a second, so a single run carries a couple of percent of noise there.
On a Pi a size takes around ten seconds. Use `--runs 3` whenever the
difference being looked for is small.

### Five sizes that are slow on the Pi

The first Pi chart (Pi 3 Model B, 2026-09-10, wipe still on) had five sizes
well below what their pixel count predicts. Megapixels drawn per second, which
should rise smoothly with size:

| size | fps | Mpx/s | neighbours |
| --- | --- | --- | --- |
| 640x360 | 49.5 | 11.4 | 640x350: 13.3, 640x400: 14.8 |
| 864x486 | 39.1 | 16.4 | 800x500: 18.0, 800x600: 19.0 |
| 960x540 | 32.5 | 16.8 | 928x580: 20.5, 960x600: 19.4 |
| 1024x576 | 26.8 | 15.8 | 1152x720: 23.5 |
| 1024x768 | 19.7 | 15.5 | 1152x720: 23.5, 1152x864: 24.2 |

640x360 matches what Mark read by hand, so it is not the wipe or a misreading.

**Resolved, 2026-09-10.** The two causes worked out below are both fixed: the
1024-wide sizes by Row Padding, and 640x360 / 864x486 by clearing before every
present. See "Cutting the present, and Row Padding" for the Pi numbers.

**The second Pi chart (wipe off, with the split) shows two separate causes**,
ms per frame:

| size | views | present | neighbour for comparison |
| --- | --- | --- | --- |
| 640x360 | 5.62 | **9.89** | 640x350: 5.01 / 7.47, 640x400: 5.27 / 8.03 |
| 864x486 | 7.47 | **13.78** | 800x600: 7.42 / 12.70 |
| 1024x576 | **17.44** | 17.01 | 960x600: 8.54 / 16.13, 1152x720: 11.04 / 19.77 |
| 1024x768 | **26.26** | 21.26 | 1152x864: 12.82 / 23.30 |

960x540 no longer stands out (38.5 fps against 37.9 at 960x600). It was the
wipe's fixed second landing on a mid-speed size.

- **The 1024-wide sizes lose it in the views: about 2x the drawing time.**
  Present is normal. The leading suspect is cache aliasing on a power-of-two
  row. At 8bpp a 1024-pixel row is exactly 1024 bytes, 16 cache lines, so every
  pixel of a column falls in 1 of 32 sets of the Pi 3's 512 KB, 16-way shared
  L2. 576 rows need 18 lines per set and 768 need 24, both more than 16 ways,
  so a wall column evicts itself, and the next column, which touches the same
  lines, starts from DRAM. The suspect fits all three points: 512-wide
  (8 lines, 64 sets, 384 rows = 6 per set) should be fine and is; 768 rows
  should be worse than 576 and is (2.5x against 2x); and 1152 or 1280 wide
  spread over far more sets. **Not yet proven.** The test is to pad the row
  pitch (`vid.ybytes`) past a power of two, say width + 64, and re-run those
  two sizes. `I_FinishUpdate` already passes the pitch it is given to SDL, but
  everything that assumes `ybytes == width` has to be found first.
- **640x360 and 864x486 lose it in the present: about +30%.** Views is normal.
  Both are exact 16:9, but the pattern is not clean: 1280x720 (16:9) and
  1280x800 (16:10) are both somewhat high per pixel, and 640x350, almost the
  same shape as 640x360, is normal. On the laptop, headless, nothing stands
  out, which fits a display-side cause. **Cause unknown.** Candidates are the texture upload
  (Mesa's vc4 driver retiles on the CPU, and the tiling depends on the texture
  dimensions) and the scale onto the panel. **Update:** the exact-fit sizes
  were the only ones the present never cleared, which is now the leading
  suspect -- see the next section.

### Cutting the present, and Row Padding (2026-09-10)

Three changes, written for the two causes above. The Pi's verdict is under
"What the Pi said" at the end of this section: Row Padding and the clear
worked, and the threaded expansion made no visible difference there.

**1. The `draw8bpp` expansion is split across the render workers.**
`R_Threads_Parallel` (`r_threads.c`) runs `fn(part, nparts, ctx)` with part 0
on the calling thread and the rest on idle workers, then waits. It is
deliberately separate from `R_Threads_Wait`, which also releases the lumps the
drawers pinned (`R_DRAW_LUMP_TAG`) -- right at the end of the view drawing,
not at present time. It uses `R_Thread_Workers()`, so at `render_threads 1`
it is a plain call and the stock serial path is untouched. It also runs
inline if any views are still in flight. `Draw8_Expand_Rows`
(`sdl/i_video.c`) takes rows `height*part/nparts` to `height*(part+1)/nparts`,
and each row is independent.

The expansion now has its own profile bucket, `FP_EXPAND`, timed inside
`I_FinishUpdate`. It is a **part** of present, printed as `expand` at the end
of the `timedemo profile:` line and never added to the frame sum. perfchart
shows it as "(of Present) Expand ms", so a chart says how much of the present
is ours and how much is SDL and the driver.

**2. The present always clears, even when the frame fills the panel.** Before,
only the letterboxed path called `SDL_RenderClear`, and the exact-fit path went
straight to `SDL_RenderCopy`. The Pi's GPU is tile based, and without a clear
the driver has to load the previous frame into each tile before drawing over
it. That fits the Pi data: the sizes that fill a 16:9 panel exactly are the
ones never cleared, and 640x360 and 864x486 paid about 30% extra present.
**Confirmed on the Pi**; see "What the Pi said". `DL_PRESENT_NOCLEAR=1`
restores the old path, so perfchart can A/B it on the same binary
(`DL_PRESENT_NOCLEAR=1 tools/perfchart.py ...`: the environment passes through
to the engine).

**3. Row Padding (`row_padding`, Performance Options, default Off).** When On,
`vid.ybytes` is rounded up to whole 64-byte cache lines and then to an odd
number of them (1024 -> 1088 bytes at 8bpp, 4096 -> 4160 at 32bpp, 640 ->
704). An odd stride in lines sends consecutive rows of a column to different
cache sets. It is **off by default and a setting**, not a fix, because it
trades memory layout against a particular cache: on the laptop it made no
measurable difference; on the Pi it made 1024x768 50% faster. It takes effect at
the next mode set (`SCR_ChangeRowPadding` -> `SCR_apply_video_settings`,
the same as `draw8bpp`). Toggling it mid-level was checked: 1024 -> 1088 ->
1024 bytes per row, the game carries on, clean quit.

WDJ built the engine to tolerate a pitch wider than the row ("padded video
buffer"). The allocation comment says "most code uses vid.ybytes now, and is
padded video safe", and a debug `#else` pads every row by 8 bytes. **"Most"
was right.** Two places were not, and both are fixed:

- `HU_Erase` (`hu_stuff.c`) stepped rows by `vid.width` and passed pixel counts
  to `R_VideoErase`, which takes bytes. With padding every row after the first
  was off. It was also already wrong at 16/32bpp. It only runs with a reduced
  view size (`view_window_x != 0`), which is why nobody saw it.
- `M_ScreenShot` (`m_misc.c`) stripped padding by comparing and copying
  `vid.width`, pixels, where it meant `vid.widthbytes`. That is harmless
  unpadded (every row moves onto itself) and scrambled with padding at 16/32bpp.

`I_ReadScreen` looks like a third and is not: it copies `widthbytes` per row
but advances both pointers by `ybytes`, so it returns the screen at its own
pitch, which is what the wipe and the screenshot expect. Also noted, not
changed: `wipe_EndScreen` restores the start screen with
`VID_BlitLinearScreen(..., vid.width, ...)`, a pixel count where bytes are
meant. That only copies part of each row at 16/32bpp. The melt then redraws
everything, so it is not visible, and it has nothing to do with padding.

**Verified**, with an instrumented copy of the final source:

| check | result |
| --- | --- |
| threaded expansion against a serial re-expansion, every frame, 3 workers | 0 of 400 frames differ, at 1024x768 and 640x350 |
| same, with part 1 told to skip its rows | 388 of 400 flagged: the check can fail |
| screen hash, Row Padding Off vs On, `render_threads 1` | identical at 1024x768, 1024x576, 640x480 and 800x600, 8bpp and 32bpp |
| same with viewsize 7, so `HU_Erase` runs (counted) | identical; **old `HU_Erase` code differs** at 8bpp |
| screenshots Off vs On, 32bpp | an On shot byte-identical to two Off shots; the other runs differ between themselves by one tic of the level clock |
| `make demotest` / `make smoke` | 102/102 no desync / 5/5 |

The screenshot row needed a control. Off against On differed by 420 bytes,
all in 12 rows at the bottom of the screen. Three shots of each showed Off
differing from Off in the same way, and one On matching two Offs exactly: the
shot lands on tic 104 or 105, and the HUD clock moves. That is the trap
described under "A pixel diff that was not a race", again.

**Laptop** (Ryzen 5 2500U, headless, 3 runs, `render_threads Auto`):

| size | present before | present after | expand serial | expand threaded | fps change |
| --- | --- | --- | --- | --- | --- |
| 640x480 | 0.95 ms | 0.77 | 0.38 | 0.20 | +11% |
| 800x600 | 1.64 | 1.31 | 0.62 | 0.31 | +12% |
| 1024x576 | 2.06 | 1.75 | 0.76 | 0.43 | +6% |
| 1024x768 | 2.68 | 2.11 | 0.96 | 0.57 | +13% |

The expansion speeds up about 1.7-1.9x on four threads, not 4x. It is memory
traffic (read one byte, write four), and more cores add little memory
bandwidth. Row Padding on the laptop was within the run-to-run noise either
way.

**For the Pi**, one chart each way on the same build:

```
tools/perfchart.py --out base
tools/perfchart.py --set row_padding=On --compare base/*.csv
DL_PRESENT_NOCLEAR=1 tools/perfchart.py --compare base/*.csv
```

What to look for: Views at 1024x576 and 1024x768 with padding (was 17.4 and
26.3 ms), Present at 640x360 and 864x486 against the no-clear run (was 9.9 and
13.8), and the Expand column against the rest of Present.

#### What the Pi said (a36b7a6, two full charts, Row Padding On then Off)

**Row Padding: the aliasing theory holds.** Views, ms per frame:

| size | Off | On | predicted from neighbours |
| --- | --- | --- | --- |
| 1024x576 | 13.58 | **8.34** | ~9 |
| 1024x768 | 27.36 | **10.36** | ~10.5 |
| 1280x960 | 15.94 | 13.66 | |
| 1280x800 | 13.38 | 12.52 | |
| 640x480 | 5.88 | 5.54 | |
| 800x600 | 7.27 | 7.28 | |

1024x768 went from 20.7 to 31.0 fps and 1024x576 from 32.6 to 37.7. Padded,
both land where their neighbours said they should. The large even-line widths
gain a little. Everything else sits within run-to-run noise. One exception
needs a repeat before anyone believes it: 928x580 read 43.3 fps Off and 38.1
On. Its Present rose too in that run, and padding cannot touch Present, so it
is probably noise; `--runs 3` at that size would settle it. The README
recommends Row Padding On for a Pi. The default stays Off, because the laptop
showed nothing and it is still a cache-dependent trade.

**The clear: confirmed without needing the no-clear run.** Compared with the
previous chart (754b035, no clear), the sizes that fill the 16:9 panel exactly
lost 2-3 ms of Present, and the letterboxed sizes, which were already cleared,
did not move:

| size | fills panel | present before | present after |
| --- | --- | --- | --- |
| 640x360 | yes | 9.89 | 7.54 |
| 864x486 | yes | 13.78 | 11.23 |
| 960x540 | yes | 15.68 | 12.88 |
| 1280x720 | yes | 26.14 | 23.73 |
| 640x350 | no | 7.47 | 7.41 |
| 640x400 | no | 8.03 | 7.86 |
| 800x600 | no | 12.70 | 13.10 |

640x360 is now level with 640x350: 72 fps against 73. Before the clear it was
59.

**The threaded expansion: no visible gain on the Pi.** It should have taken
something like the parallel Expand figure off the present (1.45 ms at 640x350).
Present at the letterboxed sizes did not move, and at small sizes, where run
noise is a tenth of a millisecond, the saving is plainly absent. The
reading that fits: on the Pi, `SDL_RenderPresent` waits for the GPU to finish,
so the present is bound by the VideoCore, not by our CPU work, and CPU time
saved before the wait becomes more waiting. It still helps where the CPU is
the limit (the laptop, 6-13%), and it costs nothing here.

The present even at 320x200 is about 4.3 ms, of which Expand is 0.5. A fixed
cost of that size, regardless of the picture, points at the part that does
not scale with the picture: the scale to the full panel, the clear, and the
swap, all at the desktop's resolution.

### Not done yet

Ranked by expected payoff on the Pi. None is started. Measure each with
`tools/perfchart.py --compare`.

1. **The rest of the present, which on the Pi is GPU work.** See "What the Pi
   said": the present waits on the VideoCore, so CPU savings before it do not
   show. Cut what the GPU does per frame instead. The fixed ~4 ms even at
   320x200 is the scale, the clear and the swap at the desktop's resolution, so
   first try a lower desktop resolution on the Pi (1280x720 instead of 1920x1080
   is 2.25x fewer pixels to fill). If that helps, a real mode switch for
   software fullscreen would do it without touching the desktop. The per-pixel
   part is the texture upload, which Mesa's vc4 driver retiles on the CPU. A
   16-bit (RGB565) texture would halve it, at the cost of colour precision.
2. **Idle cores with two or three players.** More than one view means one view
   per thread and no bands, so two players on four cores leave two idle. Split
   each view into bands as well.
3. **Equal-width bands make the frame wait for the busiest one.** A band
   looking into an open room does far more work than one facing a wall. Size the
   bands from the previous frame's per-band times. Bands are already not
   bit-identical to serial, so this does not change their acceptance test.
4. **`R_Clear_Planes` resets 40 3D-floor clip rows per column, every frame, on
   every thread**, on maps with no 3D floors, where nothing reads them. 2% of
   the profile. The same waste is in `R_RenderSegLoop`, which steps all 40
   `ffplane[]` slots on every wall column. Both were written and then dropped at
   Mark's request (2026-09-10). The analysis they rested on: the rows are written
   only for slots `R_StoreWallRange` marks `valid_mark`, read only for
   `i < numffplane`, and reset to values that move with the view size and
   `con_clipviewtop`, so the reset can be skipped when none of those changed.
   Testing it needs a map with Legacy 3D floors, and none of the cabinet's wads
   has one. Setting special 281 on a one-sided line of DOOM2 MAP01 and tagging
   the other sectors makes one without touching the geometry, so the stock
   nodes stay valid.
5. **Profile-guided optimisation**, using the timedemo as the training run.
   Cheap to try, gain unknown.
6. **`framerate_cap 35` spins a core between tics.** The default, 60, sleeps.
   It costs no frame time, but heat throttles a Pi.
7. **A column-major draw buffer.** Columns are 36% of the time and each pixel
   written is a whole row away from the last. The flip back could be folded into
   the `draw8bpp` expansion, which already touches every pixel. A large change
   with an uncertain gain, so measure on the Pi before starting.
8. **Sprite sorting and clipping are O(n^2)** (`R_NewVisSprite`'s insertion
   sort, and `R_DrawSprite` scanning every drawseg per sprite). This only
   matters on crowded maps.

## Demo safety

**Verified, not assumed.** The simulation is untouched, at `render_threads` 1 and 4
alike, so the record demos and high score times on the cabinet stay valid.

The test: a probe printing the four RNG indices plus the player's position, angle,
health and leveltime from `P_Ticker` at fixed gametics -- no rendering in it at all --
applied *identically* to this tree and to a worktree at the commit before the feature
(`5b335eb`), then a real cabinet record demo replayed through both.

| run | result |
| --- | --- |
| pre-change binary | reference |
| this tree, `render_threads 1` | **identical**, all 8 samples |
| this tree, `render_threads 4` | **identical**, all 8 samples |

Run over `doomu_E2M7_sk0_speed.lmp`, which spans several levels, so level
transitions are covered as well. Confirm the demo actually played before trusting a
pass -- the player's position must *move* and the RNG index must advance; a demo that
failed to load leaves the title screen ticking and looks like a clean run.
→ `gotchas.md`

**The rule this enforces:** with `render_threads` at 1 nothing about a frame may
differ from before this feature existed. That is why the catch-up `NetUpdate()` after
the view join is conditional on workers actually having been used -- `NetUpdate` runs
`Local_Maketic` -> `G_BuildTiccmd`, so an extra call on a serial frame is a gameplay
change, and a gameplay change rejects every record demo on the cabinet.

## The rule for R_TLS, and both ways to get it wrong

> A renderer global gets `R_TLS` if and only if it is **written while drawing a frame**.
> If setup writes it and drawing only reads it, it must **not** be marked.

Both mistakes are silent in their own way:

- **Marked when it should not be** — the worker reads a zeroed copy. The picture is *stable but
  wrong*, which reads as a rendering bug rather than a threading one.
- **Not marked when it should be** — the threads scribble on each other. The picture flickers
  and varies frame to frame.

`R_ExecuteSetViewSize` computes the same geometry for every cell, so `centerx`, `centerxfrac`,
`projection_x`, `projection_y`, `rdraw_*`, `detailshift`, `yslopetab`, `distscale`,
`viewangle_to_x`, `x_to_viewangle`, `scalelight` and `zlight` are **shared, read-only** while
drawing. `R_SetupFrame` and `R_Set_View_Window` write everything that genuinely differs per view.

**`centery` and `centeryfrac` are the trap in the other direction.** They look like setup values
— `R_ExecuteSetViewSize` does set them — but `R_SetupFrame` rewrites them every view with the
aiming offset (`r_main.c`, `centery = (rdraw_viewheight/2) + dy`), and `r_things.c` swaps them
again for the weapon sprite. They are per-view and must be marked.

**These four were marked and should not have been**, and cost a debugging session:
`pspritescale`, `pspriteiscale`, `pspriteyscale`, `clip_screen_top_min`, `clip_screen_bot_max`.
All are filled once by `R_ExecuteSetViewSize` and only read while drawing. A worker saw zeros,
drew the weapon at zero scale and clipped every sprite against a zeroed array, and produced a
picture that was perfectly repeatable and perfectly wrong.

There is a script for this in the job notes; the check it performs is worth repeating by hand
after any change: **for every `R_TLS` name, does a setup function assign it?** If yes, and
`R_SetupFrame` does not also assign it, the mark is wrong.

**The definition and the extern declaration must agree.** The linker catches a mismatch
(`TLS definition ... mismatches non-TLS reference`) but only one symbol per link, and `extern`
declarations are not all in headers — `first_subsec_seg` is declared inside `r_segs.c` and the
`cached*` arrays inside `r_splats.c`.

**A thread-local cannot be initialised with the address of another thread-local.** It is not a
compile-time constant. `vispl_free_tail` used to be `= &vispl_free_head`; it is NULL now and
`R_Clear_Planes` points it at this thread's own head on first use.

## Six things that are shared and had to be dealt with

Everything below is state the renderer touches that is **not** the renderer's own, so `R_TLS`
could not fix it.

### 1. The rebuilt BSP — the one that crashed in another subsystem

`R_Use_Render_BSP` saves the play tree into file statics guarded by one shared
`render_bsp_active` flag, and `R_RenderPlayerView` used to call it per view. With four threads,
one of them saves the *render* pointers as the play tree, and the swap back leaves `nodes`,
`segs`, `subsectors` and `vertexes` pointing at the rebuilt tree **for the simulation**. It
crashed in `P_CrossSubsector`, in `P_Ticker`, on the main thread, with every render worker idle
— a backtrace with nothing in it to suggest rendering, let alone threading.

The swap is now done **once, on the main thread, around all the views**, in `D_Display`. This is
the same hard-won rule as ever: the BSP the renderer walks must not be the one the simulation
walks. → `gotchas.md`

### 2. `W_CacheLumpNum` mutates on a cache *hit*

Not just on a miss. A hit still calls `Z_ChangeTag` on the block and writes the shared
`lump_read`. `R_GetFlat` calls it once per visplane and `R_DrawVisSprite` once per sprite, from
every thread — so this is a live race on every threaded frame, not a theoretical one, and its
damage is to the zone allocator's block lists, which surfaces as a crash somewhere unrelated
much later. It is serialised with `R_Cache_Lock` now.

**Serialising the call is only half of it**, and reading this section as though it closed the
subject is exactly how the Pi crash got shipped. The lock makes each cache call atomic; it says
nothing about the pointer the drawer keeps afterwards. See item 6.

### 3. The texture cache is check-then-act on a shared struct

`R_WallTexture_setup` and `R_MaskedDraw_setup` test `texren->cache` / `texren->detect` and then
generate into that same shared `texture_render[]` entry. Two views wanting the same texture at
once would both decide to generate, and the second would free the block the first is drawing
from. Both are serialised. `R_Draw_WallColumn`'s defensive regenerate is **per column**, so it
is double-checked inside the branch instead — the fast path pays nothing.

### 4. `sec->validcount`, and `R_Prep3DFloors`

The sprite-added mark was the one place the renderer wrote into shared level data: `R_AddSprites`
tagged `sec->validcount` so a sector split across subsectors only added its sprites once. With a
view per thread, one thread's mark makes the other skip a sector and that view silently loses
every sprite in it. It is a per-thread array tagged with a per-thread counter now, so it never
needs clearing.

`R_Prep3DFloors` rebuilds `sector->lightlist` in place and memsets it on *every* call, not only
when it reallocates — so two views reaching the same sector corrupt it, and the reallocating
case double-frees. Serialised. Stock Doom maps have no 3D floors, so this never fires on them;
a PWAD that uses them would have found it the hard way.

### 5. The corona patches are built lazily into shared statics

`Draw_Sprite_Corona_Light` builds `corona_patch` and the per-colour
`corona_image[].colored_patch` copies on demand, **while drawing**, with `Z_Malloc` and
`Z_Free`. Two threads drawing coronas at once means one reallocates the patch the other is
reading, and it crashes in `R_DrawMaskedColumn` on a column pointer into freed memory — with
nothing in the backtrace above it to say "corona", let alone "threads". Serialised.

This one is worth remembering as a *pattern*, because it is the third of its kind here: **a
cache that is filled the first time something is drawn is shared mutable state, however
read-only it looks afterwards.** The texture cache, the lump cache and the corona patches are
all this shape. When adding anything to the render path, the question is not "does it write a
global" but "does it *fill in* anything the first time it runs".

### 6. A lump let go while another thread is still drawing from it

This is the one that reached the cabinet. The Pi died with:

```
Error: Z_ChangeTag: free block has corrupt ZONEID: 2d25231d
```

after several minutes of the attract cycle, in the software renderer, with one view split into
four column bands.

**Serialising the cache call was not enough — the *lifetime* was still wrong.** `R_Cache_Lock`
(item 2) makes each `W_CacheLumpNum` call atomic, and everyone reasoned about it as though that
settled the lump cache. It does not, because a drawer does not just call the cache, it *holds
the pointer*:

```c
ds_source = R_GetFlat(...);      // W_CacheLumpNum, PU_LUMP: pinned, locked
... draw every span of the visplane ...   // unlocked, and long
Z_ChangeTag(ds_source, PU_CACHE);         // purgable again, locked
```

Serially that is airtight: nothing else runs between the two, so nothing can allocate, so
nothing can purge. With workers it falls apart, and it needs no exotic interleaving at all:

1. thread A and thread B both draw a visplane using flat F — the floor of a room spans every
   column band, so this is the *normal* case, not a corner;
2. A finishes first and hands F back to `PU_CACHE`;
3. any thread's next `Z_Malloc` needs room and purges F, which is now purgable. `Z_Free` nulls
   the lumpcache entry and merges the block into its free neighbour, so the header B's pointer
   points at is now *inside* someone else's allocation;
4. B is still drawing from it — garbage pixels — and then hands back a block whose `id` field
   has been overwritten with whatever was allocated over it. `2d25231d` is that data.

The engine's own check caught it one instruction before the real damage. It is a use-after-free
that had already been read from.

**How often step 1–2 happens**: a temporary counter that recorded each thread's current
`ds_source` and checked the other threads' at release time measured **over 20 000 early
releases in 45 seconds** standing still on MAP01 with four bands. The window is not rare; the
Pi's memory pressure is what made a purge land inside it.

**Why only the Pi.** Two reasons, and both had to hold. The cabinet's config selects OpenGL, so
the laptop never runs the software flat path at all. And the zone starts at 8 MiB and grows
(`GROW_ZONE`) — a machine with memory to spare grows instead of purging, and step 3 never
happens. `Z_Malloc`'s ordinary pass purges `PU_CACHE` and nothing else, which is exactly the tag
the drawers were putting their lumps back to.

**The fix** is `R_DRAW_LUMP_TAG` (`r_threads.h`): inside the parallel section a drawer caches
its lump as `PU_LUMP` (non-purgable) instead of `PU_CACHE`, and nobody hands anything back
mid-frame. `R_Threads_Wait` releases the lot with one `Z_ChangeTags_To(PU_LUMP, PU_CACHE)` after
the join, which is the first moment at which no worker can still be reading. Outside the
parallel section the tag is `PU_CACHE` and the release is immediate, exactly as before — the
serial renderer is byte-for-byte the code it always was.

Three call sites hold a lump across a draw and all three use the macro: the flat
(`R_DrawSinglePlane`), the sprite patch (`R_DrawVisSprite`) and the wall splat and Boom
translucency map (`r_segs.c`). Nothing else tags `PU_LUMP` while a frame is being drawn — the
only other users are `PNAMES`/`TEXTURE1`/`TEXTURE2` at load time — so the sweep releases exactly
what the drawers pinned, and if a frame is abandoned the level-load
`Z_ChangeTags_To(PU_LUMP, PU_CACHE)` in `p_setup.c` catches the leftovers.

**How it was proved, both directions.** A crash that needs memory pressure will not show up on
demand, so the pressure was supplied: a temporary `Z_FreeTags(PU_CACHE, PU_CACHE)` at the point
where the flat used to be released, standing in for "the allocator needs space right now".

- stock binary + that hack: dies in **seconds**, in `R_DrawSinglePlane`, on the sibling check in
  the same function (`Z_ChangeTag: an owner is required for purgable blocks` — the block was
  already free);
- fixed binary + the same hack: **60 seconds and still running**, then a clean quit.

That is the shape to copy for anything else in this file: **a race you cannot reproduce is one
whose window you have not widened yet.** Widen it artificially, confirm the old code dies, then
confirm the new code does not. A fix verified only against a run that never crashed anyway is
not verified.

The pixel check confirmed the fix is invisible: the fixed binary's frame is byte-identical to
the stock binary's at the same gametic.

`NetUpdate` is called three times inside `R_RenderPlayerView` to keep the client/server tick
alive through a slow frame. A worker must not: it would run the netcode from four threads at
once. `R_NetUpdate_Main` skips it off the main thread, and the main thread is drawing its own
view alongside and still calls it at the same points.

## How to verify a change here

**Do not trust "it looks right" or a frame rate.** A threaded renderer that is quietly racing
looks fine and runs fast. Two checks, both cheap:

### Bit-for-bit screen comparison

The software renderer is integer and fixed-point throughout, so threading must produce
**exactly** the same pixels. Instrument `D_Display` to FNV-1a the screen buffer at fixed
*gametics* (not frames), set `framerate_cap "35"` so there is one frame per tic and no
interpolation, park the player somewhere with `-warp`, and compare `render_threads 1` against
`render_threads 4`. They must match exactly, run to run. This is what caught the misclassified
`pspritescale`, and it is worth keeping the patch around.

### Serialise on the workers to bisect

When the checksums differ, the useful question is *which* kind of bug it is. Make each view run
on a worker but **one at a time** (submit, wait, submit, wait):

- Still differs from serial → the `R_TLS` split is wrong. Something is zero in a worker, or
  something per-frame is still shared.
- Matches serial → the split is right and the threads are racing on something shared.

That one experiment separated the two bugs in this feature in a single run, and it is the first
thing to do next time.

### ThreadSanitizer, and the one thing it cannot find

`./tools/build.sh --tsan`, then run headless with `render_threads 4`. It named
every race in this feature directly -- the variable and both stacks -- and took
the count from 275 to 0 in three passes. Summarise a log by grouping on
`Location is global '...'` plus the first in-tree frame of each *access* stack;
do not include the thread-creation stacks or every report looks like `D_Display`.

**But TSan cannot see the other half of this feature's failure mode.** Reading
your own zeroed thread-local is not a race, so a variable that is wrongly
`R_TLS` -- set once at setup, read by a worker as zero -- is invisible to it.
That is what `skycolfunc` was: `R_Setup_SkyDraw` sets it at level load and on a
video mode change, never per view, so every worker held NULL and `R_Draw_Planes`
**jumped to address 0** the first time a view could see sky. MAP01 and MAP07
start indoors, which is the only reason they passed while MAP11, MAP15 and
MAP29 died.

So the two tools are complements, not alternatives:

- **ThreadSanitizer** finds "not marked when it should be" -- the races.
- **The bit-for-bit screen comparison, and a crash** find "marked when it
  should not be" -- the zeroed reads. Nothing else will.

The audit worth re-running after any change: for every `R_TLS` name, does a
*setup* function assign it? If yes and `R_SetupFrame` does not also assign it,
the mark is wrong. **Scan `screen.c` and `r_sky.c` too** -- the renderer's state
is not only in the six `r_*` files, and scanning only those is exactly how
`skycolfunc` and `colfunc` were missed the first time.

### ThreadSanitizer, verbatim recipe

**Not yet run — `libtsan` is not installed on the development machine.** `sudo dnf install
libtsan`, then build with `ENV_CFLAGS=-std=gnu17 -g -fsanitize=thread -fno-omit-frame-pointer`
into a separate `BUILD=` directory and run headless with `render_threads 4`. This is the only
check that finds a race which happens not to change the picture, and it should be run before the
default is ever changed from 1.

## Known residual risk

The **wall texture** cache is still read without the lock while another thread may allocate.
Textures are tagged `PU_PRIV_CACHE`, which `Z_Malloc` only purges once a normal pass has failed
and it retries with `current_purgelevel = PU_PURGELEVEL`, and `precache` composes the level's
textures at load — so this is a much narrower window than the flat one below was, and it has not
been seen. `TEXTURE_LOCK` (`r_segs.c`) is the existing compile-time option that pins textures
with `PU_IN_USE` and is the fix if it ever bites.

The **flat and sprite** case that used to be listed here **did** bite, on the Pi, and is fixed —
see below.

## What to try on the cabinet

- Four players in software drawmode, on a busy map, with `render_threads` on `Auto`.
- Compare against `render_threads 1` — it should look identical, only smoother.
- Single player: **no change is expected.** One view is one thread. Splitting a single view into
  column bands across cores is the obvious next step and is not done yet; the `R_TLS` groundwork
  is what it needs, and that is now in place.

## A pixel diff that was not a race

Worth recording, because the method looked sound and was not.

Comparing `render_threads` 1 against 4 with `tools/shotsheet.py` reported 6531
of 921600 pixels different (0.7%), scattered over most of the frame. The
control — the same 1-thread configuration captured twice — came back
byte-identical, which appeared to establish that the capture was deterministic
and therefore that threading was changing the picture. Two runs at 4 threads
then differed from *each other* by 585 pixels, which looked like the clinching
evidence of a race.

All of it was one tic of timing.

`--nomonsters` removes monsters. It does not stop an animated pickup cycling
its frames, and it does not stop the level clock. The screenshot is triggered
by `wait 105` from an autoexec, and the command lands on tic 104 or tic 105
depending on how the loop happened to schedule — the serial path is stable
enough to always land on 104, which is exactly why the 1-vs-1 control passed
and gave false confidence in the method.

Measured, with the engine reporting the tic it captured on:

| threads | capture tic | image |
| --- | --- | --- |
| 1 | 104 | `befdbd36ab6f` |
| 1 | 104 | `befdbd36ab6f` |
| 4 | **105** | `b590c816de43` |
| 4 | 104 | `befdbd36ab6f` |
| 4 | 104 | `befdbd36ab6f` |

**At the same tic the threaded frame is byte-identical to the serial one**, which
is what the rule at the top of this file requires and what the threading work
was built to guarantee. There was no race.

Two things to take from it. First, **a control that only varies the thing you
are not testing proves nothing about your method** — the 1-vs-1 control varied
nothing that mattered. Pin the tic and compare that, or compare nothing.
Second, this was caught by a person looking at the picture and saying "that's
the armour bonus, it has glowing eyes, and that's the clock" — the numeric
comparison had no way to say *what* had changed, only how much. Hand over the
image.
