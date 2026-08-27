# Switching drawmode, and the "lockup" that is not one

*Part of the DoomLegacy arcade cabinet build. Read before changing `V_switch_drawmode`,
`SCR_apply_video_settings`, `SCR_SetMode`, or the per-drawmode config files.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

## The symptom

Selecting any **software** drawmode in Options → Video Options → Draw Mode and pressing Enter froze
the cabinet. Nothing on screen, no message, no response.

## It is not a freeze

The engine is still running. Measured on a headless run driven through the same code path as the
menu: **over 900,000 main-loop iterations after the "lockup"**. What died is the *display*, not the
process — which on a fullscreen cabinet is indistinguishable from a hang, because there is nothing
left to draw the difference on.

That distinction matters for diagnosis. Do not go looking for a deadlock; look for a failed video
mode change.

## The cause

`legacyhome` holds **one config file per drawmode** — `configgl.cfg`, `config8p.cfg`,
`confign.cfg` — beside the main `config.cfg`. Switching drawmode saves the outgoing drawmode's file
and loads the incoming one (`V_switch_drawmode`, under `if( change_config )`).

Each of those files carries its own `scr_depth`, and nothing checked it against the drawmode:

1. `V_switch_drawmode` picks the drawmode's real bit depth — `req_bitpp = drawmode_to_bpp[drawmode]`,
   so 8 for `DRM_8pal` — and queries the mode list with it. **This check passes.**
2. It then loads the drawmode's config file, which overwrites `cv_scr_depth`.
3. `SCR_apply_video_settings` takes `req_bitpp = cv_scr_depth.value` **straight from the config**,
   discarding the depth that was just validated.
4. `I_RequestFullGraphics` is asked for a 32-bit mode in an 8-bit palette drawmode. There is none:
   `No 32 bpp modes`, `Change Graphics failed: err=-102`.
5. The rendermode teardown in `SCR_SetMode` has **already run** by this point, so the engine carries
   on with no usable display.

On this cabinet every one of the four config files said `scr_depth "32 bits"`, including the 8-bit
palette one — so every software drawmode failed this way.

Why they all said 32: `create_initial_drawmode_config()` (`m_menu.c`, the **C** key on the Draw Mode
page) writes whatever `cv_scr_depth` happens to be *at that moment* into the new drawmode's file. Do
it from OpenGL at 32 bits and the palette drawmode's config is born broken.

## The fix

`V_switch_drawmode` now re-asserts the drawmode's own depth immediately after loading its config and
before `SCR_apply_video_settings`:

```c
if( drawmode <= DRM_32 )
    CV_SetValue( &cv_scr_depth, drawmode_to_bpp[drawmode] );
```

A fixed-bpp software drawmode's depth is not negotiable, so the config does not get a vote. This
also makes an **already-wrong config file harmless**, which matters because `create_initial_
drawmode_config()` can still create one — and it corrects the file on the next config save.

## What was tried first and rejected

The first attempt was a recovery in `SCR_SetMode`: catch the failure and re-enter `SCR_SetMode` with
the previous drawmode. **It made things worse and was backed out.**

It appeared to work — the fallback message printed and OpenGL came back — and then the process died
with `Z_ChangeTag: free block has corrupt ZONEID: 6`. The failed pass had already run the rendermode
teardown (`ST_Release_Graphics`, `HWR_Shutdown_Render`, `Z_FreeTags`) and moved `HWR_patchstore`;
re-entering ran it a second time against the already-released state. **Recovering after the teardown
is not safe.** If a future change needs a fallback, it has to refuse the switch *before* anything is
released.

The engine already has that: `V_switch_drawmode`'s `query_reject` returns 0 before `rendermode_recalc`
is set or any config is loaded, and `SCR_SetMode` treats 0 as a clean cancel. Fixing the depth is
what makes that existing up-front check meaningful, which is why no new recovery path is needed.

## A separate bug found on the way, not fixed

**The console `drawmode` command does not change the drawmode.** `Set_drawmode_OnChange`
(`v_video.c`) sets only `drawmode_recalc = true` and never touches `set_drawmode`, so `SCR_SetMode`
re-applies whatever `set_drawmode` already held — the value from startup. Typing
`drawmode "Software 8bit"` sets the cvar, reports success, and re-initialises the *old* renderer.
Only the menu works, because `change_drawmode:` in `m_menu.c` assigns `set_drawmode` explicitly.

This cost real time here: an early attempt to reproduce the bug through the console appeared to show
the switch succeeding when nothing had switched. **Do not use the console command to test drawmode
changes.**

## Verification

Headless on the real GPU, `SDL_VIDEODRIVER=offscreen`, with a temporary `dmset` console command that
does exactly what the menu's `change_drawmode:` does (`set_drawmode = n; drawmode_recalc = true;`) —
the console `drawmode` command is useless for this, per above. A temporary heartbeat in `D_DoomLoop`
supplied liveness, since `timeout` exit 124 proves nothing on its own and `echo` does not reach
stdout.

- **Before:** OpenGL → Software 8bit gave `No 32 bpp modes` / `Change Graphics failed: err=-102`,
  then 922,127 heartbeats — running, no display.
- **After:** the same switch gives a real mode change (`VID_SetMode(window,7)`), no graphics error,
  and 1,165,582 heartbeats.
- **Round trip:** OpenGL → Software 8bit → OpenGL → Software 32bit, no errors and no zone
  corruption, 999,235 heartbeats after the last switch.

All instrumentation (`dmset`, the heartbeat) was removed before committing; it is not in the tree.
