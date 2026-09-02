# Software rendering in fullscreen, and the pitch that skews it

*Part of the DoomLegacy arcade cabinet build. Read before changing `VID_SetMode_vid`,
`I_FinishUpdate`, the startup window, or anything else that touches `sdl_texture`, `vidSurface`,
`vid.direct*` or the fullscreen mode list in `sdl/i_video.c`. The startup window also involves the
self-refresh block in `console.c`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index. For the
*other* way a video mode change goes wrong — the one that looks like a freeze — see
`drawmode-switching.md`.

---

## The symptom

Software rendering in a window was perfect. Going fullscreen produced a garbled mess: the top line
of the screen correct, everything below it sheared progressively sideways. Found on a Raspberry Pi,
but nothing about it is Pi-specific.

It reads as a colour depth or a video mode problem. It is neither. It is one wrong argument.

## How the software screen reaches the display under SDL2

The engine never draws to the window. It draws into its own `malloc`ed buffer (`vid.display`, rows
`vid.ybytes` apart) and, once a frame, hands that buffer to a streaming texture:

```c
SDL_UpdateTexture( sdl_texture, NULL, vid.display, <pitch> );
SDL_RenderCopy( sdl_renderer, sdl_texture, NULL, NULL );
SDL_RenderPresent( sdl_renderer );
```

`<pitch>` is the distance between rows **in the source**, i.e. `vid.ybytes`. Stock passed
`vid.direct_rowbytes` — the pitch of `vidSurface`, which is a *completely different buffer*: the
window's own framebuffer, obtained from `SDL_GetWindowSurface`, which under SDL2 is never presented
at all. It is fetched only so `I_SetPalette` has somewhere to put an 8-bit palette.

The two are equal whenever the window is exactly the size the engine is drawing — which is always
true in a window, because the window is created at the requested size. **That is the whole reason
the bug hides until you go fullscreen.**

In fullscreen they routinely differ. The fullscreen mode list is built from the display's real
modes filtered to `w <= MAXVIDWIDTH` (1600) and `h <= MAXVIDHEIGHT` (1200), so on any 1920x1080
panel the native mode is *excluded* and fullscreen always asks for something the display is not
currently showing. Whether that request produces a real mode switch or a desktop-sized window is up
to the driver, the window manager and the compositor. When it is the latter, `SDL_GetWindowSurface`
returns a desktop-sized surface, and every row the engine sends is read from the wrong offset.

## Measured

A standalone probe running the same sequence — fullscreen window, renderer, streaming texture at the
drawing size, a source buffer filled with a known pattern, `SDL_RenderReadPixels` on a 1:1 render
target to read back exactly what landed:

```
draw   640x400  format SDL_PIXELFORMAT_RGB888  bytepp 4  ybytes 2560
window 1024x768  surface 1024x768  pitch 4096

  old: window surface pitch    pitch=4096   wrong pixels: 255357 / 256000
                               first bad row: 1
  new: source buffer ybytes    pitch=2560   wrong pixels: 0 / 256000   (clean)
```

Row 0 is correct and everything after it is wrong, which is exactly the shear that was seen. The
fix is pixel-exact. Identical under both the `dummy` and `offscreen` video drivers, so this needs no
screen to reproduce: create the window with `SDL_WINDOW_FULLSCREEN_DESKTOP` at a size other than the
desktop's and the mismatch appears on any driver.

Note the read is also *out of bounds*: 400 rows at a 4096-byte stride is 1.6 MB taken from a
1.0 MB buffer. It garbles rather than crashes only because `vid.buffer` is allocated
`NUMSCREENS` times over.

## The rule

**`SDL_UpdateTexture` takes the pitch of the data you are giving it, not the pitch of anything on
the screen.**

## The second bug, which was mine

The first attempt at this went further and took `vid.bitpp` / `vid.bytepp` from the texture as well,
via `SDL_BITSPERPIXEL()` / `SDL_BYTESPERPIXEL()` of the format enum, on the reasoning that the
texture is what gets filled so the texture is what must be described. The reasoning is right. The
macros are a trap:

```
                             SDL_PixelFormat | SDL_BITSPERPIXEL macro
  SDL_PIXELFORMAT_RGB888        32 bpp       |       24 bpp        DIFFER
  SDL_PIXELFORMAT_BGR888        32 bpp       |       24 bpp        DIFFER
  SDL_PIXELFORMAT_ARGB8888      32 bpp       |       32 bpp        same
  SDL_PIXELFORMAT_RGB565        16 bpp       |       16 bpp        same
  ...
```

For the packed 32-bit-with-no-alpha formats the macro reports the bits that carry **colour** (24)
while an `SDL_PixelFormat` reports the bits a pixel **occupies** (32). `RGB888` is the format an
ordinary X11 window has, so `vid.bitpp` went 32 → 24 on every such machine, `V_Setup_VideoDraw`
selected `DRAW24` instead of `DRAW32`, and the software renderer drew 4-byte pixels with the 3-byte
drawer. Every texture on screen came out mangled — in the software and native drawmodes only, since
OpenGL does not go through this at all.

So: derive the drawing format with **`SDL_AllocFormat( pixel_format )`**, never the macros. Where a
window surface exists that yields its `format->BitsPerPixel` and `format->BytesPerPixel` unchanged —
it is the same call SDL made to build the surface's own format — so this is bit-for-bit what stock
did. Where there is no surface it is derived the same way instead of dereferencing NULL.

The texture is then created from the **window surface's** format rather than
`SDL_GetWindowPixelFormat()`, which reports the format of the display *mode* — a different thing
that need not match the window's framebuffer. Both ends now come from one place, which is what the
original bug was really about. If the renderer cannot make a texture in that format, the code falls
back to the display mode format *and moves the drawing format with it*, so the two can never drift
apart again.

## `vidSurface` can legitimately be NULL

`SDL_GetWindowSurface` needs the video driver to supply its own framebuffer. X11 and Wayland do;
**KMSDRM does not**, and there SDL falls back to creating a renderer for the window — which already
has one — and returns NULL. Stock dereferenced it immediately (`vidSurface->format->BitsPerPixel`),
so a Pi booted straight to KMSDRM with no X server would have segfaulted in `VID_SetMode_vid`
rather than garbled. It is guarded now; the only loss without a surface is the 8-bit palette, which
the SDL2 path does not use anyway (the texture is never `INDEX8` — SDL2 has no palettized textures,
so the engine always ends up drawing at the window's depth).

## Drawing size does not have to match the window, and now it usually doesn't

`SDL_RenderCopy( ..., NULL, NULL )` scales the texture to the whole window, so the engine's
resolution is independent of the display's. `SDL_HINT_RENDER_SCALE_QUALITY` is `nearest`, so the
upscale stays sharp rather than blurring.

The mode-list filtering above was a leftover from SDL 1.2, where the drawing buffer *was* the video
mode, and it had two consequences on a machine that wants to render small:

- **A low resolution could not be selected fullscreen at all.** The list holds only what the display
  advertises, and `VID_GetModeForSize` snaps a request to the nearest entry — so a Pi set to
  320x200 came up rendering 1024x768 in software, which on that machine is the difference between
  comfortable and unplayable. Nothing reported the substitution.
- **A display that advertises nothing small enough could not run software mode at all.** Every mode
  wider than `MAXVIDWIDTH` (1600) or taller than `MAXVIDHEIGHT` (1200) is filtered out, so a panel
  offering only 1920x1080 leaves the list empty — and that is not a fallback, it is
  `I_Error( "FullGraphics: setup drawmode failed, cannot use native window." )`. The game does not
  start. Reproduced by building with `MAXVIDWIDTH`/`MAXVIDHEIGHT` lowered to 800x600 so the one
  advertised mode is filtered out:

  ```
  before:  No modes for Fullscreen, 8 bitpp
           FullGraphics: setup drawmode failed, no valid modes.
           No modes for Window, 24 bitpp
           Error: FullGraphics: setup drawmode failed, cannot use native window.
  after:   Found 5 Video Modes at 24 bpp
           VID_SetMode(fullscreen,1)
             Draw 320x200, 32 bpp, 4 bytes (SDL_PIXELFORMAT_RGB888), window 1024x768
  ```

So `VID_add_scaled_modes()` appends 320x200, 400x300, 512x384, 640x480 and 800x600 to the fullscreen
list, skipping any the display already advertises for real, and the software fullscreen window is
now **`SDL_WINDOW_FULLSCREEN_DESKTOP`** rather than `SDL_WINDOW_FULLSCREEN`: no mode switch at all,
the desktop stays as it is, and the engine's frame is scaled into it. Three things to keep straight
if this is touched again:

- **Software only.** The hardware renderer has no scaling step — a 320x200 GL fullscreen would draw
  a viewport in the corner of the screen. `VID_add_scaled_modes` returns immediately unless
  `rendermode == render_soft`, and `sdl_window_flags` is read only by `VID_SetMode_vid`, which
  `VID_SetMode` calls only for the software renderer (OpenGL builds its own window in `ogl_sdl.c`).
- **It has to be appended at `found_modes:`, not inside `VID_make_fullscreen_modelist`.** An *empty*
  list is what makes an 8-bit request fall through to `draw_8pal` and rebuild at the native depth;
  modes added before that suppress the fallthrough and the display's own modes drop out of the list
  entirely. That was the first version of this change, and the tell was `Found 5 Video Modes` where
  6 were expected.
- **`VID_Query_Modelist` had to stop applying the size bound — but only for `DRM_8pal` and
  `DRM_native`**, or the drawmode is rejected before the list is ever built and the whole thing
  falls back to a native window. The bound is on what the engine can *draw*, which is no longer the
  size of the mode it is displayed in.

  Relaxing it for *every* software drawmode, which is what the first version did, is a regression.
  The explicit-bpp ones (`Software 15/16/24/32bit`) reach `I_RequestFullGraphics` as
  `DRM_explicit_bpp`, where an empty list is a **dead end** — `No 24 bpp modes`, `goto no_modes` —
  not the fallthrough to `draw_8pal` that rebuilds at the native depth and picks up the scaled
  modes. Letting them past the test traded a clean early rejection (`V_switch_drawmode` returns 0,
  the caller falls back to a native window, nothing is torn down) for

  ```
  Change Graphics failed: err=-102, fullscreen=1
  Modes  24bpp 1024x768No 24 bpp modes
  ```

  arriving *after* the rendermode teardown — which is the dead-display-that-looks-like-a-freeze
  from `drawmode-switching.md`. An explicit bpp the display does not offer must still be refused
  here. With the bound restricted to the two recoverable drawmodes, `no_modes` is unreachable for
  the software renderer: either the depth matches and the list is non-empty, or the drawmode is
  rejected before anything is torn down.

The copy stretches to the window's aspect, so a 4:3 drawing size on a 16:9 panel comes out wide —
the same as it always was for a 4:3 fullscreen mode. `SDL_RenderSetLogicalSize` would pillarbox it
instead, if that is ever wanted.

Mouse motion is in window pixels, so a 320x200 screen scaled to 1080p moves the pointer about six
times slower relative to the game screen. The cabinet has no mouse and the mouse settings are
devmode-only, so this is left alone.

## Verifying a change here

`./doomlegacy -v` prints one line per mode set:

```
  Draw 1024x768, 32 bpp, 4 bytes (SDL_PIXELFORMAT_RGB888), window 1024x768
  Window surface SDL_PIXELFORMAT_RGB888, display mode format SDL_PIXELFORMAT_RGB888
```

**If the two sizes differ, the display is being scaled** — fine in itself, but it is the condition
under which the pitch bug appeared, so it is worth knowing. The second line names the window
surface's format and the display mode's; where *those* differ is where the second bug hid. Both are
printed for the initial window and for every later mode change, including the switch into
fullscreen.

**Check the bpp against stock before believing a change here is harmless.** The `DRAW32`→`DRAW24`
regression was invisible to everything automated: it builds, it runs, `make smoke` passes 5/5, and
it only shows as wrong *pixels*. What catches it is comparing the drawing format with what the
previous build derived — build the old commit in a throwaway worktree, add a temporary
`GenPrintf(EMSG_warn, ...)` of `vid.bitpp`/`vid.bytepp` to it, and run both:

```
STOCK: STOCKPROBE draw 1024x768 bitpp=32 bytepp=4 surf_fmt=SDL_PIXELFORMAT_RGB888
FIXED:   Draw 1024x768, 32 bpp, 4 bytes (SDL_PIXELFORMAT_RGB888), window 1024x768
```

Comparing *screenshots* between the two builds does not work, and the attempt is a good warning: a
`-warp` run is not deterministic enough. Repeated runs of the same binary produce several different
images — the shot lands a tic or two apart depending on load time — so a real difference and normal
jitter look alike. Both builds have to be checked against each other for run-to-run variation before
any frame comparison means anything, and here it meant nothing.

`make smoke` covers this path only as far as "it still runs"; the `warp` and `exitlevel` checks run
under `dummy`, where the window matches the drawing size and the wrong pitch is invisible. A change
to the present path needs the probe above, the bpp comparison, or Mark's eyes on the cabinet.

---

## The startup window

`I_StartupGraphics` opens a window **before the wads are read**, at a fixed 800x600
(`INITIAL_WINDOW_WIDTH`/`HEIGHT`), and `I_RequestFullGraphics` later throws it away and creates the
configured one. On a cabinet that is a wrong-sized window flashing up as the first thing anyone
sees.

**It is not decoration — it is the startup console.** `con_self_refresh` is true until `D_DoomLoop`
clears it, and `GenPrintf_va` (`console.c`) repaints and flips on *every* message while it is set,
so the wad list and the init messages were being drawn there. Two other things draw to it as well,
and they are the reason it cannot simply be deleted: the **fatal-error console** and the
**Launcher**, both in `d_main.c`, run before the real video mode exists and are the only way an
operator with no terminal sees a startup failure.

So it is suppressed rather than removed, in two halves:

- **`sdl/i_video.c`** creates it with `SDL_WINDOW_HIDDEN` when `graphics_state <= VGS_startup`, and
  `I_FinishUpdate` shows it the first time anything actually draws (`sdl_window_unshown`). That is
  deliberately *not* the same as the alt-tab test beside it: that one is a window which **was**
  shown and has been hidden by the window manager, and must be left alone.
- **`console.c`** returns early from both self-refresh branches while
  `graphics_state < VGS_fullactive`, so routine startup messages no longer paint. Without this the
  first `CONS_Printf` after the window exists brings it straight back up, which is exactly what the
  first attempt did.

**Gate the second branch, not just the first.** The `if( con_self_refresh || (emsg & EMSG_now) )`
block is the obvious one and is *not* the one that fires: `con_video` is not up that early, so
startup messages take the `else if( ! con_video )` branch below it. Both are gated now. This was
found by printing `__builtin_return_address(0)` from the show path and running it through
`addr2line` — worth remembering, because `console.c` is one of the **ISO-8859 files that grep
silently skips** (`gotchas.md`), so grepping it for `I_FinishUpdate` returns nothing at all and the
call sites look like they do not exist.

Verified both ways round: on a healthy launch the window is created hidden and never shown, and
the startup text still appears on stdout and in the log; with an explicit early
`I_FinishUpdate` — the shape of the fatal-error console — the show path fires and the window
appears.

The cost is that a slow load now shows whatever was on screen before rather than a progress
console. On the Pi the load is a couple of seconds, and the text is still in the terminal and the
log where it can be read after the fact.
