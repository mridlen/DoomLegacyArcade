# Gotchas found the hard way

*Part of the DoomLegacy arcade cabinet build. Debugging reference. Worth a look when something behaves impossibly — especially demo desync, a grep that finds nothing, or wrong colours.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **`SDL_BITSPERPIXEL()` and `SDL_PixelFormat.BitsPerPixel` disagree, and the difference selects
  the wrong software drawer.** For the packed 32-bit formats that carry no alpha, the macro
  reports the bits that hold **colour** and the struct reports the bits a pixel **occupies**:

  ```
                          SDL_PixelFormat | SDL_BITSPERPIXEL
    SDL_PIXELFORMAT_RGB888     32 bpp     |      24 bpp       DIFFER
    SDL_PIXELFORMAT_BGR888     32 bpp     |      24 bpp       DIFFER
    SDL_PIXELFORMAT_ARGB8888   32 bpp     |      32 bpp       same
    SDL_PIXELFORMAT_RGB565     16 bpp     |      16 bpp       same
  ```

  `RGB888` is the format of an ordinary X11 window, and `vid.bitpp` is what `V_Setup_VideoDraw`
  switches on to pick the drawer — so reading it through the macro quietly asked for `DRAW24` on a
  32bpp screen and drew 4-byte pixels with the 3-byte drawer. Every texture on screen came out
  mangled, in the software and native drawmodes only.
  - `SDL_BYTESPERPIXEL()` agrees with the struct for every format, so the *stride* stays right and
    nothing shears; only the pixel packing is wrong, which is why it reads as "glitchy textures"
    rather than as a video mode fault.
  - Use **`SDL_AllocFormat( fmt )`** and read `BitsPerPixel`/`BytesPerPixel` from it. That is the
    same call SDL makes when it builds a surface's format, so it matches what a surface would have
    reported, for any format.
  - It cost a bad build on the cabinet. Nothing automated catches it: it compiles, it runs, and
    `make smoke` passes — the damage is only in the pixels. → `software-fullscreen.md`

- **The 24 and 32 bpp software column drawers had an unsigned `heightmask`, and it crashed every
  sprite.** `R_DrawColumn_24`/`_32` (`r_draw24.c`/`r_draw32.c`) declared
  `unsigned int heightmask = dc_texheight - 1`, where `R_DrawColumn_8` (Boom's original, killough)
  declares it **signed**. A masked column — every sprite — runs with `dc_texheight == 0`, so
  `heightmask` is `-1` and `(frac>>FRACBITS) & heightmask` is meant to be a no-op. Unsigned, it
  is not one: the first row of a post can land on a slightly negative `frac`, and index `-1`
  becomes `4294967295`, a read 4GB past `dc_source`. Signed, it stays `-1` and reads the byte
  before the post, exactly as the 8bpp drawer always has.
  - The cabinet is a 32bpp desktop, so **every software-mode session segfaulted within a second
    or two of gameplay**, at any player count. It went unnoticed because the cabinet runs OpenGL;
    it only surfaced when software mode was tried while testing the four player view grid.
  - The `if( dc_texheight & heightmask )` power-of-two test is unaffected by the signedness, so
    the fix is one word in each file.
  - **Look for the same divergence elsewhere.** `r_draw8.c` is the path with decades of use
    behind it; where the wider-bpp copies differ from it, suspect the copy. `r_draw16.c` has no
    `heightmask` at all, which is worth a look if 16bpp is ever used.

- **`V_DrawString` text is red by default; `V_WHITEMAP` makes it grey, and there is no red flag.**
  This reads backwards, so it is easy to get wrong. `V_WHITEMAP` (`v_video.h`) is the *only* colour
  flag `V_DrawString` understands — there is no `V_REDMAP`/`V_GREENMAP` — because Doom's `hu_font`
  (`STCFN0xx`) is drawn in reds already: its glyphs are palette indices 168..192, verified as
  177..187 for the letters in "NEW RECORD", running from `rgb(255,0,0)` into darker reds.
  `CON_SetupBackColormap` (`console.c:334-350`) builds `whitemap` by remapping exactly that
  168..192 red band onto 80..104, which the palette shows as greys (`239,239,239` … `79,79,79`),
  with hand-patched entries for indices 45 and 47. So **passing `0` gives red and passing
  `V_WHITEMAP` gives grey/white** — the opposite of what "whitemap" suggests to anyone expecting a
  plain white. A white-on-grey label that "doesn't show up" is this. `graymap` (a darker remap) and
  `greenmap` exist alongside it but are not reachable through `V_DrawString`'s option flags; they
  need `V_DrawMappedPatch` directly. Confirm colours by reading `PLAYPAL` and the glyph lumps out
  of the IWAD rather than trusting the source comments — the `EN_heretic` branch above this one
  remaps to a *different* red range (145..168) and the comments in that function describe both.

- **15 source files are ISO-8859, not UTF-8, and grep silently skips them.** A stray `°`, `é` or
  similar in a comment makes the file invalid UTF-8, and grep treats it as binary — no match, no
  warning, exit code as if the term simply is not there. The affected files include several
  central ones: **`r_main.c`, `p_map.c`, `r_segs.c`, `r_splats.c`, `s_sound.c`, `console.c`,
  `hardware/hw_main.c`, `hardware/hw_light.c`, `sdl/i_sound.c`, `sdl/ogl_sdl.c`,
  `hardware/r_opengl/*.c`, `djgppdos/Vid_vesa.c`**. This produces **confidently wrong conclusions**
  — searching for assignments to `st_overlay_on` returned nothing but the declaration, which reads
  exactly like dead code, when in fact `r_main.c:894` sets it. Whenever a grep says a symbol is
  declared/read but never written, re-check with something encoding-agnostic before believing it:
  `nm ../objs/*.o | grep " symbol"` to find which object defines it, or
  `python3 -c "...open(p, encoding='latin1')..."`. Re-list the affected files with:
  ```
  python3 -c "import os
  for r,_,fs in os.walk('.'):
   for f in fs:
    if f.endswith(('.c','.h')):
     p=os.path.join(r,f)
     try: open(p,'rb').read().decode('utf-8')
     except UnicodeDecodeError: print(p)"
  ```

- **Returning to the title screen resets very little.** State from the finished game leaks into the
  attract screen, which has produced two separate bugs: a loaded level pack left the built-in demos
  playing against the wrong maps, and `cv_splitscreen` left them rendering in a split view. Both are
  now cleared on the way out — the pack by restarting, splitscreen in `Command_ExitGame_f()`, which
  is the single funnel for every route back to attract mode (End Game, the idle timeout, and the
  engine's own error paths). **If the attract screen ever looks wrong after play, suspect leftover
  state first**, and prefer fixing it in `Command_ExitGame_f` so every route is covered.

- **A level's palette tint outlives the level, in two places.** `ST_doPaletteStuff` is called only
  while a player view is being rendered (`R_SetupFrame`, `HWR_RenderPlayerView`), so whatever it
  last set simply stays once the level stops drawing: walk out wearing a radiation suit and
  everything after it is green, take a hit at the exit switch and it is red. `ST_Palette0()` is the
  reset — it handles the hardware flash path as well as the 8-bit one and updates `st_palette` so
  the next `ST_doPaletteStuff` still sees a correct previous value. It is now called from **both**
  places a level's palette can outlive it:
  - `D_DoAdvanceDemo`, for the attract screen (the score pages show it worst, being a full-screen
    fill, but the title and credit pages inherit it too); and
  - `G_Start_Intermission`, just before `gamestate = GS_INTERMISSION`, for the gap **between
    levels** — the intermission paints the whole screen and had been doing it through the tint, and
    the next level then kept it until its own first rendered frame.

  The only other route to `ST_Palette0` is `ST_Stop` via `ST_Start`, i.e. when the next level
  begins — far too late for anything drawn in between. `ST_Start` does set `st_palette = -1`, which
  forces the *next* rendered frame to re-set it, but that does nothing for the frames before it.

- **No PK3 support; WadSmoosh is not usable.** The file-type dispatch (`w_wad.c`, `W_...` extension
  check) recognizes only `.wad`, `.deh`, `.bex` and `.zip` — anything else is loaded as a single
  lump — and the IWAD tables (`d_main.c`, the `gamedesc` list) name `.wad` files only. WadSmoosh's
  `doom_complete.pk3` additionally depends on GZDoom-specific machinery (a `GAMEINFO` lump to
  declare itself an IWAD, ZMAPINFO/MAPINFO for episode and map definitions) that this engine does
  not implement, so renaming it to `.zip` will not help either. `.zip` support here is for
  supplementary lump archives, not IWAD replacement. Use separate `.wad` IWADs and the game
  selector above. **Gameplay mods** follow from the same limits: DEHACKED and BEX are supported
  file types (`FC_deh`/`FC_bex`, loadable with `-file`) and `MBF21` is compiled in
  (`doomdef.h`), so DEH/BEX-driven mods including MBF21 ones are the compatible category. There is
  **no DECORATE or ZScript anywhere in the tree**, so GZDoom mods (Brutal Doom and similar) cannot
  work, and no amount of repackaging changes that. Test a candidate with `-file mod.deh` before
  building anything around it.
- **Replacement music is lumps, not files — there is no music directory.** `S_ChangeMusic`
  (`s_sound.c`) resolves a track through `S_FindExtMusic` (`sounds.c:1056`), which tries
  **`o_<name>`** first and falls back to **`d_<name>`** (`%.8s`, so the lump name limit applies);
  the format is then sniffed from the lump's own header by `detect_music_type` — `MUS`, `MThd`,
  `ID3`/`\xFF\xFB\x90`, `Ogg`. So an OGG soundtrack has to be packed into a wad and loaded like any
  other PWAD. `MUSIC_OGG`/`MUSIC_MP3` are enabled in `doomdef.h`, and SDL2_mixer carries its own
  Vorbis decoder, so nothing extra has to be linked.
  - **`cv_music_source` ("music_source") gates the `o_` lookup entirely.** At `MUS` its
    `src_music_enables[]` row is `ADM_MUS | ADM_MIDI`, so the `o_` name is never tried — which is
    where the cabinet's tracked config used to sit, and it now ships at **`Auto`** (the compiled
    default anyway) so a soundtrack wad works with no settings change. Under
    `MUSIC_SELECT_ALT_IS_SILENCE` (defined), the `MP3`/`OGG` settings play *silence* for anything
    not replaced rather than falling back, so `Auto` is the only sensible choice of the four.
  - `legacyhome/levels/` is the wrong home for a music wad: `M_LevelPack_MapStyle` filters that
    directory by map lumps, so one with no maps is never listed. `addfile` from
    `legacyhome/autoexec.cfg` works, and the ordering is safe — `D_DoomLoop` runs
    `COM_BufExecute` before entering its `while(1)`, so the wad is in place before
    `D_DoAdvanceDemo` picks the title track.
  - `P_process_wadfile`'s music scan (`p_setup.c`) only *counts and prints* replacements; it does
    not reset `S_music[].lumpnum`. A track already looked up keeps the lump it found, so a wad
    added mid-session applies to tracks not yet played. Changing `cv_music_source` forces the
    re-lookup (the `CS_MODIFIED` test in `S_ChangeMusic`).
- **Demo recording must start before the game-start commands are issued.** `G_Ticker` writes demo
  data *before* `ExtraDataTicker` executes queued netxcmds, so recording started from inside
  `G_InitNew` misses the commands that create the player and load the first map. Such demos then
  segfault on playback in `P_SetupPsprites` (NULL `player->weaponinfo`). This mirrors how `-record`
  works: it begins recording before any game exists.
- **`demoname` is only 32 chars** (`DEMONAME_LEN`). `G_DoPlayDemo` used to copy full external demo
  paths into it and silently truncate them; the failure path does not advance the attract cycle, so
  the title screen froze forever. It now uses a `MAX_WADPATH` buffer for the file read.
- **Demo desync (fixed).** Demos desynced whenever any DoomLegacy gameplay extra was enabled —
  `tiredrun`, `drown`, `monster_vary`, `tele_control`, `slow_react` — which `tiredrun` is by
  default. `G_demo_defaults()` force-disables them so demos replay vanilla, but it runs **only**
  from `G_DoPlayDemo`; nothing equivalent runs while recording, and the cvars were never written to
  the demo. So a demo recorded *with* tired-run replayed *without* it: `movefactor` 2048 → 2046 once
  the player tires, a ~0.1% momentum error per tic that compounds. Fixed by writing them (plus
  `cv_viewheight`) into the header's spare option bytes. **When adding any new gameplay-affecting
  cvar, either add it to the demo header or to `G_demo_defaults()`, or demos will desync.**
- **Rocket smoke trails desynced every demo where a rocket was fired (fixed).** `A_SmokeTrailer`
  (`p_fab.c`) — Legacy's own rocket and lost-soul trail, hung on `states[S_ROCKET].action` and
  `S_SKULL_ATK3`/`4` by `G_Downgrade` — gated its 4-tic cadence on raw **`gametic`**. `gametic`
  is zeroed **once per process**, in `D_Init_ClientServer`; nothing resets it for a new game. So
  its `% 4` phase at the start of a run is just how long the cabinet had been sitting on the
  attract screen. The puff it spawns consumes `PP_Random(pL_smoketrail)`, and `PP_Random`
  **ignores its class parameter and advances the shared `prndindex`** — the gameplay RNG. Record
  at one phase, replay at another, and the run diverges on the **first rocket fired**.
  - **This is a known Doom bug that upstream had already fixed one of the two copies of.**
    `A_Tracer` (`p_enemy.c`) carries killough's comment — *"internal demos start at random
    gametics, thus the bug in which revenants cause internal demos to go out of sync"* — and uses
    **`game_comp_tic`**, which is reset in `G_DoPlayDemo`, **written into the demo header** by
    `G_BeginRecording`, restored on playback, and advanced once per *simulated* tic (it skips the
    paused/menu tics that `gametic` counts anyway). `A_SmokeTrailer` never got the same treatment.
    The fix is one line, copied verbatim from `A_Tracer`, keeping raw `gametic` for pre-1.47
    Legacy demos that were recorded against it.
  - **It surfaced with the first multi-level survival demo**, not because multi-level demos are
    special but because the earlier single-level record demos are E1M1-E1M4 runs where the player
    never has a rocket launcher. A campaign run picks one up in E1M5 and desyncs there — which
    reads as "the demo breaks halfway through E1M5" when the cause is one RNG call.
  - **Diagnosing it needs an oracle, because a desync does not stop demo playback.** Level
    transitions are driven by the recorded `XD_MAP` textcmd (`AddLmpExtradata`), and
    `G_DoWorldDone` explicitly skips issuing its own (`if(server && !demoplayback)`), so the
    replay marches through all eight maps whatever the player is actually doing. The check that
    works: **temporarily log `G_ExitLevel` (sim-driven) beside `Got_NetXCmd_Mapcmd` (demo-driven)
    and see where they stop agreeing.** In sync the sim exit lands exactly one tic before the map
    command; E1M1-E1M4 did, E1M5 produced no sim exit at all. Then sweep the four possible tic
    phases — exactly one resynced the whole demo through E1M8, which both proves the mechanism and
    identifies it as a phase error rather than anything about rockets themselves.
  - **The fix is forward-only; demos recorded before it stay broken.** An old demo's puffs were
    emitted on the *recording session's* `gametic` phase, and nothing in the header records what
    that was — only `game_comp_tic`, which differs by an arbitrary constant. The cabinet's
    `doomu_ep1_sk0_speed.lmp` is off by one and cannot be repaired; it has to be re-run, or
    deleted so the next record replaces it. Only demos where a rocket or lost soul actually fired
    are affected, so most of the single-level table is fine.
- **`-synclog`** writes one line of simulation state per tic while recording or playing back, to
  `synclog_rec.txt` / `synclog_play.txt` in the current directory. Record a demo with it, play that
  demo back with it, and diff: the first differing line is the divergence tic. Inert without the
  flag. This is what found the bug above — identical inputs/RNG/angle with drifting momentum
  immediately excluded logic and RNG causes and pointed at the movement factor.
- **`G_StopDemo`/`G_CheckDemoStatus` used to free `demobuffer` without clearing it**, leaving a
  dangling pointer for any later recording to re-free. They now NULL it.
- The background recorder writes a stray `hs_background.lmp` into the current directory on exit.

- **The level-load flicker was four separate forced page flips, not a slow load.** Every level
  transition strobed the screen. Nothing was rendering a frame — the engine was flipping the
  OpenGL buffer while the level did not exist yet, so each flip showed whatever stale content the
  back buffer happened to hold. It looked like a "loading bar" because one of the four drew a text
  box, but the flicker came from the flips, not the box. The cabinet loads a level every couple of
  minutes, so this was also a real photosensitivity hazard, which is why it was removed rather
  than slowed down.
  - **The strobe** was `loading_status()` in `hardware/hw_bsp.c`, called from the subsector case of
    `HWR_WalkBSPNode` once every `numsubsectors/50` subsectors — so ~50 times, as fast as the GPU
    would take them. It drew `CON_Drawer()` plus a `M_DrawTextBox` "Loading... N%" and called
    `I_FinishUpdate()`, i.e. `SDL_GL_SwapWindow`. **It never cleared the buffer**, so it painted a
    small box over two alternating stale frames and swapped between them fifty times.
  - **The three flashes** were `GenPrintf( ... | EMSG_now, ...)` on "Setup Level" (`p_setup.c`),
    "Solving T-joins" and "Creating polygons" (both `hw_bsp.c`). `EMSG_now` is not a log-level
    flag: in `CONS_Printf` (`console.c:1292`) it takes the same branch as `con_self_refresh` and
    does `V_Clear_Display()` + `CON_Drawer()` + `I_FinishUpdate()`. That branch exists for the
    **startup** screen, where the display loop is not running yet; `D_DoomLoop` clears
    `con_self_refresh` (`d_main.c:1211`) before any level loads, so in-game the flag is the only
    thing forcing the repaint. Dropping `EMSG_now` keeps every message in the console and the log
    and leaves the startup screen untouched, because startup repaints from `con_self_refresh`.
  - **`EMSG_now` on a message that can fire during play is nearly always wrong** for this reason.
    The one left is `p_setup.c`'s map-load error, which only fires when the level fails anyway.
  - **All of this is OpenGL-only.** `p_setup.c` gates `HWR_SetupLevel()` on
    `rendermode != render_soft`, so the software renderer never drew a loading status at all —
    which is the standing proof that nothing depends on it. Removing it is invisible to the
    simulation: no game state, no cvars, no RNG, nothing in the demo header, so **demos are
    unaffected**. Only `I_OsPolling()` was kept, so the window still pumps events while the BSP is
    walked.
  - Incidentally the wipe into the new level now starts from the last properly rendered frame
    instead of "text box over garbage", since `G_DoLoadLevel` sets `GS_FORCEWIPE` before
    `P_SetupLevel` and `D_Display` grabs the start screen afterwards.

- **`GL_CLAMP` is not `GL_CLAMP_TO_EDGE`, and the difference was a thin black box around every
  intermission animation.** OpenGL only; software mode never showed it. `SetTexture`
  (`r_opengl.c`) set `GL_TEXTURE_WRAP_S/T` to **`GL_CLAMP`** for any texture without `TF_WRAPX`/
  `TF_WRAPY` — that is every patch: sprites, HUD, menu graphics, the intermission animations.
  `GL_CLAMP` is the OpenGL 1.0 behaviour that clamps to the texture **border**, and with linear
  filtering an edge sample blends the edge texel with the border colour, which defaults to
  **transparent black**. So every patch got a half-texel dark fringe all the way round.
  `GL_CLAMP_TO_EDGE` (core since OpenGL 1.2) clamps to the edge texel and has no border to bleed.
  - **Why the intermission showed it worst.** The animation patches are small and the intermission
    is drawn from a 320x200 base, so at 1366x768 a half-texel fringe is magnified into an obvious
    2-3 pixel outline, and it sits against a flat, evenly lit background where a straight dark line
    is unmissable. The same fringe was on every sprite in the 3D view, but broken up by the scene.
  - **The patch fills its block exactly here, which is why all four sides were affected.**
    `HWR_MakePatch` puts the patch in the top-left of a power-of-two block and sets
    `max_s = newwidth/blockwidth`; when that works out to 1.0 the quad samples right up to the
    texture edge and the border bleeds in on every side. Where a patch *is* padded (`max_s < 1`)
    the clamp mode is irrelevant, because sampling never leaves [0,1] — a fringe there would come
    from the transparent padding instead and would need the edge texels replicated. Nothing
    observed needed that, but do not confuse the two failures.
  - **Do not "fix" this by forcing nearest filtering.** Note the cabinet's `gr_filtermode` is
    already set to `"Nearest"` and the render is plainly bilinear anyway, so that setting is not
    doing what it says — which is its own bug, and was not what produced the fringe.
  - Verified by screenshot, headless, on the real GPU: capture the intermission with
    `SDL_VIDEODRIVER=offscreen` and an autoexec of `wait`/`exitlevel`/`screenshot`, then compare
    crops before and after. See `CLAUDE.md` for the offscreen recipe. **This is the class of bug
    that only a picture settles** — it is invisible to logs and to every non-graphical check.


- **The demo header used to record the *previous* game's settings. Fixed — but the ordering that
  caused it is deliberate, so do not "simplify" it back.** Demos recorded before this fix (every
  `.lmp` currently in `legacyhome/demos`) still carry the wrong values; most read
  `skill=2 episode=0 map=0`, and a few look right only because a retry of the same level at the
  same skill left matching values behind.
  - **Why it happened.** `HS_NewGame` (`hs_stuff.c`) calls `G_BeginRecording` and **must** precede
    `G_DeferedInitNew`, so the player-create and `map` netxcmds land in the demo stream — both call
    sites in `m_menu.c` say so. But `G_DeferedInitNew` only *queues* its commands with
    `COM_BufAddText`; `G_InitNew` — which settles `gameskill`/`gameepisode`/`gamemap` and applies
    the `sk_nightmare` overrides — does not run until the command buffer drains, long after the
    header is written. The `-record` path has the same shape: `G_BeginRecording` is called at the
    top of `D_DoomLoop` (`d_main.c`). Measured with a probe on each side: recording a nightmare
    MAP07 run wrote `skill=4 ep=0 map=0 fastmon=0 respawn=0` while the game being recorded was
    `MAP07 fastmon=1 respawn=1`.
  - **Why it was harmless, and why it was still worth fixing.** For `demoversion >= 127` — every
    demo this build makes — `G_DoPlayDemo` ignores the header's skill/episode/map and waits for the
    `map` command in the stream, which carries `-skill N`, so `G_InitNew` re-derives
    `cv_fastmonsters` and `cv_respawnmonsters` on playback. The stale fields never desynced
    anything. They did mislead *readers* of a header, including the attempt to work out which demos
    the fast-monsters fix affected — which it got wrong twice before an A/B replay settled it (see
    `gameplay-defaults.md`).
  - **The fix.** `G_Update_Demo_Header()` (`g_game.c`) rewrites the seven affected bytes in place,
    called from the end of `G_InitNew` once the globals are settled — *not* by moving
    `G_BeginRecording`, which is where it is for the netxcmd reason above. It fires only on the
    **first** level of a recording (`demo_header_pending`, armed by `G_BeginRecording`), because a
    demo that runs a whole episode must keep describing where it started.
  - The byte positions live in the `DEMOHDR_*` enum beside the function, and `G_BeginRecording`
    checks `(demo_p - demobuffer) == DEMOHDR_playeringame` after writing the fixed part, so the
    writer and the patcher cannot drift apart silently. Nothing else about the format changed —
    same length, same `0x55` sync mark, same option area.
  - Verified: a nightmare MAP07 recording now writes `skill=4 ep=1 map=7 fastmon=1`; that demo
    replays with a bit-identical simulation (`-synclog` on both sides); and the four existing
    stale-header demos replay **bit-identically to the pre-fix build**, since the change only
    affects what is written, never what is read.
  - **A `-synclog` note:** a record-vs-playback diff always shows the final `tflags` column
    differing (1 while recording, 0 on playback) with every simulation column identical. That field
    is not carried in the ticcmd; it is not a desync. Compare the other columns.

- **A cvar's `.EV` is a byte, so any cvar whose range exceeds 255 truncates when read through it.**
  `consvar_t` carries both `int value` and `byte EV` (`command.h`), and most of the tree reads `.EV`
  because most cvars are small enums where the two agree. They stop agreeing the moment the range
  does not fit: `cv_idletimeout` used to allow 0..3600, so `idletimeout 3600` read through `.EV`
  came back as **16** — 3600 & 0xFF — and a computation built on it was quietly out by two orders
  of magnitude. Its top is 900 now (a named list, see `attract.md`), which does not make the trap
  go away: 900 & 0xFF is **132**, still wrong and still silent. Any cvar over 255 has this.
  - There is no warning and no clamp. The value in the config is right, the menu displays it right,
    and only the arithmetic downstream is wrong, which is why this reads as a logic bug in whatever
    consumed it rather than as a truncation.
  - `g_game.c` already uses `cv_idletimeout.value` for the idle timeout itself, so the two readers
    of the same cvar disagreed — worth grepping for when a cvar looks like it is being ignored.
  - Caught in the initials-seed work (`high-scores.md`) only because the instrumentation printed the
    *computed* window rather than the cvar, which is the general lesson: print what the code
    derived, not what you set.

- **A thin bright line along the top or bottom edge of a wall is a node-builder rounding error, not
  a texture or a lighting problem.** Reported on E1M6 as "linedef 1044 shows a gap through to the
  sky at the top and bottom of the wall"; there were seven more like it on that map alone.
  - **The mechanism.** A node builder splits a linedef wherever a BSP partition crosses it and
    writes that intersection into `VERTEXES` — as **integers**. On a diagonal linedef the true
    crossing is almost never at an integer, so the split vertex lands off the line it is supposed
    to lie on. On E1M6, linedef 1044 runs (1240,-192)→(1600,-112) and is split at vertex 1129,
    stored as (1373,-162) where the exact crossing is (1373,-162.444) — **0.43 map units off**.
  - **Why that shows.** `HWR_StoreWallRange` (`hw_main.c`) builds the wall quad from the *seg*
    endpoints, so the wall picks up a slight dogleg at the split. `CutOutSubsecPoly` (`hw_bsp.c`)
    clips the floor and ceiling polygons with the *original linedef* points, so those keep the
    straight line — a deliberate 2002 upstream change to avoid BSP round-off, and correct in
    itself. The two edges therefore disagree by that fraction of a unit, and the wedge between
    them is a real hole: widest at the split, tapering to nothing at each end of the linedef. The
    sky backdrop is drawn behind everything, so that is what shows through. Top *and* bottom,
    because the same deviation applies at the ceiling edge and the floor edge.
  - **The fix is the long-known one for the long-known bug.** This is the "slime trails" defect,
    and `P_Remove_Slime_Trails()` (`p_setup.c`, called from `P_SetupLevel` after every node
    format's loader) projects each node-invented vertex onto the exact line of its own linedef.
    Only vertices the builder invented move — **a linedef's own two endpoints are map data and are
    never touched** — and only diagonal linedefs are considered, since an axis-aligned line is
    split at an exact integer and has nothing to correct.
    - **It cannot desync a demo.** Collision, the blockmap and sight all work from linedefs:
      `p_sight.c` walks a subsector's segs but reads `seg->linedef->v1/v2`, never the seg's own
      vertices. Only the renderers walk seg vertices. Check this again before extending the
      function — moving a *linedef* endpoint would be a different matter entirely.
    - Seg lengths are recomputed afterwards; `P_LoadSegs` had already derived them from the
      pre-snap positions.
  - **Verified numerically, both ways.** A standalone WAD parser measured every seg endpoint on
    E1M6 against its linedef line: 16 endpoints off, pairing into **8 distinct split vertices**,
    with linedef 1044's among the worst. The fix then reports exactly **8** vertices moved — it
    counts only vertices whose coordinates actually change, not candidates considered, so the
    number is comparable against that analysis. Too many would mean it was moving things it
    should not.
  - **And photographed.** Driven headlessly under `SDL_VIDEODRIVER=offscreen` on the real GPU with
    a temporary `tppos <x> <y> <angle>` console command to stand the camera at a fixed spot, then
    `screenshot`, then the same four viewpoints before and after. A "bright pixel sandwiched
    between two much darker rows" detector counted the sliver: **333 → 11** at the clearest
    viewpoint, with the same drop at all four. The residue is HUD text, not the seam. The
    magnified before/after crop shows the white dashed line along the wall's top edge simply gone.
  - **It is not visible face-on.** A perpendicular view of the same wall looks clean, because the
    gap is sub-pixel there; it opens up at **grazing angles**, which is why the report came with a
    screenshot taken looking along the wall. Reproduce at a grazing angle or you will conclude
    there is nothing wrong.
  - **Snapping the vertex is only half the fix, and on its own it trades a horizontal seam for a
    worse vertical one.** Shipping just `P_Remove_Slime_Trails` closed the top and bottom gaps and
    immediately opened a dark line running the *full height* of the wall at the split — reported
    straight back, and measured at **39.0** column-luminance delta where the background noise is
    about 8.
    - **Why.** `AdjustSegs` (`hw_bsp.c`) does not draw a wall from the seg's own vertices: it
      snaps each endpoint to the nearest vertex of *that seg's subsector polygon* when one is
      within `VERTEX_NEAR_DIST` (0.75), which is what glues walls to flats. The two segs meeting at
      a split live in **different subsectors**, so they consult **different polygons**.
      Instrumenting the chosen `pv` for linedef 1044 showed it exactly:

      | build | seg A `pv2` | seg B `pv1` |
      | --- | --- | --- |
      | original | (1373.000, -162.000) | (1373.000, -162.000) |
      | slime fix only | (1373.000, **-163.000**) | (**1373.714**, -162.286) |
      | slime + AdjustSegs | (1373.094, -162.424) | (1373.094, -162.424) |

      Originally *neither* endpoint was near enough to snap (0.77 and 1.00 away), so both fell back
      to the raw seg vertex — agreeing with each other, disagreeing with the flats: one horizontal
      seam, no vertical one. Moving the vertex onto the line brought both **inside** the snap
      radius, so both snapped — to two different polygon vertices **0.73 units apart**. The wall
      tore open wider than the gap that was closed.
    - **The second half.** A linedef's *interior* split point must not snap: only its two real
      endpoints may. `store_polyvertex` dedupes within `SEG_SAME_VERT`, so both segs then share one
      polyvertex and the wall is continuous. This is safe **only because** the first half
      guarantees that vertex already lies exactly on the linedef's line, which is the same line
      `CutOutSubsecPoly` cuts the flats with — so the wall is flush with both flats without needing
      to snap. **The two changes must ship together**; either alone leaves a visible seam.
    - Measured over four viewpoints, horizontal sliver pixels in the wall region (HUD excluded, or
      its text swamps the count): original **64**, slime fix only **0** but with the vertical tear,
      both fixes **1** and no tear.
  - **The vertex fix does not remove every seam, and at a near edge-on angle it removes none.**
    Measured over four *genuinely* grazing viewpoints on the same wall (HUD and readout masked),
    horizontal sliver pixels before → after: 130→36, 205→33, 112→10, and **377→428**. The fourth
    is the corridor looked straight down its own length, where linedef 1044 is almost edge-on: a
    bright line still runs along the wall top, and the pre-fix and post-fix crops of it are
    **indistinguishable**. That seam is a separate, pre-existing artifact of the wall/flat junction
    at extreme grazing angles; the vertex rounding is not what causes it and snapping the vertex
    does not help. Do not read a report of "the seam is back" as the vertex fix having regressed
    without A/B-ing that exact viewpoint.
  - **Test angles, not just positions — `mo->angle` alone does not aim the camera.** The ticcmd
    carries an **absolute** angle (`g_game.c`: `localangle[pind] += cmd->angleturn<<16;
    cmd->angleturn = localangle[pind] >> 16`), so a debug teleport that writes `mo->angle` has it
    overwritten on the very next tic and the view stays wherever it was. Every "grazing angle"
    screenshot in the first two rounds of this work was actually taken at the spawn angle with
    only the *position* varying, which is exactly why the reporter kept seeing a seam that the
    measurements said was gone. Set **`localangle[0]`** too. The `Show Coordinates` readout
    (`menus.md`) is the cheap way to catch this: it prints the angle actually in force.
  - **There is a second seam family, cracks between adjacent *flat* polygons.** Reported as E1M5
    linedef 308, standing at X 370 Y 1409 ANG 217 in sector 100 (coordinates read straight off the
    `Show Coordinates` readout — this is exactly what it is for). It is untouched by the vertex
    snapping above, and it needed its own fix.
    - **Tell the two families apart by where the line sits.** The wall/flat seam hugs the top or
      bottom edge of a wall. This one runs *through the ceiling* (or floor), well clear of any
      wall, along a long straight diagonal — a **BSP partition line**. It is the boundary between
      two subsector polygons, not between a wall and a flat.
    - The software renderer does not have it: flats there are drawn by span, not as per-subsector
      polygons.

- **Both seam families are now closed, and this file had the second one's cause wrong.** It used
  to say the flat-to-flat cracks were a deliberately unfinished piece of the upstream renderer
  that could not be closed without "a real piece of work". That was a misreading. Each family
  turned out to be one small defect, and both are fixed. → the two entries below

- **A hairline of sky along the foot of a wall is the *flat* being in the wrong place, not the
  wall.** Reported on E1M2 as "a gap between the floor and wall" on linedefs 506 and 648, and it
  is a crisp one-pixel bright line running the whole length of the junction. The sky backdrop is
  drawn behind everything, so any hole reads as a bright line whatever is beyond it.
  - **The mechanism.** `P_Remove_Slime_Trails` put the *seg* vertices exactly on their linedef,
    and `AdjustSegs` stopped snapping the wall away from them — so the wall is right. The flat is
    not. `fracdivline` (`hw_bsp.c`) treats a cut landing within `DIVLINE_VERTEX_DIFF` (**0.45**)
    of a polygon vertex the BSP split already made as passing *through* that vertex, so the flat
    keeps the node builder's rounded corner while the wall uses the exact one. The wedge between
    them is a real hole.
  - Measured on E1M2 linedef 648: the wall endpoint is `(1210.0413, -663.5167)` and the polygon
    corner `(1210.2347, -663.0609)` — **0.495 units apart**, the same wrong value in all four
    surrounding subsector polygons. E1M6 linedef 1044's polygon corners were still exactly
    `(1373, -163)` and `(1373.714, -162.286)`, the two values the earlier round had removed from
    the *wall* and left in the *flats*.
  - **The fix inverts the gluing: the wall is authoritative and the flat follows it.** `AdjustSegs`
    used to move the wall endpoint onto the nearest vertex of its own subsector polygon, which
    anchors the wall to a point the node builder invented. Now the seg keeps its true map position
    and the polygon corner is pulled onto it (`pull_polyvertex`).
    - **Only vertices the node builder invented move.** `in_poly_vert()` ones are level map data
      and are left alone, so no real map geometry is distorted.
    - **Pull every corner in range, not just the nearest.** On E1M5 linedef 308 the polygon
      carries both the right corner `(344, 1312)` *and* a rounded one `(343.236, 1311.745)`; the
      nearest is already correct, so a nearest-only pull leaves the 0.8 unit notch wide open.
    - **`FLAT_PULL_DIST` is 1.5, not the old 0.75.** The worst corner actually observed is 0.86
      out, so the old radius silently missed it. 1.5 is the widest tolerance already in the file
      (`PointInSeg`'s `MAXDIST`).
    - Polyvertexes are shared between the polygons meeting at a corner, so one pull fixes every
      flat that uses it — which is why all four polygons at a split come right together.
  - **It cannot desync a demo.** Only polyvertexes and `seg_t.length` change, and `r_defs.h` marks
    that field *"length of the seg : used by the hardware renderer"*. Collision, the blockmap and
    sight all work from linedefs.

- **A BSP search that prunes on the wrong axis looks exactly like a feature that does not work.**
  `SolveTProblem` exists to close the flat-to-flat cracks and was closing almost none of them,
  because `SearchSegInBSP`'s bounding-box test compared the node's **right edge against `min_y`**
  in both children:
  ```c
  && (nodes[bspnum].bbox[0][BOXRIGHT ] >= stp->min_y)   // means min_x
  ```
  `BOXRIGHT` is an x edge. On any map whose x range sits below its y range the test fails for
  essentially every node, the whole subtree is pruned, and the function never looks there at all.
  At E1M5's reported spot (X 370, Y 1409) that is every node in the region.
  - **Counted both ways** — T-junction vertices actually inserted, with the bug reinstated versus
    fixed: E1M5 **1 → 9**, E1M3 **1 → 7**, E1M2 **7 → 16**, MAP01 **4 → 8**, E1M7 34 → 36,
    MAP15 32 → 33, E1M1 5 → 5, E1M6 2 → 2. Reinstating the bug is what proves the counter means
    something; a clean number from a check that cannot fail is worth nothing.
  - **`PointInSeg`'s bail-out is not the problem, and this file used to say it was.** The polygons
    are **clockwise** (`hw_poly.h`: *"a convex 'plane' polygon, clockwise order"*), so the "right
    side" of an edge is the polygon's **interior**. The rejected case is a T-vertex lying *inside*
    the neighbour, which is an overlap and hides no crack; the accepted case is the one outside,
    which is exactly the case that closes it and which also keeps the polygon convex. The design
    was right, it simply was not being reached. The `MOVEVERTEX` branch above it is still dead
    code — it assigns to an `a` that is not in scope and would not build. Do not "just enable" it.
  - `AdjustSegs` still carries the matching upstream admission (*"here we can do better, using
    PointInSeg ... but too much work"*), now stale: the wall/flat half is done, above.

- **`AdjustSegs` must run BEFORE `SolveTProblem`, and getting that backwards reopened a seam
  somewhere else entirely.** Reported on E1M5 at X 497 Y 1455 ANG 183 as a hairline across the
  sector 100 ceiling *and* floor — the pair is the giveaway that it is a flat/flat crack on a BSP
  partition, not a wall junction.
  - **Why.** `AdjustSegs` now *moves* polygon corners, where it only read them before.
    `SolveTProblem` places a polygon's T-vertices onto whichever neighbouring edge passes through
    them, so running it first computes those insertions against corners that `AdjustSegs` then
    shifts by up to `FLAT_PULL_DIST`, stranding them off the edge they were placed on. Subsector
    263's edge got tilted onto linedef 308's split point at `(254.4603, 1408.4274)` while its
    neighbours kept vertices at `(832, 1408)` and `(448, 1408)` — **0.286 units** off the new
    edge, running the width of the room.
  - **This was a regression the screenshot A/B had not covered**, and the seam count proved it:
    141 baseline → **243** with the fix, → **72** once the two calls were swapped. Any change that
    moves geometry has to be re-checked against everything that consumed the old geometry.

- **A crack can be at a *map* vertex, and `SolveTProblem` used to refuse to look at those.** It
  skipped any polygon corner satisfying `in_poly_vert()`, commented *"no need to process polyvertex
  from the level map"* — the assumption being that a map vertex is shared exactly by every polygon
  touching it. It is not: a long polygon edge can run straight past a map vertex that is a corner
  of the neighbour, and the node builder's rounding leaves the two a fraction of a unit apart.
  Offering map vertices as T-candidates too takes the **gap-producing T-junction count to zero on
  every map tested**.
  - It costs nothing measurable at load: E1M7 4.26 s before, 4.14 s after (both dominated by
    startup), even though insertions rise from 36 to 103.
  - **It does not wreck convexity, and convexity was never perfect anyway.** Inserting a vertex
    that lies *outside* a clockwise polygon bulges the boundary outward, which is a convex turn;
    the reflex case is the inside one, which `PointInSeg` already rejects. Counting polygons with
    mixed turn directions across six maps: **113 before, 108 after** — pre-existing, and slightly
    fewer.

- **Measure seams over the whole map, not through a viewfinder.** Three rounds of this work each
  ended with "fixed" based on screenshots, and each time the next report was a seam at a spot
  nobody had photographed. The view-independent check is to dump every subsector polygon and count
  **gap-producing T-junctions**: a vertex of polygon A lying off an edge of polygon B, on the far
  side of it, without being one of B's vertices.
  - **Only the far side counts.** A vertex displaced *into* B is an overlap and hides no crack;
    one displaced away leaves a wedge neither polygon covers. Counting both together made the
    numbers useless — E1M7 read 27 → 48 and looked like a bad regression, when the gap-producing
    subset was 13 → 17 and then 0.
  - Gap-producing T-junctions, baseline → final: E1M1 4→0, E1M2 16→0, E1M3 10→0, E1M5 16→0,
    E1M6 0→0, E1M7 13→0, MAP01 2→0, MAP15 8→0.

- **The engine reports both counts, so a regression is visible in the log.** Printed on every
  level load, next to the existing `Creating polygons` / `Solving T-joins` lines:
  `Solve T-joins: N vertices inserted.` and
  `Wall/flat junctions: N polygon corners pulled onto walls, M still not flush.`
  They are `EMSG_all`, not `EMSG_ver`, deliberately: **`EMSG_ver` messages do not appear during
  level setup even with `-v`** — the existing `Slime trails:` line has the same problem and is
  invisible for the same reason, which cost a while of thinking the fix had not run.
  **M is the one that matters** — a polygon corner can be the nearest vertex to more than one wall
  endpoint and would then be pulled twice, ending up flush with only the last. M counts corners
  still sitting within `FLAT_PULL_DIST` of a wall endpoint without being exactly on it. It is
  **0 on all nine maps tested** (E1M1, E1M2, E1M3, E1M5, E1M6, E1M7, MAP01, MAP07, MAP15). A
  non-zero M means a seam survives somewhere.

- **How the two fixes were measured.** Two baselines were built from the same tree — one binary
  from `HEAD`, one with the fix — and the same viewpoints shot with each under
  `SDL_VIDEODRIVER=offscreen` on the real GPU, counting "pixel much brighter than the rows two
  above and two below" inside the wall region only. Seam pixels, baseline → fixed:

  | viewpoint | baseline | fixed |
  | --- | --- | --- |
  | E1M5 ld 308, X 370 Y 1409 ANG 217 (ceiling crack) | 1536 | 163 |
  | E1M6 ld 1044, X 1304 Y -227 ANG 12 | 102 | 50 |
  | E1M5 X 497 Y 1455 ANG 183 (sector 100 ceiling) | 141 | 72 |
  | E1M2 ld 648, X 1874 Y -562 ANG 195 (the report) | 34 | 31 |
  | eleven unrelated viewpoints across E1M1/E1M2/E1M3/E1M5/E1M6/E1M7 | — | unchanged |

  - **The residue is not seam.** Cropping every survivor showed distant `BROWN144` banding, the
    `COMPUTE` wall texture and sprites — texture detail that trips the same detector. The E1M2
    count barely moves because only 3 of its 34 pixels were ever the seam; the *line itself* is
    gone from the magnified crop, which is what the number cannot show. Always crop before
    believing a count.
  - **Run the A/B with `-nomonsters`.** With monsters alive the two runs are at different
    animation frames, so the frames differ almost everywhere and the count picks up sprite edges.
    That is the whole of a "17 → 19 regression" that vanished (0 → 0) once monsters were off. A
    whole-frame difference bbox is the quick way to catch it: if it covers the screen, the
    comparison is not controlled.
  - The geometry can also be checked without pixels at all, and it is the stronger check: dump
    each seg's `pv` and its subsector polygon's points and measure the worst polygon corner
    against the wall endpoint it belongs to. After the fix that distance is **0.000000** on E1M2
    506 and 648, E1M5 308 and E1M6 1044, with the walls on their linedef to within 3e-5.
  - **Do not measure this with the whole frame in the detector.** The red HUD numerals are bright
    pixels between darker rows and score as slivers, which made the combined fix look *worse* than
    the broken one (529 vs 411) until the region was restricted to the wall.

- **A pointer must be cleared because its target is being freed, never because the subsystem still
  looks active.** `P_SetupLevel` cleared `camera.mo` only `if (camera.chase)`, so switching the
  chase camera off and then loading a level left `camera.mo` pointing at a freed mobj — and the next
  time the camera came on, `P_ResetCamera` wrote through it and `R_SetupFrame` read a `subsector`
  from a level that no longer existed. It crashed the cabinet only *occasionally*, hours into an
  unattended run, because a use-after-free waits for something to reuse the memory. Core dumps were
  already being kept by `systemd-coredump` — `coredumpctl list` and
  `coredumpctl debug <pid> --debugger=gdb --debugger-arguments="-batch -ex bt"` had the answer
  without any new logging. → `attract.md`

- **SDL2 minimizes a fullscreen window when it loses focus, and that is what drops the cabinet to
  the GNOME desktop.** Nothing in this tree asked for it: `SDL_WINDOW_FULLSCREEN` (both window
  paths — `sdl/i_video.c` for software, `sdl/ogl_sdl.c` for OpenGL) is an *exclusive* fullscreen
  request, and SDL iconifies such a window on focus loss so the display mode can be given back.
  With a KVM (Deskflow) that fires every time the pointer crosses to the other machine, which is
  not a request to leave the game. `I_SysInit` (`sdl/i_system.c`) now sets
  `SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS` to `"0"`, once, before any window exists — so it covers
  both window paths and survives a drawmode switch. The window stays mapped and the window manager
  decides the stacking, so Super and alt-tab still work; nothing hides the game on its own.
  - **Safe here only because the cabinet's fullscreen request is the desktop's own resolution**
    (1366x768 in `config.cfg`, and the panel's native mode). Not minimizing means SDL holds the
    mode while unfocused; if the game ever asks for a *smaller* mode, that would strand the
    desktop at the game's resolution when the operator switches away.
  - **Check the SDL you are actually linking before believing a hint fixes anything.** Fedora
    ships `sdl2-compat` over SDL3, not SDL2, so the hint is forwarded to SDL3 and SDL3 gets to
    decide what it means — and SDL3 changed this exact code. Two things had to be shown, not
    assumed:
    - *The hint reaches SDL3.* A five-line program that calls SDL2's `SDL_SetHint` and then reads
      the value back through `dlopen("libSDL3.so.0", RTLD_NOLOAD)` + SDL3's own `SDL_GetHint`,
      in the same process. Both sides reported `0`, so `sdl2-compat` passes it straight through
      (the names are identical in SDL2 and SDL3).
    - *The hint is not a no-op.* This is the part worth the trouble. SDL3's
      `ShouldMinimizeOnFocusLoss` calls `SDL_GetHintBoolean(..., false)` — **default false** — so
      it is easy to conclude SDL3 never minimizes and the fix changes nothing. It is wrong: that
      default only applies **when the hint is set**. Disassembling it (`objdump -d` on
      `libSDL3.so.0`, finding the two references to the hint string in `.rodata`) shows the unset
      path skips the `SDL_GetHintBoolean` call entirely and falls through to a heuristic on
      `window->fullscreen_exclusive` plus a video-device capability bit, which *does* reach the
      minimize call for an exclusive-fullscreen window. Setting the hint to `"0"` is what takes
      that branch out of play.
    - The disassembly beats guessing here because the machine has no SDL3 sources and no
      debuginfo, and the behaviour cannot be exercised headlessly — focus loss needs a real window
      manager, and popping a fullscreen test window over a session someone is using is not
      something to do casually. → `attract.md` for the idle timeout this interacts with (an
      unfocused cabinet still counts as idle; the timeout does not care about focus)

### The E1M1 slime trail is a rounded *node partition*, not a rounded vertex

The classic one, at X 2994 Y -2879 Z -24, ANG 295, sector 49: a thin green stripe running down from
the nukage pool through the brown floor to the bottom of the screen, in software 24bpp.

**`P_Remove_Slime_Trails` neither causes nor cures it.** Proved by A/B in one binary with a
temporary `-noslimefix` switch: the two renders differ elsewhere (bbox `(406,274)-(753,455)`, so the
switch was live) but the stripe is byte-identical in both. Killough's fix snaps *seg vertices*; the
near wall involved here has both vertices as untouched map endpoints, so the fix has no lever on it.

**The mechanism.** Measured at 1024x768:

- The stripe is exactly 4 columns, x=406..409, colour `(23,51,15)` -- NUKAGE3, sector 53, floor -48.
- The nukage plane (`pic=0 h=-48`) has `bot=553` at every neighbouring column and `bot=767` -- the
  bottom of the screen -- at 406..409. Nothing clipped it there.
- The brown floor (`pic=18 h=-24`, sectors 49/56) is drawn by two visplanes, one ending at x=405 and
  one starting at x=410, so 406..409 belong to neither and the nukage behind fills the hole.
- **Line 178, the near lip of the pool (~240 units away), asks `R_ClipPassWallSegment` for
  `[119..409]` but stores only `[378..405]`** -- columns 406..409 had already been marked solid by
  line 265, a one-sided wall **~730 units away**. A far wall clipped a near one.
- Both projections are *correct*: the far wall's corner genuinely lands at x=406 and the near lip
  ends at x=410, a real 4-column overlap the near wall should win. The BSP was simply walked out of
  order -- subsector 145 (nearest seg 694 units) before subsector 156 (nearest seg 239).

**The cause: the NODES lump stores a rounded partition direction.** A partition is recorded as the
seg the builder split on, and that seg's far endpoint is a node-invented vertex rounded to whole map
units. E1M1 node 162 is stored as anchor `(3472,-3520)` direction `(40,-54)`; its linedef 427 is
`(48,-64)` -- the same anchor, rotated about 0.34 degrees. Replaying the traversal offline from the
NODES lump reproduces the engine's order exactly, so the engine is faithful and the *data* is wrong.

At this viewpoint that rotation decides the side test outright:

| partition direction | left | right | side |
| --- | --- | --- | --- |
| stored `(40,-54)` | 25812 | 25640 | **0** |
| true linedef `(48,-64)` | 30592 | 30768 | **1** |

The margin is 172 in ~25700 -- 0.67%. The viewer is 2.5 units from the plane, and 0.34 degrees at
~800 units from the anchor is worth ~4.7 units, so the rounding swamps the real distance.

**The fix: `P_Fix_Node_Partitions` (`p_setup.c`)** recovers each partition's exact direction from
the linedef its partition seg lies on, into new `node_t.rdx/rdy`. `R_PointOnSide_Render`
(`r_main.c`) is the side test against those, used by the BSP walk in `r_bsp.c` (3 sites) and
`hw_main.c` (1).

- **It is render-only, deliberately.** `node_t.x/y/dx/dy` keep exactly what the WAD says, so
  `R_PointInSubsector` and every gameplay use of the nodes are bit-identical and demos are
  unaffected. Do not "simplify" this by writing the corrected vector back into `dx/dy`.
- **It must run before `P_Remove_Slime_Trails`**, which moves the seg vertices it matches partitions
  against.
- It reports `Node partitions: N re-aligned to their linedef.` on every level load (`EMSG_all`, not
  `EMSG_ver` -- `EMSG_ver` is invisible during level setup). Counts are small: E1M1 1, E1M2 3,
  E1M3 2, E1M4 5, E1M5 2, E1M6 0, E1M7 10, E1M8 7, E1M9 0, MAP01 1, MAP15 12, MAP29 24.

**Measured result**, same binary and viewpoint, stripe columns before to after: **4 to 0**. Across
seven viewpoints nudged perpendicular to linedef 191 the stripe went 6,5,5,4,4,3,3 to all 0. Both
renderers are clean at the viewpoint; `make smoke` 5/5.

**This is not what other ports do**, and that is worth knowing before extending it. MBF's answer is
`P_Remove_Slime_Trails` (vertices only). ZDBSP's is extended nodes, which store vertices in fixed
point so the precision is never lost -- DoomLegacy can read those (`p_extnodes.c`) but DOOM.WAD ships
vanilla nodes. ZokumBSP fixes it in the node builder. Correcting the partition at load time is ours.

**Reproducing it.** Use `setpos` (below); the artifact is sensitive to where you stand, so nothing
less than the exact coordinates is a reproduction. Measure it, never eyeball it: scan the floor band
for a column with a long run of pixels where `g > r+18 and g > b+18`. Beware comparing "before" and
"after" from *different* viewpoints -- moving even 16 units reframes the scene enough that a fixed
pixel window silently reports zero, which reads as a fix.

### `setpos` places the camera for headless rendering bugs

`setpos <x> <y> [angle] [z]` (`d_netcmd.c`, `-devmode` only) puts the player at map coordinates so a
screenshot can be taken of a *specific* reported view under `SDL_VIDEODRIVER=offscreen`. It reports
the position it reached, which should be checked against the reporter's coordinate HUD before
believing the shot.

**It must set `localangle[0]` as well as `mo->angle`.** `cmd->angleturn` is absolute in this engine
(`g_game.c`: `cmd->angleturn = localangle[pind] >> 16`), so `P_MovePlayer` rebuilds `mo->angle` from
`localangle` on the very next tic and an angle written straight to the mobj is silently discarded —
the player moves, the view keeps pointing the old way, and the screenshot looks like the wrong
place. `p_telept.c` sets both for the same reason.
