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
(`M_SINLVL`, `M_JOIN`, `M_CHEATS`, `CREDIT2`, `SBOARMBL`, `M_GAMOVR`). The user edits their live
`~/games/doom/legacy.wad` and it is copied over `common/legacy.wad` to track it; verify only the
expected lumps differ before committing.

## Build

**`tools/build.sh` does all of the below automatically** (detects the system, probes the
dependencies by test-compiling, writes `make_options`, orders `make depend` before the parallel
build). Use it unless there is a reason not to; `docs/arcade/building.md` explains its design and
what it protects against. `tools/build.ps1` + `build.bat` are the Windows equivalents (MSYS2 ucrt64); they now build
`doomlegacyarcade.exe` end to end and stage its 12 runtime DLLs into `svn1749\bin` (derived by walking the
import tables — SDL2_mixer's codec DLLs make the list longer than anyone guesses, and it is not
stable enough to hardcode). The binary loads but has not been played.

**The tree is in a Dropbox folder shared with the Linux cabinet, so `make_options` and `objs/` sync
between platforms even though they are gitignored.** Both build scripts detect a `make_options`
whose `OS=` does not match their template, regenerate it, and force a clean — without that, every
file compiles and the link fails on libraries that do not exist on the platform. See
`docs/arcade/building.md`.

Building must happen from `svn1749/src/` (the Makefile there locates the rest of the build via
relative paths and drops `bin/`, `objs/`, `dep/` in `svn1749/`, one level up from `src/`).

```
cd svn1749/src
cp ../make_options_nix ../make_options   # first time only; pick _win/_mac/_os2/_dos as appropriate
make                                      # builds using ../make_options
```

`make_options` is **gitignored** (it is machine-local), so the settings below are not recoverable
from the repo — on a fresh checkout the stock `make_options_nix` needs three edits before it will
build on a modern Linux toolchain (the first three below are hard build failures, not warnings; the
`-g` noted with the third is optional but wanted on the cabinet):

- `SDL2=1` — uncomment. The stock file targets SDL 1.2, which modern distros no longer ship; with
  it enabled the Makefile finds `sdl2-config` (via sdl2-compat) and links `-lSDL2 -lSDL2_mixer`.
- `ARCH=-march=native` — replace the stock `ARCH=-march=i686`, which is 32-bit-only and fails with
  "CPU you selected does not support x86-64 instruction set".
- `ENV_CFLAGS=-std=gnu17 -g` — add. `-std=gnu17` is required: GCC 15 defaults to `-std=gnu23`, where
  `true`/`false` are reserved keywords, which breaks this codebase's own
  `typedef enum {false, true} boolean;` in `doomtype.h`. (`ENV_CFLAGS` is appended to `CFLAGS`
  unconditionally; the Makefile's `STD` variable is dead code.)
  - **`-g` is not required to build, but keep it**: the cabinet runs unattended, `systemd-coredump`
    keeps every crash, and without it a backtrace is bare function names. The chase camera
    use-after-free was diagnosed from `R_SetupFrame ()` plus hand-decoding the faulting instruction;
    with `-g` the same trace reads `P_KillMobj (target=0x0, ...) at p_inter.c:2173` — file, line and
    argument values. It costs nothing at runtime (the binary grows ~1.9MB to ~8.2MB, disk only) and
    `-O3` is unaffected. Read a crash with `coredumpctl list` then
    `coredumpctl debug <pid> --debugger=gdb --debugger-arguments="-batch -ex bt"`.

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
The vendored ZDBSP node builder in `svn1749/src/nodebuild/` is the tree's **only C++**; its objects
are the separate `NBOBJS:=` list, built by a `$(SD)nodebuild/%.cpp` rule with `$(CXX)`/`$(CXXFLAGS)`
and linked via `-lstdc++`. → `docs/arcade/gotchas.md`

Useful targets: `make clean`, `make distclean` (also removes `make_options`), `make depend`,
`make BUILD=<dir>` (build into an alternate output directory), `make DEBUG=1 BUILD=debug`, and
`make smoke` (headless smoke test — see below; `make smoke SMOKE_ARGS="warp opengl"` for a subset).

**`make -j` races in the dependency phase**, and the error points nowhere near the cause: every
`../dep/*.dep` rule pipes through the *same* intermediate `../dep/sed.dep` and then `mv`s it, so two
parallel dep rules clobber each other and it fails with `mv: cannot stat '../dep/sed.dep'` followed
by `../dep/umapinfo.dep: No such file or directory`. Nothing is wrong with the source. Run
**`make depend` serially first, then `make -j8`** — the compile phase parallelises fine. It only
bites when the dep files are missing or stale, i.e. on a fresh checkout, a new worktree, or after
`make clean`, which is exactly when someone reaches for `-j`.

The binary finds `legacyhome`, and its wads, **next to itself**, so it must be run from an install
directory rather than from `src/`. On this cabinet `svn1749/bin/` *is* that install directory —
its `legacyhome` holds the live config, high scores, `audit.dat` and record demos — so a build
updates the cabinet in place and there is nothing to copy. (`~/games/doom/` is only the wad
directory; there is no binary there.) To install somewhere else, copy the **binary by name**:
`cp -a svn1749/bin/doomlegacyarcade <install-dir>/`. Never `cp -a svn1749/bin/*` onto an existing
install — that drags `legacyhome` along and overwrites its config, scores and demos.
(`make install`, `install_user`, `install_sys` and `install_games` also exist.)

There is no automated test suite; validating a change means building and running the game
interactively (or asking the user to). **A lot can still be checked without a screen**, though —
see below.

### Headless verification

**Most of this is packaged as `tools/smoke.sh`, run with `make smoke` from `svn1749/src`.** It sets
up the scratch directory below, then runs five checks — `startup`, `warp`, `exitlevel` (which drives
a real level exit and requires a record line in `highscores.dat`), `opengl` (on the real GPU) and
`config` — reporting pass/fail per check and exiting non-zero if any failed. Run it after any change
that touches startup, the config, level setup, scoring or the renderer; it takes about a minute.
`tools/smoke.sh -l` lists the checks, a check name runs just that one, `-k` keeps the scratch
directory, `DOOMWADDIR` points it at the IWADs, and `SMOKE_TIMEOUT=1` is a quick way to prove the
checks can actually fail. Prefer extending it over rebuilding the harness by hand — but read the
rest of this section anyway, because everything below is *why* it is shaped the way it is, and a
one-off investigation still needs it.

The game runs under SDL's dummy drivers, which is enough to exercise startup, config load, cvar
state, the attract cycle and level setup. This has caught real bugs that would otherwise have
needed a play session:

```
RD=/tmp/rundir            # NOT svn1749/bin -- that is the live cabinet now
mkdir -p "$RD" && cp svn1749/bin/doomlegacyarcade "$RD"/
cp -a svn1749/bin/legacyhome "$RD"/               # a COPY of the live home
ln -sf /home/mridlen/games/doom/* "$RD"/          # DOOM.WAD, DOOM2.WAD, legacy.wad, ...
cd "$RD" && SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SDL_NO_SIGNAL_HANDLERS=1 \
    timeout 40 ./doomlegacyarcade -game doom2 -skill 5 -warp 1 > out.txt 2>&1
sed 's/\x1b\[[0-9;]*m//g' out.txt | grep ...   # output is full of ENDOOM color escapes
```

- **`SDL_NO_SIGNAL_HANDLERS=1` is not optional, and leaving it off looks like anything but a
  missing environment variable.** SDL2 installs its own SIGINT/SIGTERM handlers which synthesise
  an `SDL_QUIT` event, and `sdl/i_system.c` maps `SDL_QUIT` straight onto `I_Quit()`. Run
  non-interactively and a signal reaches the process almost at once, so the engine performs a
  perfectly *clean* quit: no error, no message, the log simply stops — and then it hangs in
  shutdown rather than exiting.

  Every symptom of that points somewhere else. The run dies a second or two in, wherever it
  happened to be, so it reads as a crash in whatever ran last: with a level loading it stops after
  `Solving T-joins` and looks like an OpenGL failure; with `drawmode` forced to software it stops
  after the config check and looks like the video mode; with nothing at all it stops after
  `HU_Init`. It was misread for a while as "`-warp` no longer works", which it never was — plain
  `./doomlegacyarcade -game doomu` with no other argument dies exactly the same way.

  What identifies it: `SDL_NO_SIGNAL_HANDLERS=1` makes it stop happening, and so does running
  under gdb, which intercepts signals before SDL sees them. **A headless run that quits early with
  no error at all is this, until proven otherwise** — check the variable is set before reading
  anything into where the log stopped.

- **Isolating `HOME` is no longer enough, and running from `svn1749/bin` is now dangerous.** Since
  the portable install landed, `legacyhome` is found *next to the binary* and `$HOME` is never
  consulted — so `HOME=/tmp/fakehome` protects nothing, and running `svn1749/bin/doomlegacyarcade` reads
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
- **`timeout` exit code 124 does *not* prove the run survived**, and this file used to say it did.
  A quit through `I_Quit` hangs in shutdown instead of exiting, so `timeout` kills it and reports
  124 exactly as it does for a healthy run. Every run in the `SDL_QUIT` episode above returned 124.
  139 is still a segfault.
  - **The reliable check is whether the ENDOOM banner was printed**, since that only happens on a
    real quit: `sed 's/\x1b\[[0-9;]*m//g' out.txt | grep -c 'READ THE DOCS'` — 0 means it was
    still running when `timeout` killed it, 1 means it quit on its own.
  - **Strip the colour escapes first.** ENDOOM text is interleaved with them per character, so a
    plain `grep 'READ THE DOCS' out.txt` on the raw file finds nothing and hands back a false
    "still running". That mistake is what kept the `SDL_QUIT` diagnosis pointing at `-warp`.
  - The other positive signal is per-tic output from temporary instrumentation: if it is still
    printing when the timeout hits, the loop was genuinely running.
  - `coredumpctl debug doomlegacyarcade --debugger=gdb --debugger-arguments="-batch -ex bt"`
    gets a backtrace.
- **The OpenGL renderer can be tested headlessly too, on the real GPU — use
  `SDL_VIDEODRIVER=offscreen`, not `dummy`.** This file used to say the hardware path needed a play
  session, and that mistake is exactly how a GL state leak shipped to the cabinet. `dummy` gives no
  GL context and silently falls back to software (the tell is `Creating polygons` / `HWR_Startup`
  being *absent* from the log). `offscreen` brings up a real accelerated context — the log names
  the actual card, e.g. `Renderer : AMD Radeon Vega 8 Graphics (radeonsi, ...)` — with no window,
  so it does not disturb the live X session:
  ```
  cd "$RD" && DISPLAY= SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
      SDL_NO_SIGNAL_HANDLERS=1 timeout 40 ./doomlegacyarcade -game doom2 -skill 3 -warp 1 > gl.txt 2>&1
  ```
  Set `drawmode "OpenGL"` in the scratch config, and blank `DISPLAY` so it cannot fall back to the
  real screen. The drawmode string must be one of the exact values in `drawmode_sel_t`
  (`v_video.c`) — `"8 bit"` is not one, silently leaves OpenGL selected, and looks like the setting
  being ignored; the software value is `"Software 8bit"`. It must go in **`config.cfg`**, not an
  autoexec: the console `drawmode` command does not switch drawmode, and an autoexec runs long
  after the renderer is up.
  - **`fullscreen` must stay `"Yes"`, and this file used to say `"No"`.** The offscreen driver has
    no window manager, so a windowed mode request comes back with no visual and the run dies with
    `Error: SetMode: cannot draw 0 bits per pixel` — which reads as a colour-depth problem and
    sends you to `scr_depth`, correctly `"32 bits"` the whole time. With `fullscreen "Yes"` the
    same run brings up a real accelerated context and quits cleanly.
- **Never restore OpenGL state by hand — the renderer caches it.** `SetBlend` (`r_opengl.c`) only
  issues the GL calls for bits that differ from its `cur_polyflags` shadow copy, so anything changed
  behind its back stays wrong for the rest of the session, and the damage shows up as general
  renderer corruption nowhere near the code that caused it. Wrap any direct GL work in
  `glPushAttrib`/`glPopAttrib` (plus `glPushClientAttrib` for pixel-store state, which `glPushAttrib`
  does not cover). Verify by reading `glIsEnabled` before and after and diffing — and prove the
  check works by reinstating the bug before trusting a clean result. → `screen-wipe.md`
- **A GL-configured `legacyhome` run under `dummy` does not fall back to software — it segfaults.**
  The mode change fails (`Change Graphics failed: err=-100`), the engine carries on, and the first
  console line drawn goes through `HWR_DrawPic` into `gluBuild2DMipmaps` with no context and dies
  inside `D_DoomMain`. The cabinet's live config selects OpenGL, so copying `legacyhome` wholesale
  and running under `dummy` reproduces it every time. **This is pre-existing engine behaviour, not
  a regression** — verified by building the previous commit and reproducing it identically, which
  is worth doing before believing a crash like this belongs to the change under test. Set
  `drawmode "Software 8bit"` in the scratch config for dummy-driver runs (and delete the
  per-drawmode `config8p.cfg`/`configgl.cfg`/`confign.cfg`, which are executed *after* `config.cfg`
  and would put it back). `tools/smoke.sh` does both.
- **Screenshots come out entirely black under `SDL_VIDEODRIVER=dummy`, including of screens that
  obviously have content.** A temporary `M_ScreenShot()` call is otherwise the best way to check
  drawing headlessly — it writes a TGA that can be measured numerically instead of eyeballed, which
  is how the restart splash's centring was verified. Use **`offscreen`**, where the capture works
  and shows the real GL output. Always take a **control shot of a screen known to have content** in
  the same run: an all-black capture of the thing under test is indistinguishable from the thing
  not drawing, and the control is the only thing that tells them apart.
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
session is `./doomlegacyarcade -devmode`, change settings, quit; a player session is plain `./doomlegacyarcade`.

On a cabinet with no command line, the **`gc_devmode` control (*Devmode Restart* on the Setup
Controls page, default Scroll Lock on panel 1) restarts the program into `-devmode` and back out
again** — the flag cannot be toggled live, because the menu lockdown is irreversible in place.
Attract screen only, and its responder hook must stay *after* `M_Responder`/`CON_Responder` or the
key cannot be re-assigned. → `menus.md`, `input.md`

**`-devmode` must be parsed before `M_Init()`** (`d_main.c`, just above the `M_Init()` call), because
`M_Init` is what applies the menu lockdown. It was originally parsed later alongside `-devparm`, which
silently made the flag do nothing at all.

### Where the details live

The per-feature design notes — what was tried, what broke, and how each was verified — live in
`docs/arcade/`. They are **not** loaded automatically, so **read the relevant one before changing
the files it names**; each says at the top when it applies. The rules that cut across everything
are kept below, in this file.

| doc | covers | read before touching |
| --- | --- | --- |
| `docs/arcade/high-scores.md` | Scoring, record demos, the run board, the ranked ruleset, initials entry, intermission tables | `hs_stuff.c`, `HS_*` call sites, `wi_stuff.c` |
| `docs/arcade/multiplayer-views.md` | Four local players, the view grid (2x2, stacked or side-by-side), the join screen, per-panel identity, HUD placement | `D_View_Grid`, `D_NumViews`, `localplayer*[]`, viewport geometry, `st_stuff.c`, `hu_stuff.c` |
| `docs/arcade/input.md` | Panels, Xbox gamepads, analog axes, control schemes, guided setup, menu key translation | `g_input.c`, `sdl/i_system.c` joystick code, `gamecontrol_pl[]`, `M_Cabinet_Menu_Key` |
| `docs/arcade/menus.md` | Menu lockdown, naming, game selector, boot game, cheats menu, Net Options geometry | any row added, removed or reordered in `m_menu.c` |
| `docs/arcade/single-level.md` | Single Level mode and its separate scoring | `SingleLevelMenu`, `M_SingleLevel_*`, `single_level_mode` |
| `docs/arcade/attract.md` | Attract cycle, menu-over-attract backdrop, idle timeout, arcade death | `D_AdvanceDemo`, `G_Idle_Timeout_Check`, `G_Arcade_Death_Check` |
| `docs/arcade/spectre-fuzz.md` | The original fuzz effect for spectres and partial invisibility, in both renderers | `HWR_DrawFuzzSprite`, the `MF_SHADOW` branch of `HWR_DrawSprite`, `CV_Fuzzymode_OnChange`, `R_DrawFuzzColumn_*` |
| `docs/arcade/hud.md` | Status bar overlay elements (`kahmfeistb`) | `ST_overlayDrawer`, the `overlay` cvar |
| `docs/arcade/screen-wipe.md` | Melt and crossfade, the `screenlink` cvar, the hardware wipe path | `f_wipe.c`, the wipe block in `D_Display`, `ReadScreenRect`/`DrawScreenRect` |
| `docs/arcade/gameplay-defaults.md` | Weapon switching, deathmatch defaults, weapon dropping | gameplay cvar defaults (several are demo-sensitive) |
| `docs/arcade/drawmode-switching.md` | Per-drawmode config files, the software-drawmode "lockup", why recovery after teardown is unsafe | `V_switch_drawmode`, `SCR_apply_video_settings`, `SCR_SetMode`, `config8p.cfg`/`configgl.cfg`/`confign.cfg` |
| `docs/arcade/software-fullscreen.md` | The SDL2 present path: texture pitch versus the window surface, why software fullscreen skewed, the fullscreen mode list and GPU scaling | `VID_SetMode_vid`, `I_FinishUpdate`, `sdl_texture`/`vidSurface`/`vid.direct*` |
| `docs/arcade/screen-fill.md` | Why 2D pages letterboxed in software and fill now (`V_SCALEEXACT`, the fixed-point draw scale), and why the OpenGL resolution could not be changed | `V_SetupDraw`, `drawinfo` scaling in `v_video.c`, `V_SCALEEXACT` call sites, `VID_SetMode`, `OglSdl_SetMode` |
| `docs/arcade/install-config.md` | Portable `legacyhome`, config verification, command buffer size | `legacyhome` resolution, `m_misc.c`, tracked `config.cfg` |
| `docs/arcade/audit.md` | Operator bookkeeping counters, the Audit page, `audit.dat` | `au_stuff.c`, `AU_*` call sites, the Audit page in `m_menu.c` |
| `docs/arcade/building.md` | The build scripts: capability probing, the traps they encode, Windows | `tools/build.sh`, `tools/build.ps1`, `build.bat` |
| `docs/arcade/ci-releases.md` | GitHub Actions: the build-on-push workflow, the release button, the `-march` baseline | `.github/workflows/`, the `--arch`/`-Arch` options |
| `docs/arcade/branding.md` | Fork identity, `VERSION_BANNER`, `DLA_VERSION` from tags, the executable name, what must NOT be renamed | any string that names the program, the version, `EXENAME` |
| `docs/arcade/endoom.md` | The exit text screen: lump format, why SLADE will not edit it, `tools/endoom.py` | the `ENDOOM` lump, `endtxt.c`, `I_Show_EndText` |
| `docs/arcade/gotchas.md` | Debugging archaeology: demo desync, encoding, palette tints, PK3/music limits | when something behaves impossibly |

### Where the arcade code lives

The table above maps subjects to *design notes*; this one maps them to *source*. Every local
addition carries an `// [Arcade]` marker, so **`grep -rn "\[Arcade\]" svn1749/src` is the complete
inventory** — over 500 of them, and the comment on each line usually says why. Use this table to
pick a starting point, then grep from there. `hs_stuff.c`/`.h` is the only wholly new pair of files;
everything else is arcade blocks inside an upstream file.

| subject | start here |
| --- | --- |
| Operator audit counters | `au_stuff.c` entire. Hooks: `AU_Init` (`d_main.c`), `AU_Ticker` (`G_Ticker`), `AU_Game_Started` (`G_InitNew`), `AU_Level_Started`/`AU_Level_Completed` (`g_game.c`), `AU_Player_Death` (`p_inter.c`), `AU_Unranked`/`AU_Board_Placement` (`hs_stuff.c`), `AU_Save` (`d_netcmd.c`, `d_main.c`) |
| Scoring, boards, record demos | `hs_stuff.c` entire. Life cycle: `HS_NewGame` → `HS_LevelExit` → `HS_Run_Finished`, voided by `HS_Player_Died` / `HS_Player_Cheated`, ruleset in `HS_Apply_Ranked_Ruleset` / `HS_Ruleset_Is_Ranked`. Called from `g_game.c` (`G_DoCompleted`, `G_DoWorldDone`), `p_inter.c` (`P_KillMobj`), `m_cheat.c`, `wi_stuff.c` |
| Attract cycle | `d_main.c`: `D_AdvanceDemo` / `D_DoAdvanceDemo` / `D_PageTicker` / `D_PageDrawer`, the `hs_attract_page` / `hs_page_after_demo` / `hs_subpage_tic` page state, `D_Menu_Over_Attract`, `D_Demo_Advance_Retry` |
| Arcade death, idle timeout | `g_game.c`: `G_Arcade_Death_Check`, `G_Player_Death_Settled`, `G_Idle_Timeout_Check`, and the `death_ended_run` / `finale_after_intermission` flags they set for `G_DoWorldDone` |
| Four players, panels, view grid | `d_clisrv.c`: `localplayer[]`, `D_NumViews`, `D_View_Grid`, `D_Cell_Pos`, `D_View_Cell_Pos`, `D_View_Squash`, `D_Panel_Of`, `D_View_Cell`, `D_Reset_View_Cells`. Viewport geometry in `d_main.c` `D_Display`; software draw window in `r_draw.c` (`R_View_Fills_Cell`, the cell tables) |
| HUD | `st_stuff.c` `ST_overlayDrawer` (per-view, and the quarter-screen half-scale block). `hu_stuff.c` for the per-view rankings, the attract demo caption, `PRESS FIRE TO START`, the `UNRANKED` markers and `HU_Draw_Tip` |
| Menus | `m_menu.c`, by region: operator cvars ~475–530, main/new-game lockdown ~1150–1270, Single Level ~1670–1860, cheats ~2780–2930, join screen ~2935, initials entry ~3134, game select and level packs ~3520–3790, program restart ~3791 |
| Input | `g_input.c`: `gamecontrol_pl[]`, the per-panel presets, `cv_customcontrols` and the guided setup. `sdl/i_system.c` for joystick slots, hotplug, both sticks and the triggers |
| Config handling | `m_misc.c` (backup generation, `M_Verify_Config`, the player-session no-write rule) and `command.c` (command buffer size, and the loud complaint when text is dropped) |
| Demos | `g_game.c`: `G_BeginRecording` and the `DEMOHDR_*` offsets it patches, the playback overrides, `G_SnapshotDemo` for the background record-demo buffer |
| Engine fixes | `r_draw24.c`/`r_draw32.c` (heightmask), `hardware/r_opengl/r_opengl.c` (texture clamp), `hardware/hw_bsp.c` and `f_wipe.c` (wipes), `sdl/i_video.c` |
| Whole-screen 2D page scale | `v_video.c`: `V_SetupDraw` (`x_scale`/`y_scale`, `x0_scale`/`y0_scale`) and the `V_scale_x`/`V_scale_y` macros in `v_video.h`. Flag set by `D_PageDrawer`, `HS_Draw_AttractTable`, `WI_Drawer`, `F_Drawer` |
| Node rebuilding (slime trails) | `nodebuild/` — vendored ZDBSP, GPLv2+, plus `nb_build.cpp`/`nb_build.h`. Built by `P_Rebuild_Nodes` (`p_setup.c`) into `rbsp_*` and used for **rendering only**, via `R_Use_Render_BSP`/`R_Use_Play_BSP`; `-nonodebuild` disables it |

`devmode` (`extern byte devmode`, `doomincl.h`, defined in `d_main.c`) is the single flag most of
this is gated on.

## Hard-won rules

These bite across the whole tree, usually while working on something else entirely. Each is
written up in full in the doc named beside it.

- **The fork names itself; upstream keeps its credit.** `VERSION_BANNER` is
  `Doom Legacy Arcade <DLA_VERSION>` and must stay under ~56 characters — `D_Make_legacytitle`
  centres it in a line that also carries the build date and time, which overwrite anything
  longer. `DLA_VERSION` is derived from the release tags; the CI overrides it, its checkout
  being shallow. **Never touch `DL_VER_*`/`VERSION`/`REVISION` to rebrand** — those go into
  demo headers and savegames, and changing them rejects every record demo on the cabinet.
  → `branding.md`
- **Menus are addressed by hardcoded position.** Several arrays in `m_menu.c` are indexed by
  number, so inserting or removing a row shifts indices elsewhere. Keep each `*_menu`/`mcontrol_*`
  enum in step with its array, and grep the **handlers** too — an `IT_CALL` handler's `choice`
  argument *is* the item index (`grep -n "choice ==" m_menu.c`). Missing this started a dedicated
  server from a menu row about bots. → `menus.md`
- **`config.cfg` overrides compiled defaults, and only a `-devmode` session writes it.** Changing a
  default in source does nothing on a machine that already has a config. This has bitten three
  times (overlay element letters, the level clock, weapon switching) and each time read as the
  feature being broken rather than unconfigured. → `install-config.md`, `hud.md`
- **A cvar's `OnChange` can fire before the subsystem it talks to exists, and the loss is silent.**
  `config.cfg` is executed well before the renderer is set up, so the GL cvars' handlers — all
  guarded with `if( HWD.pfnSetSpecialState )` — did nothing at all, and nothing re-applied them.
  The cabinet asked for `Nearest` texture filtering and rendered `Bilinear` for the life of the
  build. Re-apply such settings once the subsystem is up. → `install-config.md`
- **Menu code that depends on the gamemode, on cvars, or on where wads are goes in `M_Configure`,
  not `M_Init`.** `config.cfg` is not loaded and `IdentifyVersion()` has not run when `M_Init`
  executes, so the values there are always the compiled defaults. → `menus.md`
- **Never eyeball text layout — measure against the real `STCFN` lumps.** `hu_font` is
  proportional, glyphs are 7 tall, lowercase folds to uppercase and there is no `|`. Measure a
  variable-width field at its **widest** glyphs (`M`/`W`, `888:88.99`) and check each gap
  **end-to-start**, not start-to-start. Derive it in a script, not by hand. → `high-scores.md`
- **`V_DrawString` colours read backwards: option `0` is red, `V_WHITEMAP` is grey.** There is no
  red flag. White text vanishes into the intermission's grey background. → `gotchas.md`
- **A cvar's `.EV` is a byte; reading a cvar whose range exceeds 255 through it truncates
  silently.** `cv_idletimeout` allows 0..3600, so `.EV` turns 3600 into 16 with no warning and no
  clamp — the config, the menu and the cvar are all correct and only the arithmetic downstream is
  wrong. Use `.value` for any cvar that can exceed 255, and note that different readers of the
  same cvar can disagree. → `gotchas.md`
- **The BSP the renderer walks must not be the one the simulation walks.** `p_sight.c` traverses
  the nodes for line-of-sight and `R_PointInSubsector` is used across the play code, so swapping in
  a rebuilt tree changes gameplay and desyncs demos — rarely enough to pass a careless test, which
  is exactly how it shipped once. The rebuilt tree lives in `rbsp_*` and is swapped in only for the
  duration of a frame. → `gotchas.md`
- **`-playdemo` plays an external *file*, never an internal lump, and a failed demo run looks like
  a passing test.** Two runs that both failed to load compare 100% identical. Confirm the demo
  started, and diff simulation state rather than pixels. → `gotchas.md`
- **A new gameplay-affecting cvar must go into the demo header *or* `G_demo_defaults()`**, or demos
  desync. Recording and playback do not otherwise agree on it. → `gotchas.md`
- **Drawers run once per frame, so they must be idempotent.** Anything a drawer mutates changes 35
  times a second — advancing a page cursor in one made the attract page flicker through every map.
  → `high-scores.md`
- **A cvar's OnChange does not fire when it is re-set to the value it already holds** (`CV_Set`
  returns early), so side effects on *other* cvars are silently skipped. Re-apply the mapping at
  the point of use rather than relying on the change notification. This is what made only the
  first deathmatch after boot respawn items. → `gameplay-defaults.md`
- **Views, local players, panels and cells are four different things.** `D_NumViews()` is how many
  viewports are drawn, `localplayer[]` who is playing, `D_Panel_Of(pind)` which physical panel
  drives them, `D_View_Cell(pind)` which quadrant they occupy. Conflating any two has broken
  something every time. → `multiplayer-views.md`
- **The view count does not say how the screen is carved up.** Two views are the stacked halves
  *or* side by side (`cv_splitvertical`), so never write `(num_views >= 4) ? (cell & 1) : 0` again:
  ask `D_View_Grid` for the columns and rows, `D_Cell_Pos`/`D_View_Cell_Pos` for where something
  goes, and `D_View_Squash` for which way a view is squashed. A cell is `vid.width / cols` by
  `vid.height / rows`. → `multiplayer-views.md`
- **When widening a per-player cvar, grep for its `[1]` registration.** The declaration being
  `[MAXSPLITSCREENPLAYERS]` proves nothing — six cvars were widened but still registered only up
  to Player2, so panels 3 and 4 could not save them at all. → `multiplayer-views.md`
- **`SDL_UpdateTexture` takes the pitch of the buffer you hand it, not the pitch of anything on
  screen.** The engine draws into its own buffer (`vid.ybytes` per row); `vidSurface` is the
  window's framebuffer, a different buffer that SDL2 never presents, and `vid.direct_rowbytes` is
  its pitch. The two are equal only while the window is exactly the size being drawn — true in a
  window, routinely false in fullscreen — so passing the wrong one worked everywhere except
  fullscreen, where it sheared every row after the first. `SDL_GetWindowSurface` can also return
  NULL (KMSDRM). → `software-fullscreen.md`
- **Never read a pixel depth with `SDL_BITSPERPIXEL()` — use `SDL_AllocFormat()`.** They disagree
  for the packed no-alpha formats: `SDL_PIXELFORMAT_RGB888`, which is what an ordinary X11 window
  has, is 24 to the macro and 32 to an `SDL_PixelFormat`. `vid.bitpp` picks the software drawer, so
  the macro drew 4-byte pixels with the 3-byte drawer and mangled every texture on screen — in the
  software and native drawmodes only, and with `make smoke` passing throughout. → `gotchas.md`
- **The software renderer scales 2D by a whole number; the hardware renderer scales by the exact
  ratio.** `vid.dupx` is `vid.width / 320` truncated, so a 320x200 page drew 1280x600 of a 1366x768
  screen and `D_PageDrawer` tiled a flat into what was left — which is what "the slides don't fill
  the screen" was. Whole-screen pages now pass `V_SCALEEXACT`; menus, HUD and status bar keep the
  whole number on purpose. Scale through `V_scale_x`/`V_scale_y`, never by multiplying out by hand:
  they round, and the rounding is what stops adjacent runs leaving a seam. → `screen-fill.md`
- **An OpenGL resolution change is a real display mode switch, and three separate things stopped it.**
  `VID_SetMode` required `sdl_texture`, which belongs to the software path alone, so every GL mode
  change took the failure branch and left `vid.modenum` describing the old mode.
  `SDL_GetWindowDisplayMode` reports the mode SDL *intends*, not the one it got — use
  `SDL_GL_GetDrawableSize`. And a window created with `SDL_WINDOW_FULLSCREEN` only has its mode
  applied once it has input focus, which a window created moments after its predecessor was destroyed
  does not reliably get: it comes back flagged fullscreen at the desktop's size (winflags 0x517, no
  `SDL_WINDOW_INPUT_FOCUS`, versus 0x717). Pin it with `SDL_SetWindowDisplayMode` and re-assert
  fullscreen. There is no scaling step in the GL renderer, so if the mode cannot switch, GL renders at
  the desktop resolution and nothing else will change that. → `screen-fill.md`
- **A standalone SDL probe that does not reproduce the fault has not exonerated the code.** Three of
  them got the resolution they asked for while the game did not; the fix had to be developed against
  the game. And `SDL_GetError()` is stale unless a call just failed — `'Invalid window'` showed up
  after a *successful* `SDL_CreateWindow` in the working probe too. → `screen-fill.md`
- **A mechanical text-match edit in this tree can land in a commented-out copy of the line.**
  Several drawers in `v_video.c` keep `//    destend = ...` next to the live statement, and the
  indented live text is a substring of the commented one — so the replacement silently hits the
  comment and the real line survives. Check the match count and check where it landed.
  → `screen-fill.md`
- **A drawmode's config file can ask for a screen depth that drawmode cannot do, and the failure looks like a frozen cabinet.** `legacyhome` has one config per drawmode; its `scr_depth` overwrites the validated one, the graphics change then fails *after* the rendermode teardown, and the engine keeps running with a dead display. Also: the console `drawmode` command does not actually switch drawmode — only the menu does. → `drawmode-switching.md`
- **Node-builder rounding shows up as hairline seams in the GL renderer, in two separate
  families, and each needed its own fix.** A diagonal linedef split by a BSP partition gets its
  intersection written to `VERTEXES` as whole numbers, up to ~0.9 map units off the line. Walls
  come from *seg* endpoints, flats from per-subsector polygons, and wherever the two disagree the
  sky backdrop shows through. **Wall against flat**: `P_Remove_Slime_Trails` (`p_setup.c`) puts
  the seg vertices back on their linedef, and `AdjustSegs` (`hw_bsp.c`) then pulls the *polygon*
  corner onto the wall rather than the old reverse — the wall is authoritative because it is
  anchored to map data. **Flat against flat**: `SolveTProblem` handles these, was pruning almost
  every node away (`SearchSegInBSP` compared `BOXRIGHT` against `min_y` instead of `min_x`), and
  was refusing to consider map vertices at all. **`AdjustSegs` must run before `SolveTProblem`** —
  it moves polygon corners now, so T-joins solved first get stranded. Both report counts on every
  level load; the "still not flush" count must stay 0. Check seams over the whole map, not
  through a screenshot: count gap-producing T-junctions. → `gotchas.md`
- **15 source files are ISO-8859, not UTF-8, and grep silently skips them** — no match, no
  warning. Includes `r_main.c`, `p_map.c`, `console.c`, `hardware/hw_main.c`. If a grep says a
  symbol is never written, re-check with `nm ../objs/*.o` before believing it. → `gotchas.md`
- **There are no dep files for most objects, so editing a header does not trigger a rebuild.**
  After changing any header, `make clean && make`. `tools/build.sh` now does this for you — it
  forces a clean when any header is newer than the oldest object — but plain `make` still will not.
  A header that changed a struct leaves the binary holding two disagreeing views of it, and it
  segfaults far from the edit and *intermittently*, since it clears up the moment anything rebuilds
  in full. → `multiplayer-views.md`, `building.md`
- **Returning to the title screen resets very little.** State leaks from the finished game into the
  attract screen. Fix leftover state in `Command_ExitGame_f`, which is the single funnel every
  route back to the title passes through. → `gotchas.md`
- **A binary anyone else will run must not be built with `-march=native`.** It is the build
  scripts' default and it is right only for a machine building for itself: it bakes in whatever the
  *builder's* CPU supports, links and packages without a murmur, and then dies on the target with a
  bare `Illegal instruction`. Pass `--arch '-march=x86-64 -mtune=generic'` (`-Arch` on Windows) for
  anything distributed — and note it implies `--reconfigure`, or an existing `make_options` is
  reused and the flag is silently ignored. → `ci-releases.md`

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
