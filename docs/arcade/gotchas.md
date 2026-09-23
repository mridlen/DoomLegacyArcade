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
- **A stale `multiplayer` byte in the demo header desynced a single-player run, and the header is
  the only thing that carries it (fixed).** `G_BeginRecording` runs *before* `G_DeferedInitNew`, so
  the header records whatever the globals held from the **previous** game — the same staleness
  `G_Update_Demo_Header` was written to correct for skill/episode/map. Those were harmless.
  `multiplayer` is not: `G_DoPlayDemo` restores it from header byte 16, and `G_DoLoadLevel` then
  runs `P_SetupLevel` with it, because nothing sets `multiplayer` between `G_InitNew` and the level
  load. So a solo run recorded in a session that had earlier been multiplayer replays with
  `multiplayer = 1` for the whole level: weapons and keys persist on pickup (`p_inter.c` ~602,
  ~1629), kill accounting changes (~2295), and the player-damage gate takes a different branch
  (~3385, the `(! multiplayer)` term).
  - **The symptom is not a crash and not a visible glitch — the replay simply never finishes the
    level.** On the E1M3 ITYTD speed run that found this, playback matched the recording *exactly*
    for 901 tics, then the recorded run took 7 damage the replay did not, and from there the player
    drifted off route, spent the last 600 tics stuck against geometry at (-2068,-1808) riding a lift
    up and down, and never reached the exit. The demo's own last ticcmd is `forwardmove=50,
    buttons=2` — the player running into the exit switch and pressing use — so the recording plainly
    *did* finish.
  - **Fixed by having `G_Update_Demo_Header` rewrite byte 16 too**, alongside the fields it already
    corrects. It is correct by construction: the call sits 11 lines above `G_DoLoadLevel` in
    `G_InitNew` and nothing in between touches `multiplayer`, so the byte written is exactly the
    value `P_SetupLevel` will use.
  - **`playeringame[]` (bytes 17..48) is stale in the same way and must be left alone.** Every demo
    ever recorded has it all-zero and replays correctly, because the recorded add-player netxcmd
    creates the players. Writing the real flags would make `P_SetupLevel` spawn a player body before
    that command runs and change the mobj count — a new desync in place of the old one.
  - **Unlike the smoke-trail phase error, this one is repairable in place**: the correct value is
    known, so patching byte 16 of an affected `.lmp` from 1 to 0 makes it replay exactly. That
    byte-flip is also what *proved* the diagnosis — same binary, one byte, `TRACE_SIMEXIT` at tic
    2149 instead of no exit at all. Only demos recorded after a multiplayer session are affected;
    of the cabinet's ~90 stored demos this was the only one.
  - **How it was found, which is the reusable part:** dump *every* registered cvar (walk
    `CV_IteratorFirst`/`CV_Iterator`), every `EN_` engine flag and the spawn state at the end of
    `P_SetupLevel`, then diff a demo playback against a plain `-warp` game of the same map. Comparing
    engine state at level start is far faster than hunting a divergence tic, because record and
    playback run the same build — anything that differs is gated on `demoplayback` or came out of
    the header.

- **A DoomLegacy demo was replayed with vanilla physics because its version was compared against
  Boom's numbering (fixed).** `G_demo_defaults()` computed `boom_200 = EN_boom && (demoversion >=
  200)` and `boom_201 = ... >= 201`. A **Boom** demo carries demoversion 200..214; a **DoomLegacy**
  demo carries 111..148. They are separate numbering schemes and are not comparable, so those tests
  were never true for a Legacy demo -- every Boom behavior was switched *off* on playback even
  though `EN_boom` (`demoversion >= 129`) was on. Nothing switches them off while **recording**:
  `G_demo_defaults` is playback-only, and recording keeps the baseline near `G_Downgrade` where
  `EN_boom_physics` and friends are simply `EN_boom`. So every Legacy demo replayed under different
  rules than it was recorded under.
  - **Measured**, playback vs a normal game, at the end of `P_SetupLevel`: `EN_boom_physics`,
    `EN_boom_floor`, `EN_doorlight`, `EN_invul_god`, `EN_skull_bounce_fix`, `EN_catch_respawn_0` all
    **0 vs 1**, and `EN_blazing_double_sound`, `EN_vile_revive_bug` **1 vs 0**. `EN_boom_physics` is
    `!comp[comp_model]`, the movement model -- which is what a speed run is made of.
  - **The header itself proves the intent.** It already stores `zerotags=1`, `invul_skymap=1` and
    `doorstuck=2` -- Boom 2.01/2.02 values, faithfully restored on playback. The same header said
    "Boom" in the fields that are written down and "pre-Boom" in the flags that were inferred.
  - **Fixed** by deriving the Boom level from `EN_boom` for a Legacy demo (`legacy_boom`), leaving
    the Boom version test in place for actual Boom demos; and by keying `EN_skull_bounce_fix` /
    `EN_catch_respawn_0` off `EV_legacy >= 147`, which is exactly what their own comment
    ("Vanilla and DoomLegacy < 1.47") always said they should be.
  - **It changes the rules without changing any stored demo.** All **89** demos in the cabinet's
    library were replayed twice, before and after, and the per-tic player trace (position, angle,
    health) compared: **0 changed**, and every single-level demo still exits at its recorded board
    time. Five of the 89 never reach an exit in the harness -- two `+dwango5`, one `+mapsofchaos`
    (the pwad is not loaded in the scratch dir) and two whose runs are longer than the 260s cap --
    but those are identical on both sides over the portion that does run, and the reason is the
    harness, not the change.
  - **The control is what makes that meaningful, and it has to come first.** With the fix in, the
    eight flags read identically in playback and a live game, where before they disagreed -- so the
    change demonstrably took effect. **A "nothing changed" result is worthless without that
    control**: it is indistinguishable from the patch not having compiled in.
  - **A crashed run diffs as a behavior change.** One demo came back `CHANGED -- no longer exits`,
    which reads as a regression; its "after" trace was **empty**, because that was the run the SDL
    audio thread segfaulted under (see below). Re-run cleanly it was identical. Treat a missing or
    truncated trace as a *missing measurement* to re-run, never as a result -- the mirror of the
    `-playdemo` trap where two runs that both failed to load compare 100% identical.
  - This is **upstream** code, unchanged since the r1749 import -- worth reporting on rather than
    carrying forever as a local patch.

- **`cv_fragsweaponfalling` is forced to 0 on playback and left at the user's value while
  recording (still open).** `G_demo_defaults()` zeroes it (correctly, for stock IWAD demos) but
  nothing pins it while recording and it is not in the header. It only matters when a **player**
  dies -- for a monster drop `drop_ammo_count` is already 0 -- so it is harmless for a clean run
  and a live hazard for the cabinet's **death demos**. The fix is to carry it in the header's spare
  option area, where there is still room.

- **The audio thread killed the process: the mixer read a sound channel the game thread was part
  way through writing (fixed twice — the first fix held on x86 and not on the Pi).**
  `I_StartSound` (`sdl/i_sound.c`) fills in a slot of `mix_channel[]` on the game thread while
  `I_UpdateSound_sdl` mixes the same table on SDL's audio thread. The mixer decides a channel is
  playable from `data_ptr` alone and then dereferences `leftvol_lookup`/`rightvol_lookup`, which are
  **NULL until a slot's first use** (`mix_channel[]` is `static`). A mixer pass that saw one without
  the other read `NULL[sample]` and took the whole process down.
  - **It looks random, and it is not.** `vol_lookup` is a static array that is never freed, so after
    a slot's first use a stale lookup is merely stale. Only the **first** use of each of the 16
    channels can crash, all in the first burst of sound after a level starts.
  - **There was no lock at all.** The `SDL_LockAudio`/`SDL_UnlockAudio` pairs were
    `#ifndef HAVE_MIXER`, and the cabinet builds with `HAVE_MIXER=1`. (The non-mixer build's pair
    also returned from the `SFX_single` branch with the lock held.)
  - **First fix — ordering the stores.** `data_ptr` published last, behind a compiler barrier.
    Proved on x86 by widening the window (`SDL_Delay(20)` between the stores: 6 of 6 crashing in the
    old order, 0 of 6 reordered), and it held there.
  - **It did not hold on the Raspberry Pi 3** (aarch64, GCC 14). Found by the Cabinet Link Phase 0
    runs: `doomu-sl_E1M1_sk2_speed.lmp` under `tools/demotest.sh` crashed 2 of ~120 replays, only
    while several replays ran at once, same backtrace, one core with `leftvol_lookup` read as NULL and
    one with `rightvol_lookup`, on channels whose memory afterwards held valid pointers. The Pi
    binary's disassembly had the lookups stored before `data_ptr` and read after the `data_ptr`
    test, so the ordering was in force and still not enough; the exact interleaving was never
    pinned down. **Ordering the writes of shared state is not a substitute for a lock**, and "the
    disassembly looks right" did not make it one.
  - **Second fix — a real lock, and the mixer works from a copy.** `mix_lock` (an `SDL_mutex`, so
    SDL 1.2 still builds) is held by `I_StartSound`, `I_UpdateSoundParams` and `I_StopSound` while
    they touch a channel. The mixer takes it only to `memcpy` the 16-channel table (1KB), mixes the
    copy with no lock held, then retakes it to write back how far each sound got — and only to a slot
    whose `data_ptr` *and* `handle` still match the copy, since every start gives a slot a new
    handle and a stop clears `data_ptr`. The game thread therefore waits at most for a 1KB copy,
    never for a buffer to be mixed. A volume change arriving mid-pass is heard one buffer (~46ms)
    later, and sound starts are not delayed: the pass fills a whole buffer in well under a
    millisecond, so a sound started during it already landed in the next buffer.
  - **Proved by widening the window again, against the lock this time**: data_ptr published *before*
    the lookups with `SDL_Delay(20)` in between (the bug put back on purpose), and the lock switched
    off by an environment variable in the same binary: **6 of 6 segfaulted** in
    `I_UpdateSound_sdl`; with the lock on, **0 of 6**. That shows the lock excludes the half-written
    channel whatever order the stores come in.
  - **Then on the Pi, unmodified, against the natural crash**: the full `tools/demotest.sh` suite
    (102 demos, 4 at a time), alternating the old and the fixed binary, four rounds each. **Old: 3
    segfaults in 408 replays, all in `I_UpdateSound_sdl` at the same line; fixed: 0 in 408.** The
    load matters and so does the variety: 180 replays of the one demo that had crashed, one at a time
    or four copies at once, produced **no** crash from the *old* binary — a loop that cannot fail
    says nothing about a fix, so it was not counted. The laptop suite matches main except one Doom 2
    demo that is unstable on the unfixed binary too (broken since the IWAD became v1.9), and
    `make smoke` passes.
  - Read a crash like this with
    `coredumpctl debug <pid> --debugger=gdb --debugger-arguments="-batch -ex bt"`; the giveaway is a
    backtrace whose only frames are SDL's audio thread, with the game thread nowhere in it. It
    happens under `SDL_AUDIODRIVER=dummy` too — the dummy driver still runs the callback. On the
    Pi, gdb can read a core against a *copy* of the binary if the original was in a deleted scratch
    directory (`coredumpctl dump <pid> -o core`, then `gdb <copy> core`).

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
    from the transparent padding instead and would need the edge texels replicated. Do not
    confuse the two failures — **and the second one did turn up**, see below.
  - **The padded case: a dark line on the right and bottom only, reading as "drawn a pixel up and
    to the left".** Reported on the Ultimate Doom episode 2 intermission (the Tower of Babel) and
    then episode 3, after the `GL_CLAMP_TO_EDGE` fix had cleared episode 1. The reason episode 1
    was fixed and 2 and 3 were not is only the patch sizes: every `WIA0xxxx` frame is 8x8 or 8x16
    (one is 24x8), so it fills its block, while `WIA10000` is 56x40 in a 64x64 block and the
    `WIA2xxxx` frames are 112x32, 232x24, 32x56 and so on. The quad samples up to `max_s`/`max_t`,
    so the last column and row blend with the padding, which `Make_Mip_Block` fills with
    transparent black. `HWR_MakePatch` now fills the **whole** padding with copies of the last
    column and row — the whole of it, not one texel, because trilinear filtering (the cabinet's
    setting) reads smaller mip levels where the padding is averaged further in. Patch sizes come
    straight out of the wad's patch headers, which is the quickest way to tell which case a
    fringe is. It also affected the `STCFN` HUD font, whose glyphs are rarely a power of two.
    Verified the same way, crops of the E2M7 and E3M1 intermissions before and after.
  - **Do not "fix" this by forcing nearest filtering.** Note the cabinet's `gr_filtermode` is
    already set to `"Nearest"` and the render is plainly bilinear anyway, so that setting is not
    doing what it says — which is its own bug, and was not what produced the fringe.
  - Verified by screenshot, headless, on the real GPU: capture the intermission with
    `SDL_VIDEODRIVER=offscreen` and an autoexec of `wait`/`exitlevel`/`screenshot`, then compare
    crops before and after. See `CLAUDE.md` for the offscreen recipe. **This is the class of bug
    that only a picture settles** — it is invisible to logs and to every non-graphical check.

- **The grey rim round every world sprite, Bilinear and Trilinear only, was the blend mode, not the
  texture.** Texels a sprite does not cover are `(0,0,0,0)` (`Make_Mip_Block`). At the silhouette
  the filter returns `colour*a` with alpha `a`, and `gluBuild2DMipmaps` averages the same way at
  every level — which is exactly a **premultiplied** texture, because every texel is either fully
  opaque or clear black. `HWR_DrawSprite` drew it with `PF_Translucent` (`SRC_ALPHA,
  ONE_MINUS_SRC_ALPHA`), multiplying by alpha a second time: `colour*a*a + dest*(1-a)`, a dark rim
  that trilinear widens at distance. Ordinary sprites now draw with `PF_Environment` (`ONE,
  ONE_MINUS_SRC_ALPHA`), which is what the weapons already used, and the rim is gone at every mip
  level with no change to the texture at all.
  - **Why the colour-bleed fix (#46, reverted in #47) put a white border on the weapons.** It
    filled the clear texels with the neighbouring sprite colour — the right fix for straight alpha,
    and the wrong one for anything drawn with `PF_Environment`, which does not multiply by alpha:
    a half-covered edge became `colour + dest*(1-a)`, brighter than either. The weapons and the 2D
    patches are `PF_Environment`. The revert blamed the 2-byte chromakey format, but OpenGL forces
    `patchformat = GR_RGBA` (`hw_main.c`), so that path never ran. **Keep the textures as they are;
    they are correct for the premultiplied blend.**
  - **Not with a coloured fog.** GL fog mixes the fog colour into the fragment before blending,
    unscaled by alpha, so a premultiplied edge would gain a fog-coloured rim — the reason upstream
    left sprites on `PF_Translucent` ("we need to fix the issue with the fog before").
    `sprite_premultiplied` is cleared by `HWR_FoggingOn` unless `gr_fogcolor` is black, since black
    fog only scales the colour. The cabinet runs `gr_fog "Off"`.
  - Translucent, shadow and smoke sprites keep `PF_Translucent`: they carry a partial vertex alpha
    (and `fx1` sprites half-alpha texels), which the premultiplied blend would not scale.
  - Verified on the real GPU: MAP01, fire once and wait 150 tics so the zombiemen come down the
    steps; the before/after differ in 854 pixels, all on the sprites, every one lighter.

- **The horizontal line across the pistol while firing was lighting, not filtering.** Two
  attempts at it (#46, and the premultiplied blend above) treated it as an edge-texel fringe; the
  reporter's observation that it was far worse in a dark room was the clue. `PISFA0`, the muzzle
  flash, carries its own copy of the top of the gun and is fullbright; its bottom row is a dead
  straight 24-texel cut lying *across* the gun (lay `PISGB0` and `PISFA0` over each other by their
  offsets to see it). Below that cut is the real gun, lit by the sector.
  - **The OpenGL weapon was lit far darker than the software one.** Software gives a psprite the
    *nearest* entry of the distance light table (`scalelight[..][MAXLIGHTSCALE-1]`,
    `R_DrawPlayerSprites`) with the flash's `extralight` added to the level first. GL used
    `LightLevelToLum`, the sector curve for walls, which is flat and near zero at the dark end.
    E1M8's opening room is light 96 and firing adds 16: GL lit the gun at lum **84** (33%),
    software at colormap 9, which is **183** (72%). The hand measured **37** in GL against **88** in
    software, and the step at the flash's edge was **67 → 26** against software's **62 → 57**.
  - `HWR_DrawPlayerSprites` now reproduces the software choice: the same `startmap - 47/DISTMAP`
    level, turned into a lum as `(32 - level) / 32`. That is not a guess: measured over `PLAYPAL`,
    COLORMAP *n* is `(32-n)/32` as bright to within 2% at every level. After: hand **82** (software
    88), step **67 → 57** (software 62 → 57). A bright room is unchanged (level 0 either way); a
    medium one gets a brighter weapon, which is what software always drew.
  - **Why it looked filter-only.** Nearest has the same brightness step, measured. But in Nearest
    every edge of the gun is a hard pixel edge, while with Bilinear/Trilinear the gun is smooth
    everywhere *except* here — the flash quad ends on that row, so its bottom edge stays hard, and
    one hard line in a soft picture is what the eye catches.
  - **How it was captured headlessly**: a temporary block at the end of `P_MovePsprites` that, once
    the pistol reaches `S_PISTOL`, sets the weapon to `S_PISTOL2` and the flash to `S_PISTOLFLASH`,
    both with `tics = -1`, and `extralight = LIGHT_UNIT` (not 1 — `A_Light1` is in light units, and 1 gave a first set of
    numbers that were wrong for software and GL alike).
    Then `-warp 1 8 -nomonsters`, an autoexec `wait 140` / `screenshot`, under Xvfb at 1366x768 with
    `localplayers "1"` in the scratch config. Compare row profiles through the gun, not the picture
    alone.
  - **That capture found a crash: an OpenGL screenshot at 1366 wide aborted the game.**
    `ReadRect` (`r_opengl.c`) read with the default `GL_PACK_ALIGNMENT` of 4, so a width whose
    3-byte row is not a multiple of 4 padded every row and overran the buffer (`free()` aborts).
    The cabinet runs 1366x768. It now packs at 1, under `glPushClientAttrib` like
    `ReadScreenRect`.
  - **The remaining step is in the original art, and Weapon Flash Fix removes it.** After the
    lighting fix the reporter noticed the same faint line in software and in dsda-doom: any faithful
    renderer draws a fullbright copy of the gun over a sector-lit one. `weaponflashfix`
    (`cv_weapon_flash_fix`, `screen.c`; *Effects Options → Next*, default On) draws the gun
    fullbright while the flash psprite is active, via `R_Weapon_Flash_Lit` (`r_things.c`), called
    from `R_DrawPSprite` and `HWR_DrawPSprite`. Measured on the same frozen frame, the row just
    below the flash's edge: software 61 → 55 off, 61 → 77 on; GL 67 → 57 off, 67 → 79 on — the gun
    below is now as bright as the flash's copy of it, or brighter where its art is lighter.
    - **Keyed on the flash psprite, not a weapon list.** Laying each flash over its gun frame in
      DOOM2.WAD, `SHT2`, `CHGF`, `MISF`, `BFGF` overlap their guns as `PISF` does, and a DEHACKED
      or MBF21 weapon gets it for free; the fist and chainsaw never raise a flash.
    - **Invisibility differs by renderer, on purpose.** Software's `MF_SHADOW` branch runs first
      and draws the gun as pure translucency with no light at all — like its flash — so there is
      nothing to match. OpenGL draws an invisible gun translucent *and* lit, beside a translucent
      fullbright flash, so the fix applies there. `fixedcolormap` (invulnerability) comes first in
      software and forces 255 in GL anyway.
    - Draw-only: not a netvar, not in the demo header, no `P_Random`. A 3D-floor sector's light
      list (`viewer_sector->numlights`) overwrites the psprite colormap in software afterwards, so
      there — as for the stock `FF_FULLBRIGHT` flash itself — it has no effect; pre-existing.

- **Sprite edges that touch the patch's bounding box stayed hard and flat after the rim was fixed**
  — the top of the imp's and sergeant's heads, the marine's helmet. Sprites are cut tight to their
  art and `HWR_MakePatch` puts the patch at texel (0,0), so a head that reaches the top row sits on
  the texture's edge: `GL_CLAMP_TO_EDGE` samples the edge texel beyond it, and the padding fill
  copies the last column and row outward on purpose. Either way the filter has nothing transparent
  to fade into and the silhouette stops in a straight line where the quad ends, one texel thick —
  several screen pixels once a sprite is close.
  - **World sprites now get their own copy of the patch with one clear texel all round**
    (`HWR_GetSpritePatch`, `TF_SpriteCopy`/`TF_SpriteMargin`), and `HWR_DrawSprite` widens the quad
    by exactly one texel each way, so the art lands on the same pixels and the extra texel is the
    fade. Clear *black* is right here because sprites draw premultiplied (above).
  - **A copy, not a change to the patch.** One cache entry per lump serves every drawer, and the
    same lump can be 2D art too (Heretic's inventory icons are sprites). 2D must keep hard edges:
    a full-screen picture that faded at its border is the intermission's dark line all over again.
    The copy lives in the colormap chain so it is purged and freed with the colormap copies;
    `HWR_GetMappedPatch` skips it. `max_s`/`max_t` on the `MipPatch_t` stay the 2D layout;
    the sprite copy's span is worked out from its block size in `HWR_DrawSprite`.
  - No margin when `gr_rounddown` is on or the patch would pass 2048 texels — the art would be
    scaled into the block — and `TF_SpriteMargin` records whether it was really applied.
  - `HWR_DrawFuzzSprite` takes the quad's texel rows (patch height plus the margin) instead of the
    patch, so the spectre's band offsets stay one texel.
  - Seen at a distance it barely registers (147 pixels changed in the 90-degree MAP01 shot): a
    shrunk sprite's texel is under a pixel. Verified close up by narrowing `gr_fov` to 40 on the same
    frame, which magnifies the zombieman like walking up to him: the helmet's flat top becomes
    rounded, nothing else moves. Console `+forward` does not move the player in an autoexec, so
    `gr_fov` is the way to get a close-up headlessly.
  - **Text, menu art and the HUD had both problems, and now get both fixes.** `HWR_DrawPatch` and
    `HWR_DrawMappedPatch` drew with `PF_Translucent` (the dark rim) from a patch at texel (0,0) (the
    hard box edge). Magnified by the 320x200 upscale it read as every word sitting in a dark
    rectangle — the intermission's `KILLS`, the HUD counters. They now draw a **2D margin copy**
    (`HWR_GetMarginPatch(..., TF_2DCopy)`) premultiplied (`PF_Environment`), through
    `HWR_Draw_Margin_Quad`; a translucent HUD sets the flat colour's RGB to its alpha as well, which
    is what premultiplied translucency needs.
  - **A picture whose whole border is opaque keeps the old layout** (`HWR_Art_Border_Solid`,
    checked once the art is drawn into the block). Those are the pictures meant to meet the screen
    edge or the piece beside them — title screen, status bar, intermission map, view-border tiles,
    the scrolling bunny — where a faded edge is a dark frame or a seam. A glyph whose top touches
    its box still has holes in the rest of its border, so it gets the margin. Verified: the title
    screen is byte-identical before and after, and on the Doom 2 intermission every changed pixel is
    on or beside text (the empty background blocks have none).
  - **Font glyphs are exempt from that rule, by lump name** (`HWR_Is_Font_Glyph`). `I`, `-`, `.`,
    `_`, `=`, `!`, `H` and `WIMINUS` are solid right out to their box, so the border test took them
    for pictures and left them as hard squares among soft letters. Size cannot separate them from
    the pieces that must stay hard: `.` is 4x3 and the view border's corner is 3x3, and the menu
    slider and save-slot pieces and Ultimate Doom's 8x8 intermission frames are the same scale. The
    prefixes are the engine's own font lumps (`STCFN`, `FONTA`/`FONTB`, `WINUM`/`WIMINUS`/`WIPCNT`/
    `WICOLON`, `STTNUM`/`STTMINUS`/`STTPRCNT`, `STYSNUM`, `STGNUM`, `SMALLIN`), which a PWAD font
    replaces under the same names. Every solid-bordered 2D lump in DOOM.WAD, DOOM2.WAD and
    `legacy.wad` was listed to pick this.
  - **Each copy carries its own span now** (`Mipmap_t.max_s/max_t`). `MipPatch_t.max_s/max_t`
    describe the base copy, which the weapon and the splats still use; a margin copy's block can be
    a larger power of two than the base one, so computing its span from the patch was only right
    by luck. Changing `Mipmap_t` changes a header: `make clean`.
  - `HWR_DrawPic` (Heretic raw pics and the `pic_t` formats) is untouched: an intensity-alpha pic is
    not premultiplied, so the old blend is right for it.


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

- **A black seam down the holes of see-through bars is a texture that was never marked
  transparent, not a geometry gap.** Reported on E1M1 where linedefs 297 and 299 meet (vertex 248,
  the bars round the nukage pool): a two-pixel near-black line through every hole, exactly on the
  vertex, OpenGL only; plus a one-pixel sliver "sometimes" in software. Three separate bugs.
  - **`HWR_GenerateTexture` scanned a quarter of the texture for holes.** `blocksize` counts pixels
    and the loop steps `i` through bytes (`for (i=3; i<blocksize; i+=4)`), so it only ever saw the
    top quarter. A texture whose holes all start lower was never given `TF_TRANSPARENT`: in the
    stock IWADs `BRNBIGC/L/R`, `MIDBRONZ`, `SKINEDGE`, `SKINTEK1`, `ZZZFACE3`. Their siblings
    `BRNSMAL*`, `MIDGRATE`, `MIDBARS*` were found, which is why only some bars misbehaved.
  - **Why an unmarked texture goes black.** It is drawn in the front-to-back pass with `PF_Masked`,
    whose blend is `(GL_SRC_ALPHA, GL_ZERO)`: any texel with 0 < alpha < 1 *erases what is behind
    it* and leaves `alpha * colour`. Wall textures are `GL_REPEAT`, so linear filtering at u=0 of
    `BRNBIGR` mixes its column 0 (a hole there) with its column 31 (solid) — half-alpha texels the
    whole height of the hole, hence a black line exactly at the join. The same blend gives every
    hole a dark rim. Marked `TF_TRANSPARENT`, the wall goes to the sorted pass with
    `PF_Environment` `(ONE, ONE_MINUS_SRC_ALPHA)`, which blends the fringe with the scene already
    drawn. Seam pixels 4/0 against 21–55 neighbours before, indistinguishable after.
  - **How it was pinned down, since every obvious theory was wrong.** Nearest filtering removed it,
    and so did forcing `GL_CLAMP_TO_EDGE` (which also smears every tiled wall, so it is not a fix):
    so, filtering across the wrap. But a 50/50 of brown and a hole cannot come out black under the
    `PF_Environment` blend, and drawing the late walls without depth writes changed nothing. Dumping
    the RGBA the driver receives showed the texture was perfect and its flags `0x13` — no `0x40`.
    **When the arithmetic says a pixel cannot be that colour, check which path drew it.**
  - **`TF_TRANSPARENT` also switched off clipping to the opening (`clip_disable`), and that had to
    go too.** The unclipped part lies in the plane of the upper or lower wall beside it and is drawn
    over that wall. Already visible before this work — E4M3's start cage hung its bottom riveted
    band over the wooden step — on **118 line sides** in DOOM.WAD and DOOM2.WAD, and fixing the scan
    alone would have added E1M9's bars (ld 278–281, 32 units over the `BROWN96` strip) and MAP13
    ld 879. Software clips every masked mid texture to the opening; GL now does too. Count candidate
    lines from the WAD (two-sided, sectors differ, texture taller than the opening, then apply
    `HWR_StoreWallRange`'s pegging) rather than hunting for them by eye.
  - **The software sliver is the classic end-of-seg column.** The pixel at a seg's end can compute
    texture column −1 (or one past the end), which the drawer masks round to the texture's far
    edge — invisible on solid walls, a solid column in a hole on bars. `R_RenderSegLoop` now clamps
    the stored `maskedtexturecol` to the columns the seg covers (`rw_maskcol_min/max`, `R_TLS` like
    the rest of the per-seg state; seg length from `P_SegLength`, since `seg->length` exists only
    under `HWRENDER`). Over eight viewpoints the only pixels that changed were five single columns,
    each going from ~300 pixels unlike both neighbours to under 30; a same-binary control run showed
    the other differences were view bob and flickering lights.
  - Verified headlessly on the real GPU with `setpos` and `screenshot` (`SDL_VIDEODRIVER=offscreen`,
    `-nomonsters`), E1M1 from three angles, E4M3 and E1M9 against software. Renderer only: 121
    demos, 0 desynced, against a baseline recorded from the unmodified commit.

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

### The E1M1 slime trail is a rounded *node partition* -- and re-aligning it does NOT work

The classic one, at X 2994 Y -2879 Z -24, ANG 295, sector 49: a green stripe running down from the
nukage pool through the brown floor to the bottom of the screen, in software 24bpp.

**`P_Remove_Slime_Trails` neither causes nor cures it.** Proved by A/B in one binary with a
temporary `-noslimefix` switch: the renders differ elsewhere (bbox `(406,274)-(753,455)`, so the
switch was live) but the stripe is byte-identical. Killough's fix snaps *seg vertices*; the near
wall here has both vertices as untouched map endpoints, so the fix has no lever on it.

**The mechanism.** Measured at 1024x768: the stripe is 4 columns, x=406..409, colour `(23,51,15)`
(NUKAGE3, sector 53, floor -48). The nukage plane has `bot=553` at neighbouring columns and
`bot=767` at 406..409, so nothing clipped it. Line 178, the near lip of the pool (~240 units), asks
`R_ClipPassWallSegment` for `[119..409]` but stores only `[378..405]` -- columns 406..409 were
already marked solid by line 265, a wall **~730 units away**. Both projections are correct; the BSP
was simply walked out of order, subsector 145 (694 units) before subsector 156 (239).

**The data is at fault.** A partition is recorded as the seg the builder split on, and that seg's
far endpoint is a node-invented vertex rounded to whole map units. E1M1 node 162 is stored as anchor
`(3472,-3520)` direction `(40,-54)`; its linedef 427 is `(48,-64)` -- rotated about 0.34 degrees.
Replaying the traversal offline from the NODES lump reproduces the engine's order exactly, so the
engine is faithful. At that viewpoint the rotation decides the side test outright: stored gives
side 0 by a margin of 172 in ~25700 (0.67%), the linedef gives side 1.

**The obvious fix does not hold up. Do not re-attempt it without reading this.** Re-aligning each
partition to its linedef (`node_t.rdx/rdy` plus an `R_PointOnSide_Render`, render-only so demos are
safe) *does* clear the stripe: 4 columns to 0, and 0 across seven nudged viewpoints. But it is a
reshuffle, not a fix. At X 3031 Y -2925 ANG 305 -- 60 units away, found within minutes of play -- it
**introduces** a one-pixel full-height hairline at x=498 that is absent without it:

- line 196 draws `[459..497]` and line 197 should draw `[498..591]`, but with the re-aligned
  partition line 265 (SOLID, `[459..498]`) is stored *first* and clips 197 to start at 499. Column
  498 is left with no window seg, the far wall is drawn full height there, and it reads as a dark
  hairline from ceiling to floor.
- Line 197 is the *nearer* wall (632 units versus 672), so this is a second ordering inversion --
  the re-aligned partition causes it. Node 162 is the decider at both viewpoints, and the viewer is
  0.21 units from the stored plane there against 4.2 units from the linedef, so the "corrected"
  answer is the confident one and still renders worse.

**Why it cannot work.** The node builder assigned segs to `child[0]`/`child[1]` using its own exact
partition, and the lump stores a rounded copy. Testing the viewpoint against a *different* plane
than the one the tree was partitioned with breaks the invariant the traversal depends on: geometry
near the plane can sit on the side the test does not put it on. Whether that helps or hurts is
luck, per viewpoint. Reverted in the commit after the one that added it.

**What actually solves this class** is better node data, which is why every other project attacks it
there: MBF does vertices only (`P_Remove_Slime_Trails`); ZDBSP stores vertices in fixed point as
extended nodes, which DoomLegacy can already read (`p_extnodes.c`); ZokumBSP fixes the builder
itself (`s=a`, "avoids seg splits that lead to slime trails"). The route with real mileage for the
cabinet is to rebuild the IWAD maps' nodes offline and load those, not to patch the side test.

**Reproducing it.** Use `setpos` (below); the artifact is sensitive to where you stand, so nothing
less than the exact coordinates is a reproduction. Measure it, never eyeball it -- and use two
detectors, because they find different things: a floor-band scan for a long run of
`g > r+18 and g > b+18` finds the green stripe, and a full-height scan for a column differing from
both neighbours (`+/-2` columns) over a long run finds the hairline. The first would have reported
the hairline viewpoint as clean. Beware comparing "before" and "after" from *different* viewpoints
-- moving even 16 units reframes the scene enough that a fixed pixel window silently reports zero,
which reads as a fix.

### Rebuilt nodes are the fix -- for *rendering only*

ZDBSP is vendored into `svn1749/src/nodebuild/` and run at level load, so the WAD's rounded node
partitions never reach the renderer. It keeps its vertices and partitions in fixed point, so the
rounding that causes the whole slime-trail family cannot happen.

**The rebuilt tree must never be the one the simulation walks.** It was, in the first version of
this, and it desynced a record demo. `p_sight.c` walks the BSP for monster line-of-sight
(`P_CrossBSPNode` -> `P_CrossSubsector`), and `R_PointInSubsector` is used by ten gameplay files;
ZDBSP's tree has different segs -- 785 against the WAD's 747 on E1M1 -- so a sight check or a
subsector assignment can land differently. Rarely, but it only has to happen once.

So `P_Rebuild_Nodes` publishes into `rbsp_*` (`r_state.h`), and `R_Use_Render_BSP` /
`R_Use_Play_BSP` swap the globals for the duration of a frame. The swap wraps
`R_RenderPlayerView`, `HWR_RenderPlayerView` and `HWR_SetupLevel` (whose plane polygons are render
geometry and must be cut from the tree the renderer walks). Everything else -- `P_GroupLines`,
`P_Remove_Slime_Trails`, the whole simulation -- sees the WAD's own tree, so gameplay is identical
to stock **by construction**, not by luck.

#### ...except that the renderers ran simulation code inside the swap

"By construction" was not quite true, and stayed untrue for a while. Both renderers service the
network while they draw, so a slow frame does not stall the client/server tick -- and every one of
those calls sits *inside* the swap window:

| | calls | how |
| --- | --- | --- |
| `R_RenderPlayerView` (`r_main.c`) | 4 | via `R_NetUpdate_Main` |
| `HWR_RenderPlayerView` (`hw_main.c`) | 3 | bare `NetUpdate()`, no guard at all |

`NetUpdate` runs `D_Process_Events` -- the menu, console and game responders. That is simulation
code, running with the rebuilt tree in the globals, in flat contradiction of the rule above. The
hardware path is the one the cabinet runs.

**Measured** on the GL path (`SDL_VIDEODRIVER=offscreen`, one 454-frame demo), with a counter on
the swap flag at each call:

    1362 of 1362 in-frame NetUpdate calls had the rebuilt tree swapped in

All of them, about 105 times a second. Nothing had gone visibly wrong -- a responder has to actually
reach `R_PointInSubsector` or `p_sight.c` for it to matter, and menus mostly do not -- but the
window was wide open every frame the machine was on.

The fix is `R_NetUpdate_In_Frame()` (`p_setup.c`): put the play tree back, call `NetUpdate`, swap
the render tree in again. Use it instead of `NetUpdate` anywhere between `R_Use_Render_BSP` and
`R_Use_Play_BSP`.

**Not** by skipping the call. Those `NetUpdate`s carry tic timing, and a frame must make exactly the
ones it always made -- one fewer is as much a gameplay change as one more, and would reject every
record demo on the cabinet just as surely.

The other half was `P_SetupLevel`'s own defensive `R_Use_Play_BSP()`, which sat **after** every node
loader had already written the new level's `nodes`/`segs`/`subsectors`/`vertexes`. Had the swap ever
still been in force on entry, that call would have pasted the *previous* level's saved pointers --
freed `PU_LEVEL` memory by then -- over the level just loaded. A guard that corrupts the thing it
guards is worse than no guard. It now runs at the top of the function, before the loaders, where
"put the globals back" and "the globals describe the old level" are still the same statement.

**A caution about how this was verified.** `make demotest` stayed green across all 94 demos, which
says the change altered no gameplay -- but it does **not** say the fixed path was exercised. The
demo harness runs with `-nodraw`, and under `SDL_VIDEODRIVER=dummy` the player-view render is never
reached at all: `D_Display` was entered 457 times in that same run and `R_RenderPlayerView` zero.
The renderer is only exercised headlessly under `SDL_VIDEODRIVER=offscreen`, which is what produced
the 1362 above and what `make smoke`'s `opengl` check uses. Green demos plus green smoke is the
right pair here; neither alone would have covered it.

**Measured**, same binary, on the E1M6 ITYTD speed record demo to tic 3900, 112 samples of the
player's position, angle and health:

| | vs stock nodes |
| --- | --- |
| rebuilt tree used by gameplay too (the old design) | **diverges at tic 2555** |
| render-only split | **identical, 112/112** |

And the artifacts stay fixed: the stripe is 0 columns, the hairline absent, E1M1 wall/flat corners
pulled 20 -> 0, E1M7 103 T-joins -> 17, "still not flush" 0 throughout.

**Testing demos: `-playdemo` takes an external *file*, and silently proves nothing if you get it
wrong.** This wasted a whole round of "verification" and let the desync ship. The code says it
outright -- *"it is NOT possible to play an internal demo using -playdemo"* (`d_main.c`) -- so
`-playdemo demo1` cannot work for DOOM.WAD's built-in demos; use the console `playdemo demo1`,
which goes through `W_CheckNumForName`. A record demo needs its path:
`-playdemo legacyhome/demos/doomu-sl_E1M6_sk0_speed` (the `.lmp` is appended). Get either wrong and
the engine prints `ERROR: couldn't open lump/file` and sits on the console screen -- **and two runs
that both failed compare 100% identical**, which reads exactly like a clean pass. Always confirm
the demo actually started before believing a comparison, and prefer diffing simulation state over
pixels: a temporary `fprintf(stderr, ...)` of `leveltime`, the player's x/y/z, angle and health from
`P_PlayerThink` is what finally settled this. `GenPrintf(EMSG_warn, ...)` does *not* reach stdout
from inside the game loop, so it is useless for this.

**Other things worth knowing.**

- **It is a command-line switch, not a cvar** (`-nonodebuild`), because nodes feed
  `R_PointInSubsector` and a gameplay-affecting cvar would have to go into the demo header or
  `G_demo_defaults()`. With the split that reasoning is weaker, but the switch is also what makes
  the A/B above possible, so it stays.
- **The tree had never compiled C++.** `r_d3d/*.cpp` has a dep rule but is never built. The
  Makefile now has `CXX`, `CXXFLAGS` (`CFLAGS` minus the C standard, which g++ rejects), a
  `$(O)/%.o: $(SD)nodebuild/%.cpp` rule and `-lstdc++`. `-fno-strict-aliasing` matters: the builder
  type-puns.
- **`-DDISABLE_SSE` is not a performance oversight.** ZDBSP picks an SSE classifier at runtime
  through a selector that lives in its command-line `main.cpp`, which is not vendored; the scalar
  path is portable -- including to ARM -- and the cost is invisible next to a level load (MAP15
  2798 -> 2842 ms).
- **Local modifications to the vendored source are marked `[Arcade]`** -- only the progress bar,
  behind `NB_QUIET`. `main.cpp` globals it needs (`MaxSegs`, `SplitCost`, `AAPreference`,
  `PointToAngle`, `Warn`) and three `FLevel` members are reimplemented in `nb_build.cpp` rather than
  vendoring `processor.cpp`, which is all wad file I/O.
- **Nothing derived from an IWAD is written to disk**, which is why this is done in the engine
  rather than by shipping a re-noded PWAD: a rebuilt map carries id Software's map data verbatim.
  ZDBSP is GPLv2-or-later (Randy Heit); `COPYING.zdbsp` travels with the source.
- **`P_Rebuild_Nodes` validates before it commits.** The builder de-duplicates vertices and drops
  any a linedef does not use, so its vertex array is a different set in a different order -- it
  hands back each line's remapped v1/v2. The checks confirm every index is in range and that no
  linedef endpoint moved. Any failure logs and keeps the wad's nodes.

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

### Reading a crash log: the level names itself now

`P_SetupLevel` prints one line per level load, before any of the work that might crash:

```
Level: E1M7  skill 4  play  chasecam off  views 1
Level: E1M1  skill 1  demo E1M1  ITYTD  SPEED  1:11.05  AAA  chasecam on  views 1
```

It carries the five things a backtrace cannot recover: the map, the skill, whether a demo is
driving it (and *which* record, via `HS_DemoLabel`), whether the chase camera is on, and how many
views are being drawn -- which is what selects the threading mode. It goes out through
`GenPrintf(EMSG_all, ...)`, so it reaches the **terminal**; a `CONS_Printf` would only reach the
in-game console, which is exactly the wrong place for something you read after the process is gone.

**It is printed as soon as `level_mapname` is known, not at the end of setup**, so a crash *during*
level setup still has the level named above it.

**Why it exists.** A cabinet crash log used to be a column of these and nothing else:

```
Nodes rebuilt for rendering: 1433 segs, 482 subsectors, 481 nodes.
Segmentation fault
```

Placing that crash meant loading every map of the IWAD headlessly and building a table of seg
counts to match against. It works -- the counts are deterministic for a given map, and identical on
ARM and x86 -- but it is a measuring run per IWAD before the investigation can even start. The table
for Ultimate Doom episode 1, since it has been measured once:

| segs | level | segs | level | segs | level |
| --- | --- | --- | --- | --- | --- |
| 785 | E1M1 | 1221 | E1M4 | 1433 | E1M7 |
| 1525 | E1M2 | 1184 | E1M5 | 634 | E1M8 |
| 1495 | E1M3 | 1909 | E1M6 | 1023 | E1M9 |

**A repeated sequence in that column is a level progression, and the shape of it is evidence.** The
crash it was built for read E1M1..E1M8 complete, then E1M1..E1M7 and back to E1M1, then E1M1..E1M7
and dead -- three runs, the last two both ending at E1M7. That narrows a "random" segfault to one
level and one transition before anyone has looked at a backtrace.

### Getting a backtrace on the Pi is not the same as on the cabinet

The cabinet is Fedora, where `systemd-coredump` is installed by default and has quietly kept every
crash -- that is how the chase-camera use-after-free was found, with no new logging. **None of that
is true on Raspberry Pi OS**, and each difference fails silently:

- **`systemd-coredump` is not installed.** `core_pattern` is a bare `core`, `coredumpctl` does not
  exist, and nothing is kept. `sudo apt install systemd-coredump gdb`.
- **The journal is in RAM.** Debian ships no `/var/log/journal`, so `journalctl -b -1` has nothing
  and a reboot erases the evidence. `sudo mkdir -p /var/log/journal && sudo systemctl restart
  systemd-journald`.
- **The kernel does not use the word "segfault" on ARM.** x86 prints `segfault at 0 ip ...`;
  arm64 prints `unhandled level 3 translation fault (11) at 0x0, esr 0x92000007, in <object>`.
  Grepping `dmesg` for "segfault" on a Pi finds nothing and reads as "the kernel logged no crash".
  Grep for `unhandled|fault|<binary name>`, and check `/proc/sys/debug/exception-trace` is 1.

With no core, the `dmesg` line is still worth having: subtract the mapped base from the `ip` and
`addr2line -e ./doomlegacyarcade -fCi <offset>` gives file and line, because `tools/build.sh` puts
`-g` in `ENV_CFLAGS`.

**And check the storage before believing any backtrace on that rig.** Two SD cards have died there
in the same way, and a corrupted library gives a perfectly plausible stack pointing at innocent
code. `sudo debsums -s` audits every installed file; a couple of `rpi-*` config files reported as
changed is normal (the Pi's own first-boot scripts rewrite them), flagged binaries or libraries are
not.

## The encoding trap, and how it was closed

Fourteen files in this tree were not valid UTF-8, and **plain `grep` skips such
a file silently** — no match, no warning, no non-zero exit. A sweep for
`R_Cache_Lock` during a review came back with four of its six call sites,
because `r_segs.c` was one of the fourteen. Nothing in the output said so.

`file` does not reliably identify them either. It called `hardware/hw_main.c`
and `p_map.c` plain "ASCII text", and `r_segs.c` "ASCII text, with NEL line
terminators" — that NEL is a stray `0x85`, and it is exactly what makes grep
treat the file as binary.

The reliable detection is to ask grep itself, by comparing a normal count with
a forced-text one:

    for f in $(find svn1749/src -name '*.c' -o -name '*.h'); do
        [ "$(grep -c "" $f)" = "$(grep -ac "" $f)" ] || echo "SKIPPED: $f"
    done

### Why iconv was the wrong tool

All 33 offending bytes were inside comments — French notes from the original
Legacy authors — and none were in a string literal or in code, which is what
made a conversion safe at all. But they came from **three different legacy
encodings**, so converting the lot from any single one would have turned the
other two into mojibake:

| byte | meant | encoding | seen in |
| --- | --- | --- | --- |
| `0xE0 0xE7 0xE8 0xE9 0xEA 0xF4 0xF9` | `à ç è é ê ô ù` | latin-1 | `déterminée`, `carré`, `intéressantes` |
| `0xB0 0xB7` | `° ·` | latin-1 | `90°`, a fog formula |
| `0x82` | `é` | **cp437** | `supporté`, `numéro`, `portée`, `départ` |
| `0x85` | `à` | **cp437** | `à la 4dos`, `à la Boom` |
| `0x96` | `–` | **cp1252** | `(1e–kw)` in the Glide fog notes |

The cp437 ones are DOS-era files; `0x82` in latin-1 is a control character, and
would have produced an invisible byte where an `é` belongs. Each one was read
from the surrounding French rather than guessed.

### How it was verified

Comments do not reach the compiler, so a correct conversion must leave the
output bit-identical. Object files were saved before and compared after:

**124 of 125 identical.** The one that differed was `d_main.o`, which was not
among the converted files — it embeds `DLA_VERSION` from the version-describe
step, and the working tree had gone from clean to dirty. Confirmed by reading
the strings out of both objects.

That is the check to repeat if this is ever done again: convert, rebuild,
compare objects. Anything that differs beyond the version string means a byte
was changed somewhere the compiler could see it.
