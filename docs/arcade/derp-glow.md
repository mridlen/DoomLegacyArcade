# DERP effects (glow, lamp light, ripples, busy panels)

Read this before touching `hardware/hw_glow.c`, `r_readout.c`, `svn1749/src/derp/`, the glow hooks in
`HWR_ProjectWall`/`HWR_RenderPlane`/`HWR_DrawSprite` (`hw_main.c`), `HWR_Glow_Composite_Texture`,
`HWR_Texture_Texels`/`HWR_Texture_Reupload` or the `hwr_glow_bound`/`hwr_glow_texnum` assignments in
`hw_cache.c`, the `DeleteTexture` driver call, or the **DERP >>** page.

## What it is

Switches on **Options → Effects Options → Next → DERP >>**, all **Off** by default (a new switch
defaults to what the cabinet already did, `gameplay-defaults.md`):

| Row | cvar | What it draws |
| --- | --- | --- |
| Glowing Lights | `gr_glow_lights` | The glowing pixels of lamp, light-strip, computer-panel and exit-sign textures and flats, drawn again at full brightness over the lit surface. |
| Glowing Liquids | `gr_glow_liquids` | Nukage, slime and lava pools add their own bright cells on top of themselves, and every wall standing in one gets a gradient of the liquid's colour climbing 96 units up from the waterline. |
| Glowing Eyes | `gr_glow_eyes` | A living monster's eye pixels, unlit, so eyes show in the dark. |
| Lamp Light | `gr_glow_lamplight` | Light from those lamps onto the walls and floors round them, in each lamp's own colour. |
| Liquid Light Ripples | `gr_glow_ripples` | Wavy light from a pool on the walls standing in it. |
| Busy Computer Panels | `busy_readouts` | The LED readouts on computer walls change now and then. **Both renderers.** |

The page was first under OpenGL 3D Card Options. It moved to Effects Options when Busy Computer
Panels came, because that page cannot be opened in software mode (`M_OpenGLOption` refuses), and
the Pi runs software. `M_Draw_DerpMenu` greys the OpenGL rows out in software mode, from the
drawmode every frame (as `M_Draw_VideoOptions` does *Keep aspect*), and says why in red. The rows
have no `IT_YOFFSET`, so the OpenGL ones can be compiled out of a build without `HWRENDER`.

"Liquid Light Ripples" is DERP's *caustics*. The word was kept off the menu on purpose: players
read "caustic" as damaging. "Ripples of light near liquid (caustics)" was the first choice and is
267 pixels in the menu font against 171 available (`hu_font`, measured from the `STCFN` lumps).
The ideas, and two pieces of data, come from **DERP**, the DOOM Engine Remaster Project
(<https://github.com/robongrui/derp>), which is GPLv3. That is why the repository's licence is now
GPLv3 as a whole: see `svn1749/src/derp/NOTICE` and the README's credits.

All but Busy Computer Panels are **OpenGL only**. DERP has its own Vulkan/Metal renderer built
around a level-load light bake and per-pixel lighting, so none of its renderer code could be used;
these are re-done as extra passes over the fixed-function GL renderer.

## What was taken from DERP, and what was not

- `derp/emissive.h` — **verbatim**. A list of glowing artwork by name (`doom_emissive_kind`) and a
  per-pixel test of how strongly a texel glows (`doom_emissive_pixel`). Plain C.
- `derp/sprite_eyes_stock.h` — **table verbatim**, lookup rewritten in C. Hand-picked eye pixels for
  every living frame of the stock Doom and Doom II monsters, each keyed by the lump's size and an
  FNV-1a hash of its decoded pixels, so a PWAD's replacement sprite simply gets no eyes.
  `glow_decode_sprite` decodes exactly as DERP does (palette index and coverage pairs, row by row,
  tall-patch posts) or the hashes never match.
- DERP's **automatic eye finder** (`sprite_eyes.h`, C++) was not taken. Without it, monsters from a
  PWAD with new sprites have no glowing eyes.

## How it is drawn

Every effect is a second polygon on the **same vertices** as the surface, right after the surface's
own draw and before the splat and dynamic-light passes (which rewrite the texture coordinates).
`PF_Decal` puts it a hair in front; the depth test is already `GL_LEQUAL`.

**The masks line up because the cache builds them.** A wall's mask is the texture composited by the
cache's own `HWR_GenerateTexture` (through `HWR_Glow_Composite_Texture`, since that is static), then
converted in place; an eye mask is a synthetic patch holding only the eye pixels, run through
`HWR_MakePatch` with the sprite copy's own flags, so it gets the same margin and the same block size.
Building them any other way would mean re-deriving the power-of-two sizing, the margin rule and the
padding fill, and getting one of them wrong would offset every glow by a texel. `HWR_Glow_Sprite` still
refuses to draw if the margin came out different.

**Lights and eyes are premultiplied, drawn `PF_Environment`** (`ONE, ONE_MINUS_SRC_ALPHA`), for the
same reason sprites are (`HWR_DrawSprite`): a linear filter over a texture whose clear texels are
black otherwise puts a dark rim round every glowing pixel.

**Liquids are additive** (`PF_Additive`). Drawing a pool's glowing pixels at full brightness, as a
lamp's are, changed it by about 1/255 in testing: DERP's test lights only the brightest third of
NUKAGE1, at 19/255 on average, because DERP uses it to emit light, not to replace the texel. Adding
the bright cells on top (`GLOW_LIQUID_ALPHA`) is what makes nukage read as glowing.

**The wall glow** is the liquid's colour (its glowing pixels' average, raised so the brightest channel
is full) through an 8x64 alpha ramp (`glow_ramp`, clamped, alpha falling as the square of height),
added over the part of any wall in a sector whose floor is a glowing liquid. Strength
`GLOW_WALL_ALPHA`, less in bright rooms.

### Lights follow blinking sectors, otherwise always on

The first version faded the light masks with the sector's light level, and that turned them off in
exactly the rooms where they show — the GL renderer draws a light-128 room near full brightness
already, so a full-bright lamp only differs from the lit one in a dark room. So a lamp is always
fully lit (GZDoom's brightmaps do the same), **except** in a sector whose light is animated (Doom
types 1, 2, 3, 4, 8, 12, 13, 17, read through `special & 31` so Boom's generalized sectors count):
there it follows the swing, off at the bottom and full at the top, as DERP's lamps do. The swing is
learnt by watching — the lowest and highest light each such sector has been drawn at this level —
so until a sector has been seen to move, its lamps are on. Rendering only; nothing is read from the
light thinkers.

Expect **Glowing Lights to be subtle** in ordinary lighting. It is meant for dark rooms.

## Liquid Light Ripples

DERP's liquid caustics, much simplified (`glow_ripple_entry`, `glow_ripple_wall`). Per liquid flat
lump a 64x64 alpha-intensity pattern is made from that flat's own brightness: softened over 3x3 so
lone bright texels drop out, minus the flat's mean, divided by its strongest streak, then a square
root (without it only the single brightest streak showed), faded with height as the square. Row 0
is the waterline; one texel is one map unit (`RIPPLE_HEIGHT` 64). Each animation frame of the flat
is its own lump, so the ripples step with the floor's animation, and `RIPPLE_DRIFT` slides them
along the wall. The fade is baked into the texture because a pass takes one texture. The pattern is
placed by the wall's position along its own direction from the map origin, so it runs on unbroken
across wall pieces in line. Added in the liquid's colour (`glow_flat_entry`'s for nukage, slime and
lava; the flat's average for water, `FWATER*`/`SWATER*`) at `RIPPLE_ALPHA`, `PF_NoTint`.

## Busy Computer Panels (`r_readout.c`)

The LED readouts on computer textures change now and then, cell by cell. DERP's finder is used
verbatim (`derp/screen_glass.h`, `doom_screen_glass` then `doom_screen_readouts`), over each
computer texture on the level's walls (`doom_screen_texture`), composited from its patches at level
load; it labels each readout texel with its offset in the readout, the readout's width and whether
cells are two texels wide. A cell shows, now and then, **the original pixels of another cell in its
row** (DERP's rule), so the panel keeps its share of lit cells and only the pattern moves. Which, and
when, is a hash of texture, cell and level clock (`ro_texel_now`): each cell holds 35-140 tics, and
changes only on 5-tic boundaries (`RO_QUANTUM`).

The texels themselves are rewritten, once a frame from `D_Display` before any view (no render worker
is running), which is why it works in **both renderers**:

- **Software**: `texren->cache`. A wall column is drawn as one solid run from `columnofs[x] +
  pixel_data_offset` whatever the storage model, so a texel is a byte store. Textures whose columns
  have holes or several posts (`TD_hole`, `TD_odd_post`) are skipped, since a write there would land
  on post headers. A purged and regenerated cache holds the art again; software re-checks every
  frame, cheaply, so it is put right.
- **OpenGL**: the cache's own RGBA copy (`HWR_Texture_Texels`, compositing it first if it was
  purged), written through the same nearest mapping `HWR_DrawPatchInCache` uses for a scaled block,
  then `HWR_Texture_Reupload`, which calls the new driver entry `DeleteTexture`: it drops that one
  texture, unlinking it from the driver's cache list, so its next `SetTexture` uploads the changed
  texels. The only way before was flushing every texture. Uploads happen only when a cell changes,
  and the 5-tic quantum caps them; each rebuilds that texture's mipmaps.

Turned off, every texel is put back. Nothing reads or writes game state and there are no random
numbers.

## Liquid ceilings

Nukage, slime and lava on a **ceiling** glow down the walls as well as glowing themselves
(`glow_liquid_band` with `down`), the ramp running from the ceiling downward.

## Lamp Light

Glowing Lights only brightens the lamp's own pixels, and on the cabinet that read as "a couple of
changes": nothing round a red panel turned red. Lamp Light is the missing half, the part of DERP's
"light comes from things that look like lights" a fixed-function renderer can do.

**Sources, found once per level** (`glow_build_lamps`, on first use): every wall part whose texture
emissive.h calls a light (`LAMP`, `PANEL`, `DIFFUSE`, `GLASS`; not the liquids, which have their own
glow) gets a source every `LAMP_SPACING` units along it, `LAMP_OUT` in front of the face, at the
middle of that part's height. Every sector with a lamp ceiling flat gets sources on a grid (at most
`LAMP_CEIL_MAX`), placed with `R_PointInSubsector`, `LAMP_CEIL_DROP` under the ceiling, with a
reach of 1.5x the room's height plus 64 so it spreads over the floor.

**Fall-off with distance is quadratic** (`glow_falloff`, `1 - (d/R)^2`), not linear: linearly, a floor
a room's height under a ceiling light kept a third of the light.

**The wall switches** (`SW1*`, `SW2*`) are added to DERP's list as panels, in `glow_kind`, for both the
glowing pixels and the light: emissive.h is kept verbatim, so the cabinet's additions live there. A
switch's indicator is a few pixels, so switches get a minimum strength of 170 whatever their share of
lit pixels.

**A switch glows only when it is on, unless it is a red/green switch.** Which state is "on" is read
from the art, not assumed from the name: a switch's light is whichever of its red, green and blue
pixel groups is *brighter than in its other state* (`glow_switch_stat`, the rise in summed brightness,
at least a handful of pixels' worth). An ordinary switch lights something in one state only, so it is
dark in the other (SW1COMP, SW1SLAD, SW1BRNGN dark; their SW2s lit). SW1BRCOM's red indicator dims
in SW2BRCOM while a green one lights, so both states rise in some group and it glows red one way,
green the other -- the red/green switches fall out of the rule. The glow mask of a lit switch keeps
only the pixels of its lit group (`glow_switch_mask_group`), or the dimmed red on SW2BRCOM still
glowed. Averaging every lit pixel, as for other lights, made SW2BRCOM orange: that dimmed red passes
DERP's test. Every switch wall gets a light at load, dark while the switch is off, so throwing it
turns the light on (`glow_lamp_follow`).

**A wall light follows its texture** (`glow_lamp_follow`): each remembers its side, which part, and the
texture its colour came from, and when the texture there changes it takes the new one's colour, or goes
dark if the new one is not a light. Throwing a switch swaps SW1 for SW2 and the light's colour with it.

**The colour sums need 64 bits.** `glow_cstat_t` sums colour times strength over every pixel, up to
4096 x 255 x 255, and normalising multiplies that by 255 again, which wrapped 32 bits on any art with
many bright pixels. For two rounds of cabinet reports the red ceiling light came out brown, the white
strips at 1,1,1 (no light at all) and only the exit sign, with few bright pixels, was right. It was found
by dumping every source's colour and strength at build time (temporary `fprintf` in `glow_wall_lamps` and
`glow_ceiling_lamps`) instead of tuning by eye; do that first if a light looks wrong again.

**Sprites and the weapon** take the lamp light too: `HWR_Glow_Tint` sums the sources reaching the point
(the mobj's middle, or the view for the weapon) and scales the sprite's light colour by `1 + lamps`,
the same rule `PF_LightMul` applies to walls. Called from `HWR_DrawSprite` and `HWR_DrawPSprite`.

**Colour and strength from the art**: the texture's patches (or the flat) are decoded and DERP's
pixel test averaged over them. A panel dotted with a few LEDs gives proportionally little light
(DERP's rule, `frac`) -- **panels only**. A `LAMP` gets at least 220/255: the first version applied the
rule to everything, and the red and blue ceiling lights (`TLITE6_5`, `FLAT22`), whose bulb is a tenth of
the flat and the rest fixture, came out at a quarter strength and visibly lit nothing, while the exit
sign (mostly lit pixels) worked. That was the cabinet's first report. Neutral strips and ceiling lamps that DERP's test passes over (TLITE6_4 has no
pixel it counts) fall back to their bright pixels (any channel >= 176).

**Drawn with `PF_LightMul`**, a blend added for this: `dest * (1 + src)` (`GL_DST_COLOR, GL_ONE`,
the ninth entry of `blend_param[]`, which is now 16 long and indexed by the full `PF_blend_field`).
A soft disc (`glow_disc`) is projected onto each wall and floor polygon near a source, in the
source's colour. Multiplying keeps the texture's detail and tints it, where the stock dynamic lights
(additive) wash a flat colour over it. The trade: it cannot light black, so a pitch-dark room stays
dark, and the brightest it can make a texel is twice what it was.

**Two passes per light, and crowding.** Multiply alone read as "very subtle" on the cabinet: it can
at most double a texel, and a grey or dark surface barely takes a colour from it. So each light is
drawn twice: the `PF_LightMul` multiply, then a `PF_Additive` pass of the same disc at `LAMP_ADD`,
which is what puts a lamp's colour onto a dark wall. The disc is broad (`1 - r^2`, three quarters left
half way out), not a spot. Both passes stack, and the multiply stacks as a product: ten panels round
one computer wall made it ten times as bright and white.

**Crowding is per lamp** (`glow_lamp_shares`, once at load): each lamp's strength is scaled by
`1 / (1 + how much the lamps round it overlap it)`. A lone switch keeps all of it, a strip of lights
or a wall of panels shares about one light's worth. It was first done **per surface** (each surface
summed the lights reaching it and scaled them to one), and that was visibly wrong: the wall pieces
beside a switch had one lamp reaching them and kept full strength, the floor and side walls had
several and were scaled almost to nothing, so every switch had a hard-edged lit rectangle beside it
and its room stayed unlit. Anything that varies between surfaces cuts the light at their edges; keep
such decisions on the lamp.

**A light near a wall lights a tight pool on it.** The disc's radius on a wall is capped at
`4 x distance + 48`: light grazing a wall falls off fast along it. A switch 12 units out lights a pool
about 100 wide round itself, on its own wall and the coplanar pieces either side alike, and the rest
of the room normally. (Before, a lamp skipped its own wall entirely, which left the switch's wall
dark between two lit neighbours.)

Sprites and the weapon take `LAMP_SPRITE_GAIN` times the light, each lamp at its share, capped at one
light's worth.

**Where the light reaches**: the source's own sector and every sector one two-sided line away, the
latter only while that opening is open *now* (`glow_opening_open`), so a closed door stops it. There
is no other occlusion, which is why the reach is short (`LAMP_RADIUS_MIN`..`MAX`). Walls only take
light on the face the source is in front of; ceiling sources do not light ceilings. Each source
follows its sector's blink, through `glow_sector_scale`.

**Cost**: one extra polygon per source per surface it reaches. Sources are bucketed per sector
(`glow_sec_first`/`glow_lamp_refs`), so a surface only looks at the few that can reach it.

Tuning is the constants at the top of the lamp section of `hw_glow.c`: `LAMP_STRENGTH` (the
multiply), `LAMP_ADD` (the additive colour) and `LAMP_SPRITE_GAIN`.

When capturing before/after shots, force `gr_glow_lamplight "Off"` in the scratch config: the live
`legacyhome` may already have it on, and then both shots have it.

## The damage and pickup flashes (`PF_NoTint`)

OpenGL does the palette flashes as a *tint* (`HWR_SetFlashPalette` → `HWD_SET_TINT_COLOR`), applied
in `DrawPolygon` to every `PF_Modulated` polygon. Stock, it was a straight average of the tint with the
polygon's light colour, and that collided with these passes twice over:

- Every glow and lamp pass is a modulated polygon too, so each one got the mix: a lamp pass meant to
  add almost nothing added half the flash colour, two passes a lamp. Getting shot under Lamp Light
  turned the screen bright red. Every pass here, and the stock dynamic lights (`LIGHTMAPFLAGS`), now
  carry **`PF_NoTint`** (0x00010000), which the driver honours: a light added on top of a surface
  that is already tinted.
- The average itself was the second report (a dark room lighting up on a pickup), stock behaviour: a
  surface light of 30 became about 140 under the gold bonus tint, and the faintest flash replaced
  half of every surface's light. The tint's alpha byte, never read before, is now the **strength**
  (`tint_mix`): `light + (tint - light) * strength`, with `palette_to_tint` giving Doom's own palette
  steps (damage 1/9..8/9, bonus 1/8..1/2, radiation suit 1/8).

## Lifetime of the masks

The mask mipmaps live in three tables: wall textures by number, flats and sprites by lump (open
addressed, 1024 and 4096 slots; a full table just means that lump does not glow). Two things write
**into** those tables after a mipmap is used: the GL driver links each uploaded mipmap into its own
list (`nextmipmap`), and `Z_Free` clears a block's owner pointer, which is `&mip.GR_data`. So the
tables are freed only in `HWR_Glow_Free`, called from `HWR_Free_TextureCache` **after both**
`pfnClearMipMapCache` and `Z_FreeTags(PU_HWRCACHE)`. Freeing them anywhere earlier leaves the driver
or the zone writing into freed memory. The lamp list is freed there too (`glow_lamps_free`), which is
also what makes it rebuild for the next level.

The texel blocks are `PU_HWRCACHE` like every other upload, so the zone may purge them; a mask with
neither `downloaded` nor `GR_data` is simply rebuilt. A palette change (`HWR_SetPalette`) and a filter
change flush everything the same way.

The glow pass rebinds whatever the cache last bound (`hwr_glow_bound`), so a caller drawing a second
piece of the same wall without calling `HWR_GetTexture` again still gets its own texture.

## Determinism

Nothing here reads or writes game state, draws a random number, or spawns anything. `make demotest`
cannot see the renderer anyway (`-nodraw`); it was run to confirm that, and showed 0 desyncs. The
"ended at a different tic" lines it printed for four or five long episode demos appear identically
with the unmodified binary: pre-existing noise in the test, not this feature.

## Testing it

- Headless captures: `SDL_VIDEODRIVER=offscreen` with `drawmode "OpenGL"`, `-devmode`, an autoexec
  that `setpos`es the camera, screenshots, turns the three cvars on and screenshots again. Space the
  two shots **24 tics apart** (`wait 24`): the nukage animation is three frames of eight tics, and two
  shots four tics apart compared different animation frames and looked like the glow had darkened the
  pool.
- A flickering sector or a monster's attack between the two shots changes the picture too. Take a
  control run with the cvars left off before believing a difference.
- To check alignment, temporarily paint every mask texel magenta in `glow_texel`; a tint through
  `FlatColor` is useless on red LEDs.
- A level change with all three on (`map E1M2` from an autoexec) exercises `HWR_Glow_Free`.
- **Play every record demo in OpenGL with all four switches on before calling a change here done.**
  `make demotest` runs with `-nodraw` under the dummy driver and never reaches this file, and a
  single-level check only reaches that level's walls. The cabinet segfaulted on the first frame of
  the E3M1 attract demo because `glow_switch_partner` -- asked of every wall part that is not a light,
  to find switches that are off -- read `textures[0]` for a blank (`-`) part; texture 0 is "no
  texture" and not a real one. Every texture-number entry point here now range-checks. The sweep that
  checks this: each demo under `SDL_VIDEODRIVER=offscreen`, `drawmode "OpenGL"`, the four `gr_glow_*`
  on, `-timedemo`, four at a time, flagging any run that dies on a signal or leaves a `crash.txt`
  (the game id and level pack parsed from the file name as `tools/demotest.sh` does).
