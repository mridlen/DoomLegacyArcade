# DoomLegacy — Arcade Cabinet Build

A build of [DoomLegacy](http://doomlegacy.sourceforge.net/) 1.48.18 customised to run unattended in
an arcade cabinet: locked-down menus, joystick-and-buttons navigation, an attract cycle, and a
persistent high-score table with saved record demos.

It plays Ultimate Doom, Doom II and Final Doom (Plutonia and TNT), plus level packs in `.wad` form.
You supply the game data — no copyrighted content is included here.

> Looking for internals, or hacking on the code? See [`CLAUDE.md`](CLAUDE.md), which documents the
> engine architecture and every local change in detail. This file is for people who want to *run*
> the thing.

---

## Use of AI disclaimer

The arcade customisations in this repository were almost entirely vibe-coded using Claude. The
original DoomLegacy underneath them, of course, was not.

Be aware that promoting AI-assisted software in places such as the Doomworld forums or the ZDoom
Discord is likely to get you banned.

## What's different from stock DoomLegacy

**For the player**

- **Menus are locked down.** New Game, a handful of Options, and Quit. No save/load, no multiplayer
  setup, no video or sound settings to get lost in.
- **The cabinet buttons drive the menus.** No keyboard needed — the stick moves the cursor, fire
  selects, use backs out.
- **Up to four players on one machine.** Two share the screen as the usual stacked halves; three or
  four get a 2x2 grid, each with their own HUD. A **join screen** after the skill select lets each
  panel press fire to be counted in, so three players at panels 1, 3 and 4 is unambiguous.
- **Single Level mode.** Play one chosen map and come straight back to the menu to retry it, with
  its own separate high score table and its own record demos.
- **Single Player - Survival.** A campaign run is scored on **how far you got in the episode**,
  with the faster run winning a tie — so dying on E1M7 beats dying on E1M3 however quick the latter
  was, and finishing the episode tops the board because nothing outranks it on progress. One record
  per episode, difficulty and category, shown at the intermission and in the attract cycle. Two categories: **speed** (just finish) and **max** (100%
  kills and secrets on every level so far). Times are kept to hundredths of a second, because whole
  seconds cannot separate two E1M1 runs.
- **A run leaderboard, with your initials on it.** Separate from the per-map best times: a board of
  whole runs, ranked by **how far you got first and how fast second**. That means a run ending in a
  death partway through — which is how most runs end — still has somewhere to land, while finishing
  the episode naturally tops the board. Ten places for the campaign, three per map for Single Level.
  Finish a run that makes the board and you are asked for three initials. The page opens on the
  last player's initials, so a regular playing run after run confirms with one press; it goes back
  to `AAA` once the cabinet has been left alone and the next person is a stranger.
- **Record demos.** The run that set each record is saved and replayed in the attract cycle,
  captioned with the span of levels it covered, its skill and its time — `E1M1-E1M5  UV  MAX
  4:32.17` — under a blinking **PRESS FIRE TO START**, the arcade "insert coin" on a machine that
  takes no coins.
- **A chase camera on some record demos.** Every third record demo in the attract cycle is shown
  from behind the player, captioned with a blinking **CHASE CAM**. Watching somebody's record run
  over their shoulder reads as a *person playing*; a first-person demo can look to a passer-by like
  the machine has frozen. Only record demos get it — the stock Doom demos are nobody's record.
- **Pacifist and Tyson runs are tracked too**, in Single Player and Single Level, each with its own
  score pages. **Pacifist** means never damaging a monster — shooting past them, running by, and
  letting them fight each other are all fine, but blowing up a barrel that hurts one is not.
  **Tyson** means 100% kills with only the fist, chainsaw and pistol; you may carry other weapons,
  you just may not fire them. Neither needs choosing in advance: every run is measured against all
  four categories at once, so a quick run of the first map usually takes the pacifist board without
  anyone trying. When a run is still holding one of them, **PACIFIST** or **TYSON** blinks at the top
  of the intermission.
- **A level clock** on the HUD, counting elapsed time — or counting down in a timed deathmatch.
  In Single Player a second line above it, **TT**, shows the total time for the whole run so far, so
  you can see both how long this level is taking and how the run is going. The two line up in
  columns. Single Level games do not show it — one map, so it would only repeat the level clock.
- **Kills / items / secrets** on the HUD, so you can see whether a max run is still alive.
- **Idle timeout.** Walk away and the cabinet returns to the attract screen by itself.
- **A game selector** listing whichever IWADs are actually installed, plus any level packs you drop
  in, so the cabinet can offer several games from one menu.

**For the operator**

- **Settings don't persist for players.** Anything changed during a session is forgotten at the next
  launch. Only an operator session writes the config.
- **A guided control setup** that asks for each control in turn and binds whatever you press —
  stick, buttons, or anything else your panel is wired to. One per panel, up to four.
- **A cheats menu** — god mode, all weapons and keys, no clipping, exit level. Operator-only by
  default, or leave it up for players. Using one voids that run's score.
- **A configurable initials timeout** (Options → Arcade Options, 60 seconds by default), for how long
  the initials page waits before accepting what is on it. Nothing is waiting on it — the cabinet is
  already back on the attract screen behind the page — so it can afford to be patient.
- **A quieter attract screen.** The cabinet advertises itself with sound, but not at playing volume
  all day. **Options → Arcade Options → Attract Volume** is a percentage of the normal volumes,
  applied whenever the attract cycle is on screen and dropped the instant a game starts; `0` makes
  the attract screen silent, `100` is the old behaviour. Defaults to 50. **Pressing anything brings
  the sound straight back up to normal** — the menu you land on should not be quieter than the demo
  that got your attention — and it drops back down again if you walk away without starting a game.
- **A chase camera switch.** **Options → Arcade Options → Chase Cam Demo** turns the third-person
  attract demos on and off. On by default; it costs nothing on a cabinet with no records yet, since
  there is then no record demo to show that way.
- **An audit page**, the way an arcade board has one: games played and how many people were
  playing, levels finished, deaths, how much of the cabinet's running time is actually being
  played, which maps get played most, and how often a run stopped being scored and why. Under
  **Options → Arcade Options → Audit**, or type `audit` at the console.
- **A boot game setting**, so the cabinet always starts in the game you chose rather than whichever
  IWAD the search finds first.
- **Deathmatch that ends by itself.** A five-minute default time limit, configurable, and dropped
  weapons — nobody can be left stuck in a stalemate on an unattended machine.
- **Config safety.** Every save keeps a backup, and lines that fail to apply are reported at startup
  instead of silently doing nothing.
- **A fixed competitive ruleset.** Gameplay settings are pinned to a vanilla baseline so scores are
  comparable. A run played outside it still plays, but is marked `UNRANKED` and records nothing.
- **Portable install.** The whole configuration lives next to the binary, so the cabinet is one
  directory to copy or back up.

---

## Fixes to the engine itself

Most of the work above sits on top of DoomLegacy. Some of it went *into* it — the list below is
bugs and shortcomings in stock DoomLegacy 1.48.18 that the cabinet ran into and fixed, rather than
anything the arcade build introduced. None of them are arcade-specific, so they may be of interest
to anyone else running this port. Each is written up in full in the commit that made it and in
[`docs/arcade/`](docs/arcade/).

**Crashes and lockups**

- **The software renderer crashed on every sprite at 24 or 32 bits per pixel.** `R_DrawColumn_24`
  and `R_DrawColumn_32` declared their height mask unsigned where the 8-bit drawer declares it
  signed, turning a no-op mask into a read four gigabytes past the texture. On a modern desktop
  colour depth that meant a segfault a second or two into any software-mode game.
- **Selecting a software drawmode from the video menu killed the display.** Each drawmode's config
  file carries its own colour depth, and nothing checked it against what that drawmode can actually
  do — so a palette mode asked for a 32-bit screen, the mode change failed *after* the renderer had
  already been torn down, and the engine carried on running with nothing on screen. It looks
  exactly like a freeze.
- **A menu on which nothing is selectable hung the game.** The cursor's up/down search is an
  unbounded loop looking for a selectable row, so a page where every row is disabled spins inside
  the event handler for ever — no tics, no redraw, no way out. Both loops are bounded now.

**OpenGL**

- **Every patch had a black outline** — sprites, the HUD, menu graphics, the intermission
  animations. Textures were clamped with OpenGL 1.0's `GL_CLAMP`, which samples the *border*
  colour, so filtering blended a transparent-black fringe into all four edges. Worst on the
  intermission animations, where magnification turns that fringe into a visible 2–3 pixel line.
- **OpenGL settings in the config never reached the driver.** `gr_filtermode`, `gr_fogdensity` and
  `gr_polygonsmooth` all have change handlers guarded on the GL function table existing — and the
  config is executed long before the renderer is set up, so the handler silently did nothing and
  nothing re-applied it afterwards. A config asking for `Nearest` filtering rendered `Bilinear` for
  the life of the build, while displaying `Nearest` in the menu.
- **The screen strobed on every level load.** The BSP walk drew a "Loading... N%" box about fifty
  times, each one forcing a page flip with no frame behind it, so it alternated between two stale
  buffers as fast as the GPU allowed. Three startup-only status messages were forcing full repaints
  on top of that.
- **The screen melt and crossfade never ran under OpenGL.** Both were implemented, the setting
  existed and the menu row was there, but the whole wipe was gated on the software renderer. It
  works in both now. Two latent bugs fell out of that: a wipe that hit its two-second timeout left
  freed state behind for the next one, and the screen capture ran even when the wipe was off.
- **The spectre fuzz effect did not exist in OpenGL** — every partially invisible thing was drawn
  as flat translucency. The original boiling-outline effect is now reproduced on the hardware path,
  as far as a fixed-function backend can.
- **A level's palette tint outlived the level.** Finishing a level in a radiation suit left
  everything after it green, and taking a hit at the exit switch left it red, right through the
  intermission and into whatever came next — the tint is only ever reset when the *next* level
  starts.

**Demos**

- **Demos desynced whenever `tiredrun` was on, which is DoomLegacy's own default.** Playback
  force-disables the Legacy gameplay extras, recording does not, and none of them were written into
  the demo — so a demo recorded with tired-run replayed without it and drifted apart over a few
  thousand tics.
- **Rocket smoke trails desynced any demo with a rocket in it.** `A_SmokeTrailer` timed itself off
  the raw tic counter, which is zeroed once per process and never per game, so its phase at the
  start of a run was however long the machine had been sitting idle. Upstream had already fixed the
  identical bug in the other copy of it (`A_Tracer`) and missed this one.
- **The demo header described the *previous* game** — skill, episode, map, deathmatch, respawn and
  fast monsters were all written before the new game's settings had been applied. Harmless to
  playback, misleading to anything that reads a header.

**Gameplay**

- **Nightmare's fast monsters never sped up demons or spectres.** Fast fireballs worked; the other
  half of the setting halves the sarge frame durations, and with MBF21 compiled in it only touches
  frames carrying a flag that nothing ever set on the vanilla frames. The restore path was broken
  as well — a bitwise `and` between `1` and `2` — so the timings would never have been put back.
- **You could climb on top of monsters and get stuck.** Vanilla Doom things are infinitely tall;
  Legacy applied Heretic's over-under passing to the Doom player unconditionally, with no setting
  for it, which is how a player ends up wedged somewhere vanilla cannot reach — the lift by the
  E1M2 exit being the cabinet's own example. There is now a **Monster Height** setting, defaulting
  to vanilla, with Heretic exempt.
- **Deathmatch rankings covered the whole screen when anybody died**, replacing both views in an
  ordinary two-player game rather than just the dead player's. They are drawn per view now.
- **Time Limit in Net Options did nothing.** The row edited the engine's own limit, which is
  rewritten at every game start — forced to five minutes for deathmatch and to zero otherwise — so
  a typed value was overwritten before anything could read it, and the row displayed whatever the
  last game had left behind.

**Input**

- **Analog sticks produced no input at all.** The only axis handling was for triggers, gated on the
  joystick's *name* matching one of two literal strings, and compiled out by default besides. A
  stick worked on its d-pad setting and was completely dead in analog mode. Both sticks and the
  triggers are read generically now, on any pad.
- **The right stick was read nowhere**, on any controller.
- **The LT/RT triggers** were behind that same name test, and posted a keypress on every event while
  held rather than once on the transition.
- **A reconnected gamepad landed on top of another player.** SDL2 event ids are per-device instance
  numbers, not slots, and the code clamped them into the four-slot array — so a pad that slept and
  woke came back as instance 4, 5, … and folded onto the last slot, silently sharing an identity
  with whoever was already there. Joysticks were also enumerated only at startup, so anything
  plugged in later was invisible for the life of the process, and all four pads shared one d-pad
  state between them.
- **The control-name table was one entry out of step with the control enum**, and had been for as
  long as the feature that shifted it has been compiled in. It round-trips, so bindings worked —
  but `config.cfg` recorded them under the wrong names, which matters the moment anyone reads or
  hand-edits that file.

**Configuration**

- **`config.cfg` was being silently truncated at 8 KB.** `exec` pushes a whole file into the command
  buffer in one go, and the buffer was capped at 8192 bytes; the cabinet's config is 8195. The only
  sign was one line scrolling past in the console. Every setting past the cut kept its compiled
  default and was then written back over the file — 28 of 188 settings lost at every load. That is
  the whole mechanism behind "my config blew itself away".
- **Settings that fail to apply are now reported at startup**, by line number, rather than leaving
  the cvar at its default with no indication. `cfgcheck` repeats the check on demand, and every
  save keeps a `config.cfg.bak`.

**Smaller things**

- **The Launcher screen** no longer appears on every launch, only after an actual startup error.
- **Screenshots are on F12** rather than the stock SysRq (Alt+PrtSc), which a GNOME desktop
  intercepts before the game ever sees it.
- **Menu letter shortcuts** no longer jump the cursor onto hidden rows.
- **Splitscreen is cleared on the way back to the title screen**, so what follows isn't drawn in a
  split view.
- **Episode-ending maps show the intermission.** Vanilla skips it on E1M8 and friends and goes
  straight to the finale, which also skips everything hanging off the intermission — the cabinet's
  per-level scoring among it. Doom II already did it the other way round for MAP30.
- **Two latent draw-layer inconsistencies**: `V_DrawString` ignores horizontal centring in hardware
  mode where fills and patches apply it, and text positions by a float scale factor where
  everything around it uses the rounded integer. Neither showed at full screen size; both throw
  anything drawn at half scale off its background.

---

## Requirements

A Linux machine with a C compiler and these development packages:

| Need | Debian/Ubuntu | Fedora |
| --- | --- | --- |
| SDL2 | `libsdl2-dev` | `SDL2-devel` |
| SDL2_mixer | `libsdl2-mixer-dev` | `SDL2_mixer-devel` |
| OpenGL | `libgl1-mesa-dev libglu1-mesa-dev` | `mesa-libGL-devel mesa-libGLU-devel` |
| libzip | `libzip-dev` | `libzip-devel` |
| zlib | `zlib1g-dev` | `zlib-devel` |

Plus `gcc` and `make`.

Hardware-wise almost anything modern is enough — the renderer is from 1993. The one thing that
matters is **single-core speed**: the game is single-threaded and its main loop never sleeps, so it
will sit at 100% of one core permanently, including while idling on the attract screen. Budget for
sustained load rather than average, and make sure a fanless machine in a sealed cabinet won't
thermally throttle.

## Building

Build from `svn1749/src`. First time only, copy the platform options file and make three edits:

```sh
cd svn1749/src
cp ../make_options_nix ../make_options
```

Then edit `svn1749/make_options`:

```make
SDL2=1                      # uncomment; the stock file targets SDL 1.2
ARCH=-march=native          # replace ARCH=-march=i686, which is 32-bit only
ENV_CFLAGS=-std=gnu17       # add; GCC 15 defaults to gnu23, which breaks this code
```

Each of those is a hard build failure if skipped, not a warning. Then:

```sh
make
```

The binary lands in `svn1749/bin/doomlegacy`, together with a `legacyhome/` folder holding the
cabinet's configuration.

On a different CPU — a Raspberry Pi or other ARM board — replace `-march=native` with the
appropriate flag, e.g. `-mcpu=cortex-a53`. No x86 assembly is involved, so nothing else changes.

## Installing the game data

Copy `common/legacy.wad` and your IWADs into the same directory as the binary:

```sh
cp ../../common/legacy.wad ../bin/
cp /path/to/DOOM2.WAD ../bin/
```

`legacy.wad` is required — it ships with this repository and holds the engine's own menu graphics.
IWADs are the commercial game data and are **not** included; supply your own from a purchased copy.

The Select Game menu offers four: **Ultimate Doom** (`DOOM.WAD`, also accepted as `DOOMU.WAD` or
`DOOM_SE.WAD`), **Doom II** (`DOOM2.WAD`), **Plutonia** (`PLUTONIA.WAD`) and **TNT** (`TNT.WAD`).
Names are case-insensitive. Only games whose IWAD is actually found are listed, and the Select Game
entry disappears altogether if fewer than two are installed.

As well as beside the binary, the game searches `<bindir>/wads/`, `~/games/doom`,
`~/games/doomwads`, `~/games/doomlegacy/wads` and the usual system locations, so an existing
install is usually found without moving anything.

The underlying engine also supports Heretic, but the cabinet's game selector does not list it —
that would need an entry adding to `gameselect_arg[]` in `m_menu.c`.

## Running

```sh
cd ../bin
./doomlegacy
```

No arguments needed. The game finds its configuration in the `legacyhome/` folder beside the
binary, so the whole directory can be copied anywhere — a USB stick, another machine — and it will
behave identically. You can launch it by absolute path from anywhere; it locates its own files.

To move the cabinet to another machine, copy that one directory.

Building a dedicated machine? See [Keeping the cabinet running](#keeping-the-cabinet-running) in the
operator guide for the restart-loop wrapper you'll want.

---

## Playing

**New Game → Single Player** or **Multiplayer** from the main menu, then pick a skill. Multiplayer
here means everyone playing on *this* cabinet, sharing the screen.

**End Game** is on the main menu, at the bottom, and appears **only while a game is actually being
played** — any kind: Single Player, Single Level or Multiplayer. On the attract screen there is
nothing to end, so it isn't there.

DoomLegacy's **networked** play between separate machines is still in there, under
**Networked Multiplayer** in a `-devmode` session, but it is hidden from players because it hasn't
been tested in this build — cabinet-to-cabinet play needs two cabinets. Treat it as untested rather
than unsupported: nothing was removed, and it may well work.

Under **Options** a player can change the crosshair, their colour, their control scheme, and pick a
game or level pack. Everything else is hidden, and nothing a player changes survives to the next
launch.

### Joining a game

On a cabinet with more than one control panel, a **join screen** appears once the skill is chosen.
Each panel presses **fire** to be counted in, and the screen is laid out as the game is about to be:
press fire and watch your own square claim itself. It starts when the countdown runs out, or as soon
as anyone already in presses **use**.

Whoever joins plays at the panel they pressed at, so a lone player can use panel 3 and still get the
whole screen. One or two players share the screen as **stacked halves**; three or four get a **2x2
grid**, one quadrant each, with the fourth left empty for three players.

The page is skipped entirely on a single-panel cabinet, and for a single player it starts on the
first press rather than making one person sit through a countdown.

### Controls

**What to build the panel from.** A leverless controller — a hitbox, or "all button" pad — is the
better choice, because an arcade stick can't switch from left to right, or forward to back, fast
enough for Doom. You might consider getting a drop in WASD controller to replace the joysticks,
such as the Mixbox or T-Spin. This is probably the most ideal for responsive control, but take care that
children don't run off with your keycaps!

That said, most people are realistically going to use an arcade joystick, and it works, just not
as well.

**How many buttons.** Six is the minimum for full control. If your panel has eight, consider
binding a run button on one of the spares and turning autorun off. Sticks that offer a mode switch
(analog / d-pad, often marked LS / DP) work either way — both are read as directions.

**The default layout**, on a stick and six buttons:

```
          [1]  [2]  [3]          1  Fire
    \|/                          2  Strafe left
    -O-   [4]  [5]  [6]          3  Strafe right
    /|\                          4  Use / Open
   stick                         5  Weapon down
                                 6  Weapon up
```

The stick both moves and turns. Binding all of this is an operator job — see
[Setting up a control panel](#setting-up-a-control-panel), which needs `-devmode`.

**Two schemes** are offered per player under Options → Player → Player setup: **Look and Move** and
**WASD**. They swap which pair of controls turns and which strafes, and both work on the same
wiring, so it's purely a player preference. Look and Move matches how most joysticks and digital
gamepads are normally set up, and is the better default.

**In the menus**, the same buttons navigate: stick up/down moves the cursor, left/right changes a
setting, **fire** selects, **use** backs out. No keyboard is needed.

### Single Level

**Single Level**, under **New Game** beside Single Player, plays one map and comes straight back to
the same page, so you can retry immediately. Pick the map and skill and the best speed and max times for that exact map are
shown right there. If a record demo exists you can watch it with **Watch speed run** or **Watch max
run**; those are greyed out when nothing has been recorded yet.

Single Level keeps its **own high score table**, separate from campaign runs — a one-map time isn't
comparable to a run that reached the same map from level one. Those times get their own pages in
the attract cycle: a best-times page per difficulty, and a rotating page showing one map's top
three at a time.

**A campaign run's first level competes here too.** Finishing E1M1 on a Single Player run is the
same thing as a Single Level run of E1M1 — a pistol start, one map — so it goes on the same board.
Only the first level: a campaign E1M2 begins with whatever you carried out of E1M1, so it stays out
of it.

### High scores

The table tracks the best **cumulative** time from the first level of a run to the exit of each
map, per skill and per category:

- **SPEED** — just reach the exit.
- **MAX** — reach the exit having taken 100% kills *and* 100% secrets on every level of the run so
  far. Items are not required. Miss either on any level and the run drops to speed-only for the
  rest of that game.

A run is scored only under the standard ruleset. Change a gameplay setting and the HUD shows
`UNRANKED` — you can play on, but nothing is recorded. **Dying also ends scoring** for the rest of
the run: levels already finished keep their records, but nothing after counts. That one is not
called out on the HUD — death ending the run is how the cabinet works, and the death itself already
says so. Start a new game to try again.

Scores are per game *and* level pack — Doom II's `MAP01` and Plutonia's `MAP01` are different
levels and keep separate records.

### Level packs and IWADs

Drop any `.wad` level pack into `legacyhome/levels/` and it appears under **Options → Select Game**
as `<game> wad: <name>`, alongside the installed games. Packs are loaded on demand rather than at
startup: selecting one loads it, its maps replace the IWAD's, and the normal Single Player or
Multiplayer flow then plays it. Selecting it again unloads it. One pack at a time.

Packs are filtered by the game they suit — a `MAPxx` pack shows under Doom II, an `ExMy` pack under
Ultimate Doom — so a mismatched pack can't be loaded by accident.

**Switching IWAD restarts the program.** The engine can only pick its game data at startup, so
choosing a different game from the menu relaunches the cabinet. It shows `SWITCHING GAME...` and
then goes black for the startup sequence, which takes a second or two. Level packs are different: they load
into the running session with no restart, and only unloading one restarts, since the engine has no
way to remove a wad it has already read.

**What works:** ordinary level wads, including Boom-format maps, and DeHackEd/BEX patches
(including MBF21). **What doesn't:** GZDoom mods. There is no DECORATE or ZScript in this engine,
so Brutal Doom and similar cannot run, and `.pk3` files are not supported at all.

---

## Operator guide

Run with `-devmode` to unlock everything:

```sh
./doomlegacy -devmode
```

That gives you the full stock menus, disables the competitive ruleset, and is the **only** mode
that saves settings. The workflow is: launch with `-devmode`, change what you want, quit. Player
sessions then start from that baseline every time.

### Setting up a control panel

**Options → Setup Controls → Guided setup P1** (devmode only). It shows the recommended layout,
then asks for each control in turn — stick directions first, then the six buttons by number —
binding whatever you press. Works with keyboards, joysticks, encoders, anything that reports as a
button. Press ESC to abandon and keep the previous layout.

There is a guided setup per panel, P1 to P4, and a **Player n Controls** page beside each one for
panels with more than six buttons — the guided setup only teaches the ten controls a standard panel
needs, so anything beyond that gets bound on the full page.

### More than one control panel

**Options → Arcade Options → Control Panels** (devmode only) is how many sets of controls the cabinet
has, 1 to 4. It ships at 1, and until you raise it the join screen never appears and panels 3 and 4
have no configuration pages — which reads as those features being broken, when the cabinet simply
hasn't been told they exist.

**Join Time** beside it is how long the join screen waits, in seconds; `0` skips the page entirely.

Set the panel count first, then run the guided setup for each panel. Panels 3 and 4 have no preset
bindings on purpose — the two built-in schemes are chosen so one keyboard can drive two players, and
there is no third set that wouldn't collide.

### Cheats

**Options → Arcade Options → Cheats Menu** (devmode only) puts a **Cheats** entry on the main menu for
players: god mode, all weapons and keys, no clipping, and exit level. It ships off, in which case
the entry is operator-only and reachable just in a `-devmode` session.

Cheats are single-player only, and using any of them — from the menu, the console, or a typed
IDDQD/IDKFA/IDCLIP — voids that run's score. The HUD then shows `PLAYER CHEATED - UNRANKED` for the
rest of the run.

### Keeping the cabinet running

On a dedicated machine, launch the game from a wrapper script that restarts it in a loop, with an
escape hatch on a key that is **not** wired to any cabinet button. Players can then quit the game —
or it can crash — and the cabinet comes straight back up on the attract screen instead of dropping
someone to a desktop.

```bash
#!/bin/bash
cd /path/to/bin || exit 1
while :; do
    ./doomlegacy
    # Escape hatch. Press this key during the pause to stop the loop.
    # Choose something no cabinet button is bound to.
    read -r -t 3 -n 1 key && [ "$key" = "q" ] && break
done
```

Because player sessions never write the config, every relaunch starts from the operator's baseline
regardless of what the last player changed.

### Choosing which game the cabinet boots into

**Options → Arcade Options → Boot Game** (devmode only). By default the cabinet starts in whichever
IWAD the search happens to find first, which is rarely the one you want. Set this to `doomu`,
`doom2`, `plutonia` or `tnt` and it boots there every time; `None` restores the default behaviour.

The names are the same ones `-game` takes. A `-game` or `-iwad` on the command line overrides the
setting, and if the chosen game is ever uninstalled the cabinet warns and falls back to the normal
search rather than refusing to start.

Like every operator setting, it is only saved from a `-devmode` session.

### Replacement music (OGG soundtracks)

There is **no music folder** — the engine only ever reads music from wad lumps, so replacement
tracks have to be packed into a `.wad`. That is the whole trick; everything below follows from it.

Two ready-made wads cover the two games, both of them Andrew Hulshult rerecordings of the original
soundtracks, so they map track-for-track onto the maps:

| Game | Wad | Where |
| --- | --- | --- |
| Ultimate Doom | `IDKFAv2.wad` | https://www.moddb.com/mods/brutal-doom/addons/idkfa-doom-soundtrack |
| Doom II | `Doom2OST.wad` | https://www.reddit.com/r/Doom/comments/1enyv5f/for_anyone_that_wants_to_use_the_new_doom_2_music/ |

Neither ships with this repository. **With a ready-made wad, skip to step 3** — steps 1 and 2 are
for packaging a soundtrack yourself. Both games can be set up at once: they use different lump
names, so `addfile` both wads and each game finds its own.

**1. Name each track after its music lump.** The engine looks for two names, in this order:

| Prefix | Meaning |
| --- | --- |
| `O_<name>` | The replacement slot — tried first, and only when OGG music is enabled |
| `D_<name>` | The normal lump; its contents are sniffed, so an OGG works here too |

Use the `O_` names. The original `D_` lumps then stay in place as a fallback, and you can drop the
wad at any time without having overwritten anything.

For Ultimate Doom, which is what IDKFA covers:

```
O_E1M1 … O_E1M9      episode 1        O_INTRO    title screen
O_E2M1 … O_E2M9      episode 2        O_INTROA   alternate title track
O_E3M1 … O_E3M9      episode 3        O_INTER    intermission
                                      O_VICTOR   victory text
                                      O_BUNNY    end credits
```

Episode 4 has no music of its own — its maps reuse episode 1–3 tracks, so they're covered
automatically once the rest are in place.

Doom II uses a different set of names, which is what a Doom II soundtrack wad carries:
`O_RUNNIN`, `O_STALKS`, `O_COUNTD`, `O_BETWEE`, `O_DOOM`, `O_THE_DA`, `O_SHAWN`, `O_DDTBLU`,
`O_IN_CIT`, `O_DEAD`, `O_STLKS2`, `O_THEDA2`, `O_DOOM2`, `O_DDTBL2`, `O_RUNNI2`, `O_DEAD2`,
`O_STLKS3`, `O_ROMERO`, `O_SHAWN2`, `O_MESSAG`, `O_COUNT2`, `O_DDTBL3`, `O_AMPIE`, `O_THEDA3`,
`O_ADRIAN`, `O_MESSG2`, `O_ROMER2`, `O_TENSE`, `O_SHAWN3`, `O_OPENIN`, `O_EVIL`, `O_ULTIMA`,
`O_READ_M`, `O_DM2TTL`, `O_DM2INT`.

**2. Build the wad.** [SLADE](https://slade.mancubus.net/) is the easy route: *New → Wad Archive*,
drag the `.ogg` files in, rename each entry to its lump name, save it as a `.wad`. Lump names are
limited to 8 characters, which every name above already fits.

**3. Check OGG music is enabled.** **Options → Sound Volume → Music src** must be `Auto`, which is
what the shipped configuration uses. If yours says `MUS` — an older config, or somebody changed it —
replacement music is ignored entirely, and this is the step people miss. Set it in a `-devmode`
session and quit to save it.

Avoid the `MP3` and `OGG` settings: those play *silence* for any track the wad doesn't replace,
rather than falling back to the original. `Auto` prefers the replacement and falls back.

**4. Load it at startup.** Put a line per wad in `legacyhome/autoexec.cfg`, creating the file if it
isn't there:

```
addfile "IDKFAv2.wad"
addfile "Doom2OST.wad"
```

The bare filename is enough as long as the wad sits in one of the usual wad directories, such as
`~/games/doom` — the engine searches them by name. An absolute path works too. `legacyhome/levels/`
is *not* the place for these: packs there are filtered by their maps, and a music wad has none, so
it will never be listed. If you'd rather not use an autoexec, `-file IDKFAv2.wad` on the command
line does the same thing.

Two things to expect: recorded music is far louder than the original MIDI, so **turn the music
volume down** — 3 or 4 rather than the default — and tracks loop from the beginning rather than at
a composed loop point, which is only noticeable on long levels.

### Resetting the high scores

From the console, or at launch:

```sh
./doomlegacy -clearhighscores
```

This clears the table *and* deletes the saved record demos. Deleting `highscores.dat` by hand is
not enough — the table is held in memory while the game runs and gets written back out.

### Taking a screenshot

Press **F12**. The image is written to the directory you launched from, named `DOOM0000.tga` and
counting up — `DOOM0001.tga`, and so on — so nothing is ever overwritten.

Files are **Targa** (`.tga`), because the cabinet uses the OpenGL renderer, and uncompressed: about
3 MB each at 1366x768. Convert before sending them anywhere:

```sh
convert DOOM0000.tga shot.png
```

PrtSc is bound as a fallback, but on a GNOME desktop it never reaches the game — the desktop's own
screenshot tool takes it first. That is why F12 is the default here rather than the stock SysRq
(Alt+PrtSc), which has the same problem and is a two-key combination besides.

To use a different key, rebind **Screenshot** on the player's controls page, or from the console:

```
setcontrol "screenshot" "f11"
```

Set the `screenshotdir` cvar to write somewhere other than the working directory. Both are settings
like any other, so they only stick from a `-devmode` session.

### Other useful flags

| Flag | Effect |
| --- | --- |
| `-devmode` | Unlock menus, save settings, disable the ruleset |
| `-clearhighscores` | Wipe scores and record demos at startup |
| `-clearaudit` | Reset the operator audit counters at startup |
| `-game <name>` | Start a specific game (`doomu`, `doom2`, `plutonia`, `tnt`) |
| `-warp <map>` | Jump straight to a map |
| `-file <wad>` | Load a wad at startup — a level pack, a soundtrack, a DEH/BEX patch |
| `-config <file>` | Use a different configuration file |
| `-v` | Verbose startup, showing which files were found |

---

## Where your data lives

Everything is in `legacyhome/` beside the binary:

| | |
| --- | --- |
| `config.cfg` | All settings. Written only by a `-devmode` session. |
| `highscores.dat` | The score table. Plain text, one record per line. |
| `demos/` | Saved record demos, one per map/skill/category. |
| `levels/` | Level packs you've added. |
| `audit.dat` | Operator bookkeeping counters. Plain text. |

**Back up `highscores.dat` and `demos/`.** They are the only things here that can't be recreated —
your players' scores and the runs that set them. Everything else can be rebuilt from this
repository or reinstalled.

If no `legacyhome/` folder exists beside the binary, the game falls back to `~/.doomlegacy/`
instead, which is the traditional location.

### Keeping configuration in version control

The tracked copy of the cabinet configuration lives at `cabinet/legacyhome/config.cfg`. A build
stages it next to the binary but never overwrites a config already there, so rebuilding won't reset
a running cabinet.

After changing settings in a `-devmode` session, bring them back into the repository with:

```sh
cd svn1749/src
make cabinet_save
git diff cabinet/legacyhome/config.cfg
```

Check the diff before committing — a devmode session saves *everything* in memory on quit, not just
what you meant to change. (`botrandom` is a random seed and always differs; that one is noise.) See
[`cabinet/README.md`](cabinet/README.md).

### If the configuration goes wrong

Every save first copies the old file to **`config.cfg.bak`**, so a bad write is one `cp` away from
being undone. A config written from defaults is easy to spot: `name` will be your Unix login rather
than the player name you set.

Settings that fail to load are **reported at startup** rather than silently ignored, naming the line
number — an unrecognised setting, or a value the engine rejects, otherwise just leaves that setting
at its compiled default and looks like the config was half-read. Run **`cfgcheck`** from the console
to repeat the check at any time.

Expect exactly four reports on a healthy cabinet: `botrandom`, plus `monstergravity`,
`monsterfriction` and `voodoo_mode`. Those three are DoomLegacy defaults that the competitive
ruleset deliberately overrides in a player session. Anything else is worth a look.

---

## Troubleshooting

**A setting I changed on the HUD or in the menus isn't showing up.**
Player sessions don't save settings — that's deliberate. Use `-devmode` to make a change stick.

**The HUD elements aren't drawing at all.**
The overlay only appears at the largest view size, with no status bar. Check `viewsize` is `11` in
`config.cfg`. Which elements show is controlled by the `overlay` line, a string of one-letter
codes — `k` keys, `a` ammo, `h` health, `m` armor, `f` frags, `e` kills, `i` items, `s` secrets,
`t` the level clock. The default is `kahmfeist`.

**A new HUD element still doesn't appear after rebuilding.**
`config.cfg` overrides the compiled default, so an existing install keeps its old `overlay` line.
Add the letter by hand, or re-save from a `-devmode` session.

**Scores aren't being recorded.**
Check the HUD for `UNRANKED`. If it's there, either a gameplay setting differs from the standard
ruleset — the console log names which one — or somebody died. Returning to the attract screen
resets the ruleset automatically, so starting a fresh game normally clears it.

**The join screen never appears, or panels 3 and 4 have no settings pages.**
`Control Panels` under Options → Arcade Options is still at 1. Nothing about the extra panels shows up
until the cabinet is told how many it has. Check `Join Time` isn't 0 while you're there.

**Replacement music isn't playing.**
Check `music_source` is `Auto` rather than `MUS` — that alone disables it, and an older config may
still carry `MUS`. Then check the lump
names inside the wad against the list above; a track named after the *file* rather than the lump
simply never gets looked for. `-v` reports the wad being loaded at startup.

**A game is missing from the Select Game menu.**
Its IWAD wasn't found. Run with `-v` and check the search paths reported at startup.

**The game won't build.**
Almost always one of the three `make_options` edits above. `-march=i686` and the default `gnu23`
standard both produce errors that don't obviously point at the cause.

---

## Credits and licence

DoomLegacy is by Fabrice Denis, Boris Pereira and the DoomLegacy team, based on the original Doom
source released by id Software. This build tracks DoomLegacy 1.48.18 (SVN r1749) with local arcade
customisations.

Licensed under the **GNU General Public License**; see
[`svn1749/docs/LICENSE.txt`](svn1749/docs/LICENSE.txt). Doom, Doom II, Final Doom and Heretic game
data remain the property of their respective owners and are not distributed here.
