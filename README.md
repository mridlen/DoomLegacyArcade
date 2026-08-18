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
- **High scores.** Best cumulative time-to-exit per map, skill and category, shown at the
  intermission and in the attract cycle. Two categories: **speed** (just finish) and **max** (100%
  kills and secrets on every level so far).
- **Record demos.** The run that set each record is saved and replayed in the attract cycle,
  captioned with its map, skill and time.
- **A level clock** on the HUD, counting elapsed time — or counting down in a timed deathmatch.
- **Kills / items / secrets** on the HUD, so you can see whether a max run is still alive.
- **Idle timeout.** Walk away and the cabinet returns to the attract screen by itself.
- **A game selector** listing whichever IWADs are actually installed, plus any level packs you drop
  in, so the cabinet can offer several games from one menu.

**For the operator**

- **Settings don't persist for players.** Anything changed during a session is forgotten at the next
  launch. Only an operator session writes the config.
- **A guided control setup** that asks for each control in turn and binds whatever you press —
  stick, buttons, or anything else your panel is wired to.
- **A fixed competitive ruleset.** Gameplay settings are pinned to a vanilla baseline so scores are
  comparable. A run played outside it still plays, but is marked `UNRANKED` and records nothing.
- **Portable install.** The whole configuration lives next to the binary, so the cabinet is one
  directory to copy or back up.

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

**One or Two Player** from the main menu, then pick a skill. Two-player is splitscreen and uses a
second set of controls on the same machine.

Under **Options** a player can change the crosshair, their colour, their control scheme, and pick a
game or level pack. Everything else is hidden, and nothing a player changes survives to the next
launch.

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

**Single Level** on the main menu plays one map and comes straight back to the same page, so you can
retry immediately. Pick the map and skill and the best speed and max times for that exact map are
shown right there. If a record demo exists you can watch it with **Watch speed run** or **Watch max
run**; those are greyed out when nothing has been recorded yet.

Single Level keeps its **own high score table**, separate from campaign runs — a one-map time isn't
comparable to a run that reached the same map from level one. Those times appear only on this page,
not in the attract cycle.

### High scores

The table tracks the best **cumulative** time from the first level of a run to the exit of each
map, per skill and per category:

- **SPEED** — just reach the exit.
- **MAX** — reach the exit having taken 100% kills *and* 100% secrets on every level of the run so
  far. Items are not required. Miss either on any level and the run drops to speed-only for the
  rest of that game.

A run is scored only under the standard ruleset. Change a gameplay setting and the HUD shows
`UNRANKED` — you can play on, but nothing is recorded. **Dying also ends scoring** for the rest of
the run: levels already finished keep their records, but nothing after counts, and the HUD shows
`PLAYER DIED - UNRANKED`. Start a new game to try again.

Scores are per game *and* level pack — Doom II's `MAP01` and Plutonia's `MAP01` are different
levels and keep separate records.

### Level packs and IWADs

Drop any `.wad` level pack into `legacyhome/levels/` and it appears under **Options → Select Game**
as `<game> wad: <name>`, alongside the installed games. Packs are loaded on demand rather than at
startup: selecting one loads it, its maps replace the IWAD's, and the normal One or Two Player flow
then plays it. Selecting it again unloads it. One pack at a time.

Packs are filtered by the game they suit — a `MAPxx` pack shows under Doom II, an `ExMy` pack under
Ultimate Doom — so a mismatched pack can't be loaded by accident.

**Switching IWAD restarts the program.** The engine can only pick its game data at startup, so
choosing a different game from the menu relaunches the cabinet — expect a black screen and the
startup sequence. This is normal and takes a second or two. Level packs are different: they load
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

**Options → Menu Options → Boot Game** (devmode only). By default the cabinet starts in whichever
IWAD the search happens to find first, which is rarely the one you want. Set this to `doomu`,
`doom2`, `plutonia` or `tnt` and it boots there every time; `None` restores the default behaviour.

The names are the same ones `-game` takes. A `-game` or `-iwad` on the command line overrides the
setting, and if the chosen game is ever uninstalled the cabinet warns and falls back to the normal
search rather than refusing to start.

Like every operator setting, it is only saved from a `-devmode` session.

### Resetting the high scores

From the console, or at launch:

```sh
./doomlegacy -clearhighscores
```

This clears the table *and* deletes the saved record demos. Deleting `highscores.dat` by hand is
not enough — the table is held in memory while the game runs and gets written back out.

### Other useful flags

| Flag | Effect |
| --- | --- |
| `-devmode` | Unlock menus, save settings, disable the ruleset |
| `-clearhighscores` | Wipe scores and record demos at startup |
| `-game <name>` | Start a specific game (`doomu`, `doom2`, `plutonia`, `tnt`) |
| `-warp <map>` | Jump straight to a map |
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
what you meant to change. See [`cabinet/README.md`](cabinet/README.md).

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
