# Gotchas found the hard way

*Part of the DoomLegacy arcade cabinet build. Debugging reference. Worth a look when something behaves impossibly — especially demo desync, a grep that finds nothing, or wrong colours.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

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
