# Ultrawide monitors — 21:9 and 32:9

Read this before changing `MAXVIDWIDTH`/`MAXVIDHEIGHT` (`screen.h`), the fullscreen mode list or
`windowedModes[]` (`sdl/i_video.c`), the `viewfit` block or `R_Init_TextureMapping` (`r_main.c`), or
the aspect filter on the Video Modes page (`m_menu.c`). For the *present* path — the texture pitch,
why software fullscreen stretches rather than letterboxes — see `software-fullscreen.md`; for the
Video Modes page's paging and sorting, `menus.md`.

It began as one request — "can it do 21:9 or 32:9" — with a suggested answer: an aspect ratio
selector in the video options, to cut down how many resolutions are listed at once. The selector was
the right idea and it is the last part of this document. It was not the hard part. Four separate
things had to change before there was anything for it to select.

## 1. The engine threw ultrawide modes away before the menu saw them

`MAXVIDWIDTH` and `MAXVIDHEIGHT` (`screen.h`) were 1600x1200, and `sdl/i_video.c` filters the
display's advertised modes against them in three places (`VID_Query_Modelist`,
`VID_make_fullscreen_modelist` for SDL2 and for SDL 1.2). On a 3440x1440 or 5120x1440 monitor
**every native mode is bigger than that**, so the list came out holding nothing but the legacy 4:3
and 16:9 sizes the panel also happens to advertise. Nothing logged the discards. This is the same
failure family as the three caps in `software-fullscreen.md`: a mode missing from the list reads as
"not supported", never as "a constant ate it".

They are now 5120x2160, which covers 32:9 (5120x1440), 21:9 (3440x1440, 3840x1600, 5120x2160) and
4K (3840x2160).

**They bound what the engine can *draw*, not what it can display**, and the two have been different
since software fullscreen started scaling (`software-fullscreen.md`). So the software renderer never
strictly needed this — it can draw 1280x540 and let the GPU scale it to 3440x1440. **The OpenGL
renderer did**: it has no scaling step at all, so without a real mode at the panel's own size it can
only render at the desktop resolution, and `R_ExecuteSetViewSize` runs for both renderers and
indexes `x_to_viewangle[MAXVIDWIDTH+1]` by `rdraw_viewwidth`. Raising the cap is what makes GL
ultrawide possible.

**The cost is static and it is not free.** The tables sized by these constants are a few tens of
bytes per column each, but `ffplane[MAXFFLOORS]` (`r_plane.h`) holds two `int16_t[MAXVIDWIDTH]`
arrays per floor across 40 floors, and it is `R_TLS` — one copy per render thread
(`render-threads.md`). Measured with `readelf -S`:

| | .tbss (per render thread) | .bss |
| --- | --- | --- |
| 1600x1200 | 320 KB | 2.5 MB |
| **5120x2160** | **964 KB** | **3.2 MB** |

At four render threads that is about 3 MB of extra resident memory. Acceptable here; worth
re-measuring before raising it again, because `ffplane` scales linearly with `MAXVIDWIDTH` and
dominates everything else.

`windowedModes[]`'s largest public entry was written `{MAXVIDWIDTH, MAXVIDHEIGHT}` and so followed
the constants up to 5120x2160 — a window size no desktop this runs on could show. It is a literal
1600x1200 now. **The largest window worth offering is not the largest size the engine can draw.**

## 2. The wide screen got a *narrower* view, and the wider the screen the worse it got

`cv_viewfit` ("View fit" on the Video Options page, `r_main.c`) decides how the 320x200 projection
is mapped onto the screen. On AUTO it read:

```c
if( r_width > 840  )  viewfit_ev = 2;  // fit width
else if( r_width < 760 )  viewfit_ev = 3;  // fit height
```

where `r_width = 600 * vid.width / vid.height`. Every widescreen shape took **fit width**, which
pins the horizontal field of view at 90 degrees and derives the vertical from the screen height — so
the wider the panel, the less of the world is on it:

| screen | fit width | fit height |
| --- | --- | --- |
| 4:3 | 90 x 64 | 80 x 64 |
| 16:9 | 90 x **58.7** | 96 x 64 |
| 21:9 | 90 x **45.7** | **112.4** x 64 |
| 32:9 | 90 x **31.4** | **131.5** x 64 |

At 32:9 that is a 31 degree vertical slit — the picture a 32:9 monitor is least able to justify.
**Fit height** is the one that keeps the vertical view of a 4:3 screen and spends the extra width on
horizontal view instead, which is what an ultrawide monitor is for. AUTO now takes it above
`r_width > 1200`, an aspect of exactly 2.0, clear of 16:9 (1067) and clear of 21:9 (1422).

**16:9 and 16:10 deliberately keep fit width.** Fit height would arguably suit them too — 96x64
instead of 90x58.7 — but that is the picture every existing install has been looking at, including
the cabinet, and changing it is a separate decision from making ultrawide work at all. Only the
shapes that had no sensible answer before are changed.

## 3. Above 127 degrees the projection table saturated

`R_Init_TextureMapping` builds `viewangle_to_x[]` by multiplying `finetangent[]` by the focal
length, and guarded the multiply with

```c
if (finetangent[i] > FRACUNIT*2)  t = -1;
```

A tangent of 2.0 is 63.43 degrees, so **no more than 127 degrees of horizontal view could be
tabulated** whatever the projection asked for. Past that, every column resolves to the same entry in
`x_to_viewangle[]` and renders as one smeared vertical band at each edge of the screen. 21:9 on fit
height needs a tangent of 1.48 and never reached it; 32:9 needs `centerx/focallength` =
`(5120/2)/(0.8*1440)` = 2.22 and crossed it.

The limit is `TANGENT_CLIP`, now `FRACUNIT*4` — 75.96 degrees, 152 degrees of view, with room to
spare. **Raising it changes nothing for a narrow field of view**: the tangents between the old limit
and the new one map to columns outside the view window, where the clamp immediately below sends them
to the same `-1` / `viewwidth+1` they had before. It cannot overflow `FixedMul` either, the product
being bounded by `4 * focallength` and `focallength` by `0.8 * MAXVIDHEIGHT`. It is render-only, so
it does not touch the simulation or demo compatibility.

Measured, headless, by printing `clipangle` from `R_Init_TextureMapping` (`clipangle * 360 / 2^32`
is the half field of view in degrees) and running at a series of draw sizes:

| draw size | viewfit chosen | clipangle | half FOV | with `TANGENT_CLIP` back at 2 |
| --- | --- | --- | --- | --- |
| 640x480 | 1 stretch | 537395200 | 45.0 | unchanged |
| 1280x800 | 2 fit width | 537395200 | 45.0 | unchanged |
| 1280x540 (21:9) | **3 fit height** | 667942912 | **56.0** | unchanged — 21:9 never needed it |
| 1280x360 (32:9) | **3 fit height** | 784859136 | **65.8** | 757071872, **63.46** — capped |

The last column is the control: reinstating the old clamp visibly truncates 32:9 and nothing else,
which is what proves the change is doing the work rather than being inert.

## 4. Every software draw size was the wrong shape

The software present path **stretches the drawn frame to fill the window** — it does not letterbox
(`software-fullscreen.md`). So a 4:3 draw size on a 21:9 panel is not a picture with black bars
either side, it is a picture smeared to two and a half times its proper width. `VID_add_scaled_modes`
offered exactly one ladder — 320x200, 400x300, 512x384, 640x480, 800x600, all 4:3 — which meant that
on any non-4:3 display *every* scaled software mode was distorted.

There is a second ladder now, of 16:10, 16:9, 21:9 and 32:9 draw sizes, and **only the entries whose
shape matches the display are added** (`VID_Display_Size` asks SDL for the desktop mode). A 16:9
machine gains five 16:9 sizes and nothing else; a 4:3 machine gains nothing. `windowedModes[]` gets
the same treatment as a static table, because a *window* has no display to take its shape from.

## 5. The aspect filter, which is what was actually asked for

A `vid_aspect` cvar (`m_menu.c`, `CV_SAVE`) filters the Video Modes list to one shape: **AUTO**
(whatever the display is), **All**, 4:3, 16:10, 16:9, 21:9, 32:9. **A** on the page cycles it, the
line at y=26 names the current filter and how many modes it is hiding, and the instruction block
says which key. It is not a row on the Video Options page — that page is full, menu rows there are
addressed by hardcoded index (`menus.md`), and the filter is only meaningful on the page it applies
to.

- **The default is AUTO, and that is a visible change** — a 16:9 machine no longer lists the 4:3
  sizes until you press A. That is the point: the hidden ones are the ones that would come out
  stretched, and it cuts a list that had grown long enough to need paging back to a single page on
  most machines. The hidden count on the header line is what stops this being a silent truncation.
- **The mode in use is listed whatever its shape.** It is the entry drawn highlighted and named in
  the instruction block; a page that cannot show where you already are is worse than a page with one
  odd entry on it.
- **AUTO on a display whose size cannot be read filters nothing.** Guessing would empty the list on
  exactly the machines least able to say why.
- **The filter can still empty the list** — the exemption above only helps when the current mode is
  in the list being built, which it is not when `cv_fullscreen` disagrees with the mode actually up.
  The page says "No modes of this shape", and `change_mode` returns early rather than indexing an
  empty `modedescs[]`. Before the filter existed an empty list was unreachable, because the game
  does not start without a usable mode.
- **320x200 is 4:3, not 16:10.** Its pixels are not square; it is meant to fill a 4:3 monitor. That
  is the same special case `r_main.c` makes — `if( vid.width == 320 ) goto std_fit` — and without
  `vidm_mode_shape()` here a 4:3 filter hid 320x200 and a 16:10 filter offered it. It is the one
  entry in the whole list whose displayed shape is not its pixel shape, and the test found it.
- **The tolerance is 3%** (`VID_ASPECT_TOLERANCE`, `i_video.h`), and it is pinned from both sides: it
  has to be loose enough to call 1366x768 (1.7786) a 16:9 screen and tight enough to keep 1280x1024
  (5:4, 1.25) out of the 4:3 bucket, 6.6% away. Shapes that match no named ratio — 5:4, 1024x600 —
  are reachable only under All, which is why All has to stay one keypress away.

### Testing it without a screen

`tools/vidaspect-test.py` **extracts `VID_Aspect_Match` from `sdl/i_video.c` and
`vidm_aspect_target`, `vidm_mode_shape` and `vidm_aspect_label` from `m_menu.c` verbatim by brace
matching**, along with the enum, the ratio table and the tolerance, stubs `VID_Display_Size`, and
drives them over 30 real resolutions. It checks that each named filter takes every resolution of its
own shape and none of any other, that All hides nothing, that AUTO follows the display, that an
unreadable display size and an out-of-range cvar both hide nothing, and that the labels read as
intended. The filter decision itself is three lines inside `M_DrawVideoMode` and cannot be lifted
out, so the script matches its **text** instead — dropping the `! is_current` term would be a silent
regression.

`--selfcheck` reinstates three real bugs (tolerance 0%, tolerance 8%, AUTO hiding everything when
the display size is unknown) and reports whether each goes red. All three do. A clean result from a
check never shown to fail is worth nothing — that lesson is from `menus.md`, where two of five
checks were silently useless until someone ran it.

`tools/vidmenu-navtest.py` still passes unchanged: the paging and sorting functions were not
touched.

### Screenshots

Screenshots at ultrawide draw sizes are taken the way `screen-fill.md` describes — under
`SDL_VIDEODRIVER=offscreen`, never `dummy`, where the capture comes out black — driven by an
`autoexec.cfg` of `wait 105` then `screenshot`, with `localplayers "1"` in the scratch config so it
is one view rather than the cabinet's 2x2 grid. A 1280x360 capture of MAP01 shows the wide field of
view with clean geometry to both edges and no smeared band, which is the thing none of the numeric
checks above can see.

## What is still not done

**The multiplayer view grid does not know about the screen's shape.** `D_View_Grid`
(`multiplayer-views.md`) offers 1x2 or 2x1 for two views and 2x2 for four, full stop, and on an
ultrawide neither of the four-player options is right. Cell shapes, with the field of view each
would get:

| | 2 stacked | 2 side by side | 3 columns | 4 as 2x2 | 4 columns |
| --- | --- | --- | --- | --- | --- |
| 16:9 1920x1080 | 3.56:1, 131 x 64 | 0.89:1, 58 x 64 | 0.59:1, 41 x 64 | **1.78:1, 90 x 59** | 0.44:1, 31 x 64 |
| 21:9 3440x1440 | 4.78:1, 143 x 64 | **1.19:1, 74 x 64** | 0.80:1, 53 x 64 | **2.39:1, 112 x 64** | 0.60:1, 41 x 64 |
| 32:9 5120x1440 | 7.11:1, 155 x 64 | **1.78:1, 90 x 59** | **1.18:1, 73 x 64** | 3.56:1, 131 x 64 | **0.89:1, 58 x 64** |

Two conclusions. **Two players on an ultrawide want side by side**, which `cv_splitvertical` already
does — it just has no reason to prefer it on a wide screen. **Four players on 32:9 want four
columns**, which nothing can express: `D_View_Grid` never returns more than two columns, and
`R_ExecuteSetViewSize` halves with `rdraw_scaledviewwidth >>= 1` against a `soft_columns` *boolean*
rather than dividing by a count. Making that general reaches into `r_draw.c`'s cell tables, the HUD
placement in `st_stuff.c`/`hu_stuff.c`, `D_View_Squash` for the hardware renderer, and the join
screen's `(panels == 2) && cv_splitvertical.EV`. It is its own piece of work, and it belongs in
`multiplayer-views.md` when it happens.

Note that 21:9 with four players is the one case where **2x2 is the better answer** — four columns
there are 0.60:1 and 41 degrees wide. So this is an operator choice, not a rule that can be derived
from the aspect alone.
