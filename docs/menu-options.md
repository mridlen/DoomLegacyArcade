# Menu options reference

Every page and row in the Doom Legacy Arcade menus, with a short note on what each one does.

- **★** marks a row or page that this fork adds or changes. Rows without it behave as they do in
  stock DoomLegacy 1.48.18.
- **Player** pages are visible on a locked cabinet. **Operator** pages appear only in a `-devmode`
  session. Get there with `./doomlegacyarcade -devmode`, or press the *Devmode Restart* key
  (Scroll Lock by default) at the attract screen. See the README's
  [Operator guide](../README.md#operator-guide).
- Only a `-devmode` session saves settings to `config.cfg`. A change made in a player session is
  lost when the program exits.
- Some rows are hidden by the cabinet's own settings, not by devmode. For example, the multiplayer
  rows need **Control Panels** set to 2 or more. Each one is noted where it applies.

The design notes behind each feature are in [`arcade/`](arcade/). The note linked from a section
explains why the feature works the way it does.

**Navigating.** Move with the stick or arrow keys. Change a value with left/right. Select with
fire or Enter. Go back with Escape or the *Main menu* control. Pressing any button on the attract
screen opens the main menu.

---

## Contents

- [Main menu](#main-menu)
- [New Game](#new-game)
  - [Campaign: episode and skill](#campaign-episode-and-skill)
  - [Deathmatch / Team Deathmatch: map](#deathmatch--team-deathmatch-map)
  - [Single Level](#single-level)
  - [Multiplayer (local)](#multiplayer-local)
  - [Networked Multiplayer](#networked-multiplayer)
- [The join screen](#the-join-screen)
- [Initials entry](#initials-entry)
- [High Scores](#high-scores)
- [Cheats](#cheats)
- [Options](#options)
  - [Player and the player config page](#player-and-the-player-config-page)
  - [Game Options](#game-options)
  - [Select Game](#select-game)
  - [Effects Options](#effects-options)
  - [Connect, Network and Server Options](#connect-network-and-server-options)
  - [Arcade Options](#arcade-options)
  - [Sound Volume](#sound-volume)
  - [Video Options](#video-options)
  - [Setup Controls](#setup-controls)
- [The launcher](#the-launcher)

---

## Main menu

| Item | Who | What it does |
| --- | --- | --- |
| **New Game** | Player | Opens the page of ways to start a game (below). |
| **Load Game** / **Save Game** | Operator | Stock savegame pages. Hidden from players. |
| **Options** | Player | Settings. Players see only a few rows; everything else is operator-only. |
| **Cheats** ★ | Operator | The cheat page. Players see it only if *Cheats Menu* is On (Arcade Options → Disable/Enable Menu Options). |
| **End Game** ★ | Player | Ends the game in progress and returns to the title. Shown only while a game is being played. Moved here from the New Game page so it is one press away. |
| **High Scores** ★ | Player | The score pages from the attract cycle, which you can flip through by hand. Replaces stock *Read This*. |
| **Quit Game** ★ | Operator | Exits the program. Hidden from players by default because a cabinet has no desktop to fall back to. *Quit Menu* shows it to players. It is always shown in `-devmode`. |

→ [`arcade/menus.md`](arcade/menus.md)

## New Game

★ The whole page is reworked for a cabinet. It shares the main menu's position on screen.

| Item | Who | What it does |
| --- | --- | --- |
| **Campaign** ★ | Player | Starts the IWAD's campaign: episode (if the game has episodes), then skill, then the join screen. One player joining gives a solo, scored run. Two or more give local co-op. Replaces *Single Player*. |
| **Deathmatch** ★ | Player, 2+ panels | A deathmatch with its rules already chosen: weapons and items respawn (`DM_both`), no monsters, no bots, medium skill, and the configured time limit. Pick a map, then players join. |
| **Team Deathmatch** ★ | Player, 2+ panels | Like Deathmatch, but in colour teams (Red, Blue, Green, Yellow) with friendly fire off. On the join screen you pick a team colour. |
| **Single Level** ★ | Player | Play one chosen map. It has its own speed-run and max-run boards and record demos. |
| **Multiplayer** ★ | Player, 2+ panels | The page for setting up a local game by hand: map, skill, co-op or deathmatch variant, monsters, bots. It is the old *Two Player Game*, now for up to four panels. *Multiplayer Menu* can hide it. |
| **Networked Multiplayer >>** ★ | Operator | The stock networked client/server menu, renamed so it is not confused with local Multiplayer. It has not been tested in this build. Linked cabinets use Cabinet Link instead. |

→ [`arcade/menus.md`](arcade/menus.md), [`arcade/single-level.md`](arcade/single-level.md)

### Campaign: episode and skill

- **Which Episode?** Stock episode list (Knee-Deep in the Dead … Episode 5). Only episodes the
  installed IWAD has are shown. UMAPINFO can replace the list.
- **Choose Skill Level** Stock: *I'm too young to die*, *Hey, not too rough*, *Hurt me plenty*,
  *Ultra-Violence*, *Nightmare!* Next comes [the join screen](#the-join-screen) when the cabinet
  has more than one panel.

### Deathmatch / Team Deathmatch: map

★ A page of its own, titled with the row that opened it.

| Item | What it does |
| --- | --- |
| **Map** | The arena. Choose from MAPxx or ExMy, whichever the game uses. It is not saved and starts at the first map on every boot. It is kept apart from the Single Level and Start Game map choices, so one does not change the others. |
| **Start** | Opens the join screen and starts the match. There is no skill page: with no monsters, skill only changes how much ammo and armour the map gives. |

### Single Level

★ Plays one map and returns to this page. The page shows the map's best times, three deep, with
initials.

| Item | What it does |
| --- | --- |
| **Map** | The map to play. |
| **Skill** | The five normal skills, plus **No Monsters** to the left of the easiest. No Monsters plays at Ultra-Violence with no monsters and is scored separately. *Enable No Monsters* can remove it. |
| **Start** | Starts the level. The run is scored on this map's own board. |
| **Watch speed run** | Plays the record demo for the fastest exit at this map and skill. |
| **Watch max run** | Plays the record demo for the fastest exit with 100% kills and secrets. With No Monsters it reads **Watch 100%S run**, since secrets are the only requirement. |

→ [`arcade/single-level.md`](arcade/single-level.md), [`arcade/high-scores.md`](arcade/high-scores.md)

### Multiplayer (local)

★ Reached from New Game → Multiplayer.

| Item | Who | What it does |
| --- | --- | --- |
| **Player1 config >>** … **Player4 config >>** ★ | Player | Each panel's own page (colour, crosshair, control scheme). One row per panel the cabinet has. Replaces the two *SETUP PLAYER* graphics. |
| **Options** | Player | The deathmatch rules (Net Options, below). |
| **Start Game** | Player | The start-a-game page, below. |
| **Networked Multiplayer >>** | Operator | As on New Game. |

**Start Game** (the stock *Start Server* page, retitled ★):

| Item | Who | What it does |
| --- | --- | --- |
| **Map** | Player | The starting map. |
| **Skill** | Player | The starting skill. |
| **Coop/Deathmatch** | Player | Co-op or one of the deathmatch variants. |
| **Monsters** | Player | Whether monsters spawn. |
| **Bots** | Player | How many bots to add. |
| **Bot Options >>** ★ | Player | Bot settings (see [Game Options](#game-options)). Placed right under the bot count. |
| **Wait Players** / **Wait Timeout** | Operator | How many players the server waits for, and for how long. |
| **Internet Server** / **Server Name** | Operator | Advertise the game on a master server, and the name to use. |
| **Start** | Player | Starts the game. |
| **Dedicated** | Operator | Starts a dedicated server with no local player. |

**Options → Net Options** (players see the rows not marked Operator):

| Item | Who | What it does |
| --- | --- | --- |
| Allow Jump / Allow Rocket Jump / Allow autoaim / Allow turbo | Operator | Server permissions. Hidden from players, since they mean nothing on a cabinet. |
| Allow exitlevel | Player | Whether the `exitlevel` command works in a netgame. |
| Allow join player | Operator | Whether players can join a game in progress. |
| Teamplay | Player | Off, or teams by colour or by skin. |
| TeamDamage | Player | Friendly fire for teams. In co-op it is the friendly-fire switch. |
| Fraglimit / Timelimit | Player | The match ends when either is reached. Timelimit is also used by the New Game Deathmatch rows. |
| Deathmatch Type | Player | Co-op variants, or DM with weapons, items, both, or neither respawning. |
| Frag's Weapon Falling | Player | A killed player drops their weapon. |
| Maxplayers | Operator | Player cap for a networked server. |
| Games Options >> | Player | Opens [Game Options](#game-options). Hidden when *Game Options* is Off. |

### Networked Multiplayer

Operator only, and stock apart from its name. **Multiplayer**, **Setup Player 1/2**, **Options**
(links to Connect, Network, Server and Game Options), **Connect Server** (search for servers and
join one), **Create Server** (the Start Game page), **End Game**.

## The join screen

★ Not a menu page. It opens after every multiplayer-capable start (Campaign, Deathmatch, Team
Deathmatch, Multiplayer) when the cabinet has more than one panel. Each panel sets itself up
independently:

- **Fire** joins the panel.
- Then set three rows: **Color** (a **Team** colour in Team Deathmatch), **Crosshair** and
  **Controls** (Tank or WASD). Left/right changes the value. Fire or back moves down a row, and
  use or forward moves up.
- **Fire** on the last row, **Ready**, locks the panel in. **Use** unlocks it again.

The game starts once every panel that joined has locked in, or when the *Join Screen Timeout*
countdown runs out. Escape abandons the join. The changes are saved to the same per-panel settings
as the player config page. The same screen handles **Cabinet Link** invites, so players on linked
cabinets can join a game started on another machine.

→ [`arcade/multiplayer-views.md`](arcade/multiplayer-views.md), [`arcade/cabinet-link.md`](arcade/cabinet-link.md)

## Initials entry

★ After a run that makes a high-score board, the player enters three initials with the stick and
fire. The page accepts what is on it after *Initials Timeout* seconds. The idle timeout is paused
while this page is up.

→ [`arcade/high-scores.md`](arcade/high-scores.md)

## High Scores

★ The same pages the attract cycle shows. Left/right flip between pages, fire steps forward,
Escape backs out. It always opens on the first page.

## Cheats

★ A cheat page for testing and casual play, usable only during a single-player game (not in multiplayer or a demo). **Every row except Show
Coordinates voids the run for scoring.** The toggles show their On/Off state and leave the menu
open.

| Item | What it does |
| --- | --- |
| **God Mode** | Invulnerability, toggled. |
| **All Weapons and Keys** | Full health, armour, ammo, keys and every weapon (IDKFA). Closes the menu. |
| **No Clipping** | Walk through walls, toggled. |
| **Exit Level** | Ends the current map at once. The quickest way to reach the intermission when testing. |
| **Show Coordinates** | Shows position, angle, the current sector and the linedef in view, for reporting where a bug is. It changes nothing in the game, so it does not void the run. Toggled. |

## Options

What a **player** sees: **Player >>**, **Game Options >>** (unless *Game Options* is Off) and
**Select Game >>** (when two or more games are installed). Every other row is operator-only.

| Item | Who | What it does |
| --- | --- | --- |
| Messages | Operator | Stock on-screen message switch (see also *Messages + Banners*). |
| Always Run | Operator | Player 1 always runs. |
| Crosshair | Operator | Player 1's crosshair. |
| **Player >>** | Player | Chooses a panel's config page (below). |
| Effects Options >> | Operator | Visual effects. |
| **Game Options >>** | Player | Gameplay rules. |
| Connect Options >> / Network Options >> / Server Options >> | Operator | Networking. |
| **Arcade Options >>** ★ | Operator | Every cabinet setting. |
| Sound Volume >> | Operator | Volumes and sound devices. |
| Video Options >> | Operator | Display and renderer. |
| Setup Controls >> | Operator | Bindings, guided setup, gamepads. |
| **Select Game >>** ★ | Player | Switch between installed IWADs and level packs. |

### Player and the player config page

**Player >>** lists **Player1 config >>** to **Player4 config >>**, one per panel the cabinet has.
★ Panels 3 and 4 are new.

The config page. ★ It has been trimmed so a player sees only what they can set for themselves.

| Item | Who | What it does |
| --- | --- | --- |
| Your name | Operator | Player name. Hidden from players; the cabinet uses initials instead. |
| **Your color** | Player | Player colour, shown on an animated player sprite. |
| Your skin | Operator | Player skin. |
| **Crosshair** ★ | Player | Crosshair style, moved onto this page so a player's settings are together. |
| **Control scheme** ★ | Player | **Tank** (the stick turns, buttons strafe) or **WASD** (the stick strafes, buttons turn). Set per panel. |
| Player config >> | Operator | Stock extras: Always Run, Autoaim, mouse settings, Original Weapon Switch, WeaponPref, and links to the setup and controls pages. |
| Player2 Controls >> / Second Mouse config >> | Operator | Stock second-player pages. |

→ [`arcade/input.md`](arcade/input.md)

### Game Options

Gameplay rules. **The ranked ruleset overrides several of these in player sessions**, so a scored
run always plays by the same rules. Changing them here affects unranked play and `-devmode`.

| Item | What it does |
| --- | --- |
| Item Respawn / Item Respawn time | Pickups come back after a delay. |
| Monster Respawn / Monster Respawn time | Dead monsters come back, as in Nightmare. |
| Monster Behavior | Whether monsters fight each other: Normal, Coop, No Infight, Infight, Full Infight, and *Force* versions that override the map. |
| Fast Monsters | Nightmare-speed monsters at any skill. |
| Predicting Monsters | Monsters aim ahead of a moving player. |
| Solid corpse | Corpses block movement. |
| **Monster Height** ★ | **Infinite** (default, as vanilla: things block each other at any height) or **Over-Under** (Legacy's old behaviour: you can climb onto monsters). Affects gameplay and is recorded in demos. |
| Tired Run / Drown | Stamina and drowning, when compiled in. |
| Adv Options >> | Two pages of Boom/MBF rules: gravity, monster friction, door stuck, monster memory, hazard avoidance, backing off, pursuit, dropoffs, lifts, helper dogs, friend distance, monkeys, falloff, voodoo dolls, insta-death, zero tags, blockmap generation. |
| Map variation >> | Monster health, pickup amounts, door delay, monster size and health variation, teleport and reaction tweaks, when compiled in. |
| Bot Options >> | Bot skill, speed, skin, respawn time, random seed, generation, grabbing, and count. |
| Network Options >> | The Net Options page. Operator only. |

→ [`arcade/gameplay-defaults.md`](arcade/gameplay-defaults.md)

### Select Game

★ One row for each IWAD that is installed: **Ultimate Doom**, **Doom II**, **Final Doom:
Plutonia**, **Final Doom: TNT**. Below them are the level packs found in `legacyhome/levels/`, and
the list scrolls. Picking an IWAD restarts the program into that game. Picking a level pack loads it
and starts its first map without a restart. On linked cabinets, *Select Game Sync* makes the others
follow.

→ [`arcade/menus.md`](arcade/menus.md), [`arcade/cabinet-link.md`](arcade/cabinet-link.md)

### Effects Options

Operator only.

| Item | What it does |
| --- | --- |
| Light Options >> | Software coronas: on/off, size, draw mode. |
| Translucency | Translucent sprites and effects. |
| **Spectre Fuzz** ★ | The original Doom fuzz effect for spectres and partial invisibility, in both renderers, instead of plain translucency. Renamed from *Fuzzy mode*. |
| **Rocket Trails** ★ | Smoke behind rockets and lost souls. On is the stock Legacy look; Off matches vanilla. This changes the random-number sequence, so it is stored in demos. |
| Splats / Max splats / BloodTime | Blood and bullet decals on walls, how many, and how long blood lasts. |
| Sprites limit | Cap on sprites drawn per frame. |
| Pickup Flash ★ | How a pickup is signalled: Off, Status, Half, or **Vanilla** (the yellow screen tint; now the default). |
| Sky | How a sky too small for the view is filled: auto, substitute, stars, extend, stretch or vanilla. |
| Water Effect / Fog Effect | Legacy's water and fog sector effects. |
| **Next** → Invul skymap | Whether the invulnerability colormap also tints the sky. |
| Boom Colormap | Boom's global colormap behaviour. |
| Sound oof 2s | Play the "oof" when the player bumps into a two-sided line, as PrBoom can. |
| Width Clip | Stock clipping-width correction switch. |
| **Weapon Flash Fix** ★ | Draws the weapon fullbright while its muzzle flash shows, hiding the hard line in dark rooms. Drawing only. |

→ [`arcade/spectre-fuzz.md`](arcade/spectre-fuzz.md), [`arcade/gameplay-defaults.md`](arcade/gameplay-defaults.md)

### Connect, Network and Server Options

Operator only, stock networking.

- **Connect Options**: Download files, Download savegame, Netgame repair, Server 1–3 (saved
  addresses).
- **Network Options**: the [Net Options](#multiplayer-local) page with every row shown.
- **Server Options**: Internet server, Master server, Server name, and whether this server provides
  files, savegames and repair to clients.

### Arcade Options

★ The operator's page. Every row is new in this fork.

| Item | What it does |
| --- | --- |
| **Menu Sounds** | Which menu sound set to use: Auto, Legacy, Doom or Heretic. (Stock setting, moved here.) |
| **Screens Link** | The transition between screens: **None**, **Crossfade** or **Melt**. |
| **Players & Views >>** | Panel count and how the screen is split (below). |
| **Boot Game** | The IWAD the cabinet starts in: **None** (the engine's own search), `doomu`, `doom2`, `plutonia`, `tnt`. If that IWAD is missing, startup falls back to the normal search. |
| **Disable/Enable Menu Options >>** | What players are allowed to reach (below). |
| **Timeouts >>** | Idle, join, initials and tally timers (below). |
| **Attract Volume** | Sound and music volume during the attract cycle, as a percentage of normal volume. 0 is silent, 100 is full volume. Default 50. |
| **Chase Cam Demo** | Occasionally plays a record demo from a third-person camera, captioned CHASE CAM. On by default. |
| **Messages + Banners >>** | In-game messages and the deathmatch winner banner (below). |
| **HUD Configuration >>** | Which full-screen HUD elements are shown (below). |
| **Audit >>** | Operator bookkeeping since the last reset: running and playing time, games by player count, levels finished, deaths, unranked runs, board placements and per-map plays. Any button backs out. Reset it with `clearaudit`. |
| **Cabinet Link >>** | Networked cabinets: this cabinet's settings and every paired cabinet's status (below). |
| **Cabinet Link Options >>** | How linked cabinets behave together. Shown on the master cabinet only (below). |

→ [`arcade/menus.md`](arcade/menus.md), [`arcade/attract.md`](arcade/attract.md), [`arcade/audit.md`](arcade/audit.md), [`arcade/screen-wipe.md`](arcade/screen-wipe.md)

**Players & Views**

| Item | What it does |
| --- | --- |
| **Control Panels** | How many sets of controls the cabinet has, 1–4. At 1, the join screen and every multiplayer row are hidden. Rows for panels 3 and 4 appear only when the count is high enough. |
| **2 Player Split** | **Top/Bottom** or **Side by Side**. Side by side gives each player the full height of the screen. |
| **3-4 Player Split** | **Grid** (2×2) or **Columns** (four side by side). Use Grid up to 21:9 and Columns at 32:9. |
| **Screen Order** | Which grid cell each panel gets. **1 3 / 2 4** (default) keeps each view on its player's side of the cabinet. **1 2 / 3 4** is reading order. |

→ [`arcade/multiplayer-views.md`](arcade/multiplayer-views.md), [`arcade/ultrawide.md`](arcade/ultrawide.md)

**Disable/Enable Menu Options**

| Item | Default | What it does |
| --- | --- | --- |
| **Cheats Menu** | Off | Show *Cheats* on the main menu to players. Cheating still voids the run. |
| **Multiplayer Menu** | On | Show the hand-tuned *Multiplayer* row on New Game. Deathmatch and Team Deathmatch are not affected. |
| **Quit Menu** | Off | Show *Quit Game* to players. |
| **Game Options** | On | Show *Options → Game Options* to players. |
| **Enable No Monsters** | On | Offer *No Monsters* on Single Level's skill row. Takes effect immediately. |

**Timeouts**

| Item | Default | What it does |
| --- | --- | --- |
| **Initials Timeout** | 60 s | How long initials entry waits before accepting what is on it. 0 waits until a player presses fire. |
| **Idle Timeout** | 60 s | With no input for this long during a game, the game ends and the cabinet returns to the attract screen. Off, or 15–900 s. Not applied in `-devmode`. |
| **Idle Warning** | 15 s | How long before the idle timeout a "returning to title" countdown appears. |
| **Join Screen Timeout** | 30 s | How long the join screen waits: 20, 30, 45 or 60 s. **Off** skips the join screen and starts with panel 1 alone. |
| **Tally Hold** | 25 s | How long the multiplayer campaign tally stays up before anyone can skip it, so every player sees the result. 0–60, and 0 turns it off. |

**Messages + Banners**

| Item | Default | What it does |
| --- | --- | --- |
| **Singleplayer Messages** | Off | Pickup, kill and locked-door messages at the top of the screen in single-player games. |
| **Multiplayer Messages** | Off | The same, in games with more than one player. |
| **Winning Banner** | Rainbow | The deathmatch WINNER banner: **Rainbow**, **Cycle** (the whole word in one colour at a time) or **Off**. Team Deathmatch uses the team colours. |

**HUD Configuration**

One On/Off row per element of the full-screen HUD overlay (shown at the largest screen size, with
no status bar): **Keys**, **Ammo**, **Health**, **Armor**, **Frags**, **Kills**, **Items**,
**Secrets**, **Level Clock**, **Ammo Breakdown**. Kills, items and secrets are shown top right, so a
player can tell whether a max run is still possible. These rows edit the `overlay` cvar's letter
string.

→ [`arcade/hud.md`](arcade/hud.md)

**Cabinet Link** (not a list of rows: the page has its own controls)

| Row | What it does |
| --- | --- |
| **ROLE** | **Off**, **Master** or **Member**. One master per group of cabinets; members connect to it. |
| **NAME** | This cabinet's name, as other cabinets show it. |
| **PASSCODE** | The shared secret used to pair cabinets. The page shows only whether it is set and how long it is. |
| **MASTER** / **ALLOWED** | On a member, the master's address. On a master, the allow list of cabinet addresses, which opens its own page for adding and removing them. |
| **PORT** | The network port. |
| **FORGET PAIRED CABINETS** | Drops every pinned cabinet identity, so pairing starts over. Press fire twice to confirm. |

Below the rows is a status block: every known cabinet, whether it is online, and whether shared
scores are in step.

**Cabinet Link Options** (master only; only the master's settings count)

| Item | Default | What it does |
| --- | --- | --- |
| **Select Game Sync** | Off | When a game is picked on any linked cabinet's Select Game page, every other cabinet that is not in a game follows. |
| **Copy Missing Wads** | Off | A member that lacks the chosen IWAD or level pack copies it from the master. |
| **Music Cabinet** | All | During a linked game, which cabinet plays the music. The others stay quiet so the music does not play out of step on several machines. **All** lets every cabinet play its own. |

→ [`arcade/cabinet-link.md`](arcade/cabinet-link.md)

### Sound Volume

Operator only. **Sound Volume** and **Music Volume** sliders, then CD volume, music source and
sound device options where the build has them, and **Random sound pitch**. The attract cycle plays
at *Attract Volume* percent of these.

### Video Options

Operator only.

| Item | What it does |
| --- | --- |
| **Drawing Options >>** | Choose the drawmode: Software 8bit/15bit/16bit/24bit/32bit, Native, or OpenGL. Each drawmode has its own config file. |
| **Video Modes >>** | The resolution list. ★ It includes wide and scaled modes and can be filtered by aspect ratio. |
| Fullscreen | Fullscreen or windowed. |
| **Gamma Options >>** ★ | Gamma Function, Gamma, Black level and Brightness, moved to their own page. F11 opens it directly. |
| Wait Retrace | Wait for vertical sync. |
| Screen Size | View size. The largest size (no status bar) shows the HUD overlay. |
| View fit | How much of the world fits into the drawn frame on non-4:3 screens. |
| **Keep aspect** ★ | **Yes** letterboxes a frame whose shape does not match the panel. **No** stretches it to fill. Software drawmodes only; greyed out in OpenGL, where the monitor does the scaling. |
| Scale Status Bar | Scale the status bar to the screen width. |
| Dark Back | Darken the screen behind the menu. |
| Console font / Message font | Text sizes. |
| **Performance Options >>** ★ | Speed settings (below). |
| OpenGL 3D Card Options >> | Hardware renderer settings (below). |

→ [`arcade/software-fullscreen.md`](arcade/software-fullscreen.md), [`arcade/ultrawide.md`](arcade/ultrawide.md), [`arcade/drawmode-switching.md`](arcade/drawmode-switching.md)

**Performance Options** ★

| Item | What it does |
| --- | --- |
| **Framerate Cap** | Uncapped, or 35–240 fps. Above 35 the view is interpolated between game tics. **35** is the stock engine exactly. Default 60. |
| **Render Threads** | Draw split-screen views on several CPU cores: Auto or 1–4. Software only; greyed out otherwise. Default 1. |
| **8bpp Draw** | Draw at 8 bits per pixel and expand through the palette at the end. It helps where memory bandwidth is the limit, such as on a Pi. |
| **Row Padding** | Pad each row of the software frame to avoid cache conflicts at power-of-two widths. Software only. Measure it with `tools/perfchart.py` before relying on it. |
| **Show Ticrate** | On-screen frame rate counter. (Stock setting, moved here.) |

→ [`arcade/uncapped-framerate.md`](arcade/uncapped-framerate.md), [`arcade/render-threads.md`](arcade/render-threads.md)

**OpenGL 3D Card Options**

| Item | What it does |
| --- | --- |
| Mouse look | How far looking up and down extends the view. |
| Field of view | Horizontal FOV. |
| Quality | Colour depth. |
| Texture Filter | Nearest, bilinear, trilinear and similar. ★ The setting from the config is now applied at startup; before, it was silently ignored. |
| Translucent HUD | HUD transparency. |
| Lighting >> | Corona drawing, dynamic and static lighting, monster ball light, and ★ **Vanilla Weapon Lighting** (light the weapon as the software renderer does, instead of through the darker sector curve). |
| Fog >> | Fog on/off, colour, density. |
| Gamma >> | Red, green and blue gamma. |
| Development >> | MD2 models, translucent walls, polygon shape. |
| **Shaders >>** ★ | A post-process screen shader: Off, zfast CRT, CRT-Pi, CRT-Lottes, CRT-Geom, FXAA, Software Look, VHS, Greyscale, Sepia, Night Vision, Game Boy. |

→ [`arcade/shaders.md`](arcade/shaders.md), [`arcade/invulnerability.md`](arcade/invulnerability.md)

### Setup Controls

Operator only.

| Item | What it does |
| --- | --- |
| **Guided setup P1** … **P4** ★ | Shows the recommended layout, then asks you to press each of the ten cabinet controls in turn: four stick directions, then buttons 1–6. It binds whatever you press. Escape cancels and keeps the old layout. Only panels the cabinet has are listed. |
| **Recommended layout** ★ | The play-tested six-button layout: 1 fire, 2/3 strafe, 4 use, 5/6 weapon down/up, and the stick to move and turn. |
| Control per key | Whether one key may drive one control or several. |
| Mouse Options >> | Use mouse, mouselook, mouse move, invert, x/y speed, double-click, motion, grab input. |
| Second Mouse config >> | Player 2's mouse. |
| **Player1 Controls >>** … **Player4 Controls >>** | The full per-action binding pages (three pages each). ★ Panels 3 and 4 are new. The third page has **Devmode Restart** ★, the key that restarts the cabinet into or out of `-devmode` from the attract screen. |
| Joystick Options >> | Joystick deadzone (and double-click where built). |
| **Xbox Controllers >>** ★ | One row per panel. Select a row and press a button on a gamepad to give that panel a complete gamepad layout in one step. It shows which pad each panel has and whether that pad is plugged in. |

→ [`arcade/input.md`](arcade/input.md), [`arcade/menus.md`](arcade/menus.md)

## The launcher

Stock. It appears only when startup fails, for example when no IWAD can be found. You can edit
**Home**, **Doomwaddir**, **Config**, **Switch** (extra arguments), **Iwad** and **Game**, then
choose **Continue** to retry or **Quit Game**.
