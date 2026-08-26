# The screen wipe (melt and crossfade)

*Part of the DoomLegacy arcade cabinet build. Read before changing `f_wipe.c`, the wipe block in
`D_Display` (`d_main.c`), or the `ReadScreenRect`/`DrawScreenRect` backend entry points.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

## What was already there

Nothing about the wipe needed inventing — this is worth knowing before anyone "adds" it again:

- `f_wipe.c` has always had **both** wipes: `wipe_Melt`, the real Doom column melt, and
  `wipe_ColorXForm`, a crossfade.
- The cvar has always existed: **`screenlink`** (`cv_screenslink`, `d_main.c`), values
  **None / Crossfade / Melt**, compiled default `"2"` = Melt.
- It has always had a menu row: **"Screens Link"**, first row group of Menu Options (`m_menu.c`).
- The cabinet's tracked `config.cfg` has always said `screenlink "Melt"`.

- **It was nevertheless dead on this machine, because the wipe was gated to the software
  renderer.** `D_Display` read
  `if (gamestate != wipegamestate && rendermode == render_soft)`, and the cabinet runs OpenGL, so
  `wipe` was never armed and `wipe_EndScreen()` was never reached. The setting was real, saved and
  displayed, and did nothing whatever it was set to.
  - The gate was **load-bearing, not cosmetic**: `I_ReadScreen` (`sdl/i_video.c`) opens with
    `if( rendermode != render_soft ) I_Error("I_ReadScreen: called while in non-software mode")`.
    Deleting the `render_soft` test on its own turns a dead feature into a hard exit on the next
    level end.

## How the hardware path works

The melt itself is renderer-agnostic — `wipe_doMelt` just walks three linear buffers — so it was
reused unchanged in substance. All the work is in getting whole screens in and out.

- **The wipe carries its own geometry now** (`wipe_width`, `wipe_ybytes`, `wipe_bytepp`,
  `wipe_widthbytes`, `wipe_screen_size`, `wipe_dupy`, `wipe_wide_pixel`, set by
  `wipe_set_geometry()`). It used to read `vid.*` directly, which describes **the software
  renderer's framebuffer only** — under OpenGL `vid.screen_size` is not even set (`hw_draw.c` says
  so in as many words). Software fills these from `vid.*` and behaves exactly as before; hardware
  fills them for tightly packed top-down 24-bit RGB. **Do not put `vid.bytepp`/`vid.ybytes` back
  into the wipe** — that is the bug this replaced.
- **The melt's two inner paths are now a run-time choice, not a compile-time one.** The wide-pixel
  branch used to be `#ifdef ENABLE_DRAWEXT` around `vid.drawmode != DRAW8PAL`; both are now always
  built and selected by `wipe_wide_pixel`, because a hardware wipe is 3 bytes per pixel whatever
  the software renderer was configured for.
- **Two new backend entry points**, `ReadScreenRect` and `DrawScreenRect` (`r_opengl.c`), reached
  through `HWR_Wipe_ReadScreen`/`HWR_Wipe_DrawScreen`/`HWR_Wipe_Supported` in `hw_draw.c`.
  Registered in `hwsym_sdl.c` and `sdl/i_video.c`. **A backend that does not implement them leaves
  the pointers NULL**, and `HWR_Wipe_Supported()` then reports no wipe rather than crashing — which
  is why glide/minigl/d3d needed no edits.
- **`ReadScreenRect` returns true RGB; the existing `ReadRect` returns BGR.** That is deliberate,
  not an oversight: `ReadRect` feeds screenshots, and Targa wants BGR. The wipe pair is
  self-consistent and neither has to know about the other.

### Three things that will silently produce nothing if you get them wrong

- **The start screen must come from the FRONT buffer.** `wipe_StartScreen()` runs at the top of
  `D_Display`, immediately after the previous frame was swapped to the monitor, so the **back
  buffer's contents at that moment are undefined** — capturing it gives whatever stale frame the
  driver happened to leave. This is the same property that made the old level-load "Loading..."
  box strobe (see `gotchas.md`). The end screen is the opposite case: it has just been drawn and
  not yet swapped, so it is the normal back-buffer read.
  - This is the one part with an environmental dependency. Reading the front buffer is reliable on
    fullscreen X11, where the compositor unredirects; under some compositors it can come back
    blank. **The symptom would be a melt sliding away a black or stale image, not a crash.** The
    fallback if that ever shows up is to keep a GPU-side copy of each frame
    (`glCopyTexSubImage2D`) instead, which costs a new entry point but no readback.
- **Row alignment.** `glReadPixels`/`glDrawPixels` default to 4-byte row alignment. At the
  cabinet's 1366 wide, a row is 1366&nbsp;×&nbsp;3 = **4098 bytes, not a multiple of 4**, so the
  default pads every row and skews the whole image into a diagonal smear. Both new functions set
  `GL_PACK_ALIGNMENT`/`GL_UNPACK_ALIGNMENT` to 1 for the transfer and put it back. (Note the
  pre-existing `ReadRect` does *not* do this, so screenshots at such widths are likely already
  skewed — untested, but worth knowing.)
- **`glRasterPos` goes through the modelview and projection matrices, and a raster position that
  lands outside the viewport is marked invalid — `glDrawPixels` then draws nothing at all, with no
  error.** `DrawScreenRect` therefore pushes its own identity ortho rather than inheriting whatever
  the last 3D frame left set, and rasters at `screen_height - 1`, not `screen_height`, which would
  be exactly off the edge and invalid. GL rows run bottom-to-top, so it draws top-down via
  `glPixelZoom(1, -1)`.

## Two latent bugs fixed on the way

- **A timed-out wipe used to poison the next one.** `D_Display` abandons a wipe after 2 seconds;
  that left `go` set and `wipes_exit` uncalled, so the *next* wipe skipped its init and ran on the
  previous wipe's state — including a `melty[]` that had been `Z_Free`d. `wipe_StartScreen()` now
  clears `go`, `wipe_initMelt` frees any surviving `melty`, and `wipe_exitMelt` NULLs it.
- **The capture used to run even with the wipe turned off.** The gate did not test
  `cv_screenslink`, so `wipe_StartScreen()` did a full screen read and `D_Display` then abandoned
  the wipe at `if (!cv_screenslink.value) return;`. Harmless in software; in hardware it would have
  leaked three screens' worth of buffers every transition. The gate tests the cvar now, and
  `wipe_StartScreen()` returning non-zero (a failed capture) disarms the wipe instead of melting
  from buffers that are not there.

## Verifying it

- **The hardware path cannot be checked headlessly** — the SDL dummy driver gives no GL context.
  That half needs a real play session.
- **The software path can, and should be, because the refactor above touches it.** Force it with
  `drawmode "Software 8bit"` in the scratch `config.cfg` — the value must be one of the exact
  strings in `drawmode_sel_t` (`v_video.c`); `"8 bit"` is not one and silently leaves OpenGL
  selected, which looks like the setting being ignored.
- Drive a level exit with the `wait`/`exitlevel` autoexec from `CLAUDE.md`. A gamestate change is a
  wipe trigger, so reaching the intermission exercises a whole melt. A temporary `GenPrintf` in
  `wipe_ScreenWipe` reporting `wipeno`, `rc` and the geometry is how this was confirmed: two melts
  of ~62 frames each, both ending `rc=1` (completed) rather than timing out, at
  `800x600 bytepp=4 ybytes=3200 dupy=3`.
