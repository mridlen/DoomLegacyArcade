# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

DoomLegacy 1.48.18, a source port of Doom/Heretic descended from id Software's original engine, at
SVN revision r1749 of the `legacy_one/trunk` repository. It is a C99 game engine built with GNU Make
(a CMake file also exists but is secondary/less maintained, SDL-only, and not kept in sync with all
Makefile options). There is no test suite.

This started as a plain source dump and is now tracked in git, with the upstream import as the first
commit — so `git log`/`git diff 8a2f980` shows exactly what has been changed locally. **It is being
customized into a locked-down arcade cabinet build**; see "Arcade cabinet customizations" below,
which is where most local divergence from upstream lives.

The actual source tree is under `svn1749/`. The top-level `bin/`, `dep/`, `objs/`, `make_options`,
and the `srcdir` symlink are leftover build-output scaffolding from the original packager's machine;
`srcdir` is a **dangling symlink** (points to a path on the original author's machine) and is not
usable as-is. Always work under `svn1749/`. The top-level `common/` holds the runtime asset package
(`legacy.wad`, `dogs.wad`) — `legacy.wad` has been **locally modified** (the `M_STSERV` menu graphic
now reads "Start Game" instead of "Start Server").

## Build

Building must happen from `svn1749/src/` (the Makefile there locates the rest of the build via
relative paths and drops `bin/`, `objs/`, `dep/` in `svn1749/`, one level up from `src/`).

```
cd svn1749/src
cp ../make_options_nix ../make_options   # first time only; pick _win/_mac/_os2/_dos as appropriate
make                                      # builds using ../make_options
```

`make_options` is **gitignored** (it is machine-local), so the settings below are not recoverable
from the repo — on a fresh checkout the stock `make_options_nix` needs three edits before it will
build on a modern Linux toolchain (each of these is a hard build failure, not a warning):

- `SDL2=1` — uncomment. The stock file targets SDL 1.2, which modern distros no longer ship; with
  it enabled the Makefile finds `sdl2-config` (via sdl2-compat) and links `-lSDL2 -lSDL2_mixer`.
- `ARCH=-march=native` — replace the stock `ARCH=-march=i686`, which is 32-bit-only and fails with
  "CPU you selected does not support x86-64 instruction set".
- `ENV_CFLAGS=-std=gnu17` — add. GCC 15 defaults to `-std=gnu23`, where `true`/`false` are reserved
  keywords, which breaks this codebase's own `typedef enum {false, true} boolean;` in `doomtype.h`.
  (`ENV_CFLAGS` is appended to `CFLAGS` unconditionally; the Makefile's `STD` variable is dead code.)

Other key `make_options` variables (set in the copied file, not on the CLI, to avoid repeating them
every invocation):
- `SMIF` — system media interface: `SDL` (default), `LINUX_X11`, `FREEBSD_X11`, `OS2_NATIVE`, `DOS_NATIVE`.
- `OS` — defaults based on `SMIF` (e.g. `LINUX_X11` implies `OS=LINUX`).
- `HAVE_MIXER=1` — enable SDL_mixer music.
- `DEBUG=1` — debug build (can also target a separate `BUILD=debug` output directory).
- `ARCH=-march=...` — CPU-specific optimization.

Build output directories (`svn1749/bin`, `objs`, `dep`) must exist first; if they don't, `make` fails
with "No such file or directory" on a `.dep` file. Create them with `make dirs` run from `svn1749/`
(**not** from `src/`, where `dirs` is an empty placeholder target).

Adding a new `.c` file requires manually adding its `.o` to the hand-maintained `MOBJS:=` list in
`svn1749/src/Makefile` — there is no wildcard or auto-discovery, and omitting it fails at link time.

Useful targets: `make clean`, `make distclean` (also removes `make_options`), `make depend`,
`make BUILD=<dir>` (build into an alternate output directory), `make DEBUG=1 BUILD=debug`.

Do not run the built binary from inside the build tree — it expects to run from an installed
directory (see `make install`, `install_user`, `install_sys`, `install_games` targets, or just copy
everything from `bin/` to a run directory alongside a `legacy.wad`).

There is no automated test suite; validating a change means building and running the game
interactively (or asking the user to).

## Compile-time feature flags

Most engine features are toggled in `svn1749/src/doomdef.h` via `#define`, independent of the Make
options (e.g. `SPLITSCREEN`, `FRAGGLESCRIPT`, `VOODOO_DOLL`, `BOOM_GLOBAL_COLORMAP`, `SAVEGAME99`).
Comment/uncomment there for engine behavior changes; use `make_options`/Makefile only for
platform/toolchain/library selection.

## Arcade cabinet customizations

Local additions on top of upstream r1749, all aimed at an unattended cabinet. Most are gated on the
`devmode` global (`extern byte devmode` in `doomincl.h`, defined in `d_main.c`), set by the
**`-devmode`** command-line flag, which unlocks everything back to stock behavior for development.

`-devmode` now carries three separate jobs — it unlocks the menus, keeps the Legacy gameplay extras
instead of forcing vanilla, and is the only mode that writes `config.cfg`. That is deliberate (an
operator-versus-player split), but it means the three cannot be chosen independently: an operator
session is `./doomlegacy -devmode`, change settings, quit; a player session is plain `./doomlegacy`.

**`-devmode` must be parsed before `M_Init()`** (`d_main.c`, just above the `M_Init()` call), because
`M_Init` is what applies the menu lockdown. It was originally parsed later alongside `-devparm`, which
silently made the flag do nothing at all.

- **Menu lockdown** (`m_menu.c`, in `M_Init` under `if( ! devmode )`). What a player can reach:

  ```
  Main:     New Game / Options / Quit Game
  Options:  Crosshair / Player >> / Game Options >> / Select Game >>
  Player:   Player1 config >> / Player2 config >>
  Config:   Crosshair / Player setup >>
  Setup:    Your color / Control scheme / Player config >>
  ```

  Hidden: Multiplayer (both entry points), Load/Save on the main menu, most of Options (Messages,
  Always Run, Effects/Connect/Network/Server/Menu Options, Sound Volume, Video Options, Setup
  Controls), Network Options again where Game Options nests it, several Start Game server options,
  Always Run/Autoaim/mouse/weaponpref/rebinding on the player config screen, and name/skin on the
  Setup Player screens. Uses **`IT_HIDDEN`**, a locally added
  `IT_DISPLAY` value — unlike stock `IT_DISABLED` (grayed but still occupying a row) the generic
  drawer skips it without advancing `y`, so entries vanish and the list closes up. Items are hidden
  *in place*, never removed from the arrays, because several menus are indexed by hardcoded position
  elsewhere. `M_DrawSetupMultiPlayerMenu` paints the name box and skin string outside the item loop,
  so those are suppressed separately. Each affected menu's `lastOn` is moved to the first item still
  shown, or the cursor starts on an invisible row (`M_SetupMenu` only walks *down* past hidden
  items, so it cannot recover when index 0 is hidden).
- **Game selector** (`m_menu.c`, `M_SelectGame` / `GameSelectDef`, reached from Options). Lists the
  installed IWADs (Ultimate Doom, Doom II, Final Doom Plutonia and TNT) and then any level packs.

  **Switching IWAD restarts the program.** The startup sequence has to run again; the engine can do
  that (the Launcher's "Iwad" item reaches `goto restart_command` in `D_DoomMain`) but only *before*
  `D_DoomLoop`, which is a `while(1)` that never returns. So `M_Restart_Program(idstr)` shuts down
  cleanly and **re-execs** with a different `-game`. Passing `NULL` restarts as-is, which the idle
  timeout uses to discard a loaded level pack.
  - `-game` takes the short name from the `gamedesc` table in `d_main.c` (`doomu`, `doom2`,
    `plutonia`, `tnt`, …), so the engine locates the IWAD itself and no wad path is hardcoded.
    Adding another game is one entry in `gameselect_arg[]` plus a display name.
  - The rebuilt command line preserves existing arguments (so `-devmode` survives a switch) and
    strips any earlier `-game`/`-iwad`.
  - `QUIT_normal` is required for the shutdown — the other severities force a 3 second sleep in
    `D_Quit_Save` — and `cv_textout.EV` is zeroed first to skip the ENDOOM screen.
  - Entries whose IWAD is missing are hidden, via `D_Game_Available()` (`d_main.c`), which tries
    each candidate filename from `game_desc_table` through the engine's own `Search_doomwaddir` —
    so the normal search paths and alternate names (`doomu.wad`/`doom_se.wad`/`doom.wad`) all
    count. The whole "Select Game" line is hidden when fewer than two choices exist.

  **Level packs are loaded, not launched.** Every `.wad` in `legacyhome/levels/` is listed below the
  games as `"<game> wad: <name>"`, with a leading `*` when loaded. Selecting one issues
  `addfile "<path>"`, which adds the PWAD to the running session; its maps then replace the IWAD's,
  so the ordinary One or Two Player flow plays it. Adding a PWAD at runtime is supported; swapping
  the IWAD is not. Selecting does **not** start a game — doing so would force the mode and the
  starting map, which suits a deathmatch set but not a single player overhaul.
  - The directory is deliberately **separate from the iwad search paths**, so no name filtering is
    needed and `legacy.wad` or an IWAD can never be listed as a pack. Created on startup if absent.
  - Packs are filtered by map style: `MAPxx` for `doom2_commercial`, `ExMy` otherwise. A mismatch
    fails to load (DWANGO5 under Ultimate Doom), so `M_LevelPack_MapStyle()` reads the wad's lump
    directory directly — loading the pack to discover whether it loads defeats the purpose. It
    returns a **bitmask**, because some packs (Maps of Chaos) ship `MAPxx` and `ExMy` versions of
    every level in one wad; stopping at the first map lump hid them under one of the two games.
  - **One pack at a time.** Selecting a different pack replaces the loaded one; selecting the loaded
    one unloads it. **The engine cannot remove a wad** — there is no `W_Unload` in `w_wad.c`, the
    lumps stay for the life of the process — so both restart, re-adding what should remain with
    `-file` (`M_Restart_Program(idstr, keep_packs)`). Only loading into an empty slot avoids a
    restart. Packs restored by `-file` are detected in `argv` during the scan so they come back
    marked, and the old `-file` list is always stripped when rebuilding so packs cannot accumulate.
  - Once a pack is loaded the attract screen is not trustworthy — the pack overrides the IWAD maps,
    so the built-in demos play against the wrong levels. `M_LevelPack_Loaded()` reports this, and
    both routes back to the attract screen (the idle timeout in `G_Ticker`, and
    `M_EndGameResponse`) restart the program instead of returning to title.
- **"Read This!" is hidden on the Doom 1 gamemodes** (`m_menu.c`, `M_Configure`). Doom 2 already
  overwrites that slot with Quit (`MainMenu[MM_readthis] = MainMenu[MM_quitdoom]`), which is why the
  entry only appeared under Ultimate Doom, where it is the help/order-form screens. This lives in
  `M_Configure` rather than the `M_Init` lockdown because **`gamemode` is not yet known at
  `M_Init`** — `IdentifyVersion()` runs later, as does the doomwaddir setup. Anything menu-related
  that depends on the game or on locating wads must go in `M_Configure`; the game selector's
  availability check is there for the same reason.
- **The menus are driven by the cabinet buttons** (`m_menu.c`, `M_Cabinet_Menu_Key`, called from
  `M_Responder`'s `ev_keydown`). The panel has no arrow keys, Enter or Escape, so both players'
  buttons are translated: forward/backward = cursor up/down, turn *or* strafe left/right =
  left/right, fire = select, use/open = back out. Read from `gamecontrol[]`/`gamecontrol2[]` **by
  action**, not by hardcoded character, so it follows the selected control scheme and any
  rebinding. Turn and strafe both map to left/right on purpose: the two schemes only swap which
  side pair is turning, so mapping both makes the `,aoe` diamond behave identically either way.
  Only applied while `menuactive` — otherwise "use" would open the menu during play instead of
  opening doors — and skipped in devmode so the keyboard behaves normally.
  Note the menu is still *opened* with Escape, which is not on the panel.
- **Menu letter shortcuts are disabled** outside devmode (`m_menu.c`, `M_Responder`'s `default:`
  case returns before the `alphaKey` search). The cabinet is buttons-only and several of those
  buttons are letters that collided with the shortcuts — player 1's turn-right button (`e`) on the
  New Game menu jumped to END GAME, one Enter from ending the run now that prompts are gone. Done
  at the dispatch point rather than by clearing `alphaKey` per menu, so it covers every menu.
  Text entry is unaffected: `IT_KEYHANDLER` items consume the key earlier in `M_Responder`.
- **Settings do not persist** (`m_misc.c`, `M_SaveAllConfig` returns early unless `devmode`).
  Anything a player changes lasts only for that session; every launch reloads the baseline from
  `config.cfg`. The operator sets that baseline by running with `-devmode`, which is the **only**
  way the config is written — including for settings not exposed in the menus, such as screen
  resolution. High scores and record demos are separate files and still persist.
- **Launcher bypass** (`d_main.c`, `#ifdef LAUNCHER` block in `D_DoomMain`). Upstream shows its
  built-in Launcher menu whenever `myargc < 2`; that condition is removed so it only appears after a
  genuine startup error.
- **No confirmation prompts** (`m_menu.c`). Quit, End Game, Nightmare skill, "already playing", and
  quicksave/quickload all take the "yes" path immediately. Only the savegame-slot `Delete Y/N?`
  survives, as it guards irreversible data loss.
- **Idle-to-title timeout** (`g_game.c`). `last_input_tic` is stamped in `D_PostEvent`, checked once
  per tic in `G_Ticker`'s `GS_LEVEL` case, and re-armed in `G_DoLoadLevel` so intermission time does
  not carry over. Ends the game via `Command_ExitGame_f()` and warns beforehand through the existing
  `HU_SetTip` centered-text mechanism, or restarts the program when a level pack is loaded (see the
  game selector). Tunable via `cv_idletimeout` / `cv_idlewarntime` (default 60s/15s, `0` disables).
  Skipped in devmode and demo playback. **Local splitscreen sets `netgame`**, so the check tests
  `(!netgame || cv_splitscreen.EV)`; gating on `!netgame` alone meant no two player game ever timed
  out, which is exactly when an unattended cabinet needs it.
- **Control schemes** (`g_input.c`). `cv_controlscheme[2]` ("Look and Move" vs "WASD") per player,
  selectable on the Setup Player 1 and 2 screens. `ControlScheme_Apply()` owns ten bindings per
  player (move/turn/strafe/fire/use/weapon cycling) and rewrites them on change and on config load,
  so hand-editing those `setcontrol` lines in `config.cfg` will not stick; everything else is left
  alone. Keys are the characters the user's **Dvorak** layout produces — the engine captures
  layout-aware SDL keycodes, not physical scancodes (`sdl/i_system.c`).
- **High scores and record demos** (`hs_stuff.c`/`.h`, new). Tracks best cumulative time-to-exit per
  **(game, map, skill)** for single player, shown on the intermission screen and as a page in the
  attract cycle. The game is part of the key because Doom 2, Plutonia and TNT all have a `MAP01`
  and they are different levels; record demos are named per game for the same reason, and only the
  running game's records and demos are shown or replayed. Records demos in the background and saves
  the run that set each record. Hooks:
  `HS_Init` from `D_DoomMain` (after `legacyhome` is resolved — `M_Init` is too early),
  `HS_NewGame` from the menu skill-select handlers (**must** precede `G_DeferedInitNew`, see below),
  `HS_LevelExit` from `WI_Init_Stats` (already the single-player-only branch of `WI_Start`), and new
  cases in `D_DoAdvanceDemo`/`D_Display`. `G_SnapshotDemo` (`g_game.c`) copies the demo buffer
  without closing it, so live recording continues after a record is saved.

- **Vanilla gameplay is forced** after the config load (`d_main.c`, unless `-devmode`): the Legacy
  extras above are set to 0 so the cabinet plays like Doom regardless of the config. Note these are
  single global `CV_NETVAR` cvars, not per-player — the two-player Options screen edits
  single-player behavior too.

Runtime data lives in `~/.doomlegacy/`: `config.cfg`, `highscores.dat` (plain text,
`game map skill tics`), `demos/<game>_<map>_sk<N>.lmp`, and `levels/` for selectable level packs.

To reset the scores, use the **`clearhighscores`** console command or the **`-clearhighscores`**
command-line flag (which runs the same code right after `HS_Init`). Both clear the in-memory table
as well as the files. Prefer them over deleting `highscores.dat` by hand: the table is cached in
memory while the game runs, so a later record writes the old entries straight back out.

### Gotchas found the hard way

- **Returning to the title screen resets very little.** State from the finished game leaks into the
  attract screen, which has produced two separate bugs: a loaded level pack left the built-in demos
  playing against the wrong maps, and `cv_splitscreen` left them rendering in a split view. Both are
  now cleared on the way out — the pack by restarting, splitscreen in `Command_ExitGame_f()`, which
  is the single funnel for every route back to attract mode (End Game, the idle timeout, and the
  engine's own error paths). **If the attract screen ever looks wrong after play, suspect leftover
  state first**, and prefer fixing it in `Command_ExitGame_f` so every route is covered.

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
- **`-synclog`** writes one line of simulation state per tic while recording or playing back, to
  `synclog_rec.txt` / `synclog_play.txt` in the current directory. Record a demo with it, play that
  demo back with it, and diff: the first differing line is the divergence tic. Inert without the
  flag. This is what found the bug above — identical inputs/RNG/angle with drifting momentum
  immediately excluded logic and RNG causes and pointed at the movement factor.
- **`G_StopDemo`/`G_CheckDemoStatus` used to free `demobuffer` without clearing it**, leaving a
  dangling pointer for any later recording to re-free. They now NULL it.
- The background recorder writes a stray `hs_background.lmp` into the current directory on exit.

## Architecture

The codebase is the classic Doom engine module layout, extended by the Legacy team. Prefixes on
source files are meaningful and consistently used across the tree:

- **`d_*`** — top-level game/doom driver: `d_main.c` (`D_DoomMain`, main loop, arg parsing, game
  mode detection), `d_net.c`/`d_clisrv.c`/`d_netcmd.c`/`d_netfil.c`/`i_tcp.c` — the client/server
  networking layer (DoomLegacy games are always run through a client/server protocol, even
  single-player, via `d_clisrv.c`'s "High Level Client/Server communications").
- **`p_*`** — play/game-logic simulation: map setup (`p_setup.c`), physics/collision (`p_map*.c`,
  `p_maputl.c`), mobj thinking (`p_mobj.c`, `p_enemy.c`, `p_user.c`, `p_inter.c`), specials/doors/
  plats/floors/lights (`p_spec.c`, `p_doors.c`, `p_plats.c`, `p_floor.c`, `p_lights.c`, `p_ceilng.c`,
  `p_switch.c`, `p_genlin.c`), and savegame I/O (`p_saveg.c`). This is the largest subsystem.
- **`r_*`** — the software renderer: BSP traversal (`r_bsp.c`), segments/planes/sprites
  (`r_segs.c`, `r_plane.c`, `r_things.c`), drawing (`r_draw.c`), main render loop (`r_main.c`).
- **`hardware/`** — the OpenGL/3D-accelerated renderer (`hw_main.c`, `hw_bsp.c`, `hw_draw.c`,
  `hw_cache.c`, `hw_md2.c` for models), with pluggable low-level GPU backends under
  `hardware/r_opengl/`, `r_glide/`, `r_minigl/`, `r_d3d/` (mostly legacy/Windows-era backends;
  `r_opengl` is the one in active use) and 3D positional audio in `hardware/s_ds3d/`.
- **`t_*`** — FraggleScript, an embedded scripting language for WAD-driven level scripting
  (`t_parse.c`, `t_script.c`, `t_prepro.c`, `t_vari.c`, `t_oper.c`, `t_func.c`, `t_array.c`).
- **`b_*`** — the bot AI (`b_game.c` bot logic/commands, `b_look.c` target acquisition, `b_node.c`/
  `b_search.c` pathfinding over the map's node graph).
- **`w_*`** — WAD file handling: `w_wad.c` (header/directory/lump I/O), `w_zip.c` (zip-based WADs,
  needs libzip).
- **`m_*`** — misc/support: `m_menu.c` (the entire options/setup menu system), `m_misc.c`,
  `m_cheat.c`, `m_random.c`, `m_fixed.c` (fixed-point math), `m_argv.c`.
- **`console.c` / `command.c`** — the in-game console and its command/cvar system; `command.c`'s
  console variables (`consvar_t`) are the primary mechanism for exposing tunables to players and
  the network protocol, each optionally backed by a callback function fired on change.
- **`dehacked.c`** — DeHackEd/BEX patch loader (runtime modification of `info.c` tables/strings).
- **`info.c`/`info.h`** — the generated-style tables of thing/state/sprite/sound definitions;
  `infoext.c`/`infoext.h` hold Legacy's extensions to the vanilla tables. `info.c.orig` is a
  reference copy of the pre-modification vanilla table, not a build input.
- **`umapinfo/`** — a separate, mostly self-contained UMAPINFO lump parser (lexer/parser/keywords)
  used to read modern map-metadata lumps from WADs.
- **Platform SMIF backends** (System Media Interface — the abstraction over OS/windowing/input/
  sound): `sdl/` (primary, cross-platform), `linux_x/` (native X11), `win32/`, `os2/`, `macos/`,
  `djgppdos/`. Only one is compiled in per build, selected by `SMIF`/`OS` in `make_options`. Files
  duplicated across these directories (`i_main.c`, `i_system.c`, `i_video.c`, `i_sound.c`, etc.)
  implement the same `i_*` interface declared in the shared headers (`i_system.h`, `i_video.h`,
  `i_sound.h`, `i_net.h`, `i_joy.h`) — when changing platform-facing behavior, check whether the
  change needs to be mirrored across backends.
- **`doomincl.h`**, **`doomdef.h`**, **`doomtype.h`**, **`doomstat.h`** — the common includes almost
  every `.c` file starts with; `doomincl.h` is explicitly documented as "not used in headers" (only
  include it from `.c` files, not from other `.h` files) to avoid include-order problems.

### Networking model

DoomLegacy has no separate single-player code path: even solo play runs a local client/server pair
through `d_clisrv.c`. This matters when changing game-state-affecting code (e.g. `p_*`, `g_game.c`)
— such changes must stay in sync across net play (deterministic simulation from `ticcmd_t` input),
not just work locally. Console variables and commands that affect gameplay are typically networked
(see `command.c`'s netcmd handling and `d_netcmd.c`).

### Message/logging

Don't use raw `printf`/`fprintf` for engine diagnostics — use the routines declared in
`doomincl.h`: `CONS_Printf` (routes through `EOUT_flags`), `GenPrintf(EMSG_*, ...)` (categorized
info/debug/dev/warn/error messages fanned out to console/log/stderr per `EMSG_e`/`EOUT_e`), or
`I_Error`/`I_SoftError` for fatal/recoverable engine errors.
