# Uncapped framerate

**Read this before touching** `r_fps.c`/`.h`, the render gate in `D_DoomLoop`, `R_SetupFrame`,
`R_ProjectSprite`/`HWR_ProjectSprite`, the `R_Interp_*` call sites in the play code, or
`I_GetTimeFrac`.

The cabinet has always drawn exactly one frame per tic — 35 fps, with no interpolation anywhere.
This makes it draw as many frames as the display can take, without changing the simulation at all.
Cvar **`uncapped`**, on the Video Options page as *Uncapped Frames*, **off by default**.

## The one thing to understand

**The simulation is not touched.** It still runs at exactly `TICRATE`. Every ticcmd, every
`P_Random` call, every stored time is what it always was. `TryRunTics` is unchanged and is still
the only thing that advances the game.

What changed is that `D_Display` may now run several times per tic. Each of those frames draws the
world **between** two states the simulation has already computed. Given `frac`, how far through the
current tic this frame falls, everything that moves is drawn at

```
prev + (now - prev) * frac
```

Nothing is ever predicted or extrapolated — only drawn part way between two known positions. That
is what keeps record demos and high score times valid: the times are in tics, the tics are
unchanged, and the interpolation exists only in the frame buffer.

**The cost** is that the picture trails the simulation by up to one tic (28 ms). That is inherent
to the technique and every port that does this pays it. It is why the cvar exists.

## Where it came from

The technique and the shape of `r_fps.c` come from **PrBoom-plus / dsda-doom** (`prboom2/src/r_fps.c`),
which is GPL v2-or-later, the same licence this tree carries. The copyright header on `r_fps.c`
names them.

**`p_tick.c` has carried the hook sites since the upstream import.** Three `#ifdef
THINKER_INTERPOLATIONS` blocks calling `R_ActivateThinkerInterpolations` and
`R_UpdateInterpolations` — PrBoom's function names, at the same three places PrBoom calls them.
WDJ copied the wiring and never wrote the implementation; the macro was never defined and the
functions existed nowhere, so it was dead scaffolding that would not have linked. This defines the
macro and supplies the functions. `P_RemoveThinker`'s hook (`R_StopInterpolationIfNeeded`) is new —
upstream did not copy that one.

## What is interpolated

| what | how | where |
| --- | --- | --- |
| Camera position, yaw | the view mobj's `PrevX/PrevY/PrevAngle` | `R_SetupFrame` |
| Camera height | `player->prev_viewz` — head bob and step easing, not just floor height | `R_SetupFrame` |
| Camera pitch | `player->prev_aiming`, or `prev_localaiming[]` for a live local player | `R_SetupFrame` |
| Chase camera | rides `camera.mo` like any mobj; only `camera.prev_aiming` is its own | `R_SetupFrame` |
| Sprites | `PrevX/PrevY/PrevZ` | `R_ProjectSprite`, `HWR_ProjectSprite` |
| Floors, ceilings | the registry, below | `R_Interp_Frame_Begin/End` |
| Wall and flat panning | the registry, below | same |

Both renderers get the camera for free: `hw_main.c` derives `gr_viewx` from the same `viewx`
globals `R_SetupFrame` sets. Sprites needed doing twice.

## Three things that are easy to get wrong

### 1. The interpolation must be an exact identity when it is off

`rendertic_frac` is `FRACUNIT` whenever the feature is off, the world is not running, or this frame
follows a discontinuity — and `FixedMul` carries a 64-bit intermediate, so `R_Interp_Fixed(prev,
now)` is then **exactly** `now` for every input, stale or uninitialised history included. Likewise
`R_Interp_Angle(p, n, FRACUNIT) == n`.

That is deliberate and load-bearing. It is what lets every call site read as a plain assignment
with no "is it on?" branch wrapped round it, and it is why turning the cvar off restores the stock
picture **bit for bit** rather than approximately. Do not "optimise" it into a conditional.

### 2. Sector movement is done by overwriting the live value

Rather than teach every plane and wall drawer about `frac`, `R_Interp_Frame_Begin` writes the
interpolated height into the real `sector_t`/`side_t` and `R_Interp_Frame_End` puts the simulation's
value back. The drawers needed no changes at all.

The price is that **nothing but rendering may happen between those two calls**, and they must
always pair. An unmatched `Begin` leaves interpolated heights in place, the next tic's collision
and sight checks read them as if the simulation had produced them, and the game genuinely diverges
— the one way this feature can affect play rather than just the picture. There is a `PARANOIA`
check in `R_UpdateInterpolations` asserting a tic never begins with `frame_interpolated` set.

The registry itself (`interp_register`/`interp_unregister`, `interp_slot[]` in `sector_t` and
`side_t`) is maintained **unconditionally**, not gated on whether interpolation is currently
active. Gating it meant that turning the cvar on mid-game interpolated from stale history. It costs
a handful of entries.

### 3. Mobj snapshots are lazy, not a sweep

A thing cannot simply snapshot itself at the top of its own think: plenty of things are moved by
something *else* — a lift carrying a player, a crusher pushing one down — and if that runs first,
the "previous" position captured afterwards is already the new one and the thing does not
interpolate at all. Sweeping every mobj before the thinkers would fix it but costs a pass over
every thing in the level every tic, almost all of which never move.

So a parity bit flips once per tic and each mobj records the parity it last captured at. Whichever
code touches the thing first that tic takes the snapshot; the rest are no-ops. `R_Interp_Capture_Mobj`
goes at the top of anything that can move a thing — currently `P_MobjThinker`,
`P_BlasterMobjThinker`, `P_ThingHeightClip` and `P_PlayerThink`.

**`P_PlayerThink` is the one that is not obvious, and leaving it out is a real bug that looks like
the feature half-working.** `P_PlayerThink` runs *before* `P_RunThinkers` and is where `pmo->angle`
is set from the ticcmd, so by the time `P_MobjThinker`'s capture runs the turn has already
happened and `PrevAngle == angle`. The symptom is that walking is smooth but **turning still snaps
35 times a second** — and turning is the more visible half. It was caught by tracing `viewangle`
per frame and noticing it was constant within each tic; it is invisible in a still.

## Discontinuities

A thing that was *placed* rather than *moved* has no step to draw, and interpolating one smears it
across the level. These draw whole:

- **Level load** — `R_Interp_Level_Init` from `P_SetupLevel`, after `P_SpawnSpecials` so a level
  that starts with a door already moving is picked up. It also clears the registry, which otherwise
  points at the previous level's freed sectors.
- **Spawn** — `R_Interp_Reset_Mobj` at the end of `P_SpawnMobj`. Without it the `Prev` fields are
  the zeroes from `Z_Malloc` and the first frame streaks the thing in from the map origin.
- **Teleport** — `P_TeleportMove`. All four teleport paths in `p_telept.c` go through it. A
  teleport that *turns* the player also needs `prev_localangle` synced, in `p_telept.c` — otherwise
  the view spins through whatever arc lies between the two headings.
- **Respawn and level entry** — the `localangle[]` assignments in `P_SpawnPlayer`.
- **A change of view target** — respawn, the chase camera coming or going, a spectator switching
  who they watch. `R_SetupFrame` keeps the last view mobj per panel and resets when it changes.
  `R_Interp_Reset_View` also forces `rendertic_frac` to `FRACUNIT` immediately, because
  `R_SetupFrame` runs *after* the frac for the frame was chosen — without that the very frame that
  spots the discontinuity is the one that smears across it.
- **Pause and menus** — `R_Interp_Set_Frac`. Nothing is moving, so interpolating would creep the
  world for one tic and then sit still.

## Vsync, and why it matters now

**OpenGL now honours "Wait Retrace".** The software path has always passed `cv_vidwait` to
`SDL_CreateRenderer` as `PRESENTVSYNC`, but nothing ever called `SDL_GL_SetSwapInterval`, so in
OpenGL the menu setting did nothing at all and the frame rate was whatever the driver defaulted to.
That did not matter while the game drew exactly one frame per tic. It matters now: **it is the only
thing stopping the main loop spinning as fast as the GPU will go**, which on an unattended cabinet
means a core at 100% and the fan running for nothing.

It is applied at GL context creation (`OglSdl_SetMode`), so changing it needs a video mode change to
take effect — the same as the software path, where it is a renderer creation flag. Late swap
tearing (`-1`) is tried first and falls back to plain vsync (`1`): it avoids the hard halving to
30 fps when a frame misses the refresh, which on a twitch cabinet is worse than the tear it allows.

**If uncapped is on, leave Wait Retrace on.**

## `I_GetTimeFrac`

New in the SMIF interface (`i_system.h`). Returns 0..`FRACUNIT` for the position within the current
tic. Only the SDL backend has a real implementation; it shares `tick_basetime` with `I_GetTime`
deliberately, so the two can never disagree about which tic it is — a frac from an independent
timer drifts and the picture jitters by a whole tic wherever the two round differently.

The five dormant backends (`linux_x`, `win32`, `macos`, `os2`, `djgppdos`) return `FRACUNIT`, which
leaves them running exactly as they did. None of them is built here — both the Linux and Windows
builds are `SMIF_SDL` — but the declaration is in the shared header, and an undefined reference is
how a dead backend stops linking.

### Unrelated latent bug found here

`I_GetTime` computes `(ticks - basetime) * TICRATE / 1000` in 32-bit. That overflows after about
**34 hours of uptime**, at which point the tic counter jumps backwards. This cabinet is left
switched on. `I_GetTimeFrac` uses a 64-bit intermediate and is not affected. `I_GetTime` was left
alone: changing the tic clock is not something to do in the same commit as a rendering change.

## How this was verified

Without a screen, and worth repeating after any change here:

- **Determinism.** Play back a real record demo with `uncapped 0` and `uncapped 1`, dumping player
  x/y/z/angle per tic from `P_Ticker`, and diff. 453 tics of `doomu_E1M1_sk0_speed.lmp` came back
  byte-identical. This is the check that matters; run it before believing anything else.
- **That interpolation actually engaged** — otherwise the identical trace above is vacuous. Count
  frames per tic and the distribution of `frac`: 3–4 frames per tic under the dummy driver, `frac`
  sweeping 0..`FRACUNIT`, and *zero* frames at all with the cvar off.
- **That the view really moves between tics.** Trace `viewx`/`viewangle` per frame for a few tics
  and check the interpolated values land between the capped ones. **This is what caught the
  `P_PlayerThink` capture being missing** — the frame counts and the determinism check both passed
  while the view angle was still snapping.
- **All five mover kinds register**, and a freed slot is reused: trace `interp_register`. On E1M1
  that is 8 wall scrollers at level start, then doors and lifts as they are triggered, with slot 10
  taken twice by different movers.
- `make smoke` (5/5), and an `SDL_VIDEODRIVER=offscreen` OpenGL run on the real GPU.

## What is not done

- **Weapon sprites (psprites) are not interpolated.** They are driven by `P_MovePsprites` at tic
  rate and still step. Less noticeable than the world, but it is the obvious next piece.
- **The automap is not interpolated.** It skips the bracketed block entirely.
- `SCROLL_carry` / `SCROLL_carry_ceiling` are deliberately not registered — they move *things*, not
  the texture, and the things they carry interpolate as mobjs like anything else.
- No frame limiter of its own; vsync is the limiter. See above.
