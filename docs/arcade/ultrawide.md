# Ultrawide monitors — 21:9 and 32:9

Read this before changing `MAXVIDWIDTH`/`MAXVIDHEIGHT` (`screen.h`), the fullscreen mode list or
`windowedModes[]` (`sdl/i_video.c`), the `viewfit` block, `R_Init_TextureMapping` or the psprite
scales (`r_main.c`), the `dupx`/`fdupx` setup in `V_Setup_VideoDraw`/`V_SetupDraw` (`v_video.c`), or
the aspect filter on the Video Modes page (`m_menu.c`). For the *present* path — the texture pitch,
why software fullscreen stretches rather than letterboxes — see `software-fullscreen.md`; for the
Video Modes page's paging and sorting, `menus.md`.

It began as one request — "can it do 21:9 or 32:9" — with a suggested answer: an aspect ratio
selector in the video options, to cut down how many resolutions are listed at once. The selector was
the right idea and it is the last part of this document. It was not the hard part. Five separate
things had to change before there was anything for it to select, and one of them — the 2D layer —
was only spotted because someone looked at a screenshot.

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

## 5. The 2D layer stretched flat, and so did the weapon

Everything above is the 3D view. The 2D layer — status bar, HUD numbers, menus,
weapon sprite — was still stretching, and on a 32:9 screenshot it is the first thing anyone
notices: the health numbers and the gun are squashed flat. Two independent places, one cause.

**`V_Setup_VideoDraw` derived the two scales separately**: `vid.dupx = vid.width/320` and
`vid.dupy = vid.height/200`. Their *ratio* therefore followed the shape of the display —
`0.625 * width/height`, which is 0.83 at 4:3. That 5:6 is Doom's non-square pixels and the
proportion all the art was drawn for. At 16:9 the ratio is 1.11, at 21:9 1.48, at 32:9 **2.22**.

**`R_ExecuteSetViewSize` does the same thing for the weapon**, `pspritescale` coming off the view
width and `pspriteyscale` off the height, and works out to exactly the same `0.625 * w/h`.

Both are now **capped at the proportions of a 16:9 screen**, and **both caps are gated on an exact
integer test of the screen shape** — `vid.width * 9 > vid.height * 16` — rather than on comparing
the two scales:

| screen | ratio | capped? |
| --- | --- | --- |
| 4:3 | 0.83 | no |
| 16:10 | 1.00 | no |
| 16:9 | 1.11 | exactly at the cap |
| 21:9 | 1.48 | yes, to 1.11 |
| 32:9 | 2.22 | yes, to 1.11 |

**16:9 is the cap rather than 4:3 deliberately.** A 4:3 cap would un-stretch every widescreen
install in existence, which is a different decision from making ultrawide usable — the same call
made for `viewfit` in part 2. Nothing at 16:9 or narrower changes: below the gate the cap does not
execute at all, so `vid.fdupx` is `vid.fdupx_fill` and `pspritescale` is untouched, which is the
same arithmetic as before this existed.

**The gate is an integer test because both float and fixed-point forms got it wrong at exactly
16:9, in ways nothing else would have caught.** Written as `fdupy * (10.0f/9.0f)`, the cap lands
just *below* the true value — `3.6f * (10.0f/9.0f)` is 3.9999998, not 4.0 — and since
`vid.dupx = (int)vid.fdupx`, the HUD art scale at 1280x720 and 2560x1440 silently dropped from 4x
to **3x**, a whole step, at the two resolutions the cap exists to leave alone. Rewriting it as
`vid.height / 180.0` in double fixed 1280x720 and not 2560x1440, because the psprite cap has the
same problem from the other direction: `pspriteyscale` is a truncated `fixed_t`, so the comparison
lands one unit on the wrong side and shaved a unit off `pspritescale`. Only the integer gate makes
"16:9 and narrower is untouched" true rather than nearly true. Both bugs were found by `cmp`-ing
two capture sets; neither changes anything a test measures, and on screen the first is just a
slightly smaller HUD.

Three things this had to get right:

- **`vid.centerofs` was written as a remainder** — `(vid.width % BASEVIDWIDTH)/2`. That is only the
  leftover width while `dupx` is `width/320`; once the cap engages the leftover is far larger, and
  the 2D layer would have sat hard against the left edge with all the spare space on the right. It
  is `(vid.width - BASEVIDWIDTH*vid.dupx)/2` now, which is identical arithmetic in the uncapped case
  and correct in the capped one.
- **A whole-screen page is meant to fill the screen and must not be capped**, or the title and
  intermission backgrounds would come back pillarboxed and undo `screen-fill.md`. Those pass
  `V_SCALEEXACT`, so `vid.dupx_fill`/`vid.fdupx_fill` keep the uncapped `width/320` and
  `V_SetupDraw` hands those out for `V_SCALEEXACT` only — for the patch scale, for the start
  coordinates, and for `x0_scale`. Scale and position have to take the same branch or a page is
  drawn at one size and placed at another.
- **`pspriteyscale` is left alone.** `R_Set_Sky_Scale` is `FixedDiv(FRACUNIT, pspriteyscale)` and
  the sky still has to fill. Only the horizontal scale is capped, and since the weapon is positioned
  relative to `centerx` a smaller horizontal scale leaves it centred rather than sliding it left.

1366x768 is a hair wider than 16:9 (1366*9 = 12294 against 768*16 = 12288), so the gate does open on
the cabinet — but `vid.dupx` is an integer and comes out identical, and only the hardware renderer's
float scale moves, by a twentieth of a percent.

### Where a thing goes is not how big it is

Capping the scale fixed the *shape* of the HUD and immediately broke its *placement*, and the second
was worse than the first: `ST_overlayDrawer` derives `xdiv`, the layout scale, from `vid.fdupx`, so
the whole 320 unit layout shrank along with the art. On a 32:9 screen the health ended up a third of
the way in and the ammo block landed **on top of the weapon**. Squashed-but-correctly-placed is a
cosmetic complaint; correctly-shaped-but-overlapping is unreadable.

`xdiv` comes from `vid.fdupx_fill` now — the uncapped `width/320` — divided by the column count,
while the art keeps the capped `vid.fdupx`. The layout spans the cell, the art keeps its proportions,
and the two no longer have to agree. That is what the comment above `xdiv` always claimed the code
did ("so the 320x200 layout spans the cell whichever shape it is... the art scale is a separate
question"); it was true until the cap made `vid.fdupx` stop meaning "the width of the cell in base
units".

The K/I/S block needed the same treatment and one thing more. Each row placed itself with
`SCX(318 - V_StringWidth(buf), x0, xdiv)` — a base-unit width subtracted in layout space, then drawn
in art space — so once the two scales parted company **every row was displaced by its own width** and
the block visibly came apart. `ST_KIS_Columns` now measures the widest label and the widest count
across all three rows, anchors the right edge in *screen* pixels, and steps left by the **drawn**
width, so the letters make one column and the counts another.

Note that this part is **not** ultrawide-only: `hu_font` is proportional, so right-aligning each row
on its own text never lined the three up, at any resolution. The bug was invisible at 4:3 and 16:9
because the rows are short and the eye forgives it; the wide screens only made it obvious. The K row
lands where it always did and the I and S rows move left to join it.

**The general lesson, and it is the one this whole part keeps teaching: a 2D scale answers two
questions, and an ultrawide screen is where they stop having the same answer.** Anything that reads
`vid.dupx`/`vid.fdupx` should be checked for which of the two it wanted — and anything that places
text against `V_StringWidth` should be checked for which space it is subtracting in.

## 6. The aspect filter, which is what was actually asked for

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

### Screenshots, and `tools/shotsheet.py`

**Checking that a screen *looks right* is the one thing a headless run cannot do and a person can do
in seconds.** `tools/shotsheet.py` runs the game once per drawing size, takes a screenshot of each,
and writes one self-contained HTML page with every shot on it at its true pixel shape, labelled with
what the engine actually did. Two runs into two directories, before and after a change, is the way
to check anything visual:

```
tools/shotsheet.py --out /tmp/before
tools/shotsheet.py --sizes 3440x1440,1280x360 --aspect 21:9,32:9 --scenes game,title
```

It already goes round every trap in this tree: `SDL_VIDEODRIVER=offscreen` (under `dummy` the
capture is entirely black), `SDL_NO_SIGNAL_HANDLERS=1`, `fullscreen "Yes"` (the offscreen driver has
no window manager and a windowed request dies with "cannot draw 0 bits per pixel"), a *copy* of
`legacyhome` per run, the per-drawmode configs deleted, and `localplayers "1"` so it is one view
rather than the cabinet's 2x2 grid. It reads `MAXVIDWIDTH`/`MAXVIDHEIGHT` out of `screen.h` and
skips anything over them, so the ladder does not have to be kept in step by hand.

**`-width`/`-height` are exact for the software renderer**, which is what makes this work at all: a
size not in the mode list used to be snapped to the nearest one by `VID_GetModeForSize`, silently, so
asking for 3440x1440 on a 16:9 laptop drew 1366x768 and the sheet would have been a page of
identical pictures. `VID_add_scaled_modes` now puts an explicitly requested size into the list.

The whole 2D scaling bug in part 5 was found this way, in about two seconds of looking at a 32:9
capture — after four numeric checks had all come back clean, because none of them was measuring the
2D layer at all.

**A capture of a live level is not reproducible, and the noise looks exactly like a regression.**
Two separate causes, and both have to go before two sheets can be compared rather than merely
looked at:

- `r_fps.c` sets `interp_active` from `cv_framerate_cap.value != TICRATE`, so above the tic rate the
  frame is drawn a wall-clock fraction of a tic ahead of the simulation (`uncapped-framerate.md`).
  The tool sets `framerate_cap "35"`, which pins it — the same thing `render-threads.md` does before
  checksumming frames.
- **That alone is not enough, and believing it was cost an hour here.** Monsters are moving and the
  shot can land a tic either side, so the same binary twice still gives two different pictures.
  `--nomonsters` is what makes it bit identical; verified by running the same size twice and
  `cmp`-ing.

The first before/after pair taken here had neither, and reported 1024x768, 1280x800, 1280x720 and
2560x1440 as changed — none of which an aspect cap can touch, 1024x768 being 4:3. Two fresh runs on
a quiet machine were identical to each other **and to the *before* sheet**, which is what identified
it: the change was innocent and the harness was not. With both settings the comparison is
mechanical:

```
for f in before/*.png; do cmp -s $f after/$(basename $f) \
  && echo "same    $(basename $f)" || echo "CHANGED $(basename $f)"; done
```

Leave the monsters in for a capture you are going to **look** at — they are what shows the sprite
scale — and take them out for one you are going to **compare**.

**And even then, a diff across two builds is a lead and not a proof.** Two runs of one binary match;
two *binaries* start up at slightly different speeds, so the shot lands a tic either side and a
level's animated textures have advanced a frame. A size can come out CHANGED with no code path
between the two builds capable of doing it — 1024x768 did exactly that here. When the diff and the
arithmetic disagree, the arithmetic wins and the pictures settle it.

## Four players on an ultrawide: `cv_split4`

`D_View_Grid` (`multiplayer-views.md`) offered 1x2 or 2x1 for two views and 2x2 for four, full stop.
On a 32:9 screen a 2x2 cell is *itself* 32:9 — a letterbox slit — so neither option was right. Cell
shapes, with the field of view each player gets:

| | 2 stacked | 2 side by side | 3 columns | 4 as 2x2 | 4 columns |
| --- | --- | --- | --- | --- | --- |
| 16:9 1920x1080 | 3.56:1, 131 x 64 | 0.89:1, 58 x 64 | 0.59:1, 41 x 64 | **1.78:1, 90 x 59** | 0.44:1, 31 x 64 |
| 21:9 3440x1440 | 4.78:1, 143 x 64 | **1.19:1, 74 x 64** | 0.80:1, 53 x 64 | **2.39:1, 112 x 64** | 0.60:1, 41 x 64 |
| 32:9 5120x1440 | 7.11:1, 155 x 64 | **1.78:1, 90 x 59** | 1.18:1, 73 x 64 | 3.56:1, 131 x 64 | **0.89:1, 58 x 64** |

**The right answer changes sign in the middle of the range**, which is why this is an operator
setting and not derived from the aspect: 2x2 is right up to *and including* 21:9 (112 degrees a
player against 41 for four columns) and wrong at 32:9. `cv_split4` — "4 Player Split", `2x2 Grid` or
`4 Columns` — is on the new Players and Views page. Three players use four cells with one empty, the
same as the grid does, so this covers "three columns" without a third setting.

What had to become general for four columns to be possible:

- **`R_ExecuteSetViewSize` halved with `rdraw_scaledviewwidth >>= 1`** against a `soft_columns`
  *boolean*. It divides by `view_cols` now, which matches `R_View_Cell_Size` (`vid.width / cols`)
  exactly — and it has to, or `R_View_Fills_Cell` decides the view no longer fills its cell and
  paints a border round every one of them. `fit_ref_width` multiplies by `view_cols` for the same
  reason it used to double.
- **`ST_overlayDrawer` shrank the HUD art by exactly 2** whenever there was more than one column.
  It is `/ cols` now. Both axes by the same divisor even though a four-column cell is full height:
  the *width* is what constrains a 320 unit layout, and the art has to stay in proportion to it.
- **The join screen carried its own copy of the grid.** It has to agree with `D_View_Grid` down to
  `cv_split4`, or the screen whose entire job is telling each player which part of the screen is
  theirs points at the wrong one. Its cell width is `BASEVIDWIDTH / gcols` now rather than a
  hardcoded half.
- `D_Grid_Cell_Pos` and `D_View_Squash` already generalised: the `cols >= 2, rows == 1` branch maps
  cell N to column N whatever N is, and a four-column cell squashes the same way a side-by-side half
  does.

**The Arcade Options page was full** — 16 rows from y=40 reaches the bottom of a 200 unit screen —
so "4 Player Split" had nowhere to go. Control Panels, 2 Player Split, 4 Player Split, Screen Order
and Join Time moved to a new **Players and Views** page reached from it, which is a better grouping
anyway and leaves Arcade Options four rows shorter than it was.

Verified by screenshot rather than by argument, `tools/shotsheet.py --cvar localplayers=4 --cvar
'split4=4 Columns' --sizes 2560x720`: four distinct views side by side, each 640x720, correct
geometry and a HUD scaled to its cell. The 2x2 path is arithmetically unchanged — `view_cols` is 2
there and `/2` is what `>>= 1` was.

## What is still not done

- **`cv_splitvertical` has no reason to prefer side by side on a wide screen.** Two players on a
  21:9 or 32:9 monitor want it (74 and 90 degrees a player, against 143 and 155 for the stacked
  halves, which are 4.8:1 and 7.1:1 slits) but the default is still Top/Bottom everywhere. An AUTO
  value that picked by aspect would be the same shape of change as `cv_split4`'s table above.
- **Nothing has been run on real ultrawide hardware.** Everything here is a software drawing size
  scaled into a 16:9 desktop, which exercises the projection, the 2D scale and the view grid, but
  not an actual OpenGL mode switch to 3440x1440 — the one thing raising `MAXVIDWIDTH` was for. That
  needs a monitor.
