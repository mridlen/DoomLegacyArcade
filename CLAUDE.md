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
(`legacy.wad`, `dogs.wad`) — `legacy.wad` has been **locally modified**: the `M_STSERV` menu
graphic now reads "Start Game" instead of "Start Server", and the cabinet's own art has been added
(`M_SINLVL`, `M_JOIN`, `M_CHEATS`, `CREDIT2`, `SBOARMBL`). The user edits their live
`~/games/doom/legacy.wad` and it is copied over `common/legacy.wad` to track it; verify only the
expected lumps differ before committing.

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
RD=/tmp/rundir            # NOT svn1749/bin -- that is the live cabinet now
mkdir -p "$RD" && cp svn1749/bin/doomlegacy "$RD"/
cp -a svn1749/bin/legacyhome "$RD"/               # a COPY of the live home
ln -sf /home/mridlen/games/doom/* "$RD"/          # DOOM.WAD, DOOM2.WAD, legacy.wad, ...
cd "$RD" && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    timeout 40 ./doomlegacy -game doom2 -skill 5 -warp 1 > out.txt 2>&1
sed 's/\x1b\[[0-9;]*m//g' out.txt | grep ...   # output is full of ENDOOM color escapes
```

- **Isolating `HOME` is no longer enough, and running from `svn1749/bin` is now dangerous.** Since
  the portable install landed, `legacyhome` is found *next to the binary* and `$HOME` is never
  consulted — so `HOME=/tmp/fakehome` protects nothing, and running `svn1749/bin/doomlegacy` reads
  and writes the **cabinet's live scores, demos and config**. Always copy the binary *and*
  `legacyhome/` into a scratch directory, as above. (The cabinet's old `~/.doomlegacy` has been
  renamed to `~/.doomlegacy-backup` and is no longer read by anything.)
- **The scratch home needs a real `config.cfg`.** Copying `legacyhome/` wholesale covers this; if
  you build one by hand instead, put a config in it.
- **Software rendering runs headless too**, and this file used to say otherwise. A software-mode
  run segfaulted in `R_DrawColumn_32` ← `R_DrawVisSprite` at any player count, which was written
  up here as a harness artifact of the dummy driver "never setting a real mode". **It was not** —
  it was the real, unsigned-`heightmask` bug below, and the cabinet hit it on real hardware the
  first time anyone played in software mode. The lesson is general: **a crash that only shows up
  in the harness is not thereby a harness bug.** Prove it against a known-good path before
  writing it off, or the note becomes the reason nobody looks again.
- `timeout` exit code **124 means it survived**, which is the pass condition for a smoke test;
  139 is a segfault. `coredumpctl debug doomlegacy --debugger=gdb --debugger-arguments="-batch -ex bt"`
  gets a backtrace.
- Temporary `GenPrintf(EMSG_warn, ...)` instrumentation plus a headless run is the fastest way to
  answer "what is this cvar actually set to at runtime" — it is how the Nightmare `cv_fastmonsters`
  bug and the high-score page timing were both pinned down. Remove it before committing.
- **Before blaming a run for changed files, compare mtimes against the run times.** The cabinet is
  played between turns, and those writes belong to the user, not the test.
- **A headless run can reach the intermission**, which used to be written off as needing a play
  session. There is a **`wait`** console command (used by the bot code, `d_main.c`), and
  `D_DoomLoop` execs `legacyhome/autoexec.cfg`, so dropping this into the *scratch* home drives a
  level exit on its own:
  ```
  printf 'wait 105\nexitlevel\n' > "$SH/.doomlegacy/autoexec.cfg"
  ```
  That is enough to exercise `WI_Init_Stats`, `HS_LevelExit`, the whole intermission drawer and the
  high-score write — it produced a real `highscores.dat` line (`doom2 MAP01 2 104 speed`) and let
  the Time/TOTAL layout be checked numerically instead of guessed. `exitlevel` alone will not do:
  a bare `+exitlevel` runs before the level exists. Combine with `wait` for **anything gated behind
  actually finishing a level**.
- **A command that starts a game does not start it where it appears in the script.**
  `G_DeferedInitNew` issues its `map` command with `COM_BufAddText`, which *appends* — so anything
  still queued behind it in `autoexec.cfg` runs first, and the level only loads once the buffer
  drains. A script reading `wait 35 / <start a game> / wait 250 / die` runs `die` with no level
  loaded and `players[].mo` still NULL. It looks like the start failed; it has not happened yet.
  (`exitlevel` survives this because it is applied once a level exists, which is why the earlier
  single-level tests scored despite the same ordering — note their tiny 31-tic times.) **To drive
  something that needs a live level, start it from the command line** (`-warp`) and set only the
  mode flag from the script.
- **`kill` segfaults with no player** — stock `Command_Kill` (`d_netcmd.c`) dereferences
  `players[consoleplayer].mo` unguarded, so typing it at the title screen or from an autoexec that
  runs too early dumps core in `P_KillMobj`. Pre-existing, not worth confusing with a real crash:
  check `mo` before blaming the change under test.

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
  New Game: Single Player / Single Level / Multiplayer / End Game
  Options:  Crosshair / Player >> / Game Options >> / Select Game >>
  Player:   Player1 config >> / Player2 config >>
  Config:   Crosshair / Player setup >>
  Setup:    Your color / Control scheme / Player config >>
  ```

  On **Multiplayer → Options** (the Net Options page) only the deathmatch ruleset a player might
  reasonably choose is left: Allow exitlevel, Teamplay, TeamDamage, Fraglimit, Timelimit,
  Deathmatch Type, Frag's Weapon Falling, and the Game Options link. Allow Jump, Allow Rocket Jump,
  Allow autoaim, Allow turbo, Allow join player and Maxplayers are hidden — server and network
  plumbing that means nothing on a cabinet. `NetOptionsMenu` is addressed by position for this, so
  its indices are named (`netoption_*`); keep the enum in step with the array.

  Hidden: Networked Multiplayer (both entry points), Load/Save on the main menu, most of Options (Messages,
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
- **Menu naming**: the New Game page offers **Single Player** and **Multiplayer**, where
  "Multiplayer" is *local* play on this cabinet (the old "Two Player Game" — no longer two player
  only) and uses the **`M_MULTI`** graphic, which reads "MULTIPLAYER". `M_2PLAYR` literally reads
  "TWO PLAYER GAME" and is now unused. The engine's networked server menu is renamed
  **"Networked Multiplayer >>"** and drawn as **plain text** rather than the `M_MULTI` graphic, so
  it cannot be mistaken for the line above. It was already devmode-only — the lockdown hides both
  of its entry points — and stays that way. **Not because it is known broken: it has never been
  exercised in this build**, since cabinet-to-cabinet play needs two cabinets and there is not yet
  one. Nothing was removed, so treat it as untested rather than unsupported, and do not "fix"
  anything there speculatively.
  - The `TwoPlayerDef` page uses `M_MULTI` as its title graphic for the same reason.
  - Its two `SETUP PLAYER` rows (`M_SETUPA`/`M_SETUPB`) are replaced by **four**
    "Player n config >>" text entries, matching Options → Player, so panels 3 and 4 needed no
    artwork. They open `PlayerOptionsDef` **without** `Pop_Menu()`, unlike `M_PlayerDirectorChoice`,
    so backing out returns to this page instead of skipping past it.
  - **`TwoPlayerMenu` is addressed by position** and the rows moved, so its indices are named
    (`twoplayer_*`) and the lockdown uses those — the networked row went from 4 to 6.
  - Rows for panels the cabinet does not have are hidden, as on the Player page.
  - **The three mouse rows are devmode-only** (`M_SetupMultiPlayer_pind`): a player has no use for
    mouse settings on a cabinet, and they were three rows of clutter on the page reached most
    often. Already hidden for panels 3 and 4, which have no mouse hardware at all.
  - **No menu indices moved**, so the lockdown's hardcoded positions (`SingleMulti_Menu[2]`,
    `TwoPlayerMenu[4]`) still point at the right rows.

- **Boot game** — `cv_defaultgame` ("defaultgame", default `None`, `CV_SAVE`), under
  **Options → Menu Options** as "Boot Game" beside `cv_twoplayer`, so it is operator-only. Picks
  which game the cabinet starts in instead of whichever IWAD the search finds first.
  - **It cannot be read as a cvar.** `IdentifyVersion()` chooses the IWAD at `d_main.c:3030`;
    `M_LoadConfig` does not run until **3216**. So `D_Read_Default_Game()` parses the single
    `defaultgame "..."` line straight out of `config.cfg` beforehand, called next to `HS_Init` —
    after `legacyhome`/`configfile_main` are resolved, before `IdentifyVersion`. A targeted parse
    was chosen over moving the config load earlier, which would reorder startup for everything.
  - The cvar's `PossibleValue` strings are **the `game_desc_table` idstrs themselves** (`doomu`,
    `doom2`, `plutonia`, `tnt`) rather than pretty labels, because config stores a cvar's *label*
    and that hand-parse needs the stored text usable as-is. They are also exactly what `-game`
    accepts, which makes the setting self-documenting.
  - Validated **before** entering the `-game` block in `IdentifyVersion`, not inside it: that
    block's `game_switch_found` label sits within its own braces and an unrecognized value there
    takes a fatal path. A boot game that is unrecognized, or whose IWAD has since been uninstalled
    (`D_Game_Available`), must never stop the cabinet booting — both cases warn and fall through to
    the normal search.
  - `-game` and `-iwad` on the command line both override it.
  - Verified headless across all six paths: unset → normal search; `doomu`/`tnt` → those games;
    `-game doom2` overriding a `tnt` default; `"banana"` → warns, normal search; and `plutonia`
    with the IWAD genuinely unreachable → warns, normal search. That last one needs `HOME` isolated
    as well as the wad removed from the run directory, or `~/games/doom` still satisfies the
    search and the test silently passes for the wrong reason.
- **Single Level mode** (`m_menu.c`, `SingleLevelMenu`/`SingleLevelDef`, from a new main menu entry
  using the locally added **`M_SINLVL`** graphic). Plays one chosen map and returns to its own menu,
  with a separate high score table.
  - **It lives on the New Game page, at index 1, directly under Single Player**
    (`SingleMulti_Menu`) — a third way to *start a game*, rather than a peer of the New Game item
    itself. It was originally inserted into `MainMenu` at index 1, and moving it back off that menu
    reverted every index that had shifted for it: `MM_cheats` 5→**4**, `MM_readthis` 6→**5**,
    `MM_quitdoom` 7→**6**, and the lockdown's Load/Save hiding 2,3→**1,2**. Those are the complete
    set on the main menu.
  - **`SingleMulti_Menu` is now addressed by position**, which it was not before, so its indices are
    named in an enum (`singlemulti_*`) and the two lockdown sites use those — the networked row went
    from 2 to 3 and the MULTIPLAYER row from 1 to 2. Keep the enum in step with the array.
  - It needed no new artwork or geometry: the same `M_SINLVL` graphic (148x17), and `MainDef` and
    `SingleMultiDef` share an origin (both 97,64), so it draws exactly where it did before.
  - Verified by dumping both menus at the *end* of `M_Configure` — dumping at the top shows a
    misleading pre-fixup state, since Doom 2's `MainMenu[MM_readthis] = MainMenu[MM_quitdoom];
    numitems--`, the Doom 1 Read This hiding and the Cheats hiding all run later. **This bit again
    during this move**: an insertion point that merely looked like the end of the function reported
    7 main items with Read This still present. Final result: Doom 2 gets 6 items with Quit at 5;
    Ultimate Doom gets 7 with Read This hidden; New Game gets 5 with Single Level at 1. Checked in
    all four states — Doom 2 player, Ultimate player, `-devmode`, and `twoplayer` off, where the
    hidden row must be MULTIPLAYER and **not** Single Level.
  - Reuses `cv_nextmap` / `cv_nextepmap` / `cv_skill` from the Start Game screen rather than
    building a level list — `M_Configure` already trims `exmy_cons_t` to the episodes present. The
    per-gamemode swap (`SingleLevelMenu_Map` vs `_EpisodeMap`) is done **in `M_Configure`**, not on
    menu open: the New Game page reaches this page with `IT_SUBMENU`, which has no handler to hook.
  - **`cv_skill` is 1-based and `skill_e` is 0-based** — `skill_cons_t` numbers the five skills
    1..5, because that is what the `map ... -skill %d` console command wants (`Command_Map_f` does
    `atoi()-1`), and `M_StartServer` passes `cv_skill.value` straight into that command. But
    `G_DeferedInitNew` and every `HS_*` entry point take a 0-based `skill_e` — `M_ChooseSkill`
    feeds `G_DeferedInitNew` the New Game menu's *item index*. Single Level handed them
    `cv_skill.value` raw, so it launched and scored **one skill too hard**: picking Ultra violence
    ran `sk_nightmare`, which is fast monsters (imp fireball speed 20 instead of 10) plus
    respawning. `M_SingleLevel_Skill()` now does the conversion in one place, used by all five
    sites — the launch, the two best-time displays, the replay-item enable and the
    `HS_Demo_Path_For` lookup. **All five have to move together**, since `HS_LevelExit` records
    `gameskill`, so the menu's lookup key must be the same number the launch produced.
    - The shift was self-consistent, which is why it hid: the run recorded under the shifted skill
      and the menu looked it up under the same shifted skill, so the table and the "Watch run"
      items always agreed. Only the difficulty actually played was wrong.
    - **Existing single-level records stay where they are**, and that is correct — they were
      genuinely played at the harder skill. They simply move one row down in the menu's view
      (an ITYTD-labelled run reappears under HNTR, which is what it was).
    - Verified headless end to end via a temporary console hook onto `M_SingleLevel_Start`:
      cv_skill 1/3/4/5 now launch `skill_e` 0/2/3/4, with `cv_fastmonsters` EV 0 and imp fireball
      speed 10 for Ultra violence and EV 1 / speed 20 only for Nightmare. A full UV run wrote
      `doom2-sl MAP01 3 31 speed` and `doom2-sl_MAP01_sk3_speed.lmp`.
  - **Separate scoring falls out of `HS_GameId_Mode()`**, which appends `-sl`. Records, record demo
    filenames and the attract page are all selected by that id, so one change covers all three.
  - **A Single Level run scores through `HS_Score_As_Single_Level` like any other, and the Survival
    refactor broke that for a while.** `HS_LevelExit` called it under
    `hs_run_levels == 1 && ! single_level_mode` — the guard was there to describe a *campaign* first
    level, and it excluded the real thing. A run started from the Single Level menu was then left
    writing **only** a `runs.dat` board entry:
    - no split record in `highscores.dat`, so the attract single-level pages and the attract demo
      rotation (both walk `hs_table`) never saw it; and
    - its demo went to `HS_Snapshot_If_Leading`, which names snapshots the **Survival** way,
      `<game>-sl_ep<N>_sk<M>_<cat>.lmp` — a name nothing reads back. The Single Level menu's
      "Watch run" items ask `HS_Demo_Path_For` for the **per-map** `<game>-sl_<map>_sk<N>_<cat>.lmp`.

    So the board showed a fresh time while the menu replayed whatever stale per-map file predated
    it, and a map whose record was *only* ever set this way had no playable demo at all. On the
    cabinet: `runs.dat` held `doomu-sl E1M5 E1M5 0 speed 2050` against a `..._E1M5_sk0_speed.lmp`
    three days older, and `doomu-sl E1M3 E1M3 1 speed 1687` with no `..._E1M3_sk1_...` file
    anywhere — its recording had gone to `doomu-sl_ep1_sk1_speed.lmp`.
    - The guard is now on the *board* half only: `HS_Score_As_Single_Level` takes **`add_to_board`**,
      false in single level mode, because such a run is committed to the same three-deep board by
      `HS_Run_Finished` when it ends and inserting here as well would put two identical entries on
      it. The split record and the per-map demo are written either way.
    - `HS_Snapshot_If_Leading` returns early for a single level game id, so there is exactly one
      snapshot per run and it lands under the name the menu looks for.
    - **The two tables can still disagree on a cabinet that ran the broken build**, since the board
      was updated and the split table was not. They re-converge as records are beaten — both are
      written from the same run at the same exit — but until then the "Watch run" demo tracks the
      *split* record, not board place 1.
  - **Single-level times never reach the attract score *pages*** — they would double the number of
    pages. `Command_ExitGame_f` clears `single_level_mode` on the way to the title, and the attract
    page asks for the *current* mode. The menu's own display therefore cannot use the flag either,
    so `HS_Board_Entry`/`HS_Demo_Path_For` take the mode as a parameter (`HS_Best_For` is gone —
    the Single Level page reads the three-deep board now). **Single-level record
    *demos*, by contrast, are replayed in the attract cycle** — see `HS_NextRecordDemoPath` below.
  - **The intermission shows the map's own Single Level record**, not the episode's Survival one.
    `HS_Draw_IntermissionTable` asked `HS_Survival_Entry`, which resolves `HS_GameId_Mode(false)` —
    always the campaign id — so a Single Level attempt was held up against a whole-episode time it
    cannot beat on one map, and the player had no idea what they were actually chasing. In
    `single_level_mode` it reads place 1 of that map's board (`HS_Board_Entry(true, ...)`) instead.
    Same two rows, same measured columns, so none of the layout notes below change.
  - Ending after one map is done in **`G_DoWorldDone`**, which is where the next map is chosen. The
    intermission and `HS_LevelExit` have both already run by then, so returning early is all that is
    needed. Guarded on `! demoplayback`: a record demo replays through real level exits and would
    otherwise be truncated.
  - **`G_NextLevel` needs the same guard**, or a map that ends an episode never reaches
    `G_DoWorldDone` at all: it starts a finale and sets `gameaction = ga_nothing` instead. A single
    level run of MAP30 dropped into the cast call, which loops for ever, and only the idle timeout
    rescued the cabinet. Single Level now returns to its menu from every map, ending map included.
  - **A death ends the run too** (`G_DoReborn`). The player watches the corpse as normal; it is the
    *"use" press* that would retry the level which returns to the menu instead. Doom retries by
    **reloading** the map (`G_DoReborn` → `G_DoLoadLevel(true)`), which for a scored cabinet is a
    free second attempt — `HS_Player_Died` has already voided the run, so the reload would hand out
    an unscored do-over of the same map with no indication why.
    - It sets **`gameaction = ga_worlddone`** rather than calling `M_SingleLevel_Finished()`
      directly. `G_DoReborn` runs from `G_Ticker`'s reborn loop, which is *above* the point where
      tearing the level down is safe; `G_Ticker` dispatches `gameaction` a few lines later, which
      is the same route an ordinary level exit already takes to that function. The dispatch loop
      terminates because `Command_ExitGame_f` → `D_StartTitle` sets `gameaction = ga_nothing`.
    - Guarded on `! demoplayback`, like the rest, so replaying a demo cannot tear the game down
      around it — but note the guard is **defensive rather than load-bearing, and cannot be
      exercised**. A saved record demo never contains a death: `HS_Player_Died` closes the
      recording through `G_CheckDemoStatus`, and the snapshots are written at record-setting level
      exits, so every saved demo ends at an exit. Stock IWAD demos *can* contain a death (the
      Ultimate Doom E4M2 one does), but those only play on the attract screen, where
      `single_level_mode` is 0 and the guard's first condition already fails. Do not go looking for
      a test case for this; there isn't one to build.
    - Placed in the `(!multiplayer && !deathmatch)` branch, which is the retry path. The other
      branch is coop/DM respawn, and `P_SetupLevel`'s own `G_DoReborn` call is deathmatch-only, so
      neither can reach this.
    - Verified headless: with `single_level_mode` set on a running level, killing the player and
      then setting `PST_REBORN` (what `P_DeathThink` does on `BT_USE`) logged
      `G_DoReborn -> single level menu` followed by `G_DoWorldDone -> M_SingleLevel_Finished`, with
      no level reload and no hang. The same sequence without single level mode still reloaded.
  - **`single_level_mode` means "a single level run is in progress"**, not "the player is looking
    at the Single Level page". It is set by `M_SingleLevel_Start` and `M_SingleLevel_PlayDemo` for
    the run each begins, and cleared by `Command_ExitGame_f` — which every route back to the title
    passes through. The page's own display never reads it: `HS_Board_Entry`/`HS_Demo_Path_For` take
    the mode as a parameter, and the menu passes `true` literally.
    - `M_SingleLevel_Finished()` used to **re-set the flag** after `Command_ExitGame_f` cleared it,
      on the reasoning that the cabinet was "staying in" single level mode. That left it stuck on
      for everything that followed: a New Game started afterwards inherited it, so **the campaign
      ended after one map** and (once a death also ended a run) a death dropped the player onto the
      Single Level page. The symptom only appears after playing Single Level at least once in the
      session, which is what makes it look like the *campaign* is wired wrong. It is not re-set any
      more.
    - `M_ChooseSkill` and `M_StartServer` **clear it explicitly** at a campaign start.
      `G_DeferedInitNew` does not funnel through `Command_ExitGame_f`, so starting a New Game from
      the menu *during* a single level run would otherwise carry the flag straight into the
      campaign. Any future game-start path needs the same line.
    - Verified headless both ways: with the re-set restored, the flag reads 1 after
      `M_SingleLevel_Finished` and stays 1 through `M_ChooseSkill`; with the fix it reads 0 at both
      points. End to end, a campaign level entered after a single level run ended took
      `G_DoReborn -> RELOAD LEVEL` on death, while a run with the flag genuinely set still took
      `G_DoReborn -> SINGLE LEVEL MENU`.
  - The two "Watch … run" items go `IT_DISABLED` rather than `IT_HIDDEN` when no demo exists, so the
    page does not change height as the player scrolls maps.
  - **Never set `singledemo` to play a demo from a menu.** `G_CheckDemoStatus` reacts to it with
    `I_Quit()` — no return, the program exits. It made watching a record demo quit DoomLegacy the
    moment the exit switch was hit. The replay items set `single_level_mode` instead, and the
    `demoplayback` branch of `G_CheckDemoStatus` routes back to the menu via
    `M_SingleLevel_Finished()`.
  - **No attract-screen consumer may use `HS_GameId()`**, because `single_level_mode` is whatever
    the cabinet was last left in (it is still set while the player sits on the Single Level page,
    with the attract cycle running behind the menu). Every one of them resolves the mode
    explicitly instead:
    - The score pages give campaign and single level times **their own page families**, each
      asking `HS_GameId_Mode()` for the mode it wants. They were campaign-only at first, partly to
      stop single level times leaking on when the flag happened to be set and partly to stop the
      page count doubling; bounding the cycle to three pages an appearance removed the second
      objection, and asking explicitly removed the first.
    - `HS_NextRecordDemoPath` accepts **both ids** and captions the single-level ones
      `SINGLE LEVEL: E1M1  HNTR  SPEED  0:13`. A single-level demo is an ordinary one map
      recording — nothing about replaying it is mode specific — and it adds no pages, just another
      demo in a rotation that already exists. It sets `is_single` from which id matched rather
      than reading the flag.
    - `HS_GameId_Mode` returns a pointer to **one static buffer**, so the two ids must be copied
      into locals before the second call overwrites the first.
    - Caption width was measured against the real `STCFN` lumps, per the font rule below: the
      widest realistic caption (`SINGLE LEVEL: MAP01  ITYTD  SPEED  888:88`) is **275px of 320**,
      and `HU_Drawer` centres on `V_StringWidth` so it follows any rewording.
    - Level exit and the intermission table deliberately still follow the current mode.
  - Verified headless: a single-level run wrote `doom2-sl MAP01 2 103 speed` and
    `doom2-sl_MAP01_sk2_speed.lmp`, did not advance to MAP02, and left the campaign table intact.
    Attract replay was verified with a **trimmed `highscores.dat`** holding one campaign row and
    one `-sl` row — the full cabinet table is large enough that a two minute run does not reach a
    given row, which reads as "it didn't work". The trimmed table alternated
    `doomu_E1M1_sk0_speed.lmp` and `doomu-sl_E1M1_sk1_speed.lmp`, the latter captioned
    `SINGLE LEVEL: E1M1  HNTR  SPEED  0:13`.

  **Replay order is a shuffled bag**, not a linear walk of the table (`hs_bag*`,
  `HS_Refill_DemoBag`). Straight cursor order played the same demos in the same sequence every
  cycle, which is monotonous on a machine that sits on the attract screen all day. Every slot
  holding a record goes into a bag, the bag is Fisher-Yates shuffled, and demos are dealt from it
  until it is empty — so **each demo plays once before any repeats**. Picking at random per demo
  was rejected: it cheerfully shows the same one three times running, which is the complaint
  rather than the cure.
  - **The PRNG is local to `hs_stuff.c`** (`HS_Shuffle_Rand`, xorshift32, seeded from `time(NULL)`
    in `HS_Init`). Deliberately not an engine one: `P_Random` is demo-sync critical, and
    `M_Random`/`N_Random` index a shared 256 entry table that **`M_ClearRandom` resets at every
    game start**, so a bag shuffled from those would come out identical after every boot. A
    self-contained generator also cannot perturb anything a recording depends on.
  - The bag is refilled when exhausted **and whenever `hs_table_count` changes**, so a record set
    during the session joins the rotation without a restart. A new record on a map row that
    already exists does not change that count and waits for the next natural refill — at most one
    pass of the cycle.
  - `hs_bag_last` stops a new bag opening with the demo that closed the previous one, the single
    repeat a shuffle cannot rule out by itself. Needs `hs_bag_count > 1`; with exactly one demo
    recorded it necessarily repeats.
  - Eligibility is still re-checked when a slot is dealt (game id, and `access()` on the file), so
    a deleted demo or a level pack swap is skipped rather than played.
  - Verified headless by calling `HS_NextRecordDemoPath` in a loop through a temporary console
    command, against the cabinet's real 21-demo table: three consecutive bags of 21 each held all
    21 distinct demos with no duplicate, no immediate repeat anywhere including at the two bag
    boundaries, and a second run produced a different order. A separate run confirmed a MAP02
    record set mid-session appeared in the rotation immediately after that level exit.
- **Local players: up to four (Phase 1 — no viewports yet)**. A multicade panel may have three or
  four sets of controls. The engine was hardcoded to two *everywhere*: `d_net.h` even defined
  `MAXSPLITSCREENPLAYERS 2`, but **nothing referenced it** — every limit was its own literal. That
  constant now lives in `doomdef.h` beside `MAXPLAYERS`, is **4**, and is the real knob.
  - **`cv_localplayers`** ("localplayers", 1..4, default 1, `CV_SAVE`) is how many players join on
    this machine — an operator setting like `cv_twoplayer`. It is **not** `cv_splitscreen`, which
    is only the two-view render toggle. `D_NumLocalPlayers()` clamps it.
  - **Players 3 and 4 played but were not drawn** *at this phase* — the renderer split into at
    most two stacked halves (`r_main.c` `rdraw_viewheight >>= 1`, and `r_draw.c` had exactly
    `ylookup1`/`ylookup2`, view 2 at `vid.height>>1`). Phases 2 and 3 below gave them viewports;
    this paragraph is the starting state, not the current one. A 2x2 split needs per-view *horizontal* offsets, which have no
    precedent in the code — that is Phase 2, along with re-deriving the HUD placement.
  - What Phase 1 changed, and why each was load-bearing:
    - `SV_commit_player` refused a third player outright (`if( pind > 1 ) return 255`).
    - **The protocol already anticipated this.** `clientcmd_pak_t` carries a `pind_mask` and is
      sized to "use only what is needed", so the send/receive paths generalised cleanly; only the
      hardcoded `0x03` and `cmd[1]` had to go.
    - **`pind == 2` was a sentinel meaning "the server"**, not a player index (`Send_NetXCmd_auto`
      routing, used by bots). A third panel would have been routed as the server. It is
      **`TEXTCMD_PIND_SERVER`** now, tied to `MAXSPLITSCREENPLAYERS` so the two cannot collide.
    - `gamecontrol` / `gamecontrol2` became one table, `gamecontrol_pl[pind]`. **The two old names
      survive as macros onto rows 0 and 1**, so all ~47 existing references across a dozen files
      keep working untouched. `setcontrol3`/`setcontrol4` bind the new panels and the config saves
      all four.
    - The whole per-player cvar family widened to `MAXSPLITSCREENPLAYERS`: name, color, skin,
      autoaim, weaponpref, originalweaponswitch, autorun, alwaysmlook, mousemove, crosshair,
      controlscheme, customcontrols. First two console names are unchanged, so **an existing
      `config.cfg` still loads exactly as before**; panels 3/4 add `name3`, `skin4` and so on.
    - **Registration is a loop now.** Only players 0 and 1 were registered; an unregistered
      `consvar_t` has a NULL string, which `Send_NameColor_pind` hands straight to the netxcmd
      packer — a third panel segfaulted the instant the server announced it.
    - `G_BuildTiccmd` took its player from **`displayplayer2_ptr`**, a *view* pointer that is NULL
      whenever the screen is not split. It reads `localplayer[pind]` now. **This is the general
      lesson: views and local players are different things**, and anything that conflates them
      breaks the moment a panel has a player but no viewport.
    - `scheme_keys[]` has no preset for panels 3/4 **on purpose** — the two presets are Dvorak
      characters chosen so one keyboard can drive two players, and there is no third set that
      would not collide. Panels 3/4 stay unbound until the guided setup fills
      `cv_customcontrols[pind]`.
    - `localplayer[]` **must keep a static initializer**: `M_LoadConfig` runs long before
      `D_Init_ClientServer`, and the config's name/skin lines reach `Send_NameColor2`, which tests
      `localplayer[1] < MAXPLAYERS`. Zeroed, that reads as "player 0" and sends a netxcmd with no
      server — an immediate startup segfault. A `typedef` guard fails the build if the initializer
      stops covering every slot.
  - Verified headless: `localplayers` 1/2/3/4 each joined exactly that many players, filling slots
    0..3 with real mobjs and surviving. Controls: `splitscreen 1` still gives the normal two player
    split (`multiplayer=1`), and the stock default is untouched at one player.
  - **Phase 2 (viewports) is done for both renderers** — the hardware one first, the software
    one in Phase 3 below. `D_NumViews()` returns 1, 2 or 4:
    one view is the whole screen, two are the stacked halves splitscreen always had, and three or
    four are a **2x2 grid** (three players leave one quadrant unused, rather than inventing a
    third layout). `pind` 0..3 reads left-to-right then top-to-bottom, so panel order matches
    screen order.
    - **A demo always plays full screen.** `D_NumViews()` returns 1 while `demoplayback`, whatever
      the cabinet's panel count says — otherwise the attract screen was carved into a 2x2 and the
      demo drew in the top-left quadrant. This is the same trap `cv_splitscreen` had, which
      `Command_ExitGame_f` clears on the way to the title; `cv_localplayers` is an operator setting
      and is *not* cleared, so the view count has to ignore it here. Covers the Single Level
      "watch run" replays too.
    - **Changing the view count needs `R_SetViewSize()`**, or the old geometry sticks: viewport
      sizes are only recomputed on request. `G_DoPlayDemo` and `G_StopDemo` both call it, as
      `D_Set_View_Cell` does. **This has now caught me twice** — the count was right and the
      rectangle was stale both times.
    - **The cabinet runs OpenGL** (`drawmode "OpenGL"` in the tracked config), which is why the
      grid went into `hw_main.c`: a viewport there is just a rectangle. `HWR_SetViewSize` halves
      `gr_viewheight` for two views and `gr_viewwidth` as well for four, and
      `HWR_RenderPlayerView` offsets `gr_viewwindowx/y` by the view's column and row.
    - **The "does the view fill the screen" test had to become "does it fill its cell"**
      (`gr_viewwidth == view_span_w`, not `vid.width`). Left as it was, a half-width quadrant took
      the status-bar centering path and the top row came out at **y = -61**.
    - A quadrant is very nearly the screen's own aspect ratio, so it must **not** get the
      2-view projection squash: `atransform.splitscreen` is now `(D_NumViews() == 2)`, and the
      same for the weapon-sprite nudge that keys off fov 90.
    - `viewsv_need_sky[]` and `view_dynlights[]` were `[2]`, indexed by view number.
    - **Phase 3: the software renderer draws the 2x2 grid too.** It used to draw at most two
      views — `r_draw.c` had exactly `ylookup1`/`ylookup2` for stacked halves, and `D_Display`
      rendered view 0 plus, for the software path, a view 1 hardwired to the lower half. With
      four panels that produced **two half-screen views and four HUDs**, which is what a
      software-mode cabinet actually showed.
      - **The mechanism was already there and unused.** The software renderer has always had a
        horizontal window offset as well as a vertical one — `view_window_x`, folded into
        `columnofs[]` by `R_Init_ViewBuffer` — but nothing ever used it for a second *column*,
        because splitscreen only ever stacked. **`R_Set_View_Window(vind)`** (`r_draw.c`) now
        places both tables on any cell of the grid, and `D_Display` calls it per view. The two
        precomputed half-screen tables are **gone**: they can only describe stacked halves and
        nothing said so at the call site.
      - **Only the software path halves the width**, gated on `rendermode` by `soft_grid` in
        `R_ExecuteSetViewSize`. The hardware renderer places its views by GL viewport and reads
        `rdraw_viewwidth` only through **`vid.fit_width`/`fit_height`**, which feed
        `atransform.scalex/scaley`, `HWR_Init_TextureMapping`'s focal length and
        `gr_pspritescale_*` — halving those visibly widened the OpenGL field of view and
        resized the weapon sprite. **Anything that touches `rdraw_*` reaches the hardware
        renderer through `vid.fit_*`**; check there before assuming the two paths are separate.
      - `r_draw.c` keeps the two questions apart: `R_View_Cell_Size()` is the **view grid the
        player sees** (both renderers, used to black out an unclaimed cell), while
        `R_Draw_Cell_Size()`/`R_Draw_Column_Split()` describe the **software draw window**,
        which still spans the screen in hardware mode. Placing a view with the wrong one of the
        two puts `view_window_x` at a negative value in OpenGL.
      - **The border tests had to become cell-relative**, the same trap the hardware renderer hit
        with `gr_viewwidth == view_span_w`: `rdraw_scaledviewwidth == vid.width` reads a
        half-width cell as a reduced view window, so `D_Display`, `R_FillBackScreen` and
        `R_DrawViewBorder` would paint a border around every quadrant. They ask
        **`R_View_Fills_Cell()`** now.
      - **The freelook aiming shift asked `cv_splitscreen`** (`R_SetupFrame`'s
        `dy = cv_splitscreen.EV ? rdraw_viewheight*2 : ...`). The view height is halved by the
        *view count*, and `cv_localplayers` halves it with that cvar still off — the same
        views-versus-local-players conflation that broke `G_BuildTiccmd`. It asks `D_NumViews()`.
      - **So did the weapon sprite's base centre**, and that one was visible from across the
        room: `R_DrawPSprite` (`r_things.c`) sets
        `vis->texturemid = cv_splitscreen.EV ? 120 : BASEYCENTER`. The 120 is a hand tuning for
        the **two-view** split, where the weapon is drawn at the *full screen* y scale inside a
        half height view — `pspriteyscale` keeps `vid.height` while `rdraw_viewwidth` is not
        halved for two views — so it has to be shifted up to sit right. A 2x2 cell is the other
        case: `rdraw_viewwidth` is halved with it, the weapon is scaled proportionally, and it
        wants the ordinary `BASEYCENTER`. It asks `(D_NumViews() == 2)` now, which is exactly
        the distinction the hardware renderer already draws with `atransform.splitscreen`.
        - Keyed on the cvar it was wrong in both directions: `cv_localplayers` puts four views
          on screen with the cvar **off**, and the Multiplayer menu turns the cvar **on** for a
          game that then runs as a 2x2 — shifting the weapon up by 20 base units, about 38px of
          a 384px cell, leaving the wrist ending in mid-air above the bottom of the view.
        - **Which route started the game therefore decided whether it looked right**, and that
          is why it survived the first round of testing: a headless run sets `cv_localplayers`
          straight from the config and never touches the cvar, so it takes the correct branch.
          **To reproduce anything the Multiplayer menu can cause, set `splitscreen 1` as well** —
          `M_StartServer` does, and `D_NumLocalPlayers` treats it as "at least two players".
        - Verified by screenshot at 1280x800 (the cabinet's `viewfit` class, unlike a 4:3 test
          resolution): with the cvar on and off the 2x2 weapon is now identical, the classic
          two-view split is **0 pixels changed**, and single player is untouched.
      - **The crosshair is per view now** (`HU_Draw_Crosshair`). It drew exactly two, the second
        only when `cv_splitscreen` was set, and both at `vid.width>>1` — the middle of the
        *screen*. That is right for the stacked halves, which span the full width, but a 2x2
        cell's centre is not the screen's, so a quadrant's crosshair would have sat on the
        boundary between two players' views. It loops over views, places each at its own cell's
        centre, and reads `cv_crosshair[D_Panel_Of(vind)]` so the setting follows the player's
        panel. One and two views are unchanged (the two-view crosshair sites are byte-identical).
      - **But the real reason panels 3 and 4 had no crosshair was that the cvar was never
        registered.** `menu_command_cvar_list` (`m_menu.c`) stopped at Player2 for **six** of the
        widened arrays: `cv_autorun`, `cv_crosshair`, `cv_alwaysfreelook`, `cv_mouse_move`,
        `cv_controlscheme` and `cv_customcontrols`. That list's own comment says it: *any cv_
        with CV_SAVE needs to be registered, even if it is not used.* Unregistered, the name is
        an unknown command, so the setting cannot be held, cannot be set, and is never written
        out — `config.cfg` reported `Unknown command 'crosshair3'` on every boot.
        - **The expensive one there is `cv_customcontrols[2]`/`[3]`**, which is where the guided
          setup stores a panel's ten keys. Unregistered means **the guided setup for panels 3
          and 4 could never be saved** — rebind them, quit, and the bindings are gone. Same for
          `cv_controlscheme[2]`/`[3]`. Worth re-running the guided setup for those panels now.
        - The six arrays that *were* fully registered are the ones covered by the
          `for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )` loop in `d_netcmd.c` (name, color,
          skin, autoaim, weaponpref, originalweaponswitch). The gap was everything registered by
          hand-written list entry instead of by loop. **When widening a per-player cvar, grep for
          its `[1]` registration** — the declaration being `[MAXSPLITSCREENPLAYERS]` proves
          nothing.
        - `cv_usemouse` stays `[2]` deliberately (two mouse devices), so the panel 3/4 entries
          for `cv_alwaysfreelook`/`cv_mouse_move` are kept out of the mouse init blocks, where
          the order of `cv_usemouse` registration matters.
      - **Auditing this by autoexec gives a false all-clear.** Querying each name from
        `legacyhome/autoexec.cfg` reported no unknown commands at all — because without `-warp`
        that run never reached `D_DoomLoop` to execute the file. The reliable signal is the
        **config load**, which names the line: `config line 350: unknown setting "crosshair3"`.
        Check the output has the lines you expect before concluding anything from its silence.
      - **One `cv_splitscreen` read remains, deliberately.** `st_stuff.c` hides the K/I/S overlay
        elements on that cvar, so they show or hide depending on which menu started the game.
        Left alone on the user's call.
      - **Pixel comparison here has a noise floor**, so read a small diff before believing it:
        the screenshot lands on tic 70 or 71 depending on timing, which moves monsters by one
        animation frame and ticks the HUD clock — about 6000 pixels of 1024000 at 1280x800.
        A real difference in this area shows up in the *weapon*; if the diff mask is monsters
        and clock digits only, nothing changed. Render the mask and look at it.
      - **A cell with no player is filled black** (`D_Display`). Three players use the 2x2 and
        leave the fourth quadrant unused, and *neither* renderer was clearing it — the hardware
        per-frame clear (`HWR_ClearView`) is depth only, and the software renderer draws straight
        into the screen buffer — so it kept the last thing drawn there, frozen, which reads as a
        crashed renderer.
      - Sprite, psprite and corona clipping already clamp to `rdraw_viewwidth`
        (`r_things.c:1919/2193/2910`), so nothing can bleed into the next cell horizontally; the
        `vid.width` tests near `r_things.c:3529` are coarse off-screen rejects, with the fine
        clip after them. The half-*height* case has relied on exactly this for years.
      - Verified first by printing each view's draw window through a temporary console command,
        with `soft_grid` temporarily forced true so the OpenGL harness computed the *software*
        numbers — software mode could not be run at all at that point (see the sprite crash
        below). At 1024x768, four views: cells 512x384 at `0,0` `512,0` `0,384` `512,384`, each
        view's `columnofs` and `ylookup` confined to its own cell, and the highest byte written
        exactly `width*height*bytepp` — the buffer end, no overrun. One and two views unchanged.
        Once the crash was fixed this was confirmed by **screenshot**: four quadrants each with
        their own view and HUD, and with three players a black fourth quadrant.
      - The OpenGL path was required to stay **byte-identical**, and is: screenshots at 1, 2 and
        4 players differ in 0 of 786432 pixels. Three players differ only inside the
        bottom-right quadrant, which is now pure black instead of stale image.
    - Verified numerically rather than by eye, printing each view rectangle at 1366x768:
      1 view `0,0 1366x768`; 2 views `0,0` and `0,384`, each 1366x384; 4 views `0,0` `683,0`
      `0,384` `683,384`, each 683x384 — tiling the screen exactly. Three players give the first
      three of those.
    - **The HUD follows the same grid** (`st_stuff.c`). `SCY(y, y0)` used to halve for
      `cv_splitscreen` and add a row offset; it and `SCX` now both take the view's origin and a
      per-axis divisor, so a quadrant halves *both* axes and shifts into its column.
      `ST_overlayDrawer` takes a view index instead of a 0/1 "status position", and `ST_Drawer`
      loops over the views, taking each player from `localplayer[]`.
    - **The HUD art shrinks for a quadrant, and can be**: it is 320x200 base art multiplied by
      `vid.dupx`/`dupy`, not fixed-size, so halving that scale gives a quarter-screen view the same
      HUD-to-view proportions the full screen has. **Round the halved floats rather than dividing
      the integers**: at 1366x768 dup is 4,3, and integer halving gives 2,1 — art twice as wide as
      tall. Rounding gives 2,2. The hardware renderer uses the floats either way.
    - **Halve the global `vid` scale, not `drawinfo`'s copy of it**, and restore it at the end of
      `ST_overlayDrawer` (which has a single exit). `ST_drawOverlayNum` and `V_DrawScalePic_Num`
      call `V_SetupDraw` *themselves*, which re-reads `vid.dupx/dupy`, and `ST_drawOverlayNum` also
      uses `vid.dupx` directly for the digit advance. Halving only `drawinfo` was silently undone
      for exactly **health, ammo and armor**, while the keys — drawn without a re-setup — did
      shrink. That split symptom is the giveaway: **if some overlay elements scale and others do
      not, the ones that do not are re-entering `V_SetupDraw`.**
    - With the global scale halved, positions shrink with the art, so `SCX`/`SCY` take divisor 1
      for the 2x2 case; only the two-view split still passes `ydiv = 2`.
    - Digit width measured: 14px base art draws at `wfv` 56 for one or two views (`dup` 4,3) and 28
      for four (`dup` 2,2) — half, matching the half-width cell.
    - **The deathmatch rankings are per view too** (`hu_stuff.c`). `playerdeadview` and
      `hu_showscores` were single globals, so one player dying painted the score table across
      *everyone's* screen at full size — including in an ordinary two player game.
      `HU_Draw_DeathmatchRankings` now takes a view index, halves the scale and draws in that
      view's cell, and `HU_Drawer` asks per view via `HU_Rankings_For_View`: that view's own
      player's death, or that panel's own `gamecontrol_pl[vind][gc_scores]` key.
    - **`V_CENTERHORZ` must be off for the multi-view case, and this is a trap worth knowing.**
      It puts its centering into `drawinfo.start_offset`, and in hardware mode **`V_DrawString`
      does not apply that** — it positions text as `x * drawfont.fdupx0` with no start offset,
      while `V_DrawScaledFill` and the patches do apply it. The two therefore disagree by exactly
      `start_offset`. At full scale that is 43px and nobody noticed; with a half-width block it
      becomes 363px, which threw the ranking text clean off the left of the screen while the
      colour bars stayed put. The block is placed by explicit coordinates instead, so text and
      fills share an origin. **Any overlay mixing `V_DrawString` with fills or patches under
      `V_CENTERHORZ` has this bug latent in it.**
    - **The float and integer scales must be pinned equal too.** Fills and patches position by the
      integer `dupx0`, `V_DrawString` by the float `fdupx0`; 2 versus 2.13 slid a name 35px off
      the colour bar it belongs on — more than half the bar's width. `vid.fdupx/fdupy` are set to
      `(float)vid.dupx/dupy` for this overlay; 6% of scale is the cheaper loss.
    - Offsets are derived from pixels, not as multiples of `BASEVIDWIDTH`/`BASEVIDHEIGHT`: with
      `dup` rounded to 2 a 200 unit block is 400px tall in a 384px cell, so stepping a row by
      `BASEVIDHEIGHT` would push the lower row off the screen bottom.
    - **`WI_Draw_Ranking` takes a `y_limit`.** It used to stop on `y >= BASEVIDHEIGHT` — "do not
      draw past the bottom of the screen" — which is right for the intermission but wrong for a
      block offset into a viewport cell: a bottom-row cell starts at base y 277, already past 200,
      so it drew **one row and broke out**. Sorted highest-first, that left only the leader
      showing, which reads as "the players on 0 points are missing". The overlay passes
      `offy + BASEVIDHEIGHT`; every other caller passes `BASEVIDHEIGHT` and is unchanged.
      **Any drawing offset into a cell has to re-examine limits expressed in base units.**
    - Verified numerically at 1366x768, with fill and text positions printed side by side and
      required to match. Two views: both at x 522, rows at y 194 and 578 (cells 0..384, 384..768).
      Four views: both at x 180 and 864 (cells 0..683, 683..1366), rows at y 170 and 554. Only the
      dead player's view draws: killing player 0 produced a table for view 0 alone.
    - Only the 2x2 grid rescales. **The two-view split still draws full size art at halved
      positions**, deliberately: that layout is measured and working, and rescaling it would move
      everything. Verified byte-identical — `lowerbar_y` is still exactly 319 for the upper half.
    - Verified numerically at 1366x768, four views: cells 683x384, `lowerbar_y` 350 (top row) and
      734 (bottom), health x at 106/789, ammo at 499/1182 — every element inside its own cell,
      with `dup=2,2`.
  - **There are no dep files for most objects** (`svn1749/dep` holds about 16), so **a header edit
    does not trigger a rebuild**. This bit during this work: `console.o` kept a stale
    `gamecontrol` symbol and failed at link. After editing any header, `make clean && make`.

- **Player config for panels 3 and 4** (`m_menu.c`). Options → Player now lists four, and Options →
  Setup Controls four guided setups. **No menu graphics were needed**: that operator route is plain
  text (`"Player3 config >>"`). The `M_SETUPA`/`M_SETUPB` patches ("SETUP PLAYER 1/2") belong to the
  upstream *Two Player Game* and *Multiplayer* screens, which the lockdown hides from players, and
  those were left alone.
  - Very little had to change, because `M_SetupMultiPlayer_pind(pind)` already repointed every
    per-player cvar from the arrays. Panels 3 and 4 needed their own thin entry points and entries
    in `M_SetupMultiPlayer[]`, `M_Setup_P_Controls[]` and the three label tables, all widened to
    `MAXSPLITSCREENPLAYERS`.
  - `M_Guided_Start` and `M_Setup_P*_Controls` took `(pind == 0) ? gamecontrol : gamecontrol2`;
    they index `gamecontrol_pl[pind]` now.
  - **`cv_usemouse` stays `[2]`** — there are two mouse devices, not four — so the three mouse rows
    (Use Mouse, Mouse Move, Always MouseLook) are **hidden for panels 3 and 4** rather than
    inventing a `use_mouse3`. An arcade panel has no mouse anyway. The compiler caught this as an
    out-of-bounds read; the other per-player cvars had already been widened.
  - **Options → Setup Controls carries both rows per panel**: *Guided setup Pn* and
    *Player n Controls >>*. The guided setup only teaches the ten controls a standard six button
    panel needs, so a panel with more buttons binds the rest on the full per-action page — which is
    the whole reason the second row has to exist for 3 and 4, not just 1 and 2.
  - **`MControlMenu` is now addressed by position**, which it explicitly was not before, so its
    indices are named in an enum (`mcontrol_*`) and the lockdown uses those. Keep the enum in step
    with the array.
  - **The binding page names the panel it is editing.** Its header was
    `controls_player ? "PLAYER2" : "PLAYER1"`, so panels 3 and 4 both announced themselves as
    PLAYER2 — exactly the confusion this screen must not create while an operator is assigning
    buttons. It prints the number now.
  - Entries for panels that do not exist are hidden in **`M_Configure`**, keyed on
    `cv_localplayers`, for the same reason as `cv_twoplayer`: config.cfg is not loaded when
    `M_Init` runs.

- **Join screen** (`m_menu.c`, `M_Join_*`, `JoinDef`). After the skill is chosen and before the
  game starts, each control panel presses fire to be counted in. Laid out as the view grid it is
  about to become, so a player presses and watches **their own cell** claim itself.
  - **`cv_jointime`** ("jointime", default 20s, `CV_SAVE`) is the countdown, and
    **`cv_localplayers`** the panel count — both operator settings under **Options → Menu Options**
    beside `cv_twoplayer`, so only a `-devmode` session writes them. `jointime 0` or a single panel
    skips the page entirely and the game starts exactly as it always did.
  - **Which panel drives a player is a different question from which cell they occupy**, and
    conflating the two was a bug. `D_View_Cell(pind)` is the screen cell; `D_Panel_Of(pind)` is the
    physical panel, and `G_BuildTiccmd` indexes `gamecontrol_pl[]` by the latter. A lone player who
    joined at panel 3 gets the whole screen (cell 0) but must still be driven by panel 3's buttons
    — with one table the compact layout overwrote the panel with the cell, so joining at panel 3
    handed you player 1's controls and the panel you were standing at did nothing.
  - **Single Player starts on the first press** (`join_first_press_starts`), with no countdown to
    sit through: there is nobody else to wait for, and the page's value on that route is letting a
    lone player claim the panel they are standing at. Multiplayer still waits.
  - **One or two players get the big layout** — the whole screen, or the stacked halves — however
    far apart their panels are; only three or more use the 2x2. Keeping a player in their own
    panel's cell only earns its keep once the grid is in use anyway: with two players top and
    bottom there is nothing to be confused about, and a quarter screen each is a poor trade.
    `M_Join_Start` assigns cells by join order when `joined <= 2` and by panel otherwise.
  - **Both routes into a game must be hooked.** The cabinet's New Game menu offers *Single Player*,
    which ends at `M_ChooseSkill` → `G_DeferedInitNew`, and *Two Player Game → Start Game*, which
    goes through `M_StartServer` and issues its own command sequence. Hooking only the first left
    the two player route skipping the page entirely. `M_Join_Open` therefore takes a **completion
    callback** rather than game parameters, and each route supplies its own starter
    (`M_NewGame_Go`, `M_StartServer_Go`).
  - Nobody pressing starts panel 1 alone rather than dead-ending on the page. Use/open from a panel
    that is already in starts immediately, so a ready group need not sit out the countdown.
  - **The page does not appear until `cv_localplayers` is raised**, which is correct but reads as
    the feature being broken: it ships at 1, and one panel has nothing to ask. Set *Control Panels*
    under Options → Menu Options in a `-devmode` session, which is the only session that saves.
  - **Keys are read before `M_Cabinet_Menu_Key`**, in `M_Responder`'s `ev_keydown`. That
    translation turns every panel's buttons into the same cursor keys, which is exactly the
    identity this page needs; taken afterwards, every panel would look alike.
  - **The page owns a `join_active` flag rather than testing `currentMenu`.** `M_Clear_Menus`
    leaves `currentMenu` pointing at the page it closed, so a `currentMenu` test kept firing the
    countdown every tic *after* the game had started — `G_DeferedInitNew` once per tic, for ever.
  - Single Level sets `D_Set_Join_Count(1)` and resets the cells: it is a scored single player mode
    and must not inherit a join from an earlier multiplayer game.
  - **Clamp the cell to 0 when only one view is drawn.** A lone player at panel 2 holds cell 1, and
    unclamped that still set `row = 1`, offsetting the HUD into a half of the screen that is not
    being drawn.
  - Verified headless by driving the page through temporary console hooks: panels 1+3 give 2 views
    (stacked halves, cells 0/1); panel 2 alone gives 1 view full screen; panels 1,2,4 give 4 views
    at cells 0,1,3 with the bottom-left empty; all four give cells 0..3. Nobody pressing yields one
    player; `localplayers 1` and `jointime 0` both skip the page.

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
- **Start Game carries a "Bot Options >>" link**, directly under the Bots count — the number that
  page sets is the only thing it otherwise says about them. Same entry point as the Game Options
  one (`M_BotOption`), so there is a single implementation. `ServerMenu` is addressed by position
  (by the lockdown, and by `M_StartServerMenu`'s per-gamemode map row swap) and inserting a row
  shifted five indices, so its indices are now named (`server_*`) like every other menu this file
  indexes. Geometry checked: `ServerDef.y` is 40 and an `IT_STRING` row is `STRINGHEIGHT` 10, so
  with every row shown Server Name occupies 130..140 and the `IT_YOFFSET` Start still sits at 150.

- **The Net Options page is at x=48, not the 60 the other option pages use.** `M_DrawGenericMenu`
  writes the label at `x` and right-justifies the value at `BASEVIDWIDTH - x`, so a row has to fit
  in `320 - 2x`. Measured against the real `STCFN` lumps: "Deathmatch Type" is **117px** and the
  widest value `deathmatch_cons_t` can show is "Coop_weapons" at **96** — 213px of the 200 that
  x=60 leaves, so the value ran **13px back over the label**. (Upstream bug; `cv_deathmatch` is the
  only row on the page whose value is a long word.) Widening the page fixes it for every value
  rather than for whichever happened to be selected: at 48 the label ends at 165 and the widest
  value starts at 176, while the page's longest label — "Frag's Weapon Falling" at 152 — ends at
  200 against an On/Off value starting at 248. The cursor is drawn at `x + SKULLXOFF` (-32), so 16,
  still on screen. **Shortening the label or the value strings was rejected**: `config.cfg` stores
  a cvar's *label*, so renaming those values would break existing configs.

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
  opening doors. Note the menu is still *opened* with Escape, which is not on the panel.
  - **Applied in devmode too, with text entry excluded.** There is no longer a devmode exemption:
    a leverless/hitbox panel is *just a keyboard*, so exempting keyboard keys left such a panel
    unable to work the menus while an operator was configuring it. The guard is now on the focused
    item being an **`IT_KEYHANDLER`** (the name and skin fields), because that dispatch happens far
    later in `M_Responder` — without it, "use" on `a` would exit the menu instead of typing an A.
    The cost is that in devmode a letter bound to a control no longer reaches the letter-shortcut
    search, since the translation claims it first.
  - Skipping joystick keys in devmode had left the panel at the mercy of **upstream's hardcoded
    `KEY_JOY0BUT0` → `KEY_ENTER` and `KEY_JOY0BUT1` → `KEY_BACKSPACE`** (`M_Responder`'s
    virtual-key `switch`), which selects and cancels by button *index*.
  - That upstream mapping is why **a stick's mode switch changed which buttons work the menus**. On
    a Mayflash F300, DirectInput/PS3 happened to put fire and use on buttons 0 and 1 and so looked
    correct, while XInput put A and B there — so A selected, B cancelled, and the operator's own
    fire button did nothing. Nothing was wrong with the bindings; the hardcoded indices simply
    pointed at different physical buttons.
  - **Bindings are stored by button index, so they do not survive that toggle.** Switching between
    XInput and DirectInput renumbers the buttons, and every `setcontrol`/guided-setup binding then
    refers to a different physical button. Pick a mode and re-run the guided setup in it.
  - `M_key_is_control` checks `gamecontrol[]` *and* `gamecontrol2[]`, so either player's panel
    drives the menus.
- **Menu letter shortcuts are disabled** outside devmode (`m_menu.c`, `M_Responder`'s `default:`
  case returns before the `alphaKey` search). The cabinet is buttons-only and several of those
  buttons are letters that collided with the shortcuts — player 1's turn-right button (`e`) on the
  New Game menu jumped to END GAME, one Enter from ending the run now that prompts are gone. Done
  at the dispatch point rather than by clearing `alphaKey` per menu, so it covers every menu.
  Text entry is unaffected *by this*, because the `IT_KEYHANDLER` dispatch (line ~6582) comes
  before the `default:` case. **Note the dispatch is not early in `M_Responder` generally** — an
  earlier version of this file said `IT_KEYHANDLER` items "consume the key earlier", which is only
  true relative to the letter shortcuts. Anything acting on the key *above* line 6582 sees it
  first, which is exactly the trap the cabinet menu translation had to be guarded against.
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
- **Cheats menu** (`m_menu.c`, `CheatsMenu`/`CheatsDef`, from a main menu entry using the locally
  added **`M_CHEATS`** graphic). Operator convenience: God Mode (`god`), All Weapons and Keys
  (`gimme health ammo armor keys weapons`, i.e. IDKFA), No Clipping (`noclip`) and Exit Level
  (`exitlevel`). Each issues the ordinary console command through `COM_BufAddText` rather than
  touching `player_t` directly, so there is one implementation of each cheat.
  - **Inserted at MainMenu index 4, before Read This**, giving `MM_cheats = 4`, `MM_readthis` 5 and
    `MM_quitdoom` 6. Before it, *not* after: the Doom 2 fixup
    `MainMenu[MM_readthis] = MainMenu[MM_quitdoom]; numitems--` copies Quit over the Read This slot
    and drops the last row, so anything appended past Quit would be cut off under Doom 2. The
    lockdown's Load/Save hiding at indices 1,2 is unaffected. Those are the complete set of index
    references — see the `grep` list under Single Level mode, which uses the same discipline.
  - Devmode only by default, hidden by the usual `IT_HIDDEN` treatment with `MainDef.lastOn` moved
    off it — but an operator can leave it up for players with **`cv_cheatsmenu`** ("cheatsmenu",
    default Off, `CV_SAVE`), under **Options → Menu Options** beside `cv_twoplayer`. A cabinet at a
    party is not the same machine as a cabinet keeping scores; cheating voids the run either way.
    - **The hiding therefore lives in `M_Configure`, not `M_Init`'s lockdown**, for the same reason
      as `cv_twoplayer` and the game selector: `config.cfg` is not loaded until long after `M_Init`
      runs, so the cvar would still read as its compiled default there. The condition is
      `! devmode && ! cv_cheatsmenu.EV`.
    - Being an operator setting, only a `-devmode` session saves it — a player cannot switch it on
      for themselves.
  - **Single player only** (the user's requirement). `Command_CheatGod_f` and `Command_CheatGimme_f`
    already `return` when `multiplayer` is set, so the engine enforces it; `M_Cheats_Usable()`
    additionally greys the items out when there is no single player level running, rather than
    offering a row that silently does nothing. The page footer says which case it is.
    - **That made this the first menu with *no* selectable item, which hard-locked the program.**
      `M_Responder`'s KEY_UPARROW/KEY_DOWNARROW handlers step the cursor in a
      `do … while(status & IT_TYPE) == IT_SPACE` loop, searching for something selectable —
      unbounded, so with every row `IT_DISABLED` (which *is* an `IT_SPACE` type) it spins for ever
      inside the event handler. Opening Cheats from the attract screen and pressing down froze the
      cabinet with no way out. Both loops are now bounded by `numitems` and leave the cursor where
      it was when nothing is selectable. **This is upstream code and a latent trap for the whole
      lockdown**: `IT_HIDDEN` is `IT_SPACE` too, so any menu the lockdown hides entirely would have
      done the same. `M_SetupMenu`'s own walk was already bounded (`&& itemOn`) and is unaffected.
  - **God Mode and No Clipping show their state, and do not close the menu.** Both are *toggles* —
    `Command_CheatGod_f`/`Command_CheatNoClip_f` XOR `player->cheats` — so a row reading only "God
    Mode" gave no way to tell an armed cheat from a disarmed one. `M_Draw_Cheat_State` draws On/Off
    where `M_DrawGenericMenu` puts a cvar's value (right justified at `BASEVIDWIDTH - x`), reading
    `players[consoleplayer].cheats`, and only while a level is running — which is exactly when the
    rows are not greyed out. `M_Cheat_Apply` gained a **`close`** flag, false for those two: closing
    the menu on a toggle made the state it had just set unreadable without reopening the page. The
    two one-shot rows (All Weapons, Exit Level) still close it. `CheatsMenu` is now addressed by
    position, so its indices are named (`cheat_*`).
  - **Using any cheat voids the run's score**, via `HS_Player_Cheated()` (`hs_stuff.c`), modelled
    exactly on `HS_Player_Died`: it latches `hs_run_cheated`, clears `hs_run_ranked` so the existing
    early return in `HS_LevelExit` stops all further scoring, and closes the background recorder
    with `G_CheckDemoStatus`. Guarded on `netgame || multiplayer || deathmatch`, which are not
    scored anyway. The HUD marker becomes **`PLAYER CHEATED - UNRANKED`** (`hu_stuff.c`), which
    takes priority over `PLAYER DIED - UNRANKED` when both happened — the cheat is the thing the
    player chose to do.
  - **The hook is in the cheat commands, not the menu**, so the console (`god`, `noclip`, `gimme`)
    and the **typed cheat codes** are covered too. For the typed codes the single hook point is
    `cht_Responder`'s closing `if (msg)` block — every cheat that changes the simulation reports
    through `msg`, and the three that do not (IDDT, IDMYPOS, IDMUS) do not affect play, so they
    still score. `M_Cheat_Apply` calls it as well because **`exitlevel` is not a cheat command** and
    would otherwise skip the rest of a map for free; the flag is latched, so the double call is
    free.
  - Verified headless with two otherwise identical `-warp 1` autoexec runs differing only by a
    `god` line: the control wrote `doom2 MAP01 2 104 speed`, the cheated run wrote nothing.
  - `hs_run_cheated` is reset in `HS_NewGame` beside `hs_run_died`.
- **The attract cycle keeps running behind an open menu** (`d_main.c`, `D_AdvanceDemo`). Upstream
  refused to advance the sequence while `menuactive` (*"do not start a demo when a menu or console
  is open"*), which on a cabinet stalls the thing whose entire job is to advertise the machine: open
  Options and the attract screen freezes on whatever page it was showing. It did not merely pause —
  `D_PageTicker` decrements `pagetic` every tic regardless of what is on screen, so once it passed
  zero it called `D_AdvanceDemo` on *every* tic and every one was discarded. Measured with a menu
  opened at the title: `pagetic` ran 170 → **-495** while `demosequence` never left 0, then jumped a
  page the instant the menu closed.
  - Only the *start* of a page was ever blocked; a demo already running when the menu opened kept
    playing. That is why the idle timeout's menu case has always had to reason about "an attract
    demo running behind an open menu" — the two are consistent now.
  - **`console_open` is deliberately kept.** Someone typing at the console is working on the
    machine, not watching it.
  - This is what "the Options screen doesn't go back to attract mode" actually was. The **idle
    timeout itself was working**: instrumented at the title with the Options page up, it fired at
    exactly `cv_idletimeout * TICRATE` (700 at 20s, 2100 at the cabinet's 60s) and closed the menu.
    Do not go looking for a second bug there.

- **Idle-to-title timeout** (`g_game.c`, `G_Idle_Timeout_Check`). `last_input_tic` is stamped in
  `D_PostEvent`, checked once per tic from `G_Ticker`, and re-armed in `G_DoLoadLevel` (but
  **not during `demoplayback`**, see below) so intermission time does not carry over. Ends the
  game via `Command_ExitGame_f()` and warns
  beforehand through the existing `HU_SetTip` centered-text mechanism, or restarts the program when
  a level pack is loaded (see the game selector). Tunable via `cv_idletimeout` / `cv_idlewarntime`
  (default 60s/15s, `0` disables). Skipped in devmode and demo playback. **Local splitscreen sets
  `netgame`**, so the check tests `(!netgame || cv_splitscreen.EV)`; gating on `!netgame` alone
  meant no two player game ever timed out, which is exactly when an unattended cabinet needs it.

  It also runs in **`GS_DEMOSCREEN` when `menuactive`** — a menu left open on the attract screen
  would otherwise sit there for ever, since the states below only cover an abandoned *game*. That
  case takes a `in_menu` argument and differs twice:
  - The **`demoplayback` early-out is skipped**. A demo playing on its own *is* the attract screen
    working normally, so playback usually means "not idle" — but an attract demo is still running
    behind an open menu, so with that guard in place the menu case could never fire.
  - The action is just `M_Clear_Menus()` plus clearing `single_level_mode`, not
    `Command_ExitGame_f()`: there is no game to tear down and the attract cycle is already running
    underneath. `last_input_tic` is re-armed so it cannot re-fire on the next tic.
  - No countdown warning is shown for this case: `D_Display` only calls `HU_Drawer` for `GS_LEVEL`,
    so `HU_SetTip` would draw nothing on the attract screen.
  - **`GS_DEMOSCREEN` is not enough on its own.** While an attract *demo* is playing behind the
    menu the gamestate is `GS_LEVEL`, not `GS_DEMOSCREEN`, and the plain check bails on
    `demoplayback` — so the menu only timed out during the attract screen's *title* pages and
    looked broken whenever a demo happened to be on. The `GS_LEVEL` call therefore passes
    `demoplayback && menuactive`. A real game with the menu open has `demoplayback` false and still
    takes the normal path, ending the game rather than merely closing the menu.
  - **It does nothing in devmode**, like every other idle timeout (`if( devmode || ... ) return`).
    An operator session will never see a menu time out; that is intentional, but it is the first
    thing to check when it "doesn't work".
  - **The `G_DoLoadLevel` re-arm had to be skipped during `demoplayback`**, or the menu case could
    not fire at the real 60s default. Every attract demo starts through `G_DoLoadLevel`, and the
    attract cycle starts one roughly every 30-45s, so each one reset `last_input_tic` and the count
    never reached 2100 tics — a menu left open on the attract screen sat there indefinitely. The
    earlier `idletimeout 8`/`10` verification passed only because those fire before the first demo
    reload. Re-arming is pointless during playback anyway: the check's own `demoplayback` guard
    means the only case that gets there is the open menu.
  - Verified headless at `idletimeout 20` and at the cabinet's `60`, with `in_menu` forced true to
    stand in for an open menu: fires at exactly 700/1400/2100/2800/3500/4200 (20s) — including
    across a demo load at 2250 — and at 2100 (60s). The same build with the re-arm unconditional
    never fired in 115s at 60, which is the reported symptom exactly.

  It runs in **`GS_LEVEL`, `GS_INTERMISSION` and `GS_FINALE`**, not just during play — both of the
  other two wait *indefinitely* for a keypress the walk-away player never gives, so covering only
  `GS_LEVEL` left the cabinet hung. The intermission stalls at `sp_state == 10` (`wi_stuff.c`)
  once the counters finish; the finale stalls in `F_Ticker`'s `finalestage 0` (the Doom 2 text
  screens need `keypressed`) and again in the cast call, which loops forever. `D_Display` calls
  `HU_Drawer` for `GS_LEVEL` **only**, so the countdown would not have been visible in the other
  two states — `HU_Draw_Tip` was un-`static`ed (declared in `hu_stuff.h`) and is called directly
  after `WI_Drawer`/`F_Drawer`. **Anything drawn by `HU_Drawer` has this same limitation.**
- **Screenshots** are on **F12** (`gc_screenshot`), with PrtSc as the second binding. The stock
  default was SysRq (Alt+PrtSc), a modifier combination; and **PrtSc itself never reaches the game
  under GNOME**, whose own screenshot tool takes it first. F12 costs the engine's hardcoded "spy
  mode" on that key (`G_Responder`) — `M_Responder` consumes the screenshot key first — which is
  single-player-only and of no use on a cabinet.
  - **`M_Responder` only tested `gamecontrol[gc_screenshot][0]`**, so a second key assigned to
    Screenshot silently did nothing. It checks both slots now.
  - **`KEY_PRINT` had no entry in the key name table**, so it could not be bound or saved by
    `setcontrol` at all; it is `"print"` now. Changing the compiled default is not enough on its
    own — `config.cfg` carries a `setcontrol "screenshot"` line that is loaded afterwards and wins,
    so both the tracked and live configs were updated to
    `setcontrol "screenshot" "print" "sysreq"`.
  - Written to the **current working directory** as `DOOM0000.tga`, counting up to the first
    unused number (`HRTC`/`CHXQ` for Heretic/Chex). Targa because the cabinet runs OpenGL; the
    software renderer writes `.pcx`. `screenshotdir` redirects it, and only then is the file named
    after the first loaded PWAD instead of the game.

- **Analog joystick axes are translated to hat keys** (`sdl/i_system.c`, the `SDL_JOYAXISMOTION`
  case). Upstream produced **no bindable input from any analog axis**: the only handling was for
  Xbox triggers, gated on `check_Joystick_Xbox[]`, which requires the joystick's *name* to match
  one of two literal strings (`"Xbox 360 Wireless Receiver (XBOX)"` / `"Microsoft X-Box 360 pad"`)
  and covers only axes 2 and 5. An arcade stick in analog mode was therefore completely dead, while
  the same stick on its d-pad setting worked, because `SDL_JOYHATMOTION` *is* translated
  generically. Diagnosed on a Mayflash F300, which enumerates as `Generic X-Box pad` — 6 axes,
  11 buttons, 1 hat — and so fails that name test.
  - Axes 0 and 1 now emit the **same `KEY_JOY0HAT*` codes as the hat**, via the existing
    `Translate_Joyhat`. That is the point: analog and d-pad modes become interchangeable and every
    existing binding keeps working. Emitting *new* key codes would have needed rebinding.
  - **It has to be key events, not `bindjoyaxis`.** The engine does have an analog axis path
    (`bindjoyaxis`, `joybinding_t`, `ja_move`/`ja_turn`/`ja_strafe`/`ja_pitch`), but those drive
    movement only. Menu navigation (`M_Cabinet_Menu_Key`) reads `gamecontrol[]` *by key*, so an
    analog-bound stick could never move the menu cursor.
  - The `case` was moved **out of** `#ifdef XBOX_CONTROLLER`. It was previously inside it, so
    without that define there was no `SDL_JOYAXISMOTION` case at all.
  - `previous_jaxis[joystick][axis]` latches the current direction so jitter inside a direction
    does not re-post keydowns, and a straight left-to-right flick releases the old direction before
    pressing the new one. Verified against a synthetic trace (jitter, flick, diagonal, deadzone):
    no repeated keydowns, no orphan keyups, nothing left stuck.
  - Deadzone is half range (`JOYAXIS_DEADZONE` 16384). An arcade stick reports full deflection, so
    this only has to clear noise; it also stops a diagonal registering until genuinely committed.
  - Only axes 0/1. Axes 2 and 5 are the triggers, handled above and `break`ing early so a trigger
    is never read as a direction; the right stick (3/4) is left alone.
  - **The triggers had the same disease and the same cure.** `LT`/`RT` arrive as axes 2 and 5, and
    their handler was gated on that identical `check_Joystick_Xbox[]` name test, so they were dead
    on the F300 too. The gate is gone; the axes are read on any joystick. That is safe because
    `KEY_JOY0LEFTTRIGGER`/`RIGHTTRIGGER` are distinct codes, unbound by default, and already have
    `setcontrol` names (`Joy0 lt`, `Joy0 rt`) — so a pad using those axes for something else costs
    nothing unless a player binds them.
    - Threshold is `> 0`, **not** `JOYAXIS_DEADZONE`: triggers rest at full *negative* on the Linux
      `xpad` driver and at zero on others, so positive means pressed under both conventions.
    - `previous_jtrigger[joystick][2]` latches the pressed state. Upstream posted a keydown on
      *every* axis event while a trigger was held past the threshold, which is a stream of them for
      an analog trigger; now only transitions post. Verified against both resting conventions, with
      LT and RT independent.
    - `check_Joystick_Xbox[]` is now **assigned but never read**. Left in place (it is upstream
      code, and harmless) rather than removed.
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
  each to whatever is pressed — a 4-way stick plus six buttons. Everything else stays on the
  ordinary Setup Controls pages. Built on the same `MM_EVENTHANDLER` message plumbing as
  `M_ChangeControl`, so the message box, key capture and event routing are shared; mouse and
  joystick buttons arrive as `ev_keydown` with codes in the key space, so any panel wiring works.
  - Order is **stick first, then the buttons by number**, each labelled with it:
    `STICK UP / DOWN / LEFT / RIGHT`, then `BUTTON 1 - FIRE`, `BUTTON 2 - STRAFE LEFT`,
    `BUTTON 3 - STRAFE RIGHT`, `BUTTON 4 - USE / OPEN`, `BUTTON 5 - WEAPON DOWN`,
    `BUTTON 6 - WEAPON UP`. **Keep this in step with the recommended layout page** — the two are
    meant to be read together, and an earlier version asked in a different order (strafe before
    fire) while the page claimed otherwise. The numbers are only the recommendation; a panel wired
    differently still works, the operator just presses what they want for that action.
  - **Recommended layout screen** (`M_Draw_RecLayout` / `RecLayoutDef`, same menu). An
    informational page showing the play-tested assignment on the standard six-button arrangement
    (1 2 3 top row, 4 5 6 bottom): 1 fire, 2/3 strafe left/right, 4 use/open, 5 weapon down,
    6 weapon up, stick moves and turns. Modelled on the "Read This" screens — a custom
    `drawroutine` plus one `IT_SUBMENU | IT_NOTHING` item that backs out.
  - **The guided setup opens on that page**, showing "PRESS ANY BUTTON TO BEGIN", so the operator
    sees the target layout in the order they are about to be asked for it. `guided_intro` gates
    the footer text, and `M_Responder` consumes the next `ev_keydown` **before** the generic menu
    handling, so the page's own invisible item cannot swallow the press; ESC falls through to the
    normal back-out. It has to be a separate screen rather than something drawn behind the
    prompts, because `M_StartMessage` makes `MessageDef` the current menu.
  - Both wizard exits (finished, and ESC part way) call `M_Guided_Leave_Intro_Page()` to
    `Pop_Menu()` that page off the stack first. `M_StartMessage` records
    `message_menu_back = currentMenu` and `M_StopMessage` returns there, so without the pop the
    closing "Controls saved" message drops the operator back onto the layout diagram instead of
    the controls menu. **Any flow that opens a page and then raises messages has this same
    trap.**
  - **`hu_font` is proportional and space is only 4px**, so monospace ASCII art does not line up:
    `V_DrawString` advances by each character patch's own width. The layout screen therefore
    positions every element at an explicit x (`M_Centre_At`) instead of padding with spaces. Only
    `'!'`..`'_'` exist and lowercase folds to uppercase — there is **no `|`** — which is why the
    stick's down arrow is the letter `V`. Extents were checked against the real `STCFN0xx` widths
    read out of the IWAD rather than estimated; the widest line is 214px of 320. Re-check with a
    script that reads those lumps if any string here changes — do not eyeball it.
  - The three new entries sit at the **top** of `MControlMenu`, which is safe only because nothing
    indexes that menu by position and `MControlDef.lastOn` is never assigned (verified) — so the
    cursor lands on "Guided setup P1", which is the point. The rest of the lockdown does address
    menu items by hardcoded index, so this is the exception, not the pattern.
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

  **A death voids the rest of the run** (`HS_Player_Died`, called from `P_KillMobj` in `p_inter.c`
  right after `playerstate = PST_DEAD`). Levels already finished keep the records they earned —
  each was written to disk by its own `HS_LevelExit` before the fatal level was entered — but
  nothing after the death scores, and the background recording is closed with `G_CheckDemoStatus`
  since it can no longer produce anything. The HUD marker becomes **`PLAYER DIED - UNRANKED`**
  (`hu_stuff.c`); `UNRANKED` alone reads as a settings problem, and the player needs to know the
  retry they are about to play is not being scored.
  - Implemented by clearing `hs_run_ranked`, exactly as an altered ruleset does, so the "no further
    scores" half falls out of the existing early return in `HS_LevelExit`. The separate
    `hs_run_died` flag only selects the *reason* shown and logged — it is not a second gate.
  - Hooked at the moment of death rather than at `G_DoReborn`, so a player who dies and walks away
    still voids the run and releases the demo buffer.
  - **Guarded on `demoplayback`** like `HS_LevelExit` is. Attract-mode record demos can contain a
    death, and replaying one must not void the cabinet's live run or close its recorder.
  - This closed a real hole rather than just adding a rule. Doom traditionally lets you retry a
    level, and the engine does that by *reloading* it — `G_DoReborn` → `G_DoLoadLevel(true)` →
    `P_SetupLevel`, which sets `leveltime = 0`. Since `HS_LevelExit` accumulates `leveltime`, the
    failed attempt used to cost the score **nothing at all**, making death a free reset button.
    Scores set before this change may include death-assisted runs; the cabinet's table was left in
    place deliberately (judged clean), so `clearhighscores` was **not** run.

  Both tables are laid out by hand against the surrounding graphics, so **the numbers matter**.
  `HS_Draw_IntermissionTable(x, y)` draws its column header 14 *above* the `y` it is passed, and
  right-justifies each category's times at `x + HS_COL_TIME + cat*HS_COL_STEP` (**108 and 72**), so
  the last column's right edge is `x + 180` and must stay inside `BASEVIDWIDTH` (320). On the single
  player intermission the free band is only what is left between the Secrets row and the Time row:
  rows start at `SP_STATSY` (50) and are `lh` apart, where `lh` is 1.5× the `WINUM0` patch height
  (12) = **18**, and the percent patches are 12 tall — so Secrets ends at 98 and `SP_TIMEY` is 168.
  The call site's `+ 12` centers header-plus-five-rows in that 70px gap. **If a row is ever added
  or the font changes, re-check both ends** — the drawer only bails at `BASEVIDHEIGHT`, so it will
  happily paint over the Time/Par row.

  A blinking **`NEW RECORD`** is drawn by the same function when the level just finished beat a
  time. `hs_new_record[HS_NUMCAT]` records which categories were beaten; it is cleared at the top
  of every `HS_LevelExit` — *before* the unranked/died early returns, so a voided exit cannot leave
  the previous level's marker on screen — and set alongside each `saved = true`. One marker covers
  both categories.
  - **It went in the horizontal space, not the vertical**, because the band above is already full:
    header-plus-rows spans `y-14 .. y+48` (102..164 at the call site's y of 116) inside a free band
    of only 98..168. The table itself occupies just `x .. x+180` (**138..318**), so everything left
    of `x` in that band is free. The text is centred there and on the table block's midpoint
    (`(y-14 + y+48)/2 = y+17`, glyphs 8 tall, so top edge `y+13`) — landing at x=30, y=129.
  - Width comes from `V_StringWidth` at runtime, not the measured 77px, so the centering follows
    the string if it is ever reworded. The 77px was read from the real `STCFN0xx` lumps, per the
    rule above about not eyeballing this font.
  - Blink is `gametic & 16`, matching the cadence `wi_stuff.c` uses for its own flashing "you are
    here" pointer. `gametic` (not `leveltime`) because only `gametic` advances during the
    intermission; `bcnt`, which `wi_stuff.c` blinks from, is static to that file.
  - Drawn with option **`0`, which is red** — it was white at first and disappeared against the
    grey intermission background. See the `V_DrawString` colour note below.

  **Cumulative run time** is shown under the intermission's Time row by `HS_Draw_TotalTime`, as
  `TOTAL` plus `hs_cumulative_time`. It already includes the level just finished — `WI_Init_Stats`
  calls `HS_LevelExit`, which adds that level's `leveltime`, before the intermission draws. Shown
  whatever the run's state: an unranked or death-voided run still has a meaningful elapsed time.
  - Layout, all measured: `WITIME` and the `WINUM` digits are both **12** tall and start at
    `SP_TIMEY` (168), so that row ends at 180; the call site's `+ 16` leaves a 4px gap, putting the
    text at 184 and — `hu_font` glyphs being 7 tall — its bottom at 191 of `BASEVIDHEIGHT` 200.
    The caption sits at `SP_TIMEX` (16) spanning 16..56, and the value is right-justified at
    `BASEVIDWIDTH/2 - SP_TIMEX` (144), at most 39px wide for a `123:45`, so it starts no further
    left than 105. Only the Par row shares that band and it is right of centre.
  - Drawn in **option 0 (red)**, not `V_WHITEMAP` — this screen's background is largely grey and
    white text vanishes into it, the same lesson as the `NEW RECORD` marker.
  - `hs_cumulative_time` is reset by `HS_NewGame`, which only the menu skill-select handlers call.
    A game started from the command line (`-warp`) never resets it, so the total can carry over
    from a previous run in that case. Not reachable from the cabinet's menus, but worth knowing
    when testing.

  The **wad combination** is `HS_GameId()`: the game's short name plus any loaded level pack, such
  as `doom2` or `doomu+mapsofchaos`. Both parts are needed — Doom 2, Plutonia and TNT all have a
  `MAP01` that is a different level, and a pack replaces the maps again. Record demos are named
  with the same key, and only the current combination's records and demos are shown or replayed
  (a demo from another combination would desync instantly). The key is recomputed per use, since a
  pack can be loaded mid-session, and sanitized to one filename-safe word because it is a space
  separated field in `highscores.dat` *and* part of the demo filename, while pack names come from
  arbitrary filenames. Renaming a wad therefore starts a fresh table — the name is the identity.

  Record demos are **captioned** during attract playback (`HS_DemoLabel()`, drawn by `HU_Drawer`)
  with the **map range**, skill, category and time — `E1M1-E1M5  ITYTD  MAX  4:32.17`.
  `HS_NextRecordDemoPath()` fills the label as a side effect of handing out a path, and
  `D_DoAdvanceDemo` clears it before every page, so a stock IWAD demo is never captioned with the
  previous record's text. Drawn on the **second** text line (y=8), because item pickups still print
  messages at y=0 during playback.
  - **The range is the point.** These times are cumulative from the start of a run, so the demo
    saved against the E1M5 record is a *five level* run. Captioned with the bare map name it was
    indistinguishable from a single level one, which badly undersold the longer runs. The range
    comes from `hs_maprecord_t.startmap[cat][skill]`, stored per record rather than inferred from
    the episode: every menu-started campaign does begin at map 1, but that is a property of the
    menus, not of the record.
  - A record whose `startmap` equals its map is captioned with the bare map name — no invented span.
  - **A campaign record with no stored `startmap` falls back to an inference** (`HS_Infer_StartMap`,
    used through `HS_Format_Range`): the first map of that map's own episode, so `MAP03` reads
    `MAP01-MAP03` and `E2M3` reads `E2M1-E2M3`. Episode-aware, not always `E1M1`.
    - This was deliberately *avoided* when the field was added, on the grounds that starting at map
      1 is a property of the menus rather than of the record. That was over-cautious for records
      written before the field existed: for those the choice is not between a fact and a guess but
      between a good guess and nothing, and a cumulative time at MAP03 can only have come from a
      run that began at MAP01, since that is the only way to accumulate time there.
    - **A stored start map always wins**, so this never overrides a recorded fact and quietly stops
      mattering as old records are beaten.
    - Single level records are excluded — one map by definition — so they stay bare.
    - Secret levels need no special case: reaching MAP31 or E1M9 still means a run from MAP01 or
      E1M1.
    - The one case it cannot describe is a run started mid-episode with `-warp`, which no route
      through the cabinet's menus produces.
  - **It still fits on one line.** Measured against the real `STCFN` lumps, the widest either form
    reaches is `SINGLE LEVEL: MAP01  ITYTD  SPEED  888:88.99` at **295px of 320** — a single level
    run is one map so it never carries a range, and a range costs less width than the
    `SINGLE LEVEL: ` prefix does.

  A blinking **`PRESS FIRE TO START`** is drawn over the same demos (`hu_stuff.c`, `HU_Drawer`) —
  the arcade "insert coin" on a machine that takes no coins. It is only a prompt: **`G_Responder`
  already pops up the menu on any keypress while a demo plays** (`g_game.c`, the "any other key
  pops up menu if in demos" branch), so fire has always started the way into a game and nothing on
  the attract screen said so.
  - Shown on `demoplayback && ! menuactive`, so it is not painted behind an open menu. That also
    means it appears over a Single Level **"watch run"** replay, which is harmless — fire ends the
    replay and opens the menu there too.
  - Blink is `gametic & 16`, the same cadence as the intermission's `NEW RECORD` marker.
  - **Placed clear of the status bar in either of its forms**, since a demo is drawn at whatever
    viewsize the cabinet is set to: the classic bar's top is `BASEVIDHEIGHT - ST_HEIGHT` = 168 and
    the overlay's lower row is at 182 (`st_stuff.c`'s `SCY(198) - 16`), so the text goes at
    **160**, spanning 160..167 for `hu_font`'s 7-tall glyphs. Width measured from the real `STCFN`
    lumps: **133px of 320**, centred by `V_StringWidth` so it follows any rewording.
  - It does cross the **weapon sprite**, which draws up the middle of the screen in every mode.
    Unavoidable for anything centred that low — the same constraint that pushed the HUD clock off
    centre — and the blink is what keeps it readable against the moving demo.
  - Only the demos carry it. The title and `CREDIT`/`CREDIT2` pages are `GS_DEMOSCREEN`, which
    `D_Display` does not call `HU_Drawer` for; they draw through `D_PageDrawer` and would need
    their own call.

  **The attract cycle is a fixed four steps** (`d_main.c`, the `attract_*` enum): title → `CREDIT`
  → `CREDIT2` → one demo, repeating. It replaces the stock 6-step sequence (7 under the retail
  divisor), which showed `CREDIT` only once per three demos and filled the rest with the help and
  order pages. `demosequence` is read outside `D_DoAdvanceDemo` — `D_PageDrawer`'s Heretic raw-screen
  hack tests it — which is why the steps are named rather than numbered.
  - `CREDIT2` (in `legacy.wad`, 320x200 like the stock `CREDIT`) runs for `CREDIT2_SECS`. Its step
    is **skipped when the lump is absent**, so a `legacy.wad` without it still cycles cleanly.
  - The stock demo used when no record demo exists now **rotates** `demo1`/`demo2`/`demo3`, since
    the single demo step would otherwise always play `demo1`. `demo4` (retail only) is dropped.
  - The high score page is still interposed after each demo, on its own flag.
  - **Keep splash art at 320x200.** `D_PageDrawer` ends at `V_DrawScaledPatch_Name`, which
    multiplies by `vid.dupx/dupy` — a 960x720 patch would be drawn at 3840x2160 and only its
    top-left corner would be on screen.

  The attract page appears **after every demo**, skipped when the current combination has no times
  (`HS_Have_Records()`) so a fresh cabinet does not show an empty page repeatedly. It is interposed
  in `D_DoAdvanceDemo` via a flag rather than added as a `demosequence` case, because those cases
  are shared between game modes and the last is reachable only under the retail divisor.

  **Single Player scoring is Survival** (`HS_Survival_Entry`, `HS_Episode_Of`). A campaign run is
  scored on **how far it got in the episode, tie-broken by time** — not on per-map cumulative
  times, which was the confusing part of the older scheme and is gone entirely.

  - The key is **(game, episode, skill, category)** and the depth is **1**. `HS_Episode_Of` is what
    makes it an episode: the board used to be keyed by game alone, which let `HS_MapOrder` compare
    *across* episodes. On the cabinet's own table two one-minute E2M1 attempts (order 201)
    outranked two **completed** Episode 1 runs (E1M8, order 108). "Furthest" only means anything
    within one episode. Doom 2 and the other flat `MAPxx` games are one episode — the whole game
    is the run.
  - **No per-map campaign record is kept any more.** `HS_LevelExit` freezes the run's state and
    nothing else; the campaign half of `highscores.dat` is dead weight from older versions and is
    simply never written or read. Single Level times are untouched, including a campaign first
    level still competing there.
  - **The demo is snapshotted at each scored level exit while the run leads its board**
    (`HS_Snapshot_If_Leading`), not at the end of the run. It has to be: a **death closes the
    recorder** (`HS_Player_Died`), and under Survival a run that died still scores — on how far it
    got. Snapshotting as it goes means the file always holds the leading run up to its last scored
    exit. One demo per (game, episode, skill, category), named `..._ep<N>_sk<M>_<cat>.lmp`.
  - The intermission shows the **episode record and where this run stands against it**, replacing
    the old five-row per-map table. `NEW RECORD` can now blink *during* a run rather than only at
    the end, because under Survival being ahead is knowable: get past the holder's furthest map and
    you already lead.
    - **The block ends in the initials, and they are 27px wide, not 24.** `HS_Draw_IntermissionTable`
      puts them at `x + HS_IM_INI_X` (158), and the block was measured with `AAA` — 8px a glyph,
      24 in all, ending exactly on `BASEVIDWIDTH`. `M` and `W` are 9px, so real initials like `MLR`
      (25) or `MMM` (27) ran to 323 and the last letter was cut off by the right edge. The
      `wi_stuff.c` call site x is **128**, not 138: the block spans 128..313 and `NEW RECORD`, which
      centres in the space to its left, still fits at 23..100. **Measure a variable-width field at
      its widest glyphs, not at `A`** — the same rule as the rest of the `hu_font` layout work.

  **The pages are enumerated, not numbered** (`HS_Build_Pages`, `hs_page_t`). Three kinds:

  | kind | one page per | content |
  | --- | --- | --- |
  | `HSPG_survival` | episode | the best run per skill and category, both categories side by side |
  | `HSPG_single` | (skill, category) | single level best times, whole map list, with initials |
  | `HSPG_slmap` | one slot | one map's single level top three, by difficulty |

  A page is only enumerated when it would have something on it. The list is rebuilt on every query
  because the tables grow during a session.

  - **Survival page layout**, measured against the real STCFN lumps. The skill label is written
    once and the two category blocks sit beside it: `"ITYTD"` 36px at 8, then blocks at 52 and 186,
    each `"E1M8 12:34.56 MLR"` at 122px — 308 of 320, and 316 with a three-digit-minute time. A
    per-episode board is what buys this; the old per-map page could not fit two categories at all.
    Rows are skills, so Doom 1 gets up to four pages and Doom 2 exactly one for the whole game,
    with no adaptive special-casing.
  - **An appearance shows `HS_PAGES_PER_CYCLE` (4) pages**, and the cursor is deliberately not
    reset between appearances. Even, so the single level speed/max pairs sit whole inside an
    appearance — but **even is not sufficient on its own**: a skill with a speed record and no max
    contributes a single page and shifts the parity of every pair after it. So
    `HS_Attract_Cycle_Pages` checks the boundary directly (`HS_Is_Pair_Tail`) and extends the
    appearance by one page rather than splitting a pair across a demo.
  - **`HSPG_slmap` is one slot, not one page per map.** A different map comes up each time it
    appears; enumerating a page per map would have put ten-plus near-identical pages in the
    rotation.
    - **The cursor is stepped by `HS_Attract_Advance_Page`, never by the drawer.** Advancing it in
      `HS_Draw_SL_Map_Page` made the page flicker through every map at frame rate, because
      **a drawer runs once per frame, not once per page** — anything it mutates changes 35 times a
      second. `HS_SL_Current_Map` resolves the cursor read-only for drawing. This is the general
      rule for these pages: the drawers must be idempotent.
  - **The page block steps once more on the way out** (`D_DoAdvanceDemo`), or the page an
    appearance ended on would be the first page of the next one.
  - **Demos: the normal rotation is single level only.** A Survival demo is a whole episode — ten
    or twenty minutes — which would park the attract screen on one run. `HS_NextSurvivalDemoPath`
    hands one out instead when `HS_Attract_Rotation_Done()` reports a full pass of the score pages
    has finished, so the long run appears roughly once every several attract cycles rather than as
    the filler between pages.

  **The per-map single level page** (`HS_Draw_SL_Map_Page`) is laid out **difficulty x place**
  rather than a stacked block per difficulty, which reads in one glance: skill label at x=10, then
  three cells at 52, 139 and 226. Measured, a cell is `AAA 0:08.57` at 75px and at worst
  `AAA 29:59.99` at 83px for a slow max run of one map, so cells clear each other by 4px and the
  last ends at 309 of 320. The max block is drawn only when it has entries, or the page would be
  half a screen of blanks.

  - The skill is shown as the **New Game menu's own graphic** (`hs_skillpatch[]` —
    `M_JKILL`/`M_ROUGH`/`M_HURT`/`M_ULTRA`/`M_NMARE`, matching `ITYTD`/`HNTR`/`HMP`/`UV`/`NM`),
    not the short text name, so it reads from across a room. Falls back to the text if the lump is
    missing, since those are the Doom names and Heretic differs.
    - **Bottom aligned** on `HS_PG_SKILL_BOT` (**34**), not top aligned: the patches are 15..19
      tall (`M_NMARE` is the tall one), so aligning tops would make the baseline jump as pages
      cycle. Widths reach 248 (`M_ROUGH`), still centred in 320.
  - **`HS_PAGE_SECS` is 8.** Its cost is bounded now that an appearance shows at most three pages,
    so raising it is cheaper than it used to be.
  - **`HS_MapOrder()` sorts by the order a player actually reaches the maps, not by name.** Doom 2
    hides MAP31/32 behind MAP15's secret exit and returns to MAP16 afterwards, so the run is
    **15 -> 31 -> 32 -> 16**; plain numeric order would list them fifteen levels from where they
    are played. Doom 1 is episode major, map minor -- E?M9 is a secret level too but sits at the
    end of its episode, which is where numeric order already puts it.
  - Verified numerically rather than by eye: Ultimate Doom enumerates 36 maps E1M1..E4M9 in order,
    18 rows a column at step 8 ending at 189 of 200, columns 10..152 and 168..310, 14 pages at 3
    per cycle; Doom 2 enumerates 32 as MAP01..MAP15, **MAP31, MAP32**, MAP16..MAP30, 16 rows at
    step 9 ending at 188.

  Records demos in the background and saves the run that set each record. Hooks:
  `HS_Init` from `D_DoomMain` (after `legacyhome` is resolved — `M_Init` is too early),
  `HS_NewGame` from the menu skill-select handlers (**must** precede `G_DeferedInitNew`, see below),
  `HS_LevelExit` from `WI_Init_Stats` (already the single-player-only branch of `WI_Start`),
  `HS_Player_Died` from `P_KillMobj` (`p_inter.c`), and new
  cases in `D_DoAdvanceDemo`/`D_Display`. `G_SnapshotDemo` (`g_game.c`) copies the demo buffer
  without closing it, so live recording continues after a record is saved.

- **The run board** (`hs_stuff.c`, `hs_run_t`, `runs.dat`). The per-map table above holds *splits* —
  the best cumulative time anyone has reached a given map in, one deep and anonymous. The board is
  the separate table of whole **runs**, which is what a player competes on and puts initials
  against. **Two tables and two files, deliberately**: retrofitting depth and names into
  `hs_maprecord_t` would have multiplied a 64×2×5 array, while a list of runs is naturally bounded
  by pruning, and `highscores.dat` needs no migration at all.
  - **Campaign runs rank furthest-then-fastest**: `HS_MapOrder(endmap)` is the primary key and time
    only breaks ties. This is what gives the great majority of cabinet runs — the ones that end in
    a death partway — a board to land on; a completed episode tops it because nothing outranks it
    on progress. Ranking partial runs on time alone is *not possible*: 8:00 to E1M5 versus 12:00 to
    E1M7 has no answer without progress as a key. Single Level runs are all one map, so the same
    comparison reduces to time.
  - `HS_MapOrder` already encoded the real play order including Doom 2's 15→31→32→16 secret detour,
    so the progress metric was already written and verified.
  - Depths differ by board: **10** for campaign (`HS_BOARD_DEPTH_RUN`, per game/skill/category) and
    **3** for single level (`HS_BOARD_DEPTH_SL`, per game/map/skill/category).
  - **Depth is only shown where the player is looking at that one thing.** The three-deep single
    level board is drawn on the **Single Level menu page**, where the player has just highlighted a
    map and is about to play it. The attract cycle still shows one row per map. That is the rule
    that stops the page count exploding.
  - **`hs_run_board_ok` is not `hs_run_ranked`.** A death clears `hs_run_ranked` (it gates the split
    records, where the free level reload would otherwise make dying a costless retry) but must not
    take the run off the board. So `hs_run_board_ok` tracks the ruleset and cheating **only**, and
    `HS_Player_Died` deliberately does not touch it. A cheat does clear it: dying is playing badly,
    cheating is not playing the same game.
  - Both are **initialised true**, matching each other. A game started from the command line
    (`-warp`) never runs `HS_NewGame`, so a board flag defaulting false would let such a game write
    split records while silently never reaching the board.
  - The run is **frozen at its last scored level exit** (`hs_run_startmap`/`endmap`/`tics`), inside
    the block already gated on `hs_run_ranked` — so it naturally stops at the last level completed
    under scoring. It is *not* `hs_cumulative_time`, which keeps counting after a death so the
    intermission can still show an honest elapsed time.
  - `HS_Run_Finished()` is hooked into **`Command_ExitGame_f`**, the single funnel every route back
    to the title passes through. Idempotent — it zeroes `hs_run_levels` — because some routes reach
    it twice.
  - **That funnel is not enough on its own: a completed episode never reaches it.** Finishing
    E?M8 (or Doom 2's MAP30) goes intermission → `G_NextLevel` → `F_StartFinale()`, and the player
    then presses through the ending or quits — neither of which passes through
    `Command_ExitGame_f`. The per-map splits recorded normally while the board entry was silently
    dropped, which is the exact opposite of what should happen: **a finished episode is the run
    that most deserves a place.** `HS_Run_Finished()` is therefore also called from `G_NextLevel`
    at the run-ending finales.
    - It looked intermittent because the **idle timeout does cover `GS_FINALE`** — waiting 60s on
      the ending screen committed the run, while pressing through it lost the run. Any "it worked
      that time" report about this is that difference.
    - **Only the run-ending finales, and `CL_Reset()` is what marks them.** Doom 2 reaches the same
      `F_StartFinale` for the *between levels* text screens after MAP06/11/20 (and 15/31 by the
      secret exit) and the campaign carries on afterwards, so committing there would end every
      Doom 2 run at map 6. The end-of-game cases are exactly the ones that call `CL_Reset()`
      ("end of game, disconnect from server"), so the two calls are kept side by side deliberately.
      There are three: `finale_after_intermission`, the UMAPINFO endbunny/endcast path, and
      `gamemap == 30`.
    - The run is committed **at the finale start**, not when the ending is over, so the place
      survives the player quitting during the credits. Only the *initials* are lost in that case,
      and the entry keeps the `---` placeholder.
    - **The prompt is raised over the ending, and `M_Initials_Ticker` deliberately does not
      exclude `GS_FINALE`.** It did at first, to avoid painting over the ending text, and that was
      wrong: a Doom 1 ending screen is *terminal* — you do not finish it, you leave it — so
      gamestate stayed `GS_FINALE` until the player quit and the entry kept its `---` placeholder.
      Only sitting out the whole 60s idle timeout on the ending screen ever produced a prompt,
      which is why this presented as "it skipped the initials again" even after the commit itself
      was fixed. Asking over the ending is also what an arcade machine does: finish the last
      level, sign the board, then watch the ending.
    - Two things make that safe, and both are worth knowing before putting any other page up
      during a finale: `M_Drawer` is called "even on top of everything" (`d_main.c`), so the page
      renders over it; and `M_Responder` runs **before** `G_Responder` in `D_ProcessEvents`, so
      the page takes the keys instead of the finale advancing underneath.
    - **Test the prompt inside a window shorter than `cv_idletimeout`.** The idle timeout covers
      `GS_FINALE` and reaches the prompt by its own route (`Command_ExitGame_f` → title), so a
      test longer than 60s passes whether or not this works. The verification run is 30s against
      the cabinet's 60s timeout.
    - Verified headless, driving the intermission with a temporary console accelerate: E1M8 gives
      `finale_pending=1` then a committed `doomu E1M8 E1M8 0 speed 104` entry with no
      `Command_ExitGame_f` involved at all, and `AAA` once the run is allowed to reach the title;
      Doom 2 MAP30 commits both categories; **Doom 2 MAP06 does not commit and writes no
      runs.dat**, which is the regression that matters.
    - **The intermission needs *two* accelerates to leave, not one** (`WI_update_Stats`): the first
      jumps `sp_state` to 10, the second takes the `sp_state == 10` branch to `WI_Init_NoState`.
      A headless test that sends one appears to prove the finale is never reached, when in fact
      the intermission simply never advanced — this cost a wrong diagnosis once already.
  - **Board entries have no demos of their own**, on purpose. The attract cycle keeps replaying the
    split-record demos exactly as before, so the demo count and the shuffle bag are unchanged; a
    three-deep board with its own demos would have tripled both.
  - `HS_Board_Entry` walks the stored order and hands back the nth match as the nth place, so
    **stored order must equal rank order**. Insertion maintains it; `HS_Runs_Load` re-sorts each
    board after reading, since `runs.dat` is plain text an operator may have edited.
  - Ties keep the **earlier** entry ahead (the comparison is strict), the arcade convention — and it
    matters now that times are kept to the tic.
  - Attract pages: one per (skill, category) that has anyone on it, appended **after** the split
    pages. Ten rows is the whole board, so a board never needs more than one page.
  - **One-time migration.** On the first boot with no `runs.dat`, the board is seeded from the
    existing *single level* split records — one of those is exactly one run, so the mapping is
    exact. Campaign splits cannot be converted (several come from one run and nothing says which),
    so the campaign board starts empty and fills as runs are played. Seeded entries get the `---`
    placeholder for initials.

- **Times are shown to hundredths where they are run times** (`HS_Format_Time_CS`). Whole seconds
  cannot separate two E1M1 runs. **This needed no format change and no migration**: `besttime` is
  `tic_t` and `highscores.dat`'s fourth field has always been a raw tic count — the precision was
  only being thrown away at the point of display.
  - Hundredths are `(tics % TICRATE) * 100 / TICRATE`. TICRATE is 35 and does not divide 100, so
    they are a scaled tic count rather than exact — which is the convention the wider Doom
    speedrunning world displays, so a cabinet time reads the same way as one from anywhere else.
  - **Where, and why not everywhere.** Run times get hundredths: the intermission TOTAL row, the
    intermission BEST table, the boards, the Single Level page and the demo captions. The
    **attract split page stays at `M:SS`**, and that is a geometry limit, not a preference: its two
    map columns are 152px each, and at `HS_PG_SPEED` 84 a `12:34.57` (51px) would start at 33 and
    collide with the 38px map name. Two hundredths-wide time columns need ~182px, which does not
    fit — and one column of 24 rows does not fit vertically either (`58 + 23*10` exceeds
    `BASEVIDHEIGHT` 200). Nobody compares E1M4 splits to the hundredth.
  - The intermission BEST table **was re-laid out** to make room: `HS_COL_TIME` 90→**108**,
    `HS_COL_STEP` 62→**72**, and the `wi_stuff.c` call site x 156→**138**. Measured: "ITYTD" 36px
    ends at 174, the widest time `888:88.99` 64px starts at 182, the last column's right edge is
    318 of 320. Done rather than left alone because BEST sits directly above TOTAL and the player
    compares the two — mismatched precision there is worse than the re-measure.

- **Initials entry** (`m_menu.c`, `M_Initials_*`, `InitialsDef`). Raised once when a run that took a
  board place has ended, and stamped onto every entry that run placed.
  - **Once per run, not per record.** A campaign run sets a split record on every level it passes,
    so prompting per record would ask eight times for one run. Both categories can place at once —
    same run, same player — and `hs_run_placed[]` records which, so one entry covers them.
  - **`cv_initialstimeout`** ("initialstimeout", default **60s**, `CV_SAVE`), under
    **Options → Menu Options** with the other operator settings. Generous on purpose: nothing is
    waiting on it (the cabinet is already back on the attract screen behind the page), and a player
    who just earned a place should not be racing a timer. `0` disables the timeout, which leaves
    the page up until somebody presses fire — supervised machines only.
  - **The place is saved before the prompt**, so every exit is an accept: ESC accepts rather than
    abandons, and the timeout accepts the default `AAA`. Backing out would only throw the name away.
  - **The idle timeout is held off while the page is up** (`G_Idle_Timeout_Check` asks
    `M_Initials_Active()`), or a player part way through entering would be closed out from under
    them. The page's own countdown still clears an abandoned one, so the cabinet can never stick.
  - **Raised from `M_Ticker`, not from the run-end path.** `Command_ExitGame_f` only *arms* it
    (`HS_Initials_Pending`). Opening from the ticker keeps it independent of the order things
    happen in on the way back to the title — `M_SingleLevel_Finished` calls `Command_ExitGame_f`
    and *then* pushes its own page, so a prompt opened inline would be buried by it. From the
    ticker it lands on top of whatever settled, and backing out returns there.
  - **Driven from the translated keys**, unlike the join screen: it is taken *after*
    `M_Cabinet_Menu_Key`, because stick up/down to cycle a letter is exactly what that produces,
    and this page does not care which panel is entering. The join screen needs the opposite.
  - Character set is A-Z then 0-9, no blank — three characters always filled keeps every board row
    the same width. Cells are fixed pitch (20px) so the row does not shift as glyph widths change.

- **The episode's last level gets an intermission** (`g_game.c` `G_Start_Intermission` /
  `G_NextLevel`, `wi_stuff.c`). Vanilla skips it on those maps and jumps straight to the finale, so
  **E1M8 / E2M8 / E3M8 / E4M8 showed no summary page and never reached `HS_LevelExit`** — a time on
  an episode-ending map was impossible to record. Doom II already did it the other way round: MAP30
  gets its intermission and the finale is started afterwards from `G_NextLevel`. The Doom 1,
  Heretic and Chex maps now follow that same order, which is why one flag was enough to get both
  the stats page and the scoring.
  - **`finale_after_intermission`** (`g_game.c`, declared in `g_game.h`) carries the decision.
    `G_Start_Intermission` clears it at the top and sets it in place of the old
    `CL_Reset(); F_StartFinale(); return;`; `G_NextLevel` does the `CL_Reset` and the
    `F_StartFinale`. Decided in one place rather than recomputed from `gamemode`/`gamemap` at each
    site, because **UMAPINFO can override the ending** — its block runs first and `goto`s past the
    map-8 case entirely, so a pack that gives E1M8 a next map keeps a real next map.
  - Only the **non-deathmatch** path changes. In deathmatch map 8 already wrapped to `lev_next = 0`
    and went through the intermission.
  - **`wi_stuff.c` must skip `ShowNextLoc`** for these maps, in both `WI_update_Stats` and
    `WI_update_NetgameStats`. There is no next location, so the "Entering ..." screen would point
    at E?M1. `WI_Init_NoState()` instead — exactly what `doom2_commercial` already does there.
  - **Doom II needed no change at all**: MAP30 (and the 6/11/20 text maps) already showed the
    summary and scored. Confirmed by test, not assumed.
  - Verified headless, `-warp` plus `wait`/`exitlevel`, with the intermission's keypress forced:
    E1M8/E2M8/E3M8/E4M8 each went stats → NoState → `F_StartFinale` and wrote a record
    (`doomu E4M8 2 98 speed`); E1M1 and Doom II MAP01/MAP30 were unchanged controls
    (`finale_pending=0`, `ShowNextLoc` still shown for E1M1). A **single level** run of E4M8 wrote
    `doomu-sl E4M8 3 31 speed` with its demo and returned to the menu with the finale skipped.

- **Kills/items/secrets on the HUD** (`st_stuff.c`, `ST_overlayDrawer`). The engine's status-bar
  overlay is driven by the `overlay` cvar, a **string of one-letter element codes** — stock
  `"kahmf"` is keys/ammo/health/armor/frags. Upstream already had `e` (kills) and `s` (secrets)
  but neither was in the default and there was **no items element**; `i` is new. The three are
  stacked top-right at `SCY(1/11/21)` with `K`/`I`/`S` labels. This pairs with the high-score
  **max** category, which needs 100% kills and secrets, so the player can see whether the run is
  still eligible. The compiled default is now **`"kahmfeist"`** — `eis` for these three plus `t`
  for the level clock below.
  - The overlay only draws when **`st_overlay_on`**, which `R_SetViewSize` (`r_main.c`) sets from
    `cv_viewsize.value == 11` — the largest view size, no status bar. At any smaller viewsize the
    classic status bar draws instead and none of this appears. The cabinet's `config.cfg` is at
    `viewsize 11`.
  - Skipped in splitscreen, like the upstream `e`/`s` cases: `killcount` is per player while
    `totalkills` is the map's, so one corner cannot speak for both. High scores are single player
    anyway.
  - **Skipped during `demoplayback` too.** The three exist so a *player* can see whether their own
    run is still eligible for the max category; on the attract screen they are somebody else's
    counters cluttering the corner of a screen that is meant to look inviting. The condition is
    shared as **`ST_KIS_ON`** so the three rows cannot drift apart — they are one block and must
    appear and disappear together. Like the gameplay-message suppression in `console.c` this covers
    the Single Level "watch run" replays, which are the same thing: a recording, not your run. The
    level clock `t` deliberately stays, since it reads as part of the demo.
  - **`config.cfg` overrides the compiled default**, and only devmode rewrites it, so changing the
    default in `st_stuff.c` does nothing on a machine with an existing config — the saved
    `overlay` line has to be edited (or re-saved from a `-devmode` session) as well.

- **Blue armour gets its own overlay icon** — **`SBOARMBL`**, added to `legacy.wad`, drawn by the
  `m` element in place of `SBOARMOR` when `armortype >= 2`. Green absorbs a third and blue a half,
  so 100 green points are worth much less than 100 blue ones and the bare number could not say
  which. `p_inter.c` tests `armortype == 1` for the weaker case, so `>= 2` is blue and also picks
  up the megasphere.
  - **The lump is a Doom patch, while every stock `SBOxxxx` icon is a `pic_t`**, and the two need
    different drawers — `V_DrawScaledPatch_Num` versus `V_DrawScalePic_Num`. Rather than require
    the artwork be converted, the format is detected once at load: a `pic_t` always has **byte 2 of
    its header zero** (`r_defs.h` calls that field out as exactly this autodetection hook), while in
    a `patch_t` those bytes are the height, never 0 for a real icon. Read with `W_ReadLumpHeader`,
    not `W_CacheLumpNum`, so the test stays clear of whatever caching the patch path then does.
  - `W_CheckNumForName`, not `W_GetNumForName`: a `legacy.wad` without the lump keeps one armour
    icon instead of failing to start.
- **Gameplay messages are off the HUD when the screen is shared, or during a demo** (`console.c`,
  the `gameplay_msg` block). A pickup line belongs to whoever triggered it but is painted across
  the top of the *whole* screen: on a splitscreen or 2x2 cabinet it covers someone else's view, and
  on the attract screen it is somebody else's pickups scrolling over a shop window. Forced to
  `viewnum = 5` for `D_NumViews() > 1` or `demoplayback` — the console-only path the
  `cv_showmessages` test already used, so nothing new had to be invented and the messages are still
  in the console and the log. The demo case covers the Single Level "watch run" replays too.
  - `HS_DemoLabel` still draws on the **second** text line even though the first is now free
    during playback; left there so the caption does not move.
  - Single player is untouched, and this is separate from `cv_showmessages`, which still works
    normally for one player.
  - **`HU_SetTip` is a different path and unaffected**, which matters: the idle-timeout countdown
    uses it and must keep showing.
  - Verified by counting suppressions: with a cheat generating messages, one player suppressed 0,
    four players 3, splitscreen 4; and an attract cycle suppressed 20 demo messages while a normal
    single player game suppressed none.

- **Level clock on the HUD** — element code **`t`**, new, so the default is now `"kahmfeist"`.
  Counts *down* the time remaining when a time limit is set and counts elapsed time *up* when one
  is not, so it serves both a deathmatch round and a speed run. Format `T 4:59`.
  - Drawn **low and left of centre on the status number row** — `SCX(CLK_CX - V_StringWidth/2)` at
    `lowerbar_y + CLK_DY*sf_dupy`, with `CLK_CX` 104 and `CLK_DY` 9. **Two thirds of the screen are
    unusable here, both learned by putting it there first:**
    - The **top** is covered by `HU_Drawer`'s pickup messages at y=0 — the same reason
      `HS_DemoLabel` sits at y=8. A top-left clock is invisible in play.
    - **Dead centre** is where the **weapon sprite** draws, in every mode. `CLK_CX` is 160 minus
      about nine characters (~6px average glyph width) to get out from behind it.
    - Along `lowerbar_y` the free span is **x 68..192**: health's number is right-justified ending
      at 50 with its 16px `SBOHEALT` icon at 52, and ammo's is right-justified at 234 with at most
      three 14px `STTNUM` digits, so it starts at 192. The widest string is `T 12:34` at 44px, so
      centred on 104 it spans 82..126 and clears health by 14px.
    - **`CLK_DY` is capped by splitscreen, not by the full screen.** `hu_font` glyphs are **7** tall
      (not 8 — measured). In the upper half `lowerbar_y` is 319 and the half ends at row 383, so
      +9 leaves the text bottom at 379 with 4px spare, while a full character down (+12) would
      bleed into player 2's view. Single player has more room but uses the same offset so the modes
      agree. Offsets scale by `sf_dupy` and are *not* halved for splitscreen, matching how
      `lowerbar_y` itself offsets from `SCY(198,y0)` — `SCY` already halves and adds `y0`, so each
      half gets its own copy.
    - Verified at 1366x768, text bottom vs limit: single 759/767, upper half 379/383, lower half
      763/767; x spans 350..537 against a health edge at 290 and ammo at 819.
  - **Not skipped in splitscreen**, unlike `e`/`i`/`s` — the clock belongs to the level rather than
    to one player, so it is correct in both halves, and two player deathmatch is exactly the case
    that wants it. `y0` already offsets it per half.
  - Reads `timelimit_tics` (`g_game.c`, externed in `d_netcmd.h`), which `TimeLimit_OnChange`
    derives from `cv_timelimit`; do not recompute from the cvar.
  - Same `config.cfg` caveat as above. This one **did** bite: the clock did not appear at all until
    the `t` was added to the saved `overlay` line, because the config value overrides the compiled
    default and only a devmode session rewrites it. The cabinet's line is now `"kahmfeist"`, but
    any *other* install still needs the letter added by hand.
- **Deathmatch defaults** — a DM round gets a time limit from **`cv_dm_timelimit`** ("dmtimelimit",
  default 5 minutes, `CV_SAVE`), and coop explicitly clears the limit, appended to the game-start
  command in `M_StartServer` (`m_menu.c`).
  - **`cv_timelimit` is the engine's limit and is rewritten at every game start**, so the Net
    Options menu row could never edit it usefully: whatever was typed there was overwritten before
    it was read, and the row then displayed the 0 that a coop start had left behind — "it says 0
    and does 5 minutes whatever you set". The row edits `cv_dm_timelimit` now, which is only ever
    *read*, so it keeps what the operator set. Same split as `cv_deathmatch_menu` versus
    `cv_deathmatch`, and the same reason.
  - The limit still cannot simply be `cv_timelimit`'s default: that would cut **single player**
    levels short too, which is why it is applied per game start. An unattended cabinet has no other way out of a DM stalemate: players cannot reach
  End Game, and the idle timeout only fires when *nobody* is touching the controls. Applied per
  game start rather than as the `cv_timelimit` default, which would also cut single player levels
  short. In `deathmatch_cons_t` the DM modes are values **1..4**; every coop variant is 0 or ≥0x10.
  - The mode itself already defaulted to what was wanted: `cv_deathmatch_menu` ("dmm") is `"3"` =
    **`DM_both`**, items respawn *and* placed weapons respawn immediately.
  - **`Deathmatch_OnChange` is now called unconditionally from `G_InitNew`** (`g_game.c`), which is
    what actually made respawn work. It is the function that derives `deathmatch`,
    `weapon_persist` and `cv_itemrespawn` from `cv_deathmatch` — but as an OnChange it only ran
    when the cvar *changed*, and **`CV_Set` returns early on an unchanged value**
    (`command.c:1582`). Meanwhile `HS_Apply_Ranked_Ruleset` pins `cv_itemrespawn` back to 0 on the
    way out to the attract screen (it is in `hs_ranked_rules[]`). So the **first** deathmatch after
    boot had item respawn and every **later** one at the same setting silently did not — which
    reads exactly like "the default is wrong". Verified headless: with `+deathmatch 3` already set
    and `+respawnitem 0` forced afterwards, `G_InitNew` now recovers `itemrespawn=1`,
    `weapon_persist=1`, while single player still reports `itemrespawn=0`.
  - **This is the general trap, not a one-off**: any cvar whose OnChange has side effects on *other*
    cvars will silently skip them when re-set to the value it already holds. If something else pins
    those other cvars in between (the ranked ruleset does), they stay pinned. Re-apply the mapping
    at the point of use rather than relying on the change notification.
- **`cv_fragsweaponfalling` defaults On** (`p_inter.c`), so killing the player holding the rocket
  launcher leaves it for whoever did it — how modern arena shooters behave, and what makes a
  cabinet deathmatch flow. It only fires when a **player** dies (`target_player`), so single play is
  unaffected in practice: a death there ends the run and reloads the level.
  - **Added to `G_demo_defaults()`** as a result. It is not in the demo header and was not in that
    function either, so it is exactly the case the rule below warns about: stock IWAD demos contain
    player deaths — the Ultimate Doom E4M2 one does — and were recorded without weapon dropping, so
    replaying them with it on spawns a weapon the recording does not have and desyncs from there.
    Playback pins it off; live play uses the new default.
  - Not in `hs_ranked_rules[]`, so it does not affect whether a run scores. It is a deathmatch
    setting and the ruleset governs single player scoring.

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
  - **The *engine* cvars `cv_respawnmonsters` and `cv_fastmonsters` are deliberately absent from
    the table.** `G_InitNew` turns both on for `sk_nightmare` (`g_game.c`, `CV_SetParam`), so their
    value belongs to the skill, not the player. **This bit once already** — `cv_fastmonsters` was
    left in the table by mistake, and every Nightmare run played MAP01 normally, then showed
    `UNRANKED` from MAP02 with no score for MAP01. That is the signature of a rule the *engine*
    changes after `HS_NewGame`: the run passes the check at menu time and fails it at the first
    level exit.
  - **What is in the table instead is the player-facing pair**, `cv_fastmonsters_menu` /
    `cv_respawnmonsters_menu` (`m_menu.c`, console names `fastmonstersopt` / `respawnmonstersopt`).
    Nothing in the engine ever writes those, so Nightmare cannot trip the check, while a player who
    turns either on from Game Options does — which is what has to happen, since both change the
    simulation and a mid-run change would desync the demo the run is being recorded into.

    This is the **third** instance of the same split in this file, after `cv_deathmatch_menu` and
    `cv_dm_timelimit`, and for the same reason: the engine cvar is written from under the player.
    Here it happens twice over —
    - `G_DeferedInitNew`'s game-start command line reset both to 0 (`fastmonsters 0;respawnmonsters
      0`), so **anything chosen in Game Options before a run was wiped before the first tic**. The
      setting only appeared to work if it was toggled *during* a run, which is what made it look
      broken rather than merely ineffective. That line now seeds the engine pair **from the menu
      pair**, which both honours the choice and still clears whatever the last game left set.
    - `sk_nightmare` forces both on, so after one Nightmare game they read as on for everything
      afterwards.

    The menu cvars are `CV_CALL` and mirror into the engine pair through `CV_Set_by_OnChange`, so
    toggling mid-game still takes effect immediately the way it always did. They are **not**
    `CV_SAVE`: `HS_Apply_Ranked_Ruleset` pins them to 0 at every route back to the title, which is
    also what now undoes Nightmare's pollution of the engine pair.
    - **Nightmare still gets fast monsters and respawn.** Order at a menu start: the command line
      sets the engine pair from the menu pair, *then* `map ... -skill 5` runs `G_InitNew`, which
      forces both to 1. `CV_SetParam` writes `.EV` directly and calls `CV_cvar_call`
      unconditionally — it has no unchanged-value early return — so `FastMonster_OnChange` always
      fires and the skill is unaffected by whatever the player chose.
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

### Portable install (`legacyhome` next to the binary)

**A `legacyhome/` directory sitting beside the executable overrides `~/.doomlegacy` entirely** —
config, high scores, demos, level packs and savegames. This is what lets a checked-out tree run
with its own tracked `config.cfg` and **no command line arguments**, which is the point: the config
is otherwise unversioned state the build silently depends on.

- Implemented in `d_main.c`, just above the `if (userhome)` legacyhome search. The engine already
  had `progdir` and a `progdir/DEFHOME` fallback; that path was **dead code**, because `$HOME` is
  always set on Linux and was tested first. The change only reorders the priority.
- **Presence of the directory is the switch.** Without one, nothing changes and an existing
  `~/.doomlegacy` install behaves exactly as before. Verified both ways.
- `progdir` comes from `readlink("/proc/self/exe")` (`I_Get_Prog_Dir`, `sdl/i_system.c`), so it
  follows the **executable, not the working directory** — a menu entry or service unit finds the
  same files as a shell. Verified by running from `/` with an absolute path.
- **The trailing slash is required.** `savegamename` concatenates directly onto `legacyhome`, and
  `cat_filename` only separates *dir from name* — it does not append one at the end. So the new
  branch passes `DEFHOME SLASH`, matching the `DEFAULTDIR1 SLASH` the userhome branch uses. The
  pre-existing `progdir` fallback below does **not** do this, which is a latent bug in a path that
  is only reachable when `$HOME` is unset.
- Kept in a local (`portable_home`) rather than testing `legacyhome` directly, because
  `D_DoomMain` re-runs this block via the launcher restart path and `legacyhome` may still hold
  the previous pass's value.

**The command buffer sizes the config.** `exec` pushes a whole file into `com_text` in one go, and
`COM_BUF_SIZE` (`command.c`) caps it. At the stock **8192** the cabinet's config outgrew it — 8195
bytes — and `VS_Print` failed, dropping the remaining lines **in silence**. Those cvars kept their
compiled defaults, and the next `-devmode` session wrote the defaults back over the file: that is
how a hand-tuned config "blows away on its own". The four player work is what pushed it over
(`setcontrol3` and `setcontrol4` are 38 lines each). Raised to **65536**, and the overflow is now
an `EMSG_error` naming the consequence rather than a `CONS_Printf` that scrolls past. **A four
panel config is ~8.5K, so watch this if the config grows much further** — and if settings ever stop
sticking, look for "Command buffer full" first.

**Config lines that fail are reported.** `M_Verify_Config` (`m_misc.c`) runs right after the load
and again on demand from the **`cfgcheck`** console command. Config lines are handed to the console
as an `exec`, so by the time they run the line numbers are gone and a failure is *silent*: an
unrecognised setting name does nothing, and `CV_Set` rejects a value that is not one of a cvar's
`PossibleValue`s without a word. Either way the cvar keeps its compiled default, which is exactly
how a config appears to have been "half loaded". Rather than instrument the command buffer, the
check re-reads the file afterwards and verifies each `name "value"` line actually left that cvar
holding that value, naming the line number when it did not.

**Comparing by `cv->string` is wrong**, and produced a page of alarming false reports the first
time — `drawmode` "did not take" on a cabinet visibly running OpenGL. `cv->string` is only the text
last *set*; the value in force is `.EV`, or `.value` for `CV_VALUE`/`CV_FLOAT`. Config files also
store a cvar's PossibleValue **label** ("On", "OpenGL", "32 bits"), so the file's text must be
resolved through that table before comparing. This is the same trap as the `cv_fastmonsters`
investigation: **resolve labels and read `.EV`/.`value`, never compare the strings.**

**Timing matters as much as the comparison.** The check runs from `D_DoomLoop`, not from
`M_LoadConfig`: at load time the video mode is not set and several subsystems have not applied
their settings, so `scr_width`, `drawmode` and friends still read as defaults and every one is
reported as failed.

Two sources of *expected* reports remain, so read the output with them in mind:
- **`botrandom`** is a random seed and always differs.
- A clean cabinet config reports exactly **four**: `botrandom` plus the three ruleset cvars below.
  Anything else is worth investigating. (The video settings used to appear here too; that was the
  command buffer overflow above, not the dummy driver.)
- **The ranked ruleset legitimately overrides the config** in a player session (see
  `hs_ranked_rules[]`), so `monstergravity`, `monsterfriction` and `voodoo_mode` report as not
  taking. That is the ruleset working, and matches the three documented above.

**Every config save keeps one generation** as `config.cfg.bak`, written by `M_SaveConfig`
(`m_misc.c`) just before the new file. A cabinet's config is hand-tuned, only a `-devmode` session
writes it, and the settings exist nowhere else on the machine — so a bad write is rare but
expensive. There *is* a `config_loaded` guard above that code, but it does not prevent a session
which started without reading the file from writing a full set of defaults over it: verified, a
devmode run with no `config.cfg` present writes 345 lines of pure defaults. The `.bak` makes any
such loss one `cp` from recovery. **A defaults-write is recognisable** by `name` being your Unix
login and `name2` being the compiled default `"big b"`.

The tracked copy lives at **`cabinet/legacyhome/config.cfg`** (see `cabinet/README.md`). `make`
stages it into `svn1749/bin/legacyhome/` via the `cabinet_home` target, using `cp -n` so a rebuild
**never resets a running cabinet's settings**. Since `bin/` is gitignored, an operator's `-devmode`
edits land in an untracked file — **`make cabinet_save`** copies the live config back over the
tracked one so the change shows up as a reviewable `git diff`. Player data and `levels/` stay
untracked by design: high scores and demos churn on every record and want backups rather than
history, and the level packs are ~26MB of wads.

Runtime data lives in the active legacyhome — `~/.doomlegacy/` unless a portable one is found:
`config.cfg`, `highscores.dat` (plain text,
`<wadcombo> map skill tics <category> <startmap>`), `runs.dat`
(`<wadcombo> <startmap> <endmap> skill <category> tics <initials>`),
`demos/<wadcombo>_<map>_sk<N>_<category>.lmp`, and
`levels/` for selectable level packs. `<wadcombo>` is `HS_GameId()`, e.g. `doom2` or
`doomu+mapsofchaos`; `<category>` is `speed` or `max`. **Fields are only ever appended** to
`highscores.dat`, so an older short line still loads: four fields is a pre-category speed record,
five adds the category, six adds the start map. A record with no start map is never written as an
empty field — `-` stands in, or it would shift every field after it on the next read.
`runs.dat` writes `---` for initials nobody entered, and reads that back as empty.

**Every placeholder must be converted back on load, or it becomes a value.** This is written twice
in each file — `-` for an unknown start map, `---` for unentered initials — and the load side has
to undo both. Missing the `-` conversion in `HS_Load` made an unknown start map read as a map
literally named `-`, which differs from the record's own map and so satisfied exactly the test the
caption uses to decide it has a range: attract captions came out as **`--E4M1`** and
**`SINGLE LEVEL: --E1M1`**. Nothing was corrupted on disk — the file was always right and only the
read was wrong — which is why it showed up as a display oddity rather than lost data, and why no
migration was needed to fix it.

To reset the scores, use the **`clearhighscores`** console command or the **`-clearhighscores`**
command-line flag (which runs the same code right after `HS_Init`). Both clear the in-memory tables
as well as the files — `runs.dat` goes with `highscores.dat`, since leaving it would put named
board entries beside an empty split table. Prefer them over deleting the files by hand: the tables
are cached in memory while the game runs, so a later record writes the old entries straight back
out. Note that deleting **only** `runs.dat` is a supported way to re-run the one-time seed from the
single level splits.

### Gotchas found the hard way

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
