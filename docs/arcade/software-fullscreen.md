# Software rendering in fullscreen, and the pitch that skews it

*Part of the DoomLegacy arcade cabinet build. Read before changing `VID_SetMode_vid`,
`I_FinishUpdate`, or anything else that touches `sdl_texture`, `vidSurface`, `vid.direct*` or the
fullscreen mode list in `sdl/i_video.c`.*

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
the screen.** More generally, in the SDL2 path the window surface describes the *window*, and the
engine's geometry must come from what it actually fills — the texture.

So `vid.bitpp` / `vid.bytepp` are now derived from the texture's pixel format
(`SDL_BITSPERPIXEL` / `SDL_BYTESPERPIXEL` of `pixel_format`) rather than from `vidSurface->format`.
On X11 the two agree (`SDL_PIXELFORMAT_RGB888`, 24 bpp in 4 bytes) so nothing changes on a working
machine; where they disagree, the texture is the one that is right, because it is the one being
filled. A verbose mismatch line reports it if they ever differ.

## `vidSurface` can legitimately be NULL

`SDL_GetWindowSurface` needs the video driver to supply its own framebuffer. X11 and Wayland do;
**KMSDRM does not**, and there SDL falls back to creating a renderer for the window — which already
has one — and returns NULL. Stock dereferenced it immediately (`vidSurface->format->BitsPerPixel`),
so a Pi booted straight to KMSDRM with no X server would have segfaulted in `VID_SetMode_vid`
rather than garbled. It is guarded now; the only loss without a surface is the 8-bit palette, which
the SDL2 path does not use anyway (the texture is never `INDEX8` — SDL2 has no palettized textures,
so the engine always ends up drawing at the window's depth).

## Drawing size does not have to match the window

`SDL_RenderCopy( ..., NULL, NULL )` scales the texture to the whole window, so the engine's
resolution is independent of the display's. Fullscreen at a mode the display cannot set still comes
out right — the GPU scales it — and `SDL_HINT_RENDER_SCALE_QUALITY` is set to `nearest`, so the
upscale stays sharp rather than blurring.

That also means the mode-list filtering above is a leftover from SDL 1.2, where the surface *had* to
be a real video mode. If low-resolution fullscreen is ever wanted on a slow machine — draw 320x200,
let the GPU stretch it — the change is to offer the small sizes in the fullscreen list, not to
touch this path. Note the copy stretches to the window's aspect, so a 4:3 mode on a 16:9 panel comes
out wide; `SDL_RenderSetLogicalSize` would pillarbox it instead.

## Verifying a change here

`./doomlegacy -v` prints one line per mode set:

```
  Draw 1024x768, 24 bpp, 4 bytes (SDL_PIXELFORMAT_RGB888), window 1024x768
```

**If the two sizes differ, the display is being scaled** — fine in itself, but it is the condition
under which the old pitch bug appeared, so it is worth knowing. The line is printed for both the
initial window and every later mode change, including the switch into fullscreen.

`make smoke` covers this path only as far as "it still runs"; the `warp` and `exitlevel` checks run
under `dummy`, where the window matches the drawing size and the wrong pitch is invisible. A change
to the present path needs either the probe above or Mark's eyes on the cabinet.
