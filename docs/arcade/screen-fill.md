# Filling the screen: the 2D page scale, and the OpenGL mode change

*Part of the DoomLegacy arcade cabinet build. Read before changing `V_SetupDraw`, anything in
`v_video.c` that scales by `drawinfo`, the `V_SCALEEXACT` call sites, or the mode-change path in
`sdl/i_video.c` / `sdl/ogl_sdl.c`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index. For the SDL2
*present* path — the texture pitch, the fullscreen mode list, the startup window — see
`software-fullscreen.md`. For a mode change that fails *after* the rendermode teardown and looks
like a freeze, see `drawmode-switching.md`.

---

Two separate faults, reported together, both of the shape "the screen is not the size I asked for".

---

## Part 1 — 2D pages did not fill the screen in software

### The symptom

On the cabinet (1366x768, `drawmode "Software 24bit"`) the attract slides and the high-score board
sat in a box in the upper middle of the screen, with the green flat border tiled down both sides and
across the bottom. Under OpenGL the same pages filled the display. It reads like the pages are the
wrong size; what is actually wrong is the *scale they are drawn at*.

### Measured, before the fix

A screenshot of `TITLEPIC` at 1366x768, with the picture's bounding box found by comparing every
pixel against the tiled border flat:

```
screen 1366x768   picture x 43..1322 (1280 wide)   y 0..599 (600 tall)
```

1280 = 320 x 4 and 600 = 200 x 3. So 43px of flat down each side, 168px across the bottom — 22% of
the screen height. At 1024x768 the same measurement gives 960x600: 32px each side, 168 bottom. It is
not specific to one mode; it happens at every resolution that is not a whole multiple of 320x200,
which is nearly all of them.

### The cause

`V_Setup_VideoDraw` (`v_video.c`) derives two scales from the video mode:

```c
vid.dupx  = vid.width / BASEVIDWIDTH;          // 1366/320 = 4   (truncated)
vid.fdupx = (float)vid.width / BASEVIDWIDTH;   // 4.26875        (exact)
```

The hardware renderer scales by `fdupx`/`fdupy` and therefore always covered the screen. Every
software drawer scaled by the **whole number** `dupx`/`dupy`, through `drawinfo.xbytes`
(`dupx * bytepp`), `drawinfo.ybytes` (`dupy * ybytes`) and their `x0bytes`/`y0bytes` start-coordinate
counterparts. A 320x200 page therefore came out `(320*dupx) x (200*dupy)` and the remainder was left
over. `D_PageDrawer` tiles a flat into that remainder, which is why the leftover reads as a
deliberate green border rather than as black.

Upstream's comment says as much — *"software mode which uses generally lower resolutions doesn't look
good when the pic is scaled, so it fills space around with a pattern"*. That was a reasonable trade
at 320x200 in 1998. At 4x on a 1366x768 panel it is not, and the cabinet's own OpenGL mode is the
counter-example sitting right there.

### The fix

`drawinfo` carries the software draw scale as **16.16 fixed point** — `x_scale`, `y_scale` for
`V_SCALEPATCH`, and `x0_scale`, `y0_scale` for `V_SCALESTART` — and a new screenflag,
**`V_SCALEEXACT`**, chooses whether that scale is the whole number or the exact one:

```c
if( screenflags & V_SCALEEXACT )
{
    drawinfo.x_scale = (fixed_t)( drawinfo.fdupx * FRACUNIT );   // 4.26875
    ...
}
else
{
    drawinfo.x_scale = drawinfo.dupx << FRACBITS;                // 4.0, as before
    ...
}
```

Every software drawer now goes through two macros (`v_video.h`) instead of multiplying by
`xbytes`/`ybytes`:

```c
#define V_scale_x(v)   ((((v) * drawinfo.x_scale) + (FRACUNIT/2)) >> FRACBITS)
#define V_scale_y(v)   ((((v) * drawinfo.y_scale) + (FRACUNIT/2)) >> FRACBITS)
```

**They round, and the rounding is the point.** A patch column is drawn as a series of posts, each
placed at `V_scale_y(topdelta)` and run for
`V_scale_y(topdelta + length) - V_scale_y(topdelta)` rows. Because both ends go through the same
rounded scale, a post ends exactly where the next one begins: no seam and no overlap, at any scale.
Computing the length as `round(length * scale)` instead leaves a one-row gap wherever the fraction
happens to fall, and those gaps are individually invisible and collectively obvious.

The same argument fixes the total width: `V_scale_x(320)` with a scale of 4.26875 is
`round(1366.0) = 1366`, exactly the screen, so `V_CENTERHORZ` contributes a start offset of zero and
the page lands flush at both edges. Truncating instead of rounding gives 1365 and leaves a
one-pixel column of flat down the right-hand side.

### Where the flag is set

Whole-screen 2D pages only — the places whose entire job is to cover the display:

| call site | page |
| --- | --- |
| `D_PageDrawer` (`d_main.c`) | attract slides: TITLEPIC, CREDIT, CREDIT2, the arcade splash |
| `HS_Draw_AttractTable` (`hs_stuff.c`) | the high-score board on the attract cycle |
| `WI_Drawer` and the two background setups (`wi_stuff.c`) | the intermission, and the arcade record tables on it |
| `F_Drawer` (`f_finale.c`) | the ending text screens and the cast call |

**The menus, the HUD and the status bar deliberately keep the whole-number scale.** They are not
pages — they are overlays positioned against a screen edge or a view cell, their arcade geometry was
measured by hand against the whole-number scale, and crisp integer pixel art is a defensible look
for them. If that is ever revisited, it is one flag per call site, not another mechanism.

### Why the refactor is safe, and how that was shown

With `V_SCALEEXACT` off, `x_scale` is `dupx << FRACBITS` and `V_scale_x(v)` reduces to `v * dupx`
exactly — the same arithmetic the old `xbytes` multiplication did. That is a provable property, so
it was proved, by building three binaries and comparing screenshots byte for byte:

```
                                                        attract pages   score board
pristine  vs  refactor with V_SCALEEXACT off  @1024x768   identical      identical
pristine  vs  refactor, OpenGL drawmode                   identical      identical
```

`make smoke` passes 5/5.

Three things about that comparison are worth keeping:

- **Establish run-to-run determinism first, and per screen.** The static attract pages reproduce
  bit-for-bit across runs; a live `-warp` level and an attract *demo* do not — the pristine binary
  differs from *itself* on exactly the demo frames. Comparing those frames proves nothing, and
  believing them would have produced a phantom regression. Check the binary against itself before
  reading anything into a diff.
- **Compare at the same resolution.** The first attempt compared a pristine build against a
  refactored one and got hundreds of thousands of differing pixels — because the pristine binary,
  lacking the test-only mode-list entry, had quietly run at 1024x768 while the other ran at
  1366x768. The tell was that the differences stopped dead at x=1023. Print the drawing size from
  both logs before diffing.
- **Prove the check can fail.** It did, twice, unprompted: the resolution mismatch above, and a real
  bug of mine (below).

### The bug the check caught

The first build filled vertically but stopped at x=1279 — the old whole-number width — with the
flat still showing down the right. The scale was right (a temporary probe printed
`xs=279756 sx320=1366 off=0`); the drawer was not. The replacement that was supposed to update

```c
destend = desttop + (patch->width * drawinfo.xbytes);   // test against desttop
```

in `V_DrawScaledPatch` had matched a **commented-out copy of the same line** in `V_DrawMappedPatch`
first — `//    destend = ...` contains the indented text being searched for — so the live line was
never touched and only the commented one changed. Nothing warned: it builds, it runs, and the
vertical axis (patched correctly) filled, which made it look like a subtle rounding problem rather
than an edit that had missed. **When a mechanical edit is applied by text match in this tree, check
the count and check where it landed** — several of these drawers keep commented-out copies of their
own key lines.

---

## Part 2 — the video mode could not be changed under OpenGL

### The symptom

Picking a different resolution in the video menu while the OpenGL drawmode was active appeared to do
nothing; the game stayed at the desktop resolution.

### The cause

`VID_SetMode` (`sdl/i_video.c`) ended, under SDL2, with:

```c
// sdl_window is shared and required for both sw and hw.
if( (sdl_window == NULL) || (sdl_texture == NULL) )  goto fail;
```

`sdl_texture` belongs to the **software** present path. It is created only in `VID_SetMode_vid`,
which `VID_SetMode` calls only for `render_soft`, and `OglSdl_SetMode` destroys whatever a previous
software mode left behind (`VID_SDL_release`). In OpenGL it is therefore always NULL, so **every**
OpenGL mode change took `goto fail`, however well `OglSdl_SetMode` had just gone:

```
stock:  TMPVID request 1024x768 -> modetype=2 index=1
        VID_SetMode(fullscreen,1)
        Warn: VID_SetMode failed to provide display        <-- every time
fixed:  TMPVID request 1024x768 -> modetype=2 index=1
        VID_SetMode(fullscreen,1)
                                                           <-- gone
```

`I_SoftError` only prints, so the failure is not fatal — but the two lines after the test are the
ones that never ran:

```c
vid.modenum = modenum;
vid.fullscreen = set_fullscreen;
```

so the engine went on describing the mode it used to be in. `M_DrawVideoMode` (`m_menu.c`) picks the
highlighted entry by comparing each mode against `vid.modenum`, so the menu kept pointing at the old
resolution, next to a console line saying the mode change had failed.

The test is now applied only where the texture is actually used:

```c
if( sdl_window == NULL )  goto fail;
if( (rendermode == render_soft) && (sdl_texture == NULL) )  goto fail;
```

### The second one, in the same path

`OglSdl_SetMode` took the engine's dimensions from the *display mode*:

```c
SDL_GetWindowDisplayMode( sdl_window, &sdl_displaymode );
vid.width  = sdl_displaymode.w;
vid.height = sdl_displaymode.h;
```

`SDL_GetWindowDisplayMode` reports the mode the window would use **if it were fullscreen**, which for
a windowed GL window is just the desktop mode — and, worse, for a fullscreen window it reports the
mode SDL *intends* rather than the one it got. It now uses `SDL_GL_GetDrawableSize`, which is the
real pixel size of the GL drawable in both window and fullscreen, and on a HiDPI display is the
backing-store size, which is what a viewport wants. The display mode is still used for the pixel
format.

### The third one, which was the actual complaint

With both of the above fixed the resolution *still* did not change. The engine was asking correctly —
the log showed the mode list intact, `VID_GetModeForSize(800,600)` resolving to index 5, and
`VID_SetMode(fullscreen,5)` calling `OglSdl_SetMode(800,600,1)` — and getting back a window the size
of the desktop:

```
DIAG request 800x600 -> modetype=2 index=5 (800x600)
VID_SetMode(fullscreen,5)
DIAG OglSdl_SetMode asked 800x600 fs=1 flags=0x113
DIAG   window 1366x768 drawable 1366x768 display 1366x768 winflags=0x517
  OpenGL Got 1366x768, 24 bpp
```

The tell is in `winflags`. The window that worked (at startup) came back **0x717**; the one that did
not came back **0x517**. The missing bit is `0x200`, `SDL_WINDOW_INPUT_FOCUS`. `SDL_WINDOW_FULLSCREEN`
is set in both — SDL believes the window is fullscreen — but its X11 backend only *applies* the
display mode once the window has input focus, and a window created in the same breath as its
predecessor was destroyed does not reliably get it. So the window sat there flagged fullscreen at the
desktop's size, `glViewport` was set to that, and every resolution the operator picked rendered at
1366x768.

Creating the window with `SDL_WINDOW_FULLSCREEN` also leaves SDL to *infer* which mode to use, from
the window's dimensions. The fix pins it and re-asserts fullscreen so it is applied without waiting
for focus:

```c
SDL_GetClosestDisplayMode( di, &want, &got );
SDL_SetWindowDisplayMode( sdl_window, &got );
SDL_SetWindowFullscreen( sdl_window, 0 );
SDL_SetWindowSize( sdl_window, w, h );
SDL_SetWindowFullscreen( sdl_window, SDL_WINDOW_FULLSCREEN );
SDL_RaiseWindow( sdl_window );
```

followed by a bounded wait — at most 20 x 10ms, and only spent if the size has not already arrived —
because the final size reaches SDL by `ConfigureNotify` and reading it immediately can read a stale
one. If it still does not match, the engine now **says so** (`OpenGL: asked 800x600 fullscreen, got
1366x768`) instead of rendering at a size nobody asked for. Verified on the cabinet's own display:

```
VID_SetMode(fullscreen,5)   OpenGL Got 800x600, 24 bpp   vid 800x600
VID_SetMode(fullscreen,6)   OpenGL Got 640x480, 24 bpp   vid 640x480
```

### Why a standalone probe did not reproduce it

Three separate SDL2 probes — a bare fullscreen GL window, one that pinned the display mode first, and
one that replicated the whole sequence (software window with renderer and texture, released, GL
fullscreen at the desktop size, released, GL fullscreen at 800x600) — **all got the size they asked
for**, and the display really switched. Only the game reproduced it, because only the game fails to
get input focus on the new window.

Two lessons worth keeping:

- **A probe that does not reproduce the fault has not exonerated the code.** It took four rounds
  before the reproducer was accepted as the only reliable one, and the fix was then developed against
  the game rather than the probe.
- **`SDL_GetError()` is a red herring here.** The failing run showed `err='Invalid window'` right
  after a *successful* `SDL_CreateWindow`, which looks damning. The probe that worked perfectly showed
  the same string: it is stale, left by an earlier call, and SDL does not clear it on success. Check
  the return value, not the error string.

There is no scaling step in the hardware renderer — `VIDGL_Set_GL_Model_View` just calls
`glViewport(0, 0, vid.width, vid.height)` — so unlike software, where the frame is drawn into a
buffer and scaled into the window by SDL, an OpenGL resolution change is *only* a real display mode
change. If a machine ever turns up where the mode cannot be switched (a Wayland session, for one),
the warning above will fire and OpenGL will simply render at the desktop resolution; making it do
otherwise would need rendering to a framebuffer object and blitting it scaled, which is not built.

## Verifying a change here

- `tools/smoke.sh` (`make smoke`) covers this only as far as "it still runs".
- For the 2D scale: screenshot a static attract page under `SDL_VIDEODRIVER=offscreen` and measure
  the picture's bounding box against the border flat, as above. Do not eyeball it — a page that is
  one pixel short of the edge looks finished.
- For a change to the scale arithmetic: build the previous commit, run both with `V_SCALEEXACT`
  removed from its call sites, and compare screenshots byte for byte. They must be identical.
  Establish run-to-run determinism first, and only trust the static pages.
- For the mode-change path: the `offscreen` driver advertises one display mode, so a headless run
  cannot change resolution and watch it take — this one needs the real display. Run the game on
  `DISPLAY=:0` with `-v -v`, drive a mode change, and read the `OpenGL Got %ix%i` line: it must be
  the size that was asked for. `grep 'VID_SetMode failed'` and `grep 'OpenGL: asked'` catch the two
  known failures.
