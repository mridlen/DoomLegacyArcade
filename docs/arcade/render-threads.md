# Render threads (software renderer)

**Read this before touching** `r_threads.c`/`.h`, the `R_TLS` macro in `doomdef.h`, any
file-scope variable in `r_main.c`, `r_bsp.c`, `r_segs.c`, `r_plane.c`, `r_things.c` or
`r_draw.c`, the view loop in `D_Display`, or `R_Use_Render_BSP` / `R_Use_Play_BSP`.

The cabinet draws one viewport per panel — up to four a frame — and each writes into its own
cell of the screen. They share nothing but read-only level data, so they can be drawn at the
same time on different cores. Cvar **`render_threads`**, values `Auto`, `1`, `2`, `3`, `4`.

## STATUS: NOT FINISHED. Do not raise `render_threads` above 1.

**Default `1`, which is the stock serial renderer, and it must stay there for now.**

The machinery works and is fast — 3.4x on four views, numbers below — and the thread-local
split is correct, which is *proven*, not assumed (see the one-at-a-time bisection below: every
view drawn on a worker, one at a time, reproduces the serial screen exactly, bit for bit).

But with the views actually running **concurrently**, the rendered screen still diverges from
the serial one on maps with movement in view, and three of seven test maps still crash. Six
separate races were found and fixed getting this far, and each fix uncovered another; every one
so far has been the same shape — **a cache filled lazily the first time something is drawn**.
There is at least one more.

The next step is not more guessing. It is ThreadSanitizer, which is not installed on the
development machine — `sudo dnf install libtsan`, then build with
`ENV_CFLAGS=-std=gnu17 -g -fsanitize=thread -fno-omit-frame-pointer` into a separate `BUILD=`
directory and run headless with `render_threads 4`. That names the racing variable and both
stacks directly, instead of one crash per afternoon.

State of the seven-map sweep as of writing (serial vs threaded, screen checksums at fixed
gametics):

| map | result |
| --- | --- |
| doom2 MAP07 | identical, repeatedly (static scene) |
| doom2 MAP01, doomu E1M1, doomu E2M4 | diverges |
| doom2 MAP11, MAP15, MAP29 | threaded run crashes |

Because the cvar defaults to 1 and nothing reads the worker path until it is raised, this is
safe to have in the tree. It is not safe to turn on.


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

The software renderer is the one that matters, and on the **Raspberry Pi 3b+** — quad-core
Cortex-A53, roughly 5-8x slower per core — it matters a great deal. The Pi is also VideoCore IV,
so `r_opengl`'s fixed-function `glBegin`/`gluBuild2DMipmaps` path only runs through Mesa's slow
compatibility layer: on a Pi the software renderer is effectively the only renderer.

Measured gain, same scene, four views: **9.6 ms serial → 2.8 ms with four threads, 3.4x.**
Worst frame 14.3 ms → 4-8 ms. The serial path is unchanged (see `R_TLS` below), and measures
the same as it did before the feature existed.

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

## Five things that are shared and had to be dealt with

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

### ThreadSanitizer

**Not yet run — `libtsan` is not installed on the development machine.** `sudo dnf install
libtsan`, then build with `ENV_CFLAGS=-std=gnu17 -g -fsanitize=thread -fno-omit-frame-pointer`
into a separate `BUILD=` directory and run headless with `render_threads 4`. This is the only
check that finds a race which happens not to change the picture, and it should be run before the
default is ever changed from 1.

## Known residual risk

A texture or flat block is read without the lock while another thread may allocate. If the zone
allocator purges a `PU_CACHE` block mid-draw, the reader is left with a dangling pointer.
`precache` is on by default, so the level's textures are composed at load and little allocates
during a frame — but this is the reason `render_threads` defaults to 1. `TEXTURE_LOCK`
(`r_segs.c`) is the existing compile-time option that pins textures with `PU_IN_USE` and is the
proper fix if this ever bites.

## What to try on the cabinet

- Four players in software drawmode, on a busy map, with `render_threads` on `Auto`.
- Compare against `render_threads 1` — it should look identical, only smoother.
- Single player: **no change is expected.** One view is one thread. Splitting a single view into
  column bands across cores is the obvious next step and is not done yet; the `R_TLS` groundwork
  is what it needs, and that is now in place.
