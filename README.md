# Doom Legacy Arcade

A fork of [DoomLegacy](http://doomlegacy.sourceforge.net/) 1.48.18 built to run unattended in an
arcade cabinet. It has locked-down menus, navigation with a stick and buttons, an attract cycle, and
a high-score table that keeps the record demos.

It plays Ultimate Doom, Doom II and Final Doom (Plutonia and TNT), plus level packs in `.wad` form.
You supply the game data. No copyrighted content is included.

The program calls itself **Doom Legacy Arcade**, the binary is `doomlegacyarcade`, and every launch
prints what it is a fork of. That way nobody takes a bug here to the upstream project, and upstream
keeps its credit. **This project is not affiliated with the DoomLegacy team.**

> Working on the code? See [`CLAUDE.md`](CLAUDE.md) for the architecture and build, and
> [`docs/arcade/`](docs/arcade/) for a write-up of each feature. This file is for people who want
> to *run* it.

## Use of AI disclaimer

The arcade customisations here were almost entirely vibe-coded using Claude. The original
DoomLegacy underneath them was not.

Promoting AI-assisted software in places such as the Doomworld forums or the ZDoom Discord is likely
to get you banned.

## Contents

- [What's different from stock DoomLegacy](#whats-different-from-stock-doomlegacy)
- [Fixes to the engine itself](#fixes-to-the-engine-itself)
- [Requirements](#requirements) and [Performance](#performance)
- [Building](#building)
- [Installing the game data](#installing-the-game-data)
- [Running](#running)
- [Playing](#playing)
- [Operator guide](#operator-guide)
- [Menu options reference](docs/menu-options.md)
- [Where your data lives](#where-your-data-lives)
- [Troubleshooting](#troubleshooting)
- [Credits and licence](#credits-and-licence)

---

## What's different from stock DoomLegacy

### For players

**Menus and game modes**

- **Locked-down menus.** New Game, a few Options, and End Game while a game is running. No
  save/load, no video or sound settings, and no Quit (the operator can put Quit back).
- **Cabinet buttons drive the menus.** The stick moves the cursor, fire selects, use backs out.
- **One-press games.** New Game offers **Campaign** (solo or co-op, decided by who presses in),
  **Deathmatch** and **Team Deathmatch** (both ask only which map), **Single Level**, and the full
  **Multiplayer** page for anyone who wants every setting.
- **Team Deathmatch** in Red, Blue, Green and Yellow, with no friendly fire. Works across linked
  cabinets.
- **Single Level mode.** Play one map, come straight back to retry it. It has its own score table
  and record demos.
- **No Monsters**, a sixth skill on the Single Level page: the map empty of monsters, played at
  Ultra-Violence, scored separately (the same rules as dsda-doom's "NoMo" category). Its Max
  category is called **100%S**, since it only means finding every secret.
- **A High Scores page** on the main menu, where Read This used to be. Flip through the pages with
  left and right. Read This and its F1 shortcut are gone.
- **A game selector** listing the installed IWADs and any level packs you add.

**Multiplayer on one machine**

- **Up to four players.** Two share the screen stacked or side by side; three or four get a 2x2
  grid, or four columns on a very wide screen. Each view gets its own CPU core, so four players cost
  about the same as one.
- **A join screen** after the game is chosen. Each panel presses fire to join and sets its colour,
  crosshair and controls. It waits **30 seconds** by default.
- **Scoreboards for up to 32 players.** Past 8 (campaign) or 12 (deathmatch) players, the
  intermission switches to a compact two-column layout. Below that it looks as it always did.
- **WINNING banner.** In Deathmatch the leader's view shows **WINNING** in rippling rainbow letters.
  In Team Deathmatch everyone on the leading team sees **BLUE TEAM WINNING** (or red, green,
  yellow) in the team's colour. Nobody gets it during a tie.

**Scoring**

- **Survival runs.** A campaign run is ranked by how far you got in the episode, then by time.
  There are four categories, all measured at once: **speed**, **max** (100% kills and secrets on
  every level so far), **pacifist** (never damage a monster) and **tyson** (100% kills with only
  fist, chainsaw and pistol). Times are kept to hundredths of a second.
- **A run leaderboard with initials.** Make the board and you enter three initials. The page
  remembers the last player's initials until the cabinet has been idle, then resets to `AAA`.
- **Record demos.** The run behind each record is saved and replayed in the attract cycle with a
  caption like `E1M1-E1M5  UV  MAX  4:32.17` under a blinking **PRESS FIRE TO START**.
- **A fixed competitive ruleset.** Gameplay settings are pinned to a vanilla baseline so scores are
  comparable. A run outside it is marked `UNRANKED` and records nothing.

**Picture and HUD**

- **Smoother than 35 FPS.** The engine draws extra frames between the 35 tics a second and
  interpolates movement. The game and demos are unchanged. Capped at 60 by default.
- **Ultrawide (21:9, 32:9) and portrait screens.** The view is drawn at the monitor's real shape with
  a matching field of view, and the HUD, menus and weapon keep Doom's proportions.
- **Full-screen pages.** The attract pages, intermission and finale fill the screen at any
  resolution.
- **HUD additions.** A level clock (plus a **TT** total run time in Single Player); kills, items and
  secrets; and all four ammo types. Each HUD item can be switched off under **Options → Arcade
  Options → HUD Configuration**.
- **No text messages over the game** by default ("Picked up a shotgun" and so on). They can be
  turned back on separately for single player and multiplayer under **Options → Arcade Options →
  Messages + Banners**.

**Attract cycle**

- Shows the title page, the cabinet splash (`CREDIT2`), the credits page (`CREDIT`), a demo, and
  the high-score pages once there are records.
- **Chase camera.** Every third record demo is shown from behind the player, captioned **CHASE
  CAM**, so passers-by see a person playing rather than a frozen screen.
- **Quieter attract sound.** The attract cycle plays at a percentage of normal volume (50% by
  default). Any button press restores full volume.
- **Freeze recovery.** If an attract demo stalls, the cabinet notices within about five seconds,
  moves on, and logs what happened.
- **Idle timeout.** An abandoned game returns to the attract screen after 60 seconds, with a
  15-second warning first.

### For operators

- **Players can't save settings.** Only an operator session (`-devmode`) writes the config.
- **An unlock key.** Press Scroll Lock at the attract screen to restart into operator mode, and
  again to save and lock. See [Unlocking the cabinet](#unlocking-the-cabinet-without-a-command-line).
- **One settings page per player** (**Options → Player → `Player1 config`** to `Player4 config`)
  with colour, crosshair and control scheme.
- **Guided control setup** for each panel: it asks for each control in turn and binds whatever you
  press.
- **Players & Views page** for the panel count and how the screen is divided.
- **Performance page** with Framerate Cap, Render Threads, 8bpp Draw, Row Padding and Show Ticrate.
- **OpenGL screen shaders:** CRT (zfast CRT, CRT-Pi, CRT-Lottes, CRT-Geom), FXAA, Software Look
  (snaps colours to Doom's palette), VHS, Greyscale, Sepia, Night Vision and Game Boy. Off by
  default. Under **Options → Video Options → OpenGL 3D Card Options → Shaders**.
- **A usable video mode list** that pages, sorts, removes duplicates and filters by aspect ratio.
- **Menu switches** under **Options → Arcade Options → Disable/Enable Menu Options**: Cheats Menu,
  Quit Menu, Multiplayer Menu, Game Options and Enable No Monsters.
- **A cheats menu** (god mode, weapons and keys, no clipping, exit level, show coordinates). Using
  one voids the run's score, except Show Coordinates.
- **An audit page** like an arcade board's: games played, player counts, levels finished, deaths,
  play time and the most-played maps. **Options → Arcade Options → Audit**, or `audit` at the
  console.
- **Cabinet Link.** Cabinets on the same network pair with a passcode over an encrypted connection,
  invite each other into games, and share high scores. See
  [Connecting cabinets together](#connecting-cabinets-together-cabinet-link).
- **Boot game setting**, so the cabinet always starts in the game you chose.
- **Deathmatch ends by itself** after five minutes by default, and dropped weapons prevent
  stalemates.
- **Config safety.** Every save keeps a backup, and lines that fail to apply are reported at startup.
- **Portable install.** Everything lives next to the binary, so the cabinet is one directory to copy
  or back up.
- **Visual defaults closer to Doom.** Rocket trails can be switched off (**Options → Effects
  Options → Rocket Trails**; this also covers the lost soul's smoke). **Pickup Flash** defaults to
  `Vanilla` (a yellow tint instead of a green block on the status bar). **Weapon Flash Fix** (on)
  lights the gun while its muzzle flash shows, hiding the hard line in the original art.
  **Translucency** defaults to `Auto`. None of these affect scores or demos.
- **PC speaker sound.** **Options → Sound Volume → PC speaker** plays the beeps Doom made on a PC
  with no sound card, instead of the sampled sound effects. It works like the real speaker: one
  sound at a time (a new one cuts off the last), no stereo, and no fading with distance. Music is
  not affected. Off by default. It does not change scores or demos. Doom and Doom II only; Heretic
  has no speaker sounds, so there it changes nothing.
- **OPL music.** **Options → Sound Volume → OPL music** plays the music the way an AdLib or Sound
  Blaster did, on an emulated FM chip using the game's own instrument set. This is the player
  prboom-plus had. Off by default. See [OPL music](#opl-music-the-sound-blaster-sound).

---

## Fixes to the engine itself

These are bugs in stock DoomLegacy 1.48.18 that the cabinet hit and fixed. None are arcade-specific.
Each is described in full in its commit and in [`docs/arcade/`](docs/arcade/).

**Crashes and lockups**

- The software renderer crashed on every sprite at 24 or 32 bits per pixel (an unsigned height mask
  read 4 GB past the texture).
- Software fullscreen was sheared sideways: the frame was handed to SDL with the wrong row stride.
- Software mode would not start on a display with no mode at or below 1600x1200. It now scales to
  the desktop.
- A colour depth left over from OpenGL in `config.cfg` stopped software mode starting, or killed the
  display when switching drawmode from the menu.
- A menu page with nothing selectable hung the game in an endless loop.
- The sound thread could crash at level start by reading a channel before it was set up. The mixer
  now works from its own locked copy of the channel list.
- A door or staircase thinker could be sorted into the monsters' target list, crashing the game
  after long unattended runs on a Raspberry Pi.

**OpenGL**

- Every patch had a black outline (textures were clamped with `GL_CLAMP`).
- Graphics that are not a power-of-two size had a dark line on their right and bottom edges.
- Sprites, text and the weapon had grey fuzzy outlines or hard flat edges with filtering on. They now
  have room to fade into and blend correctly.
- Bullet holes, imp-fireball and rocket scorches, and blood on walls had the same hard flat edges
  (blood showed straight red streaks). They fade now too, and scorches are drawn at their real
  size instead of shrunk and turned on their side.
- The weapon was lit like a wall instead of like software. **Vanilla Weapon Lighting** (**Options →
  Video Options → OpenGL 3D Card Options → Lighting**) restores the software look. It ships Off.
- A screenshot at 1366x768 crashed the game (row size not a multiple of four bytes).
- GL settings in the config (`gr_filtermode`, `gr_fogdensity`, `gr_polygonsmooth`) never reached
  the driver.
- The screen strobed on every level load.
- The screen melt and crossfade never ran under OpenGL.
- Shaders such as CRT-Geom bent the picture further during a wipe.
- Invulnerability barely showed. It now inverts the view (in cyan tones rather than greyscale).
- The spectre fuzz effect did not exist. It is now reproduced as closely as fixed-function GL allows.
- Blood vanished when it hit the floor.
- Hairline seams where walls meet floors, and where flats meet each other. Gap-producing T-junctions
  on stock maps are now zero.
- Black seams and outlines through see-through bars (such as round E1M1's nukage pool), and bars
  hanging below their openings on 118 line sides in the stock IWADs.
- A palette tint (radiation suit green, damage red) carried over past the end of a level.
- Changing resolution did nothing. Three separate bugs kept OpenGL at the desktop resolution.

**Demos**

- Demos desynced when `tiredrun` was on, which is DoomLegacy's default.
- Rocket smoke trails desynced any demo with a rocket in it.
- The demo header described the *previous* game, so a solo run could replay under multiplayer rules.
- Every DoomLegacy demo replayed with Boom behaviours switched off that were on during recording (a
  version-number comparison across two incompatible numbering schemes).

**Gameplay**

- Nightmare's fast monsters never sped up demons or spectres.
- Players could climb on top of monsters and get stuck. A **Monster Height** setting now defaults to
  vanilla (Heretic is exempt).
- The HUD overlapped itself in a split screen below 640x480.
- The weapon floated in mid-air in side-by-side two-player.
- Death-screen rankings drew every line on top of each other, and covered every player's view instead
  of just the dead player's.
- **Time Limit** in Net Options was overwritten before anything read it.

**Input**

- Analog sticks produced no input, the right stick was never read, and the triggers only worked on
  two named pads. All are now read on any pad.
- A reconnected gamepad could take over another player's slot, and pads plugged in after startup were
  ignored.
- The control-name table was one entry out of step, so `config.cfg` saved bindings under the wrong
  names.

**Configuration and display**

- `config.cfg` was silently cut off at 8 KB, losing 28 of 188 settings at every load.
- Settings that fail to apply are now reported at startup by line number.
- 4:3 resolutions were stretched across widescreen monitors. **Keep aspect** (on by default) adds
  black bars instead.
- At 800x600 the HUD was drawn a third too narrow.
- Full-screen pages were letterboxed in software mode.
- Ultrawide modes were silently dropped. The size cap is now 5120x2160.
- On portrait screens the HUD and weapon were too narrow, split-screen views too tall, the best-times
  table ran together, and wall faces went missing at the screen edges.
- The video mode list lost modes to three stacked limits and never removed duplicates.

**Software renderer speed**

- The 8bpp colour table was rebuilt on every damage and pickup flash step. It is now built once.
- Walls, floors and ceilings draw about 12% faster with 8bpp Draw on.
- Getting the picture to the screen is faster, especially on a Pi (640x360 went from 59 to 72 fps).
- Clearing HUD messages and taking screenshots now handle padded screen rows correctly.

**Smaller things**

- Gamma settings have their own page, **Video Options → Gamma Options** (F11 opens it).
- Low resolutions (320x200 up to 800x600) can be used fullscreen, scaled up sharply by the GPU.
- No 800x600 loading window flashes up at startup, and the Launcher only appears after a startup
  error.
- Screenshots are on **F12** (GNOME intercepts the stock SysRq).
- Menu letter shortcuts no longer jump onto hidden rows.
- Splitscreen is cleared on the way back to the title screen.
- Episode-ending maps (E1M8 and so on) show the intermission, so their scores count.
- The player preview on the colour page was garbled in software mode.
- **Slime trails** (as on the E1M1 stairs) are gone. Nodes are rebuilt at level load with ZDBSP and
  used for rendering only, so gameplay and demos are unchanged. `-nonodebuild` turns it off.

---

## Requirements

A Linux machine with `gcc`, `make` and these development packages:

| Need | Debian/Ubuntu | Fedora |
| --- | --- | --- |
| SDL2 | `libsdl2-dev` | `SDL2-devel` |
| SDL2_mixer | `libsdl2-mixer-dev` | `SDL2_mixer-devel` |
| OpenGL | `libgl1-mesa-dev libglu1-mesa-dev` | `mesa-libGL-devel mesa-libGLU-devel` |
| libzip | `libzip-dev` | `libzip-devel` |
| zlib | `zlib1g-dev` | `zlib-devel` |
| OpenSSL (optional, for Cabinet Link) | `libssl-dev` | `openssl-devel` |

Almost any modern machine is enough. What matters is **single-core speed first, then core count**:
the software renderer spreads a frame across cores, but the simulation and sound run on one thread.
With a framerate cap set, the main loop rests between frames instead of pinning a core. For a
fanless machine in a sealed cabinet, plan for sustained load and check it won't thermally throttle.

### Performance

**A Raspberry Pi 3 is enough.** These figures are from a Pi 3 Model B (quad-core Cortex-A53,
1.2 GHz) with the **software** renderer, **Render Threads** `Auto`, **8bpp Draw** on, **Row
Padding** on and a **1280x720** desktop. They come from `tools/perfchart.py`: the UV speed demo of
E1M1, played flat out with vsync off (September 2026).

| Resolution | Shape | FPS |
| --- | --- | --- |
| 320x200 | 16:10 | 166 |
| 320x240 | 4:3 | 158 |
| 400x300 | 4:3 | 135 |
| 512x384 | 4:3 | 105 |
| 640x350 | ~16:9 | 93 |
| 640x360 | 16:9 | 94 |
| 640x400 | 16:10 | 86 |
| 720x400 | ~16:9 | 80 |
| 640x480 | 4:3 | 77 |
| 720x480 | 3:2 | 71 |
| 768x480 | 16:10 | 68 |
| 800x500 | 16:10 | 63 |
| 864x486 | 16:9 | 61 |
| 800x600 | 4:3 | 56 |
| 960x540 | 16:9 | 53 |
| 928x580 | 16:10 | 51 |
| 960x600 | 16:10 | 48 |
| 1024x576 | 16:9 | 44 |
| 1024x768 | 4:3 | 35 |
| 1152x720 | 16:10 | 34 |
| 1280x720 | 16:9 | 33 |
| 1152x864 | 4:3 | 32 (taller than the desktop) |
| 1280x800 | 16:10 | 31 (taller than the desktop) |
| 1280x960 | 4:3 | 27 (taller than the desktop) |

Tips for a Pi:

- **Set the desktop to 1280x720.** The GPU scales every frame up to the desktop, and it shares a
  memory bus with the CPU. Going from a 1920x1080 desktop to 1280x720 made every size 14% to 46%
  faster. Don't pick a size taller than the desktop.
- **Aim for about 75 in this table to hold 60 in play.** Real play runs roughly 15% slower than the
  benchmark. That means 640x480 or smaller, or 512x384 for headroom in big fights. Check with
  **Show Ticrate** on your busiest level.
- **Set Framerate Cap to the panel's refresh rate** (usually 60). Uncapped only adds heat.
- **Turn Row Padding on.** It roughly halves the draw time of the 1024-wide sizes.
- **Use the software renderer.** The Pi's GPU runs this engine's OpenGL through a slow compatibility
  layer. On a desktop GPU the opposite is true and OpenGL is nearly free.

**Four players cost about the same as one**, within a couple of FPS. One view is split into vertical
bands, one per core; four views get a core each.

Below 35 FPS the game does not slow down; it just skips frames. Above 35, Framerate Cap draws
interpolated frames between tics, so motion gets smoother.

---

## Building

**You may not need to.** Every push to `main` is built for Linux and Windows on GitHub Actions, and
tagged releases carry the same builds on the Releases page. Those target a generic x86-64 CPU so
they run anywhere. Building yourself gets a binary tuned for your CPU.

### The easy way

On Linux, macOS or FreeBSD:

```sh
./tools/build.sh
```

On Windows, double-click `build.bat` (or run it from a command prompt).

The script detects the system and CPU, checks for the compiler and libraries, writes a build
configuration and builds. If anything is missing, it names it and prints the install command for
your distribution:

```
== Checking what is installed
  ok   : C compiler (cc)
  ok   : sdl2-config
  MISS : libzip

Install with:
    sudo dnf install -y gcc make SDL2-devel SDL2_mixer-devel libzip-devel ...
```

| Option (Linux) | Option (Windows) | Effect |
| --- | --- | --- |
| `--install-deps` | `-InstallDeps` | Install missing packages for you |
| `--deps` | | Only check dependencies |
| `--clean` | | Start fresh |
| `--jobs N` | | Limit parallel compiles |
| `--reconfigure` | | Rewrite an existing build configuration |
| `--arch '...'` | `-Arch '...'` | Set the CPU target (implies `--reconfigure`) |

It recognises the Debian, Fedora, Arch and SUSE families and their derivatives. It never overwrites a
build configuration you have tuned unless you pass `--reconfigure`.

**Building for another machine?** The default `-march=native` targets the builder's own CPU and can
fail on another machine with `Illegal instruction`. For a binary anyone else will run, pass
`--arch '-march=x86-64 -mtune=generic'`.

**Windows** builds through MSYS2/MinGW (Visual Studio cannot build this GNU Make tree). The script
tells you how to install MSYS2 and its packages if they are missing. On Windows 11 it builds
`doomlegacyarcade.exe` and copies its sixteen runtime DLLs beside it. **The Windows binary has been
started but not played**, so treat the first real session as a shakedown.

**Checks.** There is no unit test suite. `make smoke` starts the binary headlessly and tests
startup, level setup, a level exit and the OpenGL path. `make demotest` replays every record demo
and checks the simulation is unchanged, tic by tic, in about 40 seconds. Run it after **anything
that could affect gameplay**, because a gameplay change invalidates the cabinet's record demos.
Record the reference once with `make demotest_baseline` on known-good code. See
[`docs/arcade/demo-desync.md`](docs/arcade/demo-desync.md).

### The manual way

Build from `svn1749/src`. The first time, copy the platform options file:

```sh
cd svn1749/src
cp ../make_options_nix ../make_options
```

Then edit `svn1749/make_options`. Each of these is a hard build failure if skipped:

```make
SDL2=1                      # uncomment; the stock file targets SDL 1.2
ARCH=-march=native          # replace ARCH=-march=i686, which is 32-bit only
ENV_CFLAGS=-std=gnu17 -g    # add; GCC 15 defaults to gnu23, which breaks this code
```

The `-g` is optional but worth keeping: it makes crash backtraces readable and costs nothing at
runtime. On a Raspberry Pi or other ARM board, use a flag such as `-mcpu=cortex-a53` instead of
`-march=native`.

```sh
cd ..           # svn1749
make dirs       # create bin/, objs/ and dep/
cd src
make depend     # always serially, before any parallel build
make -j8
```

Run `make depend` on its own first: parallel dependency generation clobbers a shared temporary file
and fails with `mv: cannot stat '../dep/sed.dep'`. The compile itself parallelises fine.

The binary lands in `svn1749/bin/doomlegacyarcade`, beside a `legacyhome/` folder holding the
cabinet's configuration.

---

## Installing the game data

The build scripts copy `common/legacy.wad` beside the binary. You only need to add an IWAD:

```sh
cp /path/to/DOOM2.WAD svn1749/bin/
```

**IWADs** are the commercial game data and are not included. Select Game offers four:

| Game | File |
| --- | --- |
| Ultimate Doom | `DOOM.WAD` (or `DOOMU.WAD`, `DOOM_SE.WAD`) |
| Doom II | `DOOM2.WAD` |
| Plutonia | `PLUTONIA.WAD` |
| TNT | `TNT.WAD` |

Names are case-insensitive. Only installed games are listed, and Select Game is hidden when there
is only one game or pack to choose from. Besides the binary's own folder, the game searches
`<bindir>/wads/`, `~/games/doom`, `~/games/doomwads`, `~/games/doomlegacy/wads` and the usual system
locations. (The engine also supports Heretic, but the selector does not list it. That would need an
entry in `gameselect_arg[]` in `m_menu.c`.)

**`legacy.wad` is required.** It holds the engine's menu graphics, the cabinet's own art and the
`ENDOOM` exit screen, so use the copy in `common/`, not one from upstream DoomLegacy. If you build
with plain `make`, copy it yourself: `cp common/legacy.wad svn1749/bin/`.

**To change the cabinet's art, edit `common/legacy.wad` and rebuild.** The copy beside the binary
takes priority over any other on the machine. If the staged copy was edited in place, the build
keeps it as `legacy.wad.bak` rather than losing the edit. Use `tools/endoom.py` to edit the exit
screen (SLADE cannot).

**`dogs.wad`** is optional and only loaded with `-file dogs.wad`. It holds sprites and sounds for
MBF helper dogs, which only appear if an operator raises **Dogs** (Options → Game Options → Adv
Options, page 2). The ranked ruleset forbids them.

---

## Running

```sh
cd svn1749/bin
./doomlegacyarcade
```

No arguments are needed. The game finds its configuration in `legacyhome/` beside the binary, so
the whole directory can be copied to another machine or a USB stick and behave the same. You can
launch it by absolute path from anywhere.

For a dedicated machine, see [Keeping the cabinet running](#keeping-the-cabinet-running).

---

## Playing

**New Game** offers:

| | |
| --- | --- |
| **Campaign** | The normal game: episode, skill, play. Everyone who presses fire on the join screen plays together in co-op. |
| **Deathmatch** | Ready to go: respawning weapons and items, no monsters, no bots, five minutes. Asks only which map. |
| **Team Deathmatch** | Deathmatch in Red, Blue, Green and Yellow teams. You can't hurt your own team. |
| **Single Level** | One map, straight back to the menu afterwards, on its own score table. |
| **Multiplayer** | Every setting: map, skill, co-op or deathmatch variant, monsters, bots. |

"Multiplayer" here means players sharing *this* cabinet's screen. Deathmatch, Team Deathmatch and
Multiplayer are hidden on a single-panel cabinet.

**The Deathmatch map page** lists every map in the game, then **Start**. It resets to the first map
at each boot, and it is separate from Single Level's choice. Team Deathmatch shares it.

**End Game** appears at the bottom of the main menu only while a game is running.

**Under Options** a player can change the crosshair, colour and control scheme, and pick a game or
level pack. Nothing a player changes survives the next launch.

DoomLegacy's own **networked** play is still available under **Networked Multiplayer** in a
`-devmode` session. It is untested in this build, not removed. For linking cabinets, use
[Cabinet Link](#connecting-cabinets-together-cabinet-link) instead.

### Joining a game

On a cabinet with more than one panel, a **join screen** appears once the game is chosen. It is laid
out as the game will be, and each square is headed with that panel's player name (or `PLAYER N`).

1. **Press fire** to join. Your square opens your setup: **COLOR**, **CROSSHAIR** and **CONTROLS**
   (Tank or WASD), the same settings as your `PlayerN config` page.
2. Stick up/down moves between rows, left/right changes one, **fire** steps down. **Use** steps back
   up or unlocks.
3. The last row is **LOCK IN**, so pressing fire four times keeps your current settings.

The game starts **when everyone who joined has locked in**, or when the countdown ends. Panels that
never pressed in are not waited for.

**This decides whether a Campaign is co-op.** One player is a normal scored solo run; two or more
is co-op, which is not scored.

Each player plays at the panel they pressed in at, so a lone player at panel 3 still gets the whole
screen. How the screen splits is set by the operator; see
[Players, panels, and how the screen is divided](#players-panels-and-how-the-screen-is-divided).

**Team Deathmatch** renames COLOR to **TEAM** and offers only the four team colours. A panel already
on one of those colours keeps it; others go to the smallest team.

### Controls

**What to build the panel from.** A leverless ("all button" or hitbox) controller is best, since an
arcade stick can't reverse direction fast enough for Doom. A drop-in WASD controller such as the
Mixbox or T-Spin is probably the most responsive, though children may run off with the keycaps. A
normal arcade stick works, just not as well.

**How many buttons.** Six is the minimum. With eight, consider a run button with autorun off. Sticks
with an analog / d-pad switch work either way.

**Default layout**, on a stick and six buttons:

```
          [1]  [2]  [3]          1  Fire
    \|/                          2  Strafe left
    -O-   [4]  [5]  [6]          3  Strafe right
    /|\                          4  Use / Open
   stick                         5  Weapon down
                                 6  Weapon up
```

The stick moves and turns. Binding is an operator job; see
[Setting up a control panel](#setting-up-a-control-panel).

**Two schemes** are offered per player: **Tank** (the default, called *Look and Move* in older
builds) and **WASD**. They swap which controls turn and which strafe, on the same wiring. Tank suits
most joysticks and digital pads.

**In menus**, stick up/down moves, left/right changes a setting, **fire** selects and **use** backs
out.

### Single Level

Under **New Game**, Single Level plays one map and returns to the same page so you can retry at once.
Pick the map and skill to see the best times for that map. **Watch speed run** and **Watch max run**
replay the record demos when they exist.

Single Level has its **own high score table**, since a one-map time isn't comparable to a run that
reached the same map from level one. The first level of a campaign run also counts here, since it is
the same pistol start.

**No Monsters** is the leftmost skill: no monsters, Ultra-Violence, scored separately. Max is shown
as **100%S** (every secret). There are no Pacifist or Tyson records for it. Some maps cannot be
finished this way (E1M8's exit is behind the Barons).

### High scores

Records are the best **cumulative** time from the start of a run to each map's exit, per skill and
category:

| Category | Rule |
| --- | --- |
| **SPEED** | Reach the exit. |
| **MAX** | 100% kills and 100% secrets on every level so far. Items don't count. |
| **PACIFIST** | Never damage a monster, directly or with a barrel. Monsters fighting each other is fine. |
| **TYSON** | 100% kills using only fist, chainsaw and pistol. You may carry other weapons but not fire them. |

Every run is measured against all four at once. If a run still qualifies for Pacifist or Tyson,
it blinks at the top of the intermission.

- **Unreachable monsters don't count.** Monsters inside a sector that kills you on entry (15 of
  E1M8's 41) are left out of the kill requirement. The tally still shows the real percentage.
- **Only the standard ruleset scores.** Change a gameplay setting and the HUD shows `UNRANKED`.
- **Dying ends scoring** for the rest of the run. Finished levels keep their records.
- **Scores are per game and level pack**, so Doom II MAP01 and Plutonia MAP01 are separate.

**To browse them, choose High Scores on the main menu.** Left and right flip through Survival for
each episode, Single Level best times per difficulty, and a page per map with Single Level times.

### Level packs and IWADs

Drop a `.wad` level pack into `legacyhome/levels/` and it appears under **Options → Select Game** as
`<game> wad: <name>`. Packs are filtered by game (`MAPxx` under Doom II, `ExMy` under Ultimate
Doom). Selecting a pack loads it; selecting it again unloads it. One pack at a time. A long list
scrolls, with `MORE ABOVE`/`MORE BELOW` showing how many rows are off screen. Up to 64 packs per
game are listed, in alphabetical order; any beyond that are left off (the log says how many).

**Switching IWAD restarts the program**, showing `SWITCHING GAME...` for a second or two. Loading a
pack does not restart, but unloading one does.

**Works:** ordinary level wads, Boom-format maps, DeHackEd/BEX patches (including MBF21).
**Doesn't:** GZDoom mods (no DECORATE or ZScript, so no Brutal Doom) and `.pk3` files.

---

## Operator guide

Run with `-devmode` to unlock everything:

```sh
./doomlegacyarcade -devmode
```

This gives you the full stock menus, disables the competitive ruleset, and is the **only** mode
that saves settings. Launch with `-devmode`, change what you want, quit. Player sessions then start
from that baseline.

Every menu page and row, what it does, and whether players can see it, is listed in
[`docs/menu-options.md`](docs/menu-options.md).

### Unlocking the cabinet without a command line

Plug in a keyboard and press **Scroll Lock** at the attract screen (a title page or demo). The
cabinet shows `ENTERING DEVMODE...` and restarts with `-devmode`. Press it again when done: it shows
`LEAVING DEVMODE...`, **saves the config**, and restarts locked.

- **It only works at the attract screen**, never during a game, intermission, finale or initials
  entry, so nobody loses a run. End the game first.
- **The restarted program takes the foreground.** If you run it windowed on a desktop, expect it to
  steal focus at launch.
- **To change the key**, go to **Options → Setup Controls → `Player1 Controls >>`**, press **next**
  twice, and rebind **Devmode Restart**. Any panel's binding works. Only Player 1 has a default.
- **Keep it on the keyboard.** On a panel button, players could restart the machine from the
  attract screen. F12 and Print Screen are taken by screenshots.
- **To disable it**, bind it to nothing. `-devmode` is then the only way in.
- A new binding is saved on the way out of devmode, as `setcontrol "devmode" "scroll lock"`.

### Setting up a control panel

**Options → Setup Controls → Guided setup P1** (devmode only) shows the recommended layout, then asks
for each control in turn: stick directions, then the six buttons. It binds whatever you press
(keyboard, joystick, encoder). Press ESC to abandon and keep the old layout.

There is a guided setup for each panel, P1 to P4. Use the **Player n Controls** page beside each
one to bind anything past the ten standard controls.

### Players, panels, and how the screen is divided

All on **Options → Arcade Options → Players & Views** (devmode only):

| Setting | Values | Notes |
| --- | --- | --- |
| **Control Panels** | 1 to 4 | Ships at 1. Until raised, there is no join screen and no config pages for panels 3 and 4. |
| **2 Player Split** | `Top/Bottom`, `Side by Side` | Side by side suits wide screens. |
| **3-4 Player Split** | `Grid`, `Columns` | Grid suits screens up to 21:9. Use Columns at 32:9. |
| **Screen Order** | `1 3 / 2 4`, `1 2 / 3 4` | The default fills the grid by columns, so each view is on its player's side. |

Set the panel count first, then run the guided setup for each panel. Panels 3 and 4 have no preset
bindings.

**End-of-level screens in multiplayer:**

- The deathmatch scoreboard shows **Frags** and **Deaths** with room for full names.
- A multiplayer campaign tally **stays up for 25 seconds** before anyone can skip it. Set it with
  **Options → Arcade Options → Timeouts → Tally Hold** (0 to 60, 0 is off). The player who hit the
  exit gets an **EXIT** sign.

### Timeouts and banners

Under **Options → Arcade Options → Timeouts**:

| Setting | Default | Notes |
| --- | --- | --- |
| **Idle Timeout** | 60 s | Returns an abandoned game to the attract screen. Not applied in `-devmode`. |
| **Idle Warning** | 15 s | Countdown shown before the idle timeout. |
| **Join Screen Timeout** | 30 s | 20, 30, 45, 60, or **Off** (skip the join screen, start with panel 1). |
| **Initials timeout** | 60 s | How long the initials page waits before accepting what's entered. |
| **Tally Hold** | 25 s | Minimum time the multiplayer campaign tally stays up. |

Other Arcade Options:

- **Messages + Banners → Winning Banner**: **Rainbow** (default), **Cycle** (whole word, one colour
  at a time) or **Off**. Team Deathmatch always uses team colours.
- **Messages + Banners**: turn pickup and frag messages back on for single player and multiplayer
  separately. Both off by default.
- **Attract Volume**: a percentage of normal volume during the attract cycle. `0` is silent, `100`
  is full. Default 50.
- **Chase Cam Demo**: third-person attract demos on or off. On by default.

### Menu switches

Under **Options → Arcade Options → Disable/Enable Menu Options**. All are ignored in `-devmode`.

| Switch | Default | Effect |
| --- | --- | --- |
| **Cheats Menu** | Off | Puts Cheats on the main menu for players. |
| **Quit Menu** | Off | Puts Quit Game back for players. |
| **Multiplayer Menu** | On | Off removes the **Multiplayer** row from New Game. Deathmatch is not affected. |
| **Game Options** | On | Off removes the **Game Options** page from Options. |
| **Enable No Monsters** | On | Off removes No Monsters from Single Level. |

### Choosing a resolution, and tuning performance

Both are under **Options → Video Options** (devmode only).

**Video Modes** lists what the display offers, largest first, without duplicates. It **pages**
(Left/Right for more), so check the page number before deciding a mode is missing. The
**`Aspect:`** line above it filters the list; press **`A`** to cycle `AUTO` (default), `All`,
`4:3`, `16:10`, `16:9`, `21:9` and `32:9`. It says how many modes are hidden.

**Keep aspect** decides what happens when the resolution is a different shape from the monitor.
**Yes** (default) adds black bars; **No** stretches to fill. It does nothing when the shapes already
match, and is greyed out in OpenGL, where the monitor does the scaling. This is different from
**View fit**, which decides how much of the world goes into the picture, not where the picture sits.

**Gamma Options** holds Gamma Function, Gamma, Black level and Brightness. **F11** opens it directly.

**Performance Options**:

| Setting | Default | What it does |
| --- | --- | --- |
| **Framerate Cap** | 60 | `Uncapped`, or 35 / 60 / 75 / 100 / 120 / 144 / 165 / 240 frames a second. |
| **Render Threads** | 1 | `Auto` or 1 to 4. Software renderer only. |
| **8bpp Draw** | Off | Draw at 8 bits and expand through the palette at the end. |
| **Row Padding** | Off | Pad each row in memory. Software renderer only. |
| **Show Ticrate** | | Show the frame rate (averaged over half a second). |

- **Framerate Cap** never affects the simulation or demos. Set it to your panel's refresh rate.
  `35` is the original one-frame-per-tic behaviour, and the setting to fall back to if anything
  looks wrong. `Uncapped` is for benchmarking only.
- **Render Threads** gives each player's view its own core, or splits a single view into bands.
  That's close to 4x on four cores. It ships at 1 because a threaded frame can differ from a serial
  one by a few scattered pixels. Scores, demos and gameplay are identical either way. On a Pi, turn
  it on. Greyed out under OpenGL.
- **8bpp Draw** helps when memory bandwidth is the limit: a lot on a Pi, usually nothing on a
  desktop.
- **Row Padding** fixes sizes that are oddly slow because rows line up badly in the CPU cache. On a
  Pi 3 it took 1024x768 from 21 to 31 fps; on a laptop it made no difference.

**To benchmark your own machine**, run `tools/perfchart.py` from the top of the source tree. It
plays a fixed demo at each resolution in the [Performance](#performance) table, using a copy of the
settings folder so it never touches the real config, scores or demos. A full run takes about seven
minutes on a Pi 3.

```
tools/perfchart.py                   # every size in the Performance table
tools/perfchart.py --quick           # four sizes, a minute or two
tools/perfchart.py --runs 3          # each size three times, shown as a range
tools/perfchart.py --compare perfchart-<host>-<date>.csv   # add a "before" column
```

Each run writes a `.md` table and a `.csv` for `--compare`. Next to each frame rate it shows
milliseconds per frame spent on **Views** (the 3D view), **Present** (getting it on screen) and
**Other** (game logic and HUD). On a Pi it also reports temperature and warns about throttling,
which looks exactly like the game being slow. Over SSH, set `DISPLAY=:0` to use the Pi's screen, or
pass `--headless`.

### Cheats

With **Cheats Menu** on, the main menu gets a **Cheats** entry: god mode, all weapons and keys, no
clipping, exit level, and **Show Coordinates**. With it off, the menu is only in `-devmode`.

Cheats are single-player only. Any cheat, from the menu, the console or typed (IDDQD, IDKFA,
IDCLIP), voids the run and shows `PLAYER CHEATED - UNRANKED`. **Show Coordinates** is the exception:
it only displays position, angle, sector and the linedef in view, like IDDT and IDMYPOS. It is there
to locate bugs on a cabinet with no console.

### Keeping the cabinet running

On a dedicated machine, launch the game from a wrapper that restarts it in a loop, with an escape
key that no cabinet button is bound to. A quit or crash then goes straight back to the attract
screen.

```bash
#!/bin/bash
cd /path/to/bin || exit 1
while :; do
    ./doomlegacyarcade
    # Press this key during the pause to stop the loop.
    # Choose something no cabinet button is bound to.
    read -r -t 3 -n 1 key && [ "$key" = "q" ] && break
done
```

Player sessions never write the config, so every relaunch starts from the operator's settings.

### Choosing which game the cabinet boots into

**Options → Arcade Options → Boot Game** (devmode only). Set it to `doomu`, `doom2`, `plutonia` or
`tnt` to always boot that game. `None` boots whichever IWAD the search finds first. `-game` or
`-iwad` on the command line overrides it. If the chosen game is uninstalled, the cabinet warns and
falls back to the search.

### Connecting cabinets together (Cabinet Link)

Two or more cabinets on the same network pair with a passcode over an encrypted connection. They
show each other on **Options → Arcade Options → Cabinet Link** with name, short ID, online status
and activity. Cabinet Link is off until you set it up.

**It needs OpenSSL at build time.** `tools/build.sh` enables it when OpenSSL is found. If the page
says **NOT BUILT INTO THIS BINARY**, install `libssl-dev` (Debian, Raspberry Pi OS) or
`openssl-devel` (Fedora) and rebuild. On Windows, `build.bat -InstallDeps` installs it.

#### Setting up

One cabinet is the **master**; the others are **members** and connect to it. Configure each in an
operator session on **Options → Arcade Options → Cabinet Link**:

| Row | Use |
| --- | --- |
| **Role** | Fire, or left/right, cycles *off*, *master*, *member*. |
| **Name**, **Passcode**, **Master**, **Port** | Fire opens an on-screen keyboard. Fire types, *use* deletes, **DONE** saves. The bottom-left key switches case and symbols (lower case shows red). A real keyboard also works. |
| **Allowed** | Master only. A member that tried to connect shows as **ALLOW 192.168.1.68**; press fire to allow it. **ADD AN ADDRESS** types one in. Fire twice on an entry removes it. |
| **Forget paired cabinets** | Fire twice. |

1. On the master: role *master*, a name, a passcode.
2. On each member: role *member*, a name, the **same passcode**, and the master's address.
3. On the master's **Allowed** page, allow each member. It connects within a minute.

Every change saves at once and restarts the link. The console has the same controls:
`link_set name|passcode|master|port|role|allow|unallow <value>`, `link_forget`, and `link` for status.

**Verify the first connection.** Compare the **ID** each cabinet shows for the other with the ID the
other shows for itself (beside RUNNING). If they match, nothing is in between. From then on the
member refuses anything else claiming to be that master.

- **Wrong passcode:** after three tries the master ignores that address for a minute.
- **"MASTER IDENTITY CHANGED"** means something new answers at the master's address. If you really
  replaced the master, use **Forget paired cabinets** on the member.
- **Changing the master's passcode** disconnects every member until they have the new one.
- Settings and the private key are in `legacyhome/link/`. **Never copy that folder** to another
  cabinet; each needs its own identity.
- The master listens on port **5030**. Give the cabinets fixed addresses, and on untrusted Wi-Fi
  use wired Ethernet and allow only those addresses.

**Windows** (built but not yet tried on a real Windows cabinet):

- Windows Firewall's prompt is hidden behind a fullscreen game, so the link never connects. Allow it
  once from an administrator prompt:
  `netsh advfirewall firewall add rule name="Doom Legacy Arcade" dir=in action=allow program="C:\path\to\doomlegacyarcade.exe"`
- Switching games restarts the program as a new process. Start the cabinet from a shortcut or the
  Startup folder, not a relaunching script, or you will get two copies.
- The program has no console window. Add **`-logfile cabinet.log`** to the shortcut to capture its
  messages, with timestamps. This also works on Linux.

#### Linked games

Starting a **Deathmatch**, **Team Deathmatch** or **Campaign** on one cabinet invites the others.

- A cabinet on its attract screen or in its menus, running the **same game** (IWAD and level pack),
  shows a join screen such as `DEATHMATCH ON LAPTOP, 1 IN THERE`. Cabinets mid-game or entering
  initials are left alone.
- Press fire to join. Each cabinet keeps its own screen. The host shows who is coming
  (`RASPBERRYPI: 1 IN`).
- If nobody joins on a cabinet, its join screen closes when the game starts.
- The game starts when everyone who joined, on every cabinet, has locked in, or when the countdown
  ends. A cabinet that joined but never arrives is dropped after 15 seconds.
- A Campaign with players on several cabinets is co-op. One nobody else joins is a normal scored
  solo run.
- If two cabinets start the same kind of game at once, one becomes a join of the other.
- Up to eight players: four per cabinet.
- **The host's timeouts apply to everyone**: idle timeout, idle warning and join countdown. A host
  with **Join Screen Timeout** Off never invites.
- **The idle timeout counts everyone.** The game continues while anyone on any cabinet is playing.
- If the other cabinet leaves, press **fire** to dismiss "Server has Shutdown".
- Both cabinets need the same build. A linked cabinet ignores game traffic from unlinked machines.
- **The IWAD must match, not the file name.** `DOOM.WAD` and `doomu.wad` are fine together; Doom 2
  v1.666 and v1.9 are refused as "different version of DOOM2.WAD".
- **Music and sound packs don't need to match.** Any other extra wad the host loaded (maps, patches,
  textures) must be on the joining cabinet too.

**If a linked game shows PAUSE when nobody pressed it**, a cabinet has fallen out of step and the
host is resyncing it. First check every cabinet runs the **same build**: the Cabinet Link page shows
**SCORES: DIFFERENT BUILD** if not. Update each with `git checkout main`, `git pull --ff-only` and a
rebuild, and check `git rev-parse --short HEAD` matches. If pauses continue, start each cabinet with
`-logfile <file>`, play until it happens, and run `tools/nettrace-diff.py` on the logs.

#### Shared high scores

A record set on one linked cabinet (with its record demo) appears on the others' attract screens,
boards and intermissions.

- Syncing happens automatically, a few seconds after cabinets see each other and whenever a board
  changes. A cabinet that was off catches up later.
- Nothing changes during a game, initials entry or an operator session. New records wait for the
  attract screen, so a player's target never moves mid-run.
- Only the game both cabinets are **running** is shared.
- Scores are only shared between cabinets with the **same build**, the **same wads**, and the same
  **rocket trails**, **view height** and **invulnerability sky** settings. Otherwise the Cabinet Link
  page explains in red (**SCORES: DIFFERENT BUILD**, **DIFFERENT WADS**, **DIFFERENT SETTINGS**, or
  **SCORES: PLAYING** another game).
- **`clearhighscores` on the master clears every cabinet**, including ones that were off, but keeps
  records set there after the clear. Members refuse it. The master needs its clock set. To undo a
  clear, use `-restorehighscores` on the master (see
  [Resetting the high scores](#resetting-the-high-scores)).
- A demo that arrives incomplete is refused, with its record.

The score files now carry when and where each record was set. Older files still load.

#### Cabinet Link Options (master only)

These appear under **Options → Arcade Options → Cabinet Link Options** on the master only, and apply
to every cabinet. All are saved from a `-devmode` session.

**Select Game Sync** (default Off). A game or level pack picked on **Options → Select Game** on any
linked cabinet is picked on all of them.

- Each cabinet shows `SWITCHING GAME...` and restarts (a level pack loads without a restart).
- A cabinet mid-game, entering initials, on a join screen or in an operator session switches once
  it is back on its attract screen. Someone browsing menus *is* interrupted.
- A cabinet without the game keeps its own. The master shows why in red, such as **GAME SYNC: TNT
  NOT INSTALLED** or **GAME SYNC: NO LEVEL PACK DWANGO5**. Packs match by file name, ignoring case.
- **A cabinet on its attract screen always runs the master's game**, so the master's game is
  effectively every member's boot game. A cabinet without it is asked again every five minutes.
- When a game with a level pack ends, the pack is dropped on every cabinet that shared it.
- A pick made in an operator session is passed on, but an operator session is never switched by
  someone else's pick.

**Copy Missing Wads** (default Off). Every member keeps a copy of **everything on the master's Select
Game list**, copied in the background without anyone picking anything: each IWAD the master has
and every level pack in its `legacyhome/levels/`. To put a new pack on every cabinet, drop it into
the master's `levels/` folder; the members pick it up within five minutes.

- IWADs go into a **`wads`** folder beside the program; level packs into **`legacyhome/levels/`**.
  A copied game shows on that cabinet's Select Game straight away, with no restart (once its menus
  are closed).
- Copying happens only while **both** cabinets are on attract, in the menus or in an operator
  session. It pauses the moment either one starts a game and carries on afterwards, so it never
  costs a player a frame.
- One file at a time. An 18 MB IWAD took about 20 seconds on a laptop; a Pi on Wi-Fi is slower.
- A pick still comes first: a cabinet that lacks the picked game gets that file next, then
  switches. If the background copy was already fetching it, the pick takes it over rather than
  starting again.
- The master's page shows progress, such as **WAD SYNC: COPYING TNT.WAD 45%**, then **COPIED
  TNT.WAD**.
- Only files Select Game could offer are sent. Each is checked on arrival; a damaged copy is
  discarded (**WAD SYNC: NO TNT - IT ARRIVED DAMAGED**). Existing files are never overwritten, so a
  pack changed on the master is not re-sent to a cabinet that already has one by that name.
- A cabinet stops copying while its disk would be left with less than 256 MB free (**NOT ENOUGH
  DISK SPACE**).
- Files are only ever copied **from the master to the members**; a pack that only a member has
  stays there.
- Turning the setting on starts every member at once. With it off, nothing is copied and a pick
  that needs a file says **COPY MISSING WADS IS OFF**.

**Music Cabinet** (default **All**). Music on several machines drifts out of step within a few bars,
so pick one cabinet to carry it during linked games. The list shows this cabinet first, then every
paired cabinet. **Pick the middle cabinet of the row.**

- The master sends the choice round the link every few seconds, so late or offline cabinets catch up.
- If the chosen cabinet isn't in the game, the host plays the music instead. There is always
  exactly one.
- It only applies to linked games. A cabinet playing alone always plays its own music.
- **All** means every cabinet plays its own music. Sound effects are never affected.
- The cabinet is remembered by name, so the setting survives restarts.

### OPL music (the Sound Blaster sound)

**Options → Sound Volume → OPL music** plays Doom's music on an emulated OPL2, the FM chip on an
AdLib or Sound Blaster, using the instrument set stored in the game itself (the `GENMIDI` lump).
This is how most people heard Doom in 1993. With it off, music goes to the computer's MIDI synth,
which sounds like whatever that synth's instruments are.

- It switches straight away, restarting the current song.
- **Music Volume** works as usual. At the same setting, OPL is somewhat quieter than the MIDI synth,
  so you may want the slider a notch or two higher.
- **It wins over a soundtrack wad.** With a soundtrack wad (below) loaded, OPL plays the game's
  original song instead of the recording. Turn OPL off to get the recordings back.
- **Music src `MUS` now means the original songs too**, through the MIDI synth, even with a
  soundtrack wad loaded. *Auto* with OPL off plays the recordings.
- If the game has no usable instrument set, the MIDI synth plays instead and the console says
  `OPL music: no usable GENMIDI lump, using MIDI`.
- It does not change scores or demos.

### Replacement music (OGG soundtracks)

The engine only reads music from wad lumps; there is **no music folder**. Replacement tracks must be
packed into a `.wad`.

Two ready-made wads, both Andrew Hulshult recordings that map track for track:

| Game | Wad | Where |
| --- | --- | --- |
| Ultimate Doom | `IDKFAv2.wad` | https://www.moddb.com/mods/brutal-doom/addons/idkfa-doom-soundtrack |
| Doom II | `Doom2OST.wad` | https://www.reddit.com/r/Doom/comments/1enyv5f/for_anyone_that_wants_to_use_the_new_doom_2_music/ |

Neither ships here. **With a ready-made wad, skip to step 3.** Both can be loaded at once; they use
different lump names.

**1. Name each track after its music lump.** The engine tries `O_<name>` first (only when OGG music
is enabled), then `D_<name>` (whose contents are sniffed, so an OGG works there too). Use the `O_`
names so the originals stay as a fallback.

Ultimate Doom:

```
O_E1M1 … O_E1M9      episode 1        O_INTRO    title screen
O_E2M1 … O_E2M9      episode 2        O_INTROA   alternate title track
O_E3M1 … O_E3M9      episode 3        O_INTER    intermission
                                      O_VICTOR   victory text
                                      O_BUNNY    end credits
```

Episode 4 reuses episode 1 to 3 tracks, so it is covered automatically.

Doom II:
`O_RUNNIN`, `O_STALKS`, `O_COUNTD`, `O_BETWEE`, `O_DOOM`, `O_THE_DA`, `O_SHAWN`, `O_DDTBLU`,
`O_IN_CIT`, `O_DEAD`, `O_STLKS2`, `O_THEDA2`, `O_DOOM2`, `O_DDTBL2`, `O_RUNNI2`, `O_DEAD2`,
`O_STLKS3`, `O_ROMERO`, `O_SHAWN2`, `O_MESSAG`, `O_COUNT2`, `O_DDTBL3`, `O_AMPIE`, `O_THEDA3`,
`O_ADRIAN`, `O_MESSG2`, `O_ROMER2`, `O_TENSE`, `O_SHAWN3`, `O_OPENIN`, `O_EVIL`, `O_ULTIMA`,
`O_READ_M`, `O_DM2TTL`, `O_DM2INT`.

**2. Build the wad.** In [SLADE](https://slade.mancubus.net/): *New → Wad Archive*, drag in the
`.ogg` files, rename each to its lump name (8 characters max), and save as `.wad`.

**3. Check OGG music is enabled.** **Options → Sound Volume → Music src** must be `Auto` (the
shipped default). `MUS` ignores replacement music entirely. Avoid `MP3` and `OGG` too: they play
silence for any track the wad doesn't replace. Set it in a `-devmode` session.

**4. Load it at startup.** Add a line per wad to `legacyhome/autoexec.cfg` (create it if needed):

```
addfile "IDKFAv2.wad"
addfile "Doom2OST.wad"
```

A bare filename works if the wad is in a usual wad directory such as `~/games/doom`; an absolute path
also works. Don't put music wads in `legacyhome/levels/`, which only lists wads with maps.
`-file IDKFAv2.wad` on the command line also works.

Recorded music is much louder than MIDI, so **turn music volume down** to 3 or 4. Tracks loop from
the start rather than at a composed loop point.

### Resetting the high scores

```sh
./doomlegacyarcade -clearhighscores
```

(or `clearhighscores` at the console). This clears `highscores.dat`, the `runs.dat` leaderboard and
the record demos. Deleting the files by hand doesn't work, because the game writes its in-memory
copy back. **Remove `-clearhighscores` afterwards**, or it clears on every start.

**Every clear keeps a backup first**, in `legacyhome/scores-backup/<date>-<time>/`. The newest ten
are kept. If the backup fails, nothing is cleared.

**To undo a clear, restore rather than copying files back:**

```sh
./doomlegacyarcade -restorehighscores                     # the newest backup
./doomlegacyarcade -restorehighscores 20260914-080808     # a backup by name
./doomlegacyarcade -restorehighscores /path/to/old/legacyhome   # any folder with score files
```

(or `restorehighscores [folder]` at the console). The backup is **merged** with current scores, so
newer records survive, and restored records get their demos back. With linked cabinets, restore on
the master; hand-copied files would be cleared again at the next sync. You can't restore during a
game. Remove `-restorehighscores` afterwards too.

### Taking a screenshot

Press **F12**. Images are written to the launch directory as `DOOM0000.tga`, `DOOM0001.tga` and so
on, never overwriting. They are uncompressed Targa (about 3 MB at 1366x768), so convert before
sharing:

```sh
convert DOOM0000.tga shot.png
```

PrtSc is a fallback binding, but GNOME's screenshot tool takes it first. To rebind, change
**Screenshot** on the controls page, or use `setcontrol "screenshot" "f11"` at the console. The
`screenshotdir` cvar changes where files go. Both only stick from a `-devmode` session.

### Other useful flags

| Flag | Effect |
| --- | --- |
| `-devmode` | Unlock menus, save settings, disable the ruleset (or press Scroll Lock at the attract screen) |
| `-clearhighscores` | Wipe scores and record demos at startup (after a backup) |
| `-restorehighscores [folder]` | Merge backed-up scores back in (newest backup if no folder) |
| `-clearaudit` | Reset the operator audit counters at startup |
| `-game <name>` | Start a specific game (`doomu`, `doom2`, `plutonia`, `tnt`) |
| `-warp <map>` | Jump straight to a map |
| `-file <wad>` | Load a wad at startup: a level pack, soundtrack or DEH/BEX patch |
| `-config <file>` | Use a different configuration file |
| `-logfile <file>` | Write every message to a file, with timestamps |
| `-v` | Verbose startup, showing which files were found |
| `-nonodebuild` | Don't rebuild BSP nodes at level load (turns off the slime-trail fix) |
| `-frameprofile` | Print where each frame's time goes |
| `-playdemo <file>` | Replay a record demo from `legacyhome/demos`, then quit |
| `-synclog` | Write one line of simulation state per tic to `synclog_rec.txt` / `synclog_play.txt`; the first differing line is where a demo desynced |
| `-noendtext` | Skip the exit text screen |
| `--version` | Print the version and what it is a fork of, then exit |

---

## Where your data lives

Everything is in `legacyhome/` beside the binary:

| | |
| --- | --- |
| `config.cfg` | All settings. Written only by a `-devmode` session. |
| `config8p.cfg`, `configgl.cfg`, `confign.cfg` | Video settings per drawmode, applied after `config.cfg`. |
| `highscores.dat` | Best cumulative times per map, skill and category. Plain text. |
| `runs.dat` | The run leaderboard with initials. Plain text. |
| `demos/` | Record demos, one per map, skill and category. |
| `levels/` | Level packs you've added. |
| `audit.dat` | Operator audit counters. Plain text. |
| `crash.txt` | Crash reports (Linux), one appended per crash. Safe to delete. |
| `autoexec.cfg` | Optional console commands run at startup, such as `addfile` lines. |
| `link/` | Cabinet Link settings and this cabinet's private key. Never copy between cabinets. |

**Back up `highscores.dat`, `runs.dat` and `demos/`.** They are the only things that can't be
recreated.

If there is no `legacyhome/` beside the binary, the game uses `~/.doomlegacy/` instead.

### Keeping configuration in version control

The tracked cabinet config is `cabinet/legacyhome/config.cfg`. A build stages it next to the binary
but never overwrites an existing config. After changing settings in `-devmode`, bring them back with:

```sh
cd svn1749/src
make cabinet_save
git diff cabinet/legacyhome/config.cfg
```

Check the diff before committing: devmode saves *every* setting, not just the ones you changed.
(`botrandom` is a random seed and always differs.) See [`cabinet/README.md`](cabinet/README.md).

### If the configuration goes wrong

Every save first copies the old file to **`config.cfg.bak`**. A config rebuilt from defaults is easy
to spot: `name` is your Unix login rather than the player name you set.

Settings that fail to load are **reported at startup** by line number. Run **`cfgcheck`** at the
console to repeat the check. A healthy cabinet reports exactly four: `botrandom`, plus
`monstergravity`, `monsterfriction` and `voodoo_mode`, which the ruleset deliberately overrides.
Anything else is worth a look.

---

## Troubleshooting

**A setting I changed isn't sticking.**
Player sessions don't save settings. Use `-devmode`.

**HUD elements aren't drawing.**
They only appear at the largest view size (`viewsize` `11`). Check **Options → Arcade Options → HUD
Configuration** first. The `overlay` line in `config.cfg` lists them as letters: `k` keys, `a` ammo,
`h` health, `m` armor, `f` frags, `e` kills, `i` items, `s` secrets, `t` level clock, `b` ammo
breakdown. The default is `kahmfeistb`.

**A new HUD element doesn't appear after rebuilding.**
`config.cfg` overrides compiled defaults, so an existing `overlay` line is kept. Switch the item on
in HUD Configuration in a `-devmode` session, or add its letter by hand.

**Scores aren't being recorded.**
Look for `UNRANKED` on the HUD. Either a gameplay setting differs from the ruleset (the console log
names it) or the player died. Returning to the attract screen resets the ruleset.

**The join screen never appears, or panels 3 and 4 have no settings pages.**
**Control Panels** on **Players & Views** is still 1. Also check **Join Screen Timeout** isn't Off.

**Replacement music isn't playing.**
Check `music_source` is `Auto`, not `MUS`. Then check the lump names inside the wad against the list
above. `-v` shows whether the wad loaded.

**One cabinet in a linked game has no sound.**
An older Windows build muted everything when **Music Cabinet** muted the music. Update, or set Music
Cabinet to `All`. If it still happens, run that cabinet with `-volog`, which traces each sound to
`volog.txt` next to the program.

**A sound effect cuts out, or a weapon goes quiet during rapid fire.**
Older builds with **Framerate Cap** above 35 could fill every sound channel with finished sounds and
refuse new ones. The plasma rifle was the worst case. Update. If a sound still stops early, run with
`-sndlog`, or `-sndlog plasma` for one sound. It writes `sndlog.txt` next to the program, with one
line for each sound stopped early and the reason.

**A game is missing from Select Game.**
Its IWAD wasn't found. Run with `-v` to see the search paths.

**The game is slow or choppy.**
Open **Options → Video Options → Performance Options** and turn on **Show Ticrate**. On a Pi, follow
the tips under [Performance](#performance). On a desktop, use OpenGL and set **Framerate Cap** to the
panel's refresh rate.

**My monitor's resolution isn't in Video Modes.**
Press **`A`** until the `Aspect:` line reads `All`, and check every page with Left/Right.

**Motion looks different from what I remember.**
That's **Framerate Cap** interpolating between tics. Set it to `35` for the original behaviour.

**The game crashed.**
The terminal log names what was on screen. Each level load prints a line like
`Level: E1M7  skill 4  play  chasecam off  views 1`, and attract demos add
`demo E1M1  ITYTD  SPEED  1:11.05  AAA` and a `Demo file:` line.

On Linux the game also writes a **crash report** (backtrace, tic, level, skill, demo and recent
monster-list activity) to the terminal and to **`legacyhome/crash.txt`**. Send that file. Earlier
`CLASS-LIST ANOMALY` lines in the terminal are worth sending even without a crash.

For a full backtrace, install `systemd-coredump` and `gdb` (`apt` on Raspberry Pi OS, `dnf` on
Fedora), then after a crash:

```
coredumpctl list
coredumpctl debug doomlegacyarcade --debugger=gdb \
    --debugger-arguments="-batch -ex 'thread apply all bt full'"
```

On a Pi, also run `sudo mkdir -p /var/log/journal && sudo systemctl restart systemd-journald`, or
the logs are lost at the next reboot.

**The game won't build.**
Usually one of the three `make_options` edits under [The manual way](#the-manual-way). `-march=i686`
and the default `gnu23` standard both give errors that don't point at the cause.

---

## Credits and licence

DoomLegacy is by Fabrice Denis, Boris Pereira and the DoomLegacy team, based on the Doom source
released by id Software. This build tracks DoomLegacy 1.48.18 (SVN r1749) with local arcade changes.

Licensed under the **GNU General Public License**; see [`LICENSE`](LICENSE), and
[`svn1749/docs/LICENSE.txt`](svn1749/docs/LICENSE.txt) for the copy that came with the port. Doom,
Doom II, Final Doom and Heretic game data belong to their respective owners and are not distributed
here.

The CRT shaders in [`svn1749/src/sdl/shaders/`](svn1749/src/sdl/shaders/) come from libretro's
`glsl-shaders` collection: zfast_crt by Greg Hogan (SoltanGris42), crt-pi by davej and crt-geom by
cgwg, Themaister and DOLLS, all GPL v2 or later, and crt-lottes by Timothy Lottes, public domain.
The other shaders were written for this project; the FXAA one follows Timothy Lottes' published
method.
