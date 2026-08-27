# The original spectre / partial-invisibility fuzz effect

*Part of the DoomLegacy arcade cabinet build. Read before changing `HWR_DrawSprite` or
`HWR_DrawFuzzSprite` in `hardware/hw_main.c`, `CV_Fuzzymode_OnChange` in `screen.c`, or the
`R_DrawFuzzColumn_*` drawers.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

## What this is

Vanilla Doom drew spectres, and any player holding a Partial Invisibility sphere, with the **fuzz**
effect: the sprite is never drawn at all, and instead the framebuffer inside its silhouette is
sampled one row up or down and put back through COLORMAP light level 6. The result is a dark,
boiling distortion rather than a translucent monster. DoomLegacy defaults to translucency instead.

Both are still available, selected by the existing **`fuzzymode`** cvar
(`screen.c`, `CV_SAVE | CV_CALL`, default `Off` = translucent).

## Operator-only, deliberately

The row is **Options → Effects Options >> → "Spectre Fuzz"**. The lockdown hides the whole Effects
page from players (`OptionsMenu[4]`), so this is reachable only under `-devmode` — which is what
was wanted: it is a cabinet-wide look the operator picks, not a per-player preference. Set it in a
`-devmode` session so `config.cfg` is written; a plain player session never writes the config.

The row was renamed from upstream's "Fuzzy Shadow", which says nothing about what it does. The
*label* is free to change — `config.cfg` stores the cvar's **name and value** (`fuzzymode "On"`),
not the menu text, so this breaks no existing config. Do not rename the `Off`/`On` values.

## Software renderer — the original effect, untouched

`R_DrawFuzzColumn_8` (and the 16/24/32 variants) are stock and do the real per-pixel effect.
`CV_Fuzzymode_OnChange` swaps `fuzzcolfunc` between the fuzz and translucent column drawers for the
current `vid.drawmode`; `r_things.c` picks `fuzzcolfunc` for any vissprite carrying `MF_SHADOW`.
Nothing here needed changing.

## Hardware renderer — an approximation, and why

**The cabinet runs OpenGL** (`drawmode "OpenGL"` in the tracked config), and until now the setting
did *nothing at all* there. `CV_Fuzzymode_OnChange` printed "OpenGL has only translucent shadow."
and `HWR_DrawSprite` drew every `MF_SHADOW` sprite at alpha `0x40` regardless. So the option
existed, was already in a menu, and was inert on the only renderer the cabinet uses — which is why
this needed code, not just a menu row.

The hardware renderer cannot read the framebuffer per pixel, so the per-pixel effect cannot be
reproduced without shaders. `HWR_DrawFuzzSprite` reproduces the two things that actually make it
read as a spectre:

- **The sprite disappears, leaving only its silhouette.** `Surf.FlatColor` RGB is set to 0 and the
  poly is drawn `PF_Translucent | PF_Modulated`. With `GL_MODULATE` the texture colour is
  multiplied by the flat colour, so *whatever* the sprite texture holds becomes black — only its
  alpha survives, which is exactly the silhouette. Alpha-test still punches the holes.
- **The outline boils.** The quad is split into horizontal bands (4 patch texels tall, capped at 24
  bands) and each band's `tow` is displaced one texel up or down from `hwr_fuzzoffset`, a copy of
  the `+1/-1` walk in `r_draw.c`. The index advances with `hwr_fuzzpos`, bumped once per frame in
  `HWR_RenderPlayerView`, plus a per-mobj salt so two spectres are not in lockstep.

### The alpha is derived, not eyeballed

`HWR_FUZZ_ALPHA` is `0x30`. COLORMAP light level 6 — the one `R_DrawFuzzColumn_*` uses — maps the
DOOM palette to a mean **81.2%** of its luma (computed over PLAYPAL with Rec.709 weights, ignoring
near-black entries). A black overlay reproduces that at alpha `1 - 0.812 = 0.188`, i.e. `48/255`.
Do not "brighten" this by taste; if it looks wrong, recompute it against the real lumps.

### What is deliberately *not* reproduced

The software effect smears the **background** inside the silhouette, because it samples displaced
framebuffer pixels. The hardware version darkens the background uniformly, so its boil is visible
at the **outline** only. That is a real difference, not a bug — reproducing the interior needs a
fragment shader, and this GL backend is fixed-function.

### State safety

Everything goes through `HWD.pfnDrawPolygon` with `PF_` flags, so `SetBlend`'s `cur_polyflags`
shadow copy stays accurate. **No direct GL calls** — see the `screen-wipe.md` rule about the
renderer caching its own state.

`PF_Occlude` is intentionally absent (no depth write), matching the translucent shadow path it
replaces.

## Verification

Headless on the real GPU, `SDL_VIDEODRIVER=offscreen` (see `CLAUDE.md`), with temporary counters in
`HWR_DrawSprite`:

- **DOOM2 MAP19, `-skill 3`** has three spectres in view from the player start, so no play session
  was needed. `Renderer : AMD Radeon Vega 8 Graphics` confirms a real accelerated context.
- `fuzzymode "On"`: every `MF_SHADOW` sprite took the new path — 14394 fuzz draws from 14394 shadow
  sprites over 4800 frames, ~13.5 bands each — no crash, still rendering when `timeout` killed it.
- `fuzzymode "Off"`: same 3 spectres per frame, **0** fuzz draws. The old translucent path is
  untouched.
- Software (`drawmode "Software 8bit"`, `scr_depth "8 bits"`) on the same map: no crash, no
  `I_Error`.
  - **`config8p.cfg` overrides `config.cfg`** for `scr_depth`/`fullscreen` and asks for 32bpp
    fullscreen. Leave it alone in a scratch home and the run dies at `Change Graphics failed:
    err=-102` before the level ever loads — which reads as the software renderer being broken. Set
    the depth in *both* files.

## Not done

`hw_md2.c` still draws `MF_SHADOW` models translucent — `gr_md2` defaults to `Off` and the cabinet
does not enable it, so the fuzz path never reaches models.
