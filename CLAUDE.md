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
interactively (or asking the user to). **A lot can still be checked without a screen**, though —
see below.

### Headless verification

The game runs under SDL's dummy drivers, which is enough to exercise startup, config load, cvar
state, the attract cycle and level setup. This has caught real bugs that would otherwise have
needed a play session:

```
RD=/tmp/rundir            # NOT the build tree; needs the IWADs + legacy.wad
mkdir -p "$RD" && cp svn1749/bin/doomlegacy "$RD"/
ln -sf /home/mridlen/games/doom/* "$RD"/          # DOOM.WAD, DOOM2.WAD, legacy.wad, ...
SH=/tmp/fakehome && mkdir -p "$SH/.doomlegacy"
cp ~/.doomlegacy/config.cfg "$SH/.doomlegacy/"    # see the crash note below
cd "$RD" && HOME="$SH" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    timeout 40 ./doomlegacy -game doom2 -skill 5 -warp 1 > out.txt 2>&1
sed 's/\x1b\[[0-9;]*m//g' out.txt | grep ...   # output is full of ENDOOM color escapes
```

- **Always set `HOME` to a scratch directory.** `legacyhome` comes from `$HOME`, so a plain run
  reads *and can write* the cabinet's live `~/.doomlegacy/highscores.dat` and `demos/` —
  `HS_Save()` and `G_SnapshotDemo()` fire from `HS_LevelExit` whenever a record is beaten. A
  no-input run cannot reach a level exit so in practice it writes nothing, but that is luck.
- **Copy the real `config.cfg` into the scratch home.** With no config the software renderer
  segfaults in `R_DrawColumn_32` ← `R_DrawPlayerSprites` after `Change Graphics failed: err=-100`
  — the dummy driver never set a real mode. That is a harness artifact, not a fresh-install bug,
  but it looks alarming and wastes time.
- `timeout` exit code **124 means it survived**, which is the pass condition for a smoke test;
  139 is a segfault. `coredumpctl debug doomlegacy --debugger=gdb --debugger-arguments="-batch -ex bt"`
  gets a backtrace.
- Temporary `GenPrintf(EMSG_warn, ...)` instrumentation plus a headless run is the fastest way to
  answer "what is this cvar actually set to at runtime" — it is how the Nightmare `cv_fastmonsters`
  bug and the high-score page timing were both pinned down. Remove it before committing.
- **Before blaming a run for changed files, compare mtimes against the run times.** The cabinet is
  played between turns, and those writes belong to the user, not the test.

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
- **Two player mode is an operator setting** — `cv_twoplayer` ("twoplayer", default On, `CV_SAVE`),
  for cabinets built without a second set of controls. Toggled under **Options → Menu Options** in
  devmode (that whole submenu is hidden from players, so the entry needs no extra guard), and
  saved like any other setting: only a `-devmode` session writes it. When off, a player sees
  neither *Two Player Game* on the New Game menu nor *Player2 config* under Options → Player.
  - Applied in **`M_Configure`, not `M_Init`** — `config.cfg` is not loaded until `d_main.c:3181`,
    long after `M_Init` runs at 2598, so the value there would always still be the compiled
    default. Same rule as the game selector and the "Read This!" hiding.
  - The menu entry is **appended** to `MenuOptionsMenu` rather than inserted, since the lockdown
    addresses menu items by hardcoded index.
  - Hiding *Two Player Game* removes the only player-reachable route to `TwoPlayerDef`; the other
    entry point, `MultiPlayerMenu[0]`, is already behind the hidden Multiplayer item.
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
- **Idle-to-title timeout** (`g_game.c`, `G_Idle_Timeout_Check`). `last_input_tic` is stamped in
  `D_PostEvent`, checked once per tic from `G_Ticker`, and re-armed in `G_DoLoadLevel` so
  intermission time does not carry over. Ends the game via `Command_ExitGame_f()` and warns
  beforehand through the existing `HU_SetTip` centered-text mechanism, or restarts the program when
  a level pack is loaded (see the game selector). Tunable via `cv_idletimeout` / `cv_idlewarntime`
  (default 60s/15s, `0` disables). Skipped in devmode and demo playback. **Local splitscreen sets
  `netgame`**, so the check tests `(!netgame || cv_splitscreen.EV)`; gating on `!netgame` alone
  meant no two player game ever timed out, which is exactly when an unattended cabinet needs it.

  It runs in **`GS_LEVEL`, `GS_INTERMISSION` and `GS_FINALE`**, not just during play — both of the
  other two wait *indefinitely* for a keypress the walk-away player never gives, so covering only
  `GS_LEVEL` left the cabinet hung. The intermission stalls at `sp_state == 10` (`wi_stuff.c`)
  once the counters finish; the finale stalls in `F_Ticker`'s `finalestage 0` (the Doom 2 text
  screens need `keypressed`) and again in the cast call, which loops forever. `D_Display` calls
  `HU_Drawer` for `GS_LEVEL` **only**, so the countdown would not have been visible in the other
  two states — `HU_Draw_Tip` was un-`static`ed (declared in `hu_stuff.h`) and is called directly
  after `WI_Drawer`/`F_Drawer`. **Anything drawn by `HU_Drawer` has this same limitation.**
- **Control schemes** (`g_input.c`). `cv_controlscheme[2]` ("Look and Move" vs "WASD") per player,
  selectable on the Setup Player 1 and 2 screens. `ControlScheme_Apply()` owns ten bindings per
  player (move/turn/strafe/fire/use/weapon cycling); everything else is left alone. The two schemes
  differ **only** in which pair turns and which strafes — `pair_a` turns under "Look and Move" and
  strafes under "WASD", `pair_b` the reverse. The built-in presets' keys are the characters the
  user's **Dvorak** layout produces — the engine captures layout-aware SDL keycodes, not physical
  scancodes (`sdl/i_system.c`).
  - `ControlScheme_Apply` is driven **only** by the cvar's `CV_CALL` OnChange — there is no other
    caller. It therefore fires *during* config load, at the `controlscheme` line, which
    `config.cfg` writes well **before** the `setcontrol` lines. So the `setcontrol` lines execute
    last and win: hand-edited bindings *do* survive a load. (An earlier version of this file
    claimed they would not stick; a headless test with `setcontrol "forward" "z"` showed `z`
    surviving under **both** presets.) What actually loses them is *toggling the scheme cvar*,
    which re-stamps all ten immediately.
- **Guided control setup** (`m_menu.c`, `M_Guided_Controls_P1`/`_P2`, under Options → Setup
  Controls, which is devmode-only). Prompts for the ten controls a cabinet panel needs, binding
  each to whatever is pressed: **STICK UP / DOWN / LEFT / RIGHT**, then STRAFE LEFT, STRAFE RIGHT,
  FIRE, USE / OPEN, NEXT WEAPON, PREVIOUS WEAPON — a 4-way stick plus six buttons. Everything else
  stays on the ordinary Setup Controls pages. Built on the same `MM_EVENTHANDLER` message plumbing
  as `M_ChangeControl`, so the message box, key capture and event routing are shared; mouse and
  joystick buttons arrive as `ev_keydown` with codes in the key space, so any panel wiring works.
  - The prompts deliberately name the **physical control, not the game action**. They first said
    "TURN LEFT" with a note that WASD mode swaps turn and strafe, which reads as confusing to
    someone actually wiring a panel: they know they are pushing the stick left and should not have
    to reason about the simulation to answer. Keep it that way.
  - Finishing does **not** write bindings directly. It hands the ten captured keys to
    `G_Save_CustomControls()`, which stores them in **`cv_customcontrols[2]`** — a `CV_SAVE` string
    cvar per player holding ten space-separated key codes in `CK_*` order. `ControlScheme_Apply`
    prefers that table over the compiled-in `scheme_keys[]` whenever it parses, so **the cabinet's
    control layout lives in `config.cfg`** and the hardcoded table is only the fallback preset.
    An empty or malformed string falls back silently.
  - Going through the scheme machinery instead of writing `gamecontrol[]` is what keeps
    **"Look and Move" / "WASD" working on a custom panel**. The stick's left/right lands in pair A
    and the strafe buttons in pair B, which is the Look-and-Move arrangement; picking WASD
    afterwards swaps which pair turns and which strafes. The operator is never asked about this —
    it is a player preference applied after the fact. Verified headless with a distinct key per
    slot: under "Look and Move" turn was `a`/`e` and strafe `t`/`n`; under "WASD" they traded,
    with forward and fire unmoved.
  - `cv_customcontrols` is `CV_CALL` onto the same OnChange as the scheme cvar, so whichever of the
    two lines config.cfg happens to list last still leaves the bindings correct.
  - Only `ev_keydown` is accepted. Taking `ev_keyup` too would let the release of one press land
    on the next prompt and bind two actions to the same button.
  - ESC abandons the whole table rather than keeping a partial one — a half-taught panel is worse
    than the layout that was already working.
- **High scores and record demos** (`hs_stuff.c`/`.h`, new). Tracks best cumulative time-to-exit per
  **(wad combination, map, skill, category)** for single player, shown on the intermission screen
  and as a page in the attract cycle.

  There are two **categories** (`HS_NUMCAT`, `HS_CAT_speed`/`HS_CAT_max`), timed identically and
  displayed as side-by-side time columns. A **max** run additionally requires 100% kills and 100%
  secrets on *every* level so far — items are deliberately not required. `WI_Init_Stats` computes
  that per level from `wb_plyr[me].skills`/`.ssecret` against `wbs->maxkills`/`->maxsecret` (a map
  with none of a category counts as satisfied, matching how the percentage display treats it) and
  passes it to `HS_LevelExit`. The run-level flag `hs_run_is_max` starts true in `HS_NewGame` and
  latches false on the first level exited short; from then on only the speed record can be beaten
  for the rest of that run. Each category keeps its **own** record demo.

  Both tables are laid out by hand against the surrounding graphics, so **the numbers matter**.
  `HS_Draw_IntermissionTable(x, y)` draws its column header 14 *above* the `y` it is passed, and
  right-justifies each category's times at `x + HS_COL_TIME + cat*HS_COL_STEP` (90 and 62), so the
  last column's right edge is `x + 152` and must stay inside `BASEVIDWIDTH` (320). On the single
  player intermission the free band is only what is left between the Secrets row and the Time row:
  rows start at `SP_STATSY` (50) and are `lh` apart, where `lh` is 1.5× the `WINUM0` patch height
  (12) = **18**, and the percent patches are 12 tall — so Secrets ends at 98 and `SP_TIMEY` is 168.
  The call site's `+ 12` centers header-plus-five-rows in that 70px gap. **If a row is ever added
  or the font changes, re-check both ends** — the drawer only bails at `BASEVIDHEIGHT`, so it will
  happily paint over the Time/Par row.

  The **wad combination** is `HS_GameId()`: the game's short name plus any loaded level pack, such
  as `doom2` or `doomu+mapsofchaos`. Both parts are needed — Doom 2, Plutonia and TNT all have a
  `MAP01` that is a different level, and a pack replaces the maps again. Record demos are named
  with the same key, and only the current combination's records and demos are shown or replayed
  (a demo from another combination would desync instantly). The key is recomputed per use, since a
  pack can be loaded mid-session, and sanitized to one filename-safe word because it is a space
  separated field in `highscores.dat` *and* part of the demo filename, while pack names come from
  arbitrary filenames. Renaming a wad therefore starts a fresh table — the name is the identity.

  Record demos are **captioned** during attract playback (`HS_DemoLabel()`, drawn by `HU_Drawer`)
  with map, skill, category and time — `E1M1  ITYTD  MAX  4:32`. `HS_NextRecordDemoPath()` fills the
  label as a side effect of handing out a path, and `D_DoAdvanceDemo` clears it before every page,
  so a stock IWAD demo is never captioned with the previous record's text. Drawn on the **second**
  text line (y=8), because item pickups still print messages at y=0 during playback.

  The attract page appears **after every demo**, skipped when the current combination has no times
  (`HS_Have_Records()`) so a fresh cabinet does not show an empty page repeatedly. It is interposed
  in `D_DoAdvanceDemo` via a flag rather than added as a `demosequence` case, because those cases
  are shared between game modes and the last is reachable only under the retail divisor.

  The page is **paginated one map at a time**, and steps through **every** map with a time before
  handing back to the demo cycle — five skills by two categories always fits, so nothing is ever
  cut off however many maps get recorded, and an `n of m` footer shows the position. Maps with no
  time at all are skipped (`HS_Entry_Eligible()`); skills with no time still get a `--:--` row,
  since a gap in five rows reads as missing rather than as an open slot.
  - Timing: `D_DoAdvanceDemo` arms it with `pagetic = TICRATE * HS_PAGE_SECS * page_count - 1`,
    and `D_PageTicker` runs a second countdown (`hs_subpage_tic`) that calls
    `HS_Attract_Advance_Page()` as each map's slice elapses. **The whole segment therefore grows
    with the table** — at `HS_PAGE_SECS` = 3 and 14 maps recorded it is 42 seconds between demos.
    That is the intent (show everything), but it is the knob to turn if the attract loop feels
    slow; `HS_PAGE_SECS` is in `hs_stuff.h`.
  - The sub-page step is checked **after** `pagetic` expires and returns, so the last map is not
    replaced by a one-tic flash of the first on the way out. The `-1` on `pagetic` is part of the
    same fencepost.
  - The current page is tracked **by map name, not by table index**. `hs_table` is in the order
    maps were first played and grows mid-session, so an index would silently start pointing at a
    different map. Rotation picks the smallest name greater than the current one, wrapping to the
    smallest overall — which also recovers automatically when the map on screen disappears
    (scores cleared, game switched, pack loaded).
  - Plain `strcmp` gives the right order for both map styles (`E1M1`..`E4M9`, `MAP01`..`MAP32`).
    This matters: real tables are badly out of order (a live `highscores.dat` had E1M1, then
    E2M1–E2M7, then E1M2–E1M7), and name-sorting is the only reason the pages advance sensibly.

  Records demos in the background and saves the run that set each record. Hooks:
  `HS_Init` from `D_DoomMain` (after `legacyhome` is resolved — `M_Init` is too early),
  `HS_NewGame` from the menu skill-select handlers (**must** precede `G_DeferedInitNew`, see below),
  `HS_LevelExit` from `WI_Init_Stats` (already the single-player-only branch of `WI_Start`), and new
  cases in `D_DoAdvanceDemo`/`D_Display`. `G_SnapshotDemo` (`g_game.c`) copies the demo buffer
  without closing it, so live recording continues after a record is saved.

- **Kills/items/secrets on the HUD** (`st_stuff.c`, `ST_overlayDrawer`). The engine's status-bar
  overlay is driven by the `overlay` cvar, a **string of one-letter element codes** — stock
  `"kahmf"` is keys/ammo/health/armor/frags. Upstream already had `e` (kills) and `s` (secrets)
  but neither was in the default and there was **no items element**; `i` is new. The three are
  stacked top-right at `SCY(1/11/21)` with `K`/`I`/`S` labels, and the default is now
  `"kahmfeis"`. This pairs with the high-score **max** category, which needs 100% kills and
  secrets, so the player can see whether the run is still eligible.
  - The overlay only draws when **`st_overlay_on`**, which `R_SetViewSize` (`r_main.c`) sets from
    `cv_viewsize.value == 11` — the largest view size, no status bar. At any smaller viewsize the
    classic status bar draws instead and none of this appears. The cabinet's `config.cfg` is at
    `viewsize 11`.
  - Skipped in splitscreen, like the upstream `e`/`s` cases: `killcount` is per player while
    `totalkills` is the map's, so one corner cannot speak for both. High scores are single player
    anyway.
  - **`config.cfg` overrides the compiled default**, and only devmode rewrites it, so changing the
    default in `st_stuff.c` does nothing on a machine with an existing config — the saved
    `overlay` line has to be edited (or re-saved from a `-devmode` session) as well.
- **The ranked ruleset** (`hs_stuff.c`, `hs_ranked_rules[]`). **Game options are not multiplayer
  only.** DoomLegacy has no separate single-player path — solo play runs the same client/server
  simulation — and every gameplay setting is a single global `CV_NETVAR`, so anything reachable
  under Options applies to a scored single-player run. The two-player Options screen edits
  single-player behavior for the same reason.

  The baseline is **vanilla difficulty knobs with Boom/MBF engine behavior left at its defaults**
  (roughly complevel 11): gravity, monster/item respawn, monster health and pickup multipliers,
  dogs, voodoo mode, insta-death, weapon recoil, jumping, tired run/drown, monster vary and the
  rest are pinned to vanilla, while the Boom/MBF AI and physics fixes stay on so Boom-format level
  packs still work. `HS_Apply_Ranked_Ruleset()` runs after the config load (`d_main.c`) and again
  in `Command_ExitGame_f`, so **one player's tinkering cannot leave the next player unable to
  score**; settings are not saved outside devmode, so this only ever undoes a within-session change.

  Rather than hide the menus, an altered ruleset **plays on but records nothing**:
  `HS_Ruleset_Is_Ranked()` is checked in `HS_NewGame` (which then skips starting the background
  recording at all) and again at every `HS_LevelExit`, latching `hs_run_ranked` false so a change
  made *mid-run* voids it rather than only the levels after. The player is told twice — a warning
  under the item list on the Game Options and Adv Options screens (`M_Draw_Unranked_Warning`,
  called from `M_Drawer` so all three screens are covered in one place), and an `UNRANKED` marker
  at the top of the HUD during play.
  - **`cv_respawnmonsters` and `cv_fastmonsters` are deliberately absent from the table.**
    `G_InitNew` turns both on for `sk_nightmare` (`g_game.c`, `CV_SetParam`), so they belong to
    the skill, not the player. Leaving them out costs nothing: Nightmare overrides the player
    either way, and on other skills both default to off and can only be switched *on*, which
    makes the game harder. **This bit once already** — `cv_fastmonsters` was left in the table by
    mistake, and every Nightmare run played MAP01 normally, then showed `UNRANKED` from MAP02 with
    no score for MAP01. That is the signature of a rule the *engine* changes after `HS_NewGame`:
    the run passes the check at menu time and fails it at the first level exit.
  - When a run is voided, `HS_LevelExit` logs `Run is unranked: "<cvar>" differs...` once via
    `GenPrintf(EMSG_info, ...)`. A run that silently scores nothing is near-impossible to diagnose
    from the outside; check the console/log first if scores stop appearing.
  - **`config.cfg` legitimately disagrees with the ruleset, and that is not a bug.** `-devmode`
    skips `HS_Apply_Ranked_Ruleset()` entirely, so an operator session keeps whatever the config
    holds and writes it straight back; a player session loads the same file and then overrides the
    relevant cvars in memory, never saving. On the current cabinet config exactly three saved
    values differ — `monsterfriction "Momentum"` (2 vs 0), `monstergravity "On"` (2 vs 0) and
    `voodoo_mode "Auto"` (3 vs 0), all DoomLegacy defaults the ruleset pins to vanilla. Everything
    else in the table already matches. **Do not "fix" the config to match**, and do not compare the
    two by eye: config stores the *label* (`"On"`, `"Boom"`, `"x3"`, `"50"`) while the table stores
    the number, so most apparent mismatches are the same value. Resolve labels through the cvar's
    `PossibleValue` table before concluding anything.
  - `HS_Apply_Ranked_Ruleset` **self-checks** and warns via `GenPrintf(EMSG_warn, ...)`. If a value
    in the table is not one of a cvar's `PossibleValue`s, `CV_Set` rejects it silently and the
    cabinet would record nothing forever — this turns that into a visible message. All current
    values were verified against their `*_cons_t` tables and by a headless run.
  - Values are in menu units; `hs_rule_expected` scales `CV_FLOAT` cvars (gravity) by `FRACUNIT`,
    and `hs_rule_current` reads `.value` for `CV_FLOAT`/`CV_VALUE` cvars and the `.EV` byte
    otherwise — mirroring `command.c`'s own split. **`.EV` is a byte**, so a fixed-point cvar's
    `.EV` is its low 8 bits and useless for comparison.
  - The demo header records nearly all of these (38 bytes at `G_BeginRecording`, plus the 6 added
    for the Legacy extras), so pinning values is demo-safe. **`cv_gravity`, `cv_predictingmonsters`
    and `cv_blockmap_gen` are the exceptions — not recorded anywhere**, so changing one would
    desync a record demo. All three are pinned by the ruleset, which is now the only thing keeping
    them consistent. Worth remembering if a demo ever desyncs mysteriously.

Runtime data lives in `~/.doomlegacy/`: `config.cfg`, `highscores.dat` (plain text,
`<wadcombo> map skill tics <category>`), `demos/<wadcombo>_<map>_sk<N>_<category>.lmp`, and
`levels/` for selectable level packs. `<wadcombo>` is `HS_GameId()`, e.g. `doom2` or
`doomu+mapsofchaos`; `<category>` is `speed` or `max`. The category is written **last** so a
four-field line from before categories existed still loads, as a speed record.

To reset the scores, use the **`clearhighscores`** console command or the **`-clearhighscores`**
command-line flag (which runs the same code right after `HS_Init`). Both clear the in-memory table
as well as the files. Prefer them over deleting `highscores.dat` by hand: the table is cached in
memory while the game runs, so a later record writes the old entries straight back out.

### Gotchas found the hard way

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
