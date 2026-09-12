# Doom Legacy Arcade

A fork of [DoomLegacy](http://doomlegacy.sourceforge.net/) 1.48.18 customised to run unattended in
an arcade cabinet: locked-down menus, joystick-and-buttons navigation, an attract cycle, and a
persistent high-score table with saved record demos.

It plays Ultimate Doom, Doom II and Final Doom (Plutonia and TNT), plus level packs in `.wad` form.
You supply the game data — no copyrighted content is included here.

The program calls itself **Doom Legacy Arcade**, the binary is `doomlegacyarcade`, and it prints
what it is a fork of on every launch. That is deliberate in both directions: nobody should mistake
this for stock DoomLegacy and take a bug here to that project, and upstream keeps every bit of the
credit it is owed. **It is unaffiliated with the DoomLegacy team**, who have not asked for any of
this and are not responsible for it.

> Looking for internals, or hacking on the code? See [`CLAUDE.md`](CLAUDE.md), which documents the
> engine architecture and the build, and [`docs/arcade/`](docs/arcade/), which has a write-up per
> feature — what was tried, what broke, and how it was verified. This file is for people who want to
> *run* the thing.

---

## Use of AI disclaimer

The arcade customisations in this repository were almost entirely vibe-coded using Claude. The
original DoomLegacy underneath them, of course, was not.

Be aware that promoting AI-assisted software in places such as the Doomworld forums or the ZDoom
Discord is likely to get you banned.

## What's different from stock DoomLegacy

**For the player**

- **Menus are locked down.** New Game, a handful of Options, and — while a game is running — End
  Game. No save/load, no multiplayer setup, no video or sound settings to get lost in. Quit is
  hidden as well, since a cabinet has nothing to quit *to*; the operator can put it back.
- **The cabinet buttons drive the menus.** No keyboard needed — the stick moves the cursor, fire
  selects, use backs out.
- **Up to four players on one machine.** Two share the screen as the usual stacked halves — or side
  by side, if the operator prefers — and three or four get a 2x2 grid, or four columns on a very
  wide screen, each with their own HUD. A **join screen** after the skill select lets each panel
  press fire to be counted in, so three players at panels 1, 3 and 4 is unambiguous. Four players
  cost about the same as one: each view is drawn on its own core.
- **Campaign and Deathmatch start in one press.** New Game offers the two games people actually
  ask for, already set up: **Campaign** plays the episode, solo or as co-op depending on how many
  press fire on the join screen, and **Deathmatch** is a deathmatch with its ruleset chosen. The
  full **Multiplayer** page, with every setting on it, is still there for anyone who wants it.
- **Smoother than 35 FPS.** Doom's simulation runs at 35 tics a second and the engine used to draw
  exactly one frame per tic. It now draws as many as the display can take, with everything moving
  interpolated between tics — so on a 60 or 144 Hz panel the motion is genuinely smoother, while the
  game itself, and every recorded demo, is untouched. Capped at 60 by default.
- **Ultrawide screens.** 21:9 and 32:9 panels are supported properly — the view is drawn at the
  monitor's real shape with the field of view widened to match, rather than a 4:3 picture stretched
  across it, and the menus, HUD and full-screen pages no longer stretch with it.
- **Full-screen menus and score pages.** The attract pages, the intermission and the finale fill the
  screen at any resolution instead of sitting in a letterboxed 4:3 box with a tiled floor texture
  around the edges.
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
  the episode naturally tops the board. One place per episode, skill and category for the campaign
  — "who got furthest, and fastest among those" has one answer — and three per map for Single Level.
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
- **Kills / items / secrets** on the HUD, so you can see whether a max run is still alive, and a
  breakdown of all four ammo types, stacked above the keys. Both are single player only.
- **Idle timeout.** Walk away and the cabinet returns to the attract screen by itself — 60 seconds
  by default, with a 15-second warning counting down first. Both are console settings
  (`idletimeout`, `idlewarntime`) rather than menu rows, and neither applies in a `-devmode`
  session.
- **A game selector** listing whichever IWADs are actually installed, plus any level packs you drop
  in, so the cabinet can offer several games from one menu.

**For the operator**

- **Settings don't persist for players.** Anything changed during a session is forgotten at the next
  launch. Only an operator session writes the config.
- **One settings page per player.** Colour, crosshair and control scheme are together on a single
  page — **Options → Player → `Player1 config`**, through `Player4 config` — instead of being spread
  across two pages and three menu levels. Only the panels the cabinet has are listed. Operator-only
  settings (autoaim, always run, mouse, weapon preference) are still there under *Player config* in
  a `-devmode` session.
- **A guided control setup** that asks for each control in turn and binds whatever you press —
  stick, buttons, or anything else your panel is wired to. One per panel, up to four.
- **A Players and Views page** (Options → Arcade Options → **Players & Views**) gathers everything
  about how many people can play and how the screen is divided: how many control panels the cabinet
  has, whether two players get stacked halves or side-by-side, whether three or four get a 2x2 grid
  or four columns, which quadrant each panel drives, and how long the join screen waits.
- **A Performance page** (Options → Video Options → **Performance Options**) holds the settings that
  trade picture for speed: **Framerate Cap**, **Render Threads**, **8bpp Draw** and **Show
  Ticrate**. Render Threads is what lets a Pi run a four-way split at full speed; it applies to the
  software renderer only and is greyed out under OpenGL, where it would gain nothing.
- **A usable video mode list.** Every mode the display offers is now reachable — the list pages
  instead of stopping dead partway through, sorts by size, drops the duplicate entries a monitor
  advertises once per refresh rate, and can be filtered to one aspect ratio so a 32:9 panel isn't
  buried in 4:3 modes it will never use.
- **A switch for the rocket trails.** **Options → Effects Options → Rocket Trails** (in a
  `-devmode` session — the Effects page is hidden on a locked cabinet) turns off the trail of smoke
  behind a rocket, which vanilla Doom does not have — DoomLegacy added it. It ships **on**, the way
  DoomLegacy has always drawn it, so nothing changes until you decide you want vanilla. The same
  switch also covers the smoke behind a charging lost soul, because the engine draws both from one
  routine; there is no way to keep one and not the other. Turning it on or off is safe for the
  high-score board either way: each record demo remembers which way it was set and replays
  correctly, so old records keep working after you change it.
- **A cheats menu** — god mode, all weapons and keys, no clipping, exit level, and a position
  readout. Operator-only by default, or leave it up for players. Using one voids that run's score,
  except the position readout, which only shows information.
- **A configurable initials timeout** (Options → Arcade Options, 60 seconds by default), for how long
  the initials page waits before accepting what is on it. Nothing is waiting on it — the cabinet is
  already back on the attract screen behind the page — so it can afford to be patient.
- **A quieter attract screen.** The cabinet advertises itself with sound, but not at playing volume
  all day. **Options → Arcade Options → Attract Volume** is a percentage of the normal volumes,
  applied whenever the attract cycle is on screen and dropped the instant a game starts; `0` makes
  the attract screen silent, `100` is the old behaviour. Defaults to 50. **Pressing anything brings
  the sound straight back up to normal** — the menu you land on should not be quieter than the demo
  that got your attention — and it drops back down again if you walk away without starting a game.
- **Quit is off the menu.** **Options → Arcade Options → Quit Menu** puts the Quit Game entry back
  for players; it ships off, because quitting drops whoever pressed it onto a desktop they should
  never see. A `-devmode` session always keeps Quit whatever the setting says, so the operator is
  never locked in.
- **A chase camera switch.** **Options → Arcade Options → Chase Cam Demo** turns the third-person
  attract demos on and off. On by default; it costs nothing on a cabinet with no records yet, since
  there is then no record demo to show that way.
- **An audit page**, the way an arcade board has one: games played and how many people were
  playing, levels finished, deaths, how much of the cabinet's running time is actually being
  played, which maps get played most, and how often a run stopped being scored and why. Under
  **Options → Arcade Options → Audit**, or type `audit` at the console.
- **A boot game setting**, so the cabinet always starts in the game you chose rather than whichever
  IWAD the search finds first.
- **A key that unlocks the cabinet**, so operator settings can be reached on a built cabinet with no
  command line. Plug a keyboard in, press Scroll Lock at the attract screen, and the cabinet
  relaunches itself in operator mode; press it again and it saves your changes and comes back
  locked. It is an assignable control like any other (**Devmode Restart**, on the Setup Controls
  pages), and it is ignored during a game, so nobody can lose a run to it.
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
- **Software rendering was garbled in fullscreen.** The top line of the screen correct and
  everything below it sheared sideways. The engine hands its finished frame to SDL with the wrong
  row stride — that of the window's own framebuffer, a different buffer that is never displayed,
  rather than that of the buffer being sent. The two match exactly while the window is the size
  being drawn, which is always true in a window and rarely true in fullscreen, so the bug hid until
  you went fullscreen. It also read 1.6 MB out of a 1.0 MB buffer.
- **Software mode would not start at all on a display that offers no small resolution.** The
  fullscreen list is built from the modes the display advertises, with anything bigger than the
  engine's own 1600x1200 drawing limit filtered out — so a panel offering only 1920x1080 leaves it
  empty, and that is a fatal error at startup rather than a fallback: "setup drawmode failed, cannot
  use native window". The software renderer now scales into whatever the desktop is, so it no longer
  needs the display to offer a mode it can draw at.
- **A colour depth left over from another drawmode stopped the game starting in software mode.**
  Each drawmode has a fixed depth and the engine checks it, then immediately discards that check and
  takes the depth from the config instead — so a `config.cfg` still saying 32 bits from an OpenGL
  session asked a 24-bit display for a 32-bit mode, found none, and gave up. The engine keeps
  running in its 800x600 startup window, which reads as the loading screen setting the resolution
  rather than as a failure. The drawmode's own depth is now asserted at startup as well as on the
  menu path.
- **Selecting a software drawmode from the video menu killed the display.** Each drawmode's config
  file carries its own colour depth, and nothing checked it against what that drawmode can actually
  do — so a palette mode asked for a 32-bit screen, the mode change failed *after* the renderer had
  already been torn down, and the engine carried on running with nothing on screen. It looks
  exactly like a freeze.
- **A menu on which nothing is selectable hung the game.** The cursor's up/down search is an
  unbounded loop looking for a selectable row, so a page where every row is disabled spins inside
  the event handler for ever — no tics, no redraw, no way out. Both loops are bounded now.
- **The sound thread could kill the game at the start of a level.** Starting a sound published the
  channel to the mixer before it had filled in the volume table the mixer reads a line later, so an
  audio callback landing in that window dereferenced a null pointer and took the whole process down
  from SDL's audio thread — with the game thread nowhere in the backtrace. Only ever on the first
  use of each of the sixteen channels, which is the first burst of sound after a level loads, so it
  read as a random crash on startup. It showed up once in about 110 headless demo replays.

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
- **Hairline seams where surfaces meet.** Thin bright lines along walls and across flats, in two
  separate families with two separate causes — the sky is drawn behind everything, so either hole
  shows as a one-pixel white line. The node builder rounds a split vertex to whole units, so the
  wall (built from segs) and the flat (built from subsector polygons) disagree by up to ~0.9 map
  units. Wall against flat is now closed by pulling the *polygon* corner onto the wall, which is
  anchored to real map data; flat against flat by `SolveTProblem`, which was pruning nearly every
  candidate away on a bbox test comparing an x edge against a y bound. Checked over whole maps by
  counting gap-producing T-junctions rather than through screenshots: E1M1 4→0, E1M2 16→0,
  E1M3 10→0, E1M5 16→0, E1M7 13→0, MAP01 2→0, MAP15 8→0. Software rendering is unaffected, and
  neither change can touch gameplay or demos.
- **A level's palette tint outlived the level.** Finishing a level in a radiation suit left
  everything after it green, and taking a hit at the exit switch left it red, right through the
  intermission and into whatever came next — the tint is only ever reset when the *next* level
  starts.
- **Changing resolution in OpenGL did nothing.** Three separate things stopped it: the mode change
  required a texture that belongs to the software path alone and so always took the failure branch;
  the code then asked SDL what mode it had got and was told the mode SDL *intended*; and a
  fullscreen window created moments after its predecessor was destroyed does not reliably get input
  focus, without which SDL never applies its mode at all. The result was that OpenGL always rendered
  at the desktop resolution, whatever the menu said — and there is no scaling step in the hardware
  renderer, so nothing else could correct it.

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
  fast monsters were all written before the new game's settings had been applied. Those were only
  misleading to anything that reads a header. One field was worse: the multiplayer byte. Playback
  restores it from the header and sets the level up with it, so a solo run recorded in a session
  that had earlier been multiplayer replayed with *multiplayer rules* for the whole level — weapons
  and keys persisting on pickup, different kill accounting, a different damage path. The E1M3 run
  that found this matched its recording for 901 tics, then took damage the replay didn't, drifted
  off route and spent its last 600 tics stuck on a lift.
- **Every DoomLegacy demo replayed under different rules than it was recorded with.** Playback
  decided which Boom-era behaviours to switch off by comparing the demo's version number against
  Boom's numbering — but Legacy demos are numbered 111–148 and Boom demos 200–214, two separate
  schemes that are not comparable, so the test was never true and the whole Boom behaviour set was
  switched off on playback while recording left it on. Eight engine behaviours differed between
  recording and replay, the movement model among them.

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
- **The HUD overlapped itself in splitscreen below 640x480.** Where the elements *sit* scales
  smoothly with the screen, but the size they are *drawn* at is a whole-number multiple that cannot
  go below 1:1 — so in a four-way split at 320x200 the ammo and armour counts ran into each other,
  the health count ran off the left edge of its quarter, and the three key icons were drawn on top
  of one another. A view too small for the big status numbers now uses the small ones (the same
  digits the classic status bar uses for ammo) and drops the icons beside them, and the key icons
  never step by less than they are wide. Full screen play is unchanged at every resolution, and so
  is a four-way split at 640x480 and above.
- **The weapon floated in mid-air in side-by-side two-player.** The weapon sprite is scaled from
  the width of the view, which is only correct while the view is as tall as it is wide in
  proportion — true full screen and in a four-way split, false for the side-by-side halves, which
  are half width and full height. So it was drawn at half size but still anchored to the middle of
  the screen, leaving it hanging a quarter of the view above the floor at every resolution. It is
  anchored to the bottom of the view now.
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

- **A 4:3 resolution was stretched across a widescreen monitor instead of getting black bars.**
  In software fullscreen the finished picture was scaled to fill the whole panel whatever shape it
  was, so 640x480, 800x600 or 1024x768 on a 16:9 screen came out **33% too wide** — and the
  renderer had drawn that picture *for* a 4:3 frame, so the distortion was real and not a matter
  of taste. It looked like OpenGL was doing it right, but OpenGL only changes the display mode and
  lets the monitor's own scaler add the bars; on a monitor set to stretch, it would have looked
  just as wrong.

  There is a **Keep aspect** setting on Video Options now, on by default. The picture is fitted to
  the screen at its own shape with black bars filling the rest — down the sides normally, along the
  top and bottom on a monitor turned on its side. Set it to **No** for the old behaviour if you
  would rather fill the screen than keep the proportions. It does nothing at a resolution that
  already matches the monitor's shape, so it changes nothing on a cabinet running at native
  resolution, and it is greyed out in OpenGL, where the monitor is doing the scaling.

  Note this is **not** what *View fit* does, which is easy to assume. View fit decides how much of
  the world goes into the picture the engine draws; Keep aspect decides how that finished picture
  is placed on the screen. No View fit setting can add black bars.

- **At 800x600 the HUD was drawn a third narrower than it should be.** Only at 800x600 — every
  other 4:3 resolution was fine, which is what made it odd. The status numbers and icons were the
  same width as at 640x480 but half again as tall; the health cross, which is square, came out a
  tall rectangle. The 2D art is drawn at whole-number scales, and 800x600 is the one resolution
  where the width could not take the scale the height had picked, so it kept a mismatched pair
  instead of bringing them back together. It now uses the same scale on both axes, as its
  neighbours do. Nothing else moves — 800x600 is the only resolution affected.

**Software renderer speed**

- **Wasted work on every damage and pickup flash, with 8bpp Draw on.** Every step of the red damage
  flash and the gold pickup flash rebuilt a 4,096-entry colour table from scratch: about a million
  colour comparisons, 3.6 ms on a laptop. The table depends only on the normal palette, never on the
  flash, so every rebuild produced exactly the table that was already there. It is now rebuilt only
  when the normal colours really change (a gamma change, a different palette): once per game
  instead of 85 times in a three-level run. Nobody noticed it in play, even on the Pi, but it was a
  spike on exactly the frames where the most is happening. Software renderer with 8bpp Draw on
  only, which is how a Pi runs.
- **Walls, floors and ceilings draw about 12% faster with 8bpp Draw on.** The drawing loops re-read
  their settings from memory on every pixel instead of once per line, because the compiler could
  not prove that writing a pixel had not changed them. Measured at 1024x768 on a laptop, the time
  spent drawing the 3D view went from 3.38 ms to 2.97 ms a frame; the picture is identical, checked
  frame by frame. The same change at 32 bits per pixel measured slightly *slower*, so the higher
  colour depths were left as they were.
- **Getting the picture onto the screen is faster in the software renderer.** On a Pi 3 that last
  step was more than half of every frame, bigger than drawing the 3D view. Two changes. The screen
  is now cleared before the picture is drawn onto it even when the picture fills the screen: the
  Pi's graphics chip works in tiles, and a frame that does not start with a clear makes it read the
  previous frame back before drawing over it. That is what made 640x360, 864x486 and 960x540 —
  the sizes that fill a 16:9 panel exactly, and so were never cleared — slow for their size: with
  the clear, 640x360 went from 59 fps to 72 on the Pi, level with 640x350. And with **Render
  Threads** above 1 the 8bpp palette expansion is shared across the cores instead of being done by
  one. On the laptop that halved its cost; on the Pi it made no visible difference, because at that
  point the Pi is waiting for its graphics chip rather than its processor. The picture itself is
  unchanged.
- **Clearing HUD messages erased the wrong part of the screen at 16 and 32 bits per pixel**, and
  would have at any depth with a padded screen buffer; **screenshots** would have come out
  scrambled with a padded buffer at 16 and 32 bits. Both stepped through the screen by the width in
  pixels where the engine's own rule is the row length in bytes. Nobody saw either: nothing padded
  the buffer, and the message clearing only runs with a reduced view size. Found while adding
  **Row Padding**, and fixed.

**Smaller things**

- **Gamma settings have their own page.** *Gamma Function*, *Gamma*, *Black level* and
  *Brightness* moved off Video Options to **Video Options → Gamma Options**, which is also where
  F11 now takes you. Video Options had run out of room; this made space for Keep aspect and leaves
  some over.
- **Low resolutions can be chosen in fullscreen**, not just in a window. 320x200, 400x300, 512x384,
  640x480 and 800x600 are offered fullscreen for the software renderer and scaled up by the GPU with
  nearest-neighbour filtering, so they stay sharp. Previously the fullscreen list held only the modes
  the display advertised and a request for anything else was silently snapped to the nearest one — a
  Raspberry Pi asked for 320x200 came up rendering 1024x768 in software. Software fullscreen also no
  longer changes the display mode at all, so switching to it is instant and does not make the monitor
  re-sync.
- **No loading window at startup.** The engine used to open a fixed 800x600 window before it had
  even read the wads, paint the startup messages into it, and then throw it away when the configured
  video mode was set — so launching flashed a wrong-sized window before the game appeared. The
  startup messages go to the terminal and the log as they always did, and nothing is put on screen
  until the real video mode is up. A startup *failure* still shows itself: the error console and the
  Launcher bring the window up when they draw.
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
- **Slime trails.** The thin ragged strips of floor showing through a wall, most famously on the
  E1M1 stairs — an artefact of the node data id's own builder wrote in 1993, baked into every IWAD.
  The engine now rebuilds the BSP nodes at level load with a modern builder (ZDBSP, vendored here)
  and uses the result **for rendering only**, so the simulation still walks the map's original tree
  and nothing about gameplay or demo playback changes. `-nonodebuild` turns it off.
- **Full-screen pages were letterboxed in software.** The software renderer scales the 320x200 art
  by a whole number, so a 1366x768 screen got a 1280x600 page with a tiled floor texture filling the
  rest. Whole-screen pages — the attract slides, the intermission, the finale — now scale by the
  exact ratio and fill the screen. Menus, HUD and status bar deliberately keep the whole-number
  scale, which is what keeps them sharp.
- **Ultrawide monitors were unusable.** The engine capped what it could draw at 1600x1200 and
  filtered the display's modes against that in three places without logging a thing — so on a
  3440x1440 or 5120x1440 panel every native mode was silently discarded and the list came back
  holding only the legacy 4:3 and 16:9 sizes the monitor also happens to advertise. It reads as "not
  supported" rather than "a constant ate it". The cap is now 5120x2160, and the view, field of view,
  weapon and 2D layer all follow the real aspect instead of stretching.
- **The video mode list lost modes before the menu ever saw them**, at three separate stacked caps,
  none of which logged anything. It also never removed the duplicate entries a monitor advertises
  once per refresh rate, so the caps were being spent on repeats. The list now dedupes, sorts by
  size, pages rather than truncating, and can be filtered by aspect ratio.

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

Hardware-wise almost anything modern is enough — the renderer is from 1993. What matters is
**single-core speed first, then cores**: the software renderer now spreads a frame across as many
cores as you give it (see [Performance](#performance) below), but everything else — the simulation,
the sound, the game logic — is still one thread. The main loop yields between frames once a
framerate cap is set, so a capped cabinet no longer sits at 100% of a core permanently the way it
used to; leave the cap off and it will. Budget for sustained load rather than average, and make sure
a fanless machine in a sealed cabinet won't thermally throttle.

### Performance

**A Raspberry Pi 3 is enough**, and that is the point of the threaded renderer. Measured on a Pi 3
Model B — quad-core Cortex-A53 at 1.2 GHz — set up the way this section recommends: the **software**
renderer, **Render Threads** `Auto`, **8bpp Draw** on, **Row Padding** on, and the Pi's desktop at
**1280x720**. Measured by `tools/perfchart.py`: the UV speed demo of E1M1 played flat out at each
size, vsync off, so the numbers are what the board can draw rather than what the screen shows.
Smallest first (September 2026). To make the same table for your own machine, see
[Choosing a resolution, and tuning performance](#choosing-a-resolution-and-tuning-performance):

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
| 1152x864 | 4:3 | 32 — taller than the desktop, see below |
| 1280x800 | 16:10 | 31 — taller than the desktop |
| 1280x960 | 4:3 | 27 — taller than the desktop |

**On a Pi, set the desktop to 1280x720.** The game draws at whatever size you pick, then the Pi's
graphics chip scales that picture up to the full desktop every frame — and with the desktop at
1920x1080 the chip was filling, and sending to the monitor, 2.25 times as many pixels. Worse, the
graphics chip and the processor share one memory bus, so all that traffic slowed the processor's own
drawing as well. Dropping the desktop from 1920x1080 to 1280x720 made every size faster, by 14% at
1024x768 up to 46% at 320x200: 640x480 went from 62 fps to 77, 640x350 from 75 to 93. There is no
point drawing more lines than the desktop has, so on a 720p desktop leave the three sizes taller than
720 lines alone; they are drawn big and then shrunk.

**These are benchmark numbers; play runs slower.** The demo is the opening of E1M1, and in play the
same Pi read about 15% lower: with the desktop at 1920x1080, 640x350 gave 61–64 in play against 72–75
in the benchmark. **On a 60 Hz panel, aim for about 75 here to hold 60 in play** — 640x480 or smaller
on a 1280x720 desktop, and 512x384 if you want room to spare in the busiest fights — and check it with
**Show Ticrate** on in the busiest level you have. Anything above the panel's refresh rate is
never shown, so for play set **Framerate Cap** to the panel's rate (60) rather than leaving it
uncapped. Uncapped is for measuring, and on a Pi it only adds heat.

**On a Pi, turn Row Padding on** (Performance Options). Without it the two 1024-wide sizes take
about twice as long to draw as their neighbours: with the desktop at 1920x1080, 1024x768 went from
21 fps to 31 with it on, and 1024x576 from 33 to 38. At every other size it made no difference
beyond the run-to-run noise. (640x360 and 864x486 used to be slow for their size as well. That was
the engine, and is fixed.)

**Four players cost about the same as one** — within a couple of FPS at every size it was checked
at. With one player the renderer cuts the single view into vertical bands, one per core; with four
it gives each player's view its own core. Either way the work is spread over all four, so the numbers above
are what the cabinet does *full*, not what it does empty. That was not true before: the old figures
here were 35 FPS at 640x480 and ~30 at 800x600 for a four-way split, single-threaded.

35 FPS is worth knowing as a landmark. The simulation runs at exactly 35 tics a second and always
has, and until recently the engine drew exactly one frame per tic, so 35 was a hard ceiling. It
isn't any more — **Framerate Cap** draws extra frames between tics with everything interpolated, so
above 35 the motion genuinely gets smoother. Below 35 the game is not slowing down; it is simply
skipping frames, and it stays playable well under it.

On a Pi, prefer the **software** renderer. The Pi's VideoCore IV has no fast path for this engine's
fixed-function OpenGL, so the hardware renderer runs through Mesa's slow compatibility layer and is
the *worse* of the two there. On a desktop GPU the reverse is true and OpenGL is nearly free.

If a heavier level or a bigger screen falls short, the low resolutions can be selected fullscreen
and are scaled up by the GPU with nearest-neighbour filtering, so dropping the render resolution
costs sharpness rather than screen size.

## Building

**You may not need to.** Every push to `main` is built for Linux and Windows on GitHub Actions and
the packages are attached to the run, and tagged releases carry the same two builds. Grab one from
the repository's Releases page if you just want to run the thing. Those are built for a generic
x86-64 baseline so they run anywhere; building it yourself gets you a binary tuned for your own CPU.

**The easy way — one command, and it tells you what to install if anything is missing.**

On Linux, macOS or FreeBSD:

```sh
./tools/build.sh
```

On Windows, double-click `build.bat` (or run it from a command prompt).

The script works out which system and CPU it is on, checks that the compiler and libraries are
present, writes a build configuration for this machine, and builds. If something is missing it says
exactly what and gives you the install command for *your* distribution, rather than failing halfway
through a compile:

```
== Checking what is installed
  ok   : C compiler (cc)
  ok   : sdl2-config
  MISS : libzip

Install with:
    sudo dnf install -y gcc make SDL2-devel SDL2_mixer-devel libzip-devel ...
```

Add `--install-deps` (or `-InstallDeps` on Windows) and it will install them for you. Other useful
switches: `--deps` to only check, `--clean` to start fresh, `--jobs N` to limit parallel compiles.

It knows the Debian, Fedora, Arch and SUSE families and their derivatives — Ubuntu, Mint, Manjaro,
Rocky and so on are all recognised through the same mechanism. It will **not** overwrite a build
configuration you have already tuned; pass `--reconfigure` if you want it rewritten.

**Building for another machine?** The default is `-march=native`, which bakes in whatever the
*builder's* CPU supports. That is right for a machine building for itself and wrong for anything you
hand to somebody else: it links and packages without a murmur and then dies on the target with a
bare `Illegal instruction`. Pass `--arch '-march=x86-64 -mtune=generic'` (`-Arch` on Windows) for a
binary anyone else will run — and note it implies `--reconfigure`, or an existing configuration is
reused and the flag is silently ignored.

Windows builds through MSYS2/MinGW (this project is a GNU Make tree, so Visual Studio cannot build
it as it stands). If MSYS2 is not installed the script tells you how to get it; if MSYS2 is there but
empty — which is how it arrives — it lists the packages to install and can install them for you.
Confirmed on Windows 11: the script builds `doomlegacyarcade.exe` end to end and stages the twelve
runtime DLLs beside it — SDL2, SDL2_mixer and the codec libraries SDL2_mixer pulls in, which is a
longer list than anyone guesses. **The resulting binary has not been played**, only started, so
treat the first real session as the shakedown.

There is no unit test suite. The two checks that exist are `make smoke`, which starts the built
binary headlessly and exercises startup, level setup, a level exit and the OpenGL path, and
`make demotest`, which replays all of the cabinet's record demos and verifies the simulation is
unchanged tic by tic. Run the second after **anything that could affect how the game plays** — the
demos on the cabinet are people's high scores, and a gameplay change does not just alter them, it
invalidates them. Record the reference once with `make demotest_baseline` while the code is known
good; after that `make demotest` answers in three lines, in about forty seconds. See
`docs/arcade/demo-desync.md`.

### The manual way

Build from `svn1749/src`. First time only, copy the platform options file and make three edits:

```sh
cd svn1749/src
cp ../make_options_nix ../make_options
```

Then edit `svn1749/make_options`:

```make
SDL2=1                      # uncomment; the stock file targets SDL 1.2
ARCH=-march=native          # replace ARCH=-march=i686, which is 32-bit only
ENV_CFLAGS=-std=gnu17 -g    # add; GCC 15 defaults to gnu23, which breaks this code
```

Each of the three is a hard build failure if skipped, not a warning. The `-g` is optional, but
worth keeping on a cabinet that runs unattended: without it a crash backtrace is bare function
names, and it costs nothing at runtime. Then:

```sh
cd ..           # svn1749
make dirs       # create bin/, objs/ and dep/ -- they are build output, not in the repo
cd src
make depend     # run this serially, before any parallel build
make -j8
```

`make depend` first is not optional if you want `-j`: every dependency rule pipes through the same
temporary file, so parallel dep generation clobbers itself and fails with
`mv: cannot stat '../dep/sed.dep'`, which points nowhere near the cause. The compile phase
parallelises fine. Plain `make` on its own also works.

The binary lands in `svn1749/bin/doomlegacyarcade`, together with a `legacyhome/` folder holding the
cabinet's configuration.

On a different CPU — a Raspberry Pi or other ARM board — replace `-march=native` with the
appropriate flag, e.g. `-mcpu=cortex-a53`. No x86 assembly is involved, so nothing else changes.

## Installing the game data

Copy `common/legacy.wad`, `common/dogs.wad` and your IWADs into the same directory as the binary:

```sh
cp ../../common/legacy.wad ../../common/dogs.wad ../bin/
cp /path/to/DOOM2.WAD ../bin/
```

`legacy.wad` is required — it ships with this repository and holds the engine's own menu graphics,
including the cabinet's own artwork (the Single Level, join, cheats and game-over screens) and the
`ENDOOM` text screen printed on exit, so use the copy from `common/` rather than one from an
upstream DoomLegacy release. `tools/endoom.py` edits that exit screen; SLADE will not, which is why
the tool exists.

`dogs.wad` is optional and lives in exactly the same place, beside the binary. It carries the
sprites and sounds for MBF helper dogs, which the engine has none of its own for. It does nothing
unless the **Dogs** setting is raised (Options → Game Options → Adv Options, second page), and that
is an operator-only `-devmode` affair: the competitive ruleset pins helper dogs to none, like bots,
so a scored run never has them.

IWADs are the commercial game data and are **not** included; supply your own from a purchased copy.

The Select Game menu offers four: **Ultimate Doom** (`DOOM.WAD`, also accepted as `DOOMU.WAD` or
`DOOM_SE.WAD`), **Doom II** (`DOOM2.WAD`), **Plutonia** (`PLUTONIA.WAD`) and **TNT** (`TNT.WAD`).
Names are case-insensitive. Only games whose IWAD is actually found are listed, and the Select Game
entry disappears altogether when there are fewer than two things to switch between — installed
games and level packs both count.

As well as beside the binary, the game searches `<bindir>/wads/`, `~/games/doom`,
`~/games/doomwads`, `~/games/doomlegacy/wads` and the usual system locations, so an existing
install is usually found without moving anything.

The underlying engine also supports Heretic, but the cabinet's game selector does not list it —
that would need an entry adding to `gameselect_arg[]` in `m_menu.c`.

## Running

```sh
cd ../bin
./doomlegacyarcade
```

No arguments needed. The game finds its configuration in the `legacyhome/` folder beside the
binary, so the whole directory can be copied anywhere — a USB stick, another machine — and it will
behave identically. You can launch it by absolute path from anywhere; it locates its own files.

To move the cabinet to another machine, copy that one directory.

Building a dedicated machine? See [Keeping the cabinet running](#keeping-the-cabinet-running) in the
operator guide for the restart-loop wrapper you'll want.

---

## Playing

**New Game** offers four rows:

| | |
| --- | --- |
| **Campaign** | The normal game — episode, skill, play it through. One player or several: everyone who presses fire on the join screen plays it together in **co-op**, with monsters on. |
| **Deathmatch** | Players against each other, set up already: weapons and items respawning, no monsters, no bots, five minutes on the clock. Picks an episode where the game has them, and otherwise starts on MAP01. |
| **Single Level** | One chosen map, straight back to the menu afterwards, on its own score table. |
| **Multiplayer** | The page with every setting on it — map, skill, which co-op or deathmatch variant, monsters, bots. |

Campaign and Deathmatch are the two games somebody standing at the cabinet actually asks for, so
they take no setting-up: pick one, everyone presses fire, play. **Multiplayer** is still there
unchanged for anyone who wants to pick the exact map, run co-op with bots, or play a deathmatch
variant — nothing was taken away, it just isn't in the way any more.

Multiplayer here, in all of these, means everyone playing on *this* cabinet, sharing the screen.
Deathmatch is hidden on a single-panel cabinet, alongside Multiplayer — one person can't have one.

**End Game** is on the main menu, at the bottom, and appears **only while a game is actually being
played** — any kind: Campaign, Single Level or Multiplayer. On the attract screen there is
nothing to end, so it isn't there.

DoomLegacy's **networked** play between separate machines is still in there, under
**Networked Multiplayer** in a `-devmode` session, but it is hidden from players because it hasn't
been tested in this build — cabinet-to-cabinet play needs two cabinets. Treat it as untested rather
than unsupported: nothing was removed, and it may well work.

Under **Options** a player can change the crosshair, their colour, their control scheme, and pick a
game or level pack. Everything else is hidden, and nothing a player changes survives to the next
launch.

### Joining a game

On a cabinet with more than one control panel, a **join screen** appears once the game has been
chosen — after the skill on a Campaign, after the episode on a Deathmatch. Each panel presses
**fire** to be counted in, and the screen is laid out as the game is about to be: press fire and
watch your own square claim itself. It starts when the countdown runs out, or as soon as anyone
already in presses **use**, which the page says once somebody is in.

**This is what decides whether a Campaign is co-op.** One panel in and it is the solo run it has
always been, scored and recorded as usual; two or more and the same game starts as co-op instead.
Nobody has to choose the mode in advance — the people at the cabinet answer it by pressing fire.
A co-op game isn't scored, the same as any other game with more than one person in it.

Whoever joins plays at the panel they pressed at, so a lone player can use panel 3 and still get the
whole screen. One player gets the whole screen; two share it as **stacked halves**, or **side by
side** if the operator has set it that way; three or four get a **2x2 grid**, one quadrant each with
the fourth left empty for three players, or **four columns** on a very wide screen. Which quadrant
each panel drives is an operator setting too, so each player's view can be on the side of the screen
they are actually standing at. All of that is under
[Players, panels, and how the screen is divided](#players-panels-and-how-the-screen-is-divided).

The page is skipped entirely on a single-panel cabinet. It used to start the moment one person
pressed fire on a single player game, which is exactly what made co-op impossible to ask for — the
first hand on a button ended the question. It now always waits, and **use** is the way to skip the
rest of the countdown.

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

**Two schemes** are offered per player under Options → Player → `Player1 config` (through
`Player4 config`): **Look and Move** and **WASD**. They swap which pair of controls turns and which
strafes, and both work on the same wiring, so it's purely a player preference. Look and Move
matches how most joysticks and digital gamepads are normally set up, and is the better default.

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
./doomlegacyarcade -devmode
```

That gives you the full stock menus, disables the competitive ruleset, and is the **only** mode
that saves settings. The workflow is: launch with `-devmode`, change what you want, quit. Player
sessions then start from that baseline every time.

### Unlocking the cabinet without a command line

A built cabinet has no terminal to type that into, so there is a key for it instead. Plug a keyboard
in and press **Scroll Lock** at the attract screen. The cabinet shows `ENTERING DEVMODE...` and
relaunches itself unlocked — same as `-devmode`, because that is literally what it does: it restarts
the program with the flag added.

Change what you want, then press **Scroll Lock** again. It shows `LEAVING DEVMODE...`, **writes the
config on the way out**, and comes back up locked. There is no separate save step and no quitting to
a desktop; the settings are saved by the same code that saves them when a `-devmode` session quits
normally.

**The key only works at the attract screen.** During a game, an intermission, a finale or an
initials entry it does nothing at all. That is deliberate — the restart throws away whatever is
running, so without the rule a stray press could take a player's run, or a record they had earned
but not yet put their initials on. If you want to unlock mid-game, end the game first.

**Changing the key.** It is an ordinary assignable control, not a special setting.
**Options → Setup Controls → `Player1 Controls >>`**, then **next** twice to reach the third page.
It is there as **Devmode Restart**, just under Screenshot and above the "Joystick and Mouse Only"
heading. Select it and press the key you want, the same as rebinding anything else.

There is one per panel — `Player1 Controls` through `Player4 Controls` — and **any of the four
works**, since which page you set it on doesn't decide who gets to press it. Only Player 1 has a
default (Scroll Lock); panels 2 to 4 start unbound, which is how you leave them unless you have a
reason not to.

**Think before binding it to a panel button.** Because it is a normal control, nothing stops you —
but a cabinet button is a button players press, and at the attract screen this key restarts the
machine. The attract-screen rule is the only thing standing between that and a player pressing it
mid-game, so keep it on the keyboard unless you have a good reason. Scroll Lock and Pause are the
obvious choices; F12 and Print Screen are already the screenshot bindings, and the screenshot
handler sees the key first, so those two will take a picture instead of unlocking.

To clear it entirely, bind it to nothing — then `-devmode` on the command line is the only way in.

Because controls are only saved from an operator session, changing it is a two-step job the first
time: unlock, rebind, lock again — the lock step is what writes it to `config.cfg`, as
`setcontrol "devmode" "scroll lock"`.

### Setting up a control panel

**Options → Setup Controls → Guided setup P1** (devmode only). It shows the recommended layout,
then asks for each control in turn — stick directions first, then the six buttons by number —
binding whatever you press. Works with keyboards, joysticks, encoders, anything that reports as a
button. Press ESC to abandon and keep the previous layout.

There is a guided setup per panel, P1 to P4, and a **Player n Controls** page beside each one for
panels with more than six buttons — the guided setup only teaches the ten controls a standard panel
needs, so anything beyond that gets bound on the full page.

### Players, panels, and how the screen is divided

Everything about this is on one page: **Options → Arcade Options → Players & Views** (devmode only).

**Control Panels** is how many sets of controls the cabinet has, 1 to 4. It ships at 1, and until
you raise it the join screen never appears and panels 3 and 4 have no configuration pages — which
reads as those features being broken, when the cabinet simply hasn't been told they exist.

**2 Player Split** is `Top/Bottom` (the classic stacked halves) or `Side by Side`. On a wide screen
side-by-side gives each player a more natural shape than a letterbox slit.

**3-4 Player Split** is `Grid` (the 2x2 quadrants) or `Columns` (four full-height strips). Grid is
right up to and including 21:9. At 32:9 it is wrong — a quadrant of a 32:9 screen is itself 32:9, a
letterbox slit — where a column comes out close to the shape of a portrait arcade monitor. Three
players use four cells with one empty either way, so this covers "three columns" as well.

**Screen Order** is which quadrant of the 2x2 grid each panel drives, `1 3 / 2 4` or `1 2 / 3 4`.
The panels stand in a row across the front of a cabinet, so filling the grid in reading order puts
panel 2's view on the far side of the screen from where panel 2 is standing; the default fills it by
columns instead, and every player watches their own side. Reading order is kept for four people on
gamepads sitting wherever they like, which is what they will expect.

**Join Time** is how long the join screen waits, in seconds; `0` skips the page entirely.

Set the panel count first, then run the guided setup for each panel. Panels 3 and 4 have no preset
bindings on purpose — the two built-in schemes are chosen so one keyboard can drive two players, and
there is no third set that wouldn't collide.

### Choosing a resolution, and tuning performance

Both pages are under **Options → Video Options** (devmode only — Video Options is hidden from
players).

**Video Modes** lists what the display can do. It sorts largest first, hides the duplicate entries a
monitor advertises once per refresh rate, and **pages** rather than stopping partway through —
*Left/Right for more*, and the page number is shown, so look for that before concluding a mode is
missing.

The line above it reads **`Aspect: <shape>`**, and **pressing `A` cycles it**: `AUTO` (the default)
shows the shapes that suit the display, `All` shows everything, then `4:3`, `16:10`, `16:9`, `21:9`
and `32:9`. When the filter is hiding anything the line says how many, so a mode you cannot find is
never silently gone. On an ultrawide panel `AUTO` is what stops the list being buried in 4:3 modes
it will never use.

**Keep aspect**, just under *View fit*, decides what happens when the resolution you picked is not
the same shape as the monitor. **Yes** (the default) fits the picture to the screen at its own shape
and fills the rest with black bars — down the sides normally, along the top and bottom on a monitor
turned on its side. **No** stretches it to fill, which is what the cabinet used to do always: a 4:3
mode on a 16:9 screen came out a third too wide. It does nothing at a resolution that already
matches the monitor's shape, and it is greyed out in OpenGL, where the monitor's own scaler places
the picture rather than the engine.

It is **not** the same thing as *View fit* above it, which is easy to assume. View fit decides how
much of the world goes into the picture the engine draws; Keep aspect decides how that finished
picture is placed on the screen. No View fit setting can produce black bars.

**Gamma Options**, higher up the same page, holds *Gamma Function*, *Gamma*, *Black level* and
*Brightness*. **F11** opens that page directly from anywhere.

**Performance Options**, near the bottom of Video Options, holds the settings that trade picture —
or memory — for speed:

| Setting | What it does |
| --- | --- |
| **Framerate Cap** | `Uncapped`, or 35 / 60 / 75 / 100 / 120 / 144 / 165 / 240. Default **60**. |
| **Render Threads** | `Auto`, or 1 to 4. Default **1**. Software renderer only. |
| **8bpp Draw** | Draw the world at 8 bits and expand it through the palette at the last moment. Default **Off**. |
| **Row Padding** | Lay the picture out in memory with a little spare space at the end of each row. Default **Off**. Software renderer only. |
| **Show Ticrate** | Put the frame rate on screen — how you read the effect of the others. The number is an average over the last half second. |

**Framerate Cap** is how many frames a second are drawn. The simulation is not affected by it in any
way: it still runs at exactly 35 tics a second, and every recorded demo plays back identically at
any setting. Above 35 the extra frames are drawn *between* tics with everything moving interpolated,
which is real added smoothness rather than repeated pictures. Set it to your panel's refresh rate.
`35` is the old behaviour, one frame per tic with interpolation off entirely, and is the setting to
fall back to if anything looks wrong. `Uncapped` measured about 600 fps on a 60 Hz panel — ten times
the work for frames the display cannot show, which on a machine left switched on is heat and
electricity and nothing else. It is there for measuring what the hardware can do.

**Render Threads** spreads the software renderer over several cores. With more than one player each
view gets its own thread; with one player the single view is cut into vertical bands, one per core.
Either way it is close to a 4x gain on four cores, which is what makes a Pi 3 run a four-way split
at full speed. `Auto` picks a count from the machine. It is greyed out under OpenGL, where it would
gain nothing — the hardware renderer issues its GL calls from inside the walk of the level and a GL
context belongs to one thread.

**It ships at 1, deliberately.** Threading is still opt-in. Nothing crashes, it has run clean under
a thread sanitiser, and **nothing about the simulation changes** — scores, demos and gameplay are
identical either way, because only the drawing is threaded. What is not yet perfect is the picture:
splitting one view into bands makes about 1–2% of pixels sample the neighbouring texel, which is
inherent to slicing the drawing up and is what GZDoom's banded renderer does too; and on a few maps
with sky and open space a threaded frame still differs slightly from a serial one, and from itself
run to run. It is a handful of scattered pixels, not something you would notice playing. Turn it on
if you need the speed — which on a Pi you will — and set it to 1 if you ever want to rule it out.

**8bpp Draw** helps exactly when memory bandwidth is the limit and not otherwise. There are no 8-bit
display modes any more, so even in the software drawmode the renderer normally writes four bytes per
pixel; this makes it write one and expand at the end. Worth a lot on a Pi, usually nothing on a
desktop. Try it with **Show Ticrate** on.

**Row Padding** is for the sizes that run oddly slowly for their size. On a Pi 3 the two 1024-wide
sizes took about twice as long to draw as their neighbours: at exactly 1024 pixels a row, a column
of the picture lands in the same few slots of the processor's cache, so drawing a wall keeps
throwing its own data out. Padding each row by a little breaks that pattern. It changes nothing
you can see — the picture is identical, checked frame by frame — only where it sits in memory, and
whether that helps depends on the processor. On a Pi 3 it took 1024x768 from 21 fps to 31; on the
development laptop it made no measurable difference at all. So it is off unless you turn it on, and
on a Pi you should. Measure it on your own machine, with
`tools/perfchart.py --compare` against a run with it off (see below), rather than taking it on
trust; it takes effect straight away, with a moment's blank screen while the video mode is set
again.

**On a Pi, use the software renderer** — see [Performance](#performance) for the measured numbers.
The Pi's GPU has no fast path for this engine's fixed-function OpenGL, so the hardware renderer goes
through a slow compatibility layer and is the worse of the two there.

**To measure your own machine, run `tools/perfchart.py`** from the top of the source tree. It plays
a short fixed demo (UV speed on E1M1) once at each resolution in the table under
[Performance](#performance) and prints the same table for your machine, frame rates and all. It
uses a copy of the cabinet's settings folder, so it never touches the real config, scores or demos.
The whole list takes about seven minutes on a Pi 3.

```
tools/perfchart.py                   # every size in the Performance table
tools/perfchart.py --quick           # four sizes, a minute or two
tools/perfchart.py --runs 3          # each size three times, shown as a range like 61–64
tools/perfchart.py --compare perfchart-<host>-<date>.csv   # add a "before" column
```

Each run leaves a `.md` file, which is the table ready to paste, and a `.csv` that `--compare` reads
back, so measuring before and after a change is two commands. Beside each frame rate it shows where
the time went: **Views** (drawing the 3D view), **Present** (getting the finished picture onto the
screen) and **Other** (game logic and HUD), in milliseconds per frame. When one resolution is
oddly slow, those columns say whether the drawing or the display is to blame. On a Pi it also shows the board's
temperature after each size and warns if the Pi slowed itself down during the run. A Pi that runs
hot or on a weak power supply throttles its CPU, which looks exactly like the game being slow.
The windows it opens go fullscreen one after another; from SSH, set `DISPLAY=:0` first to use the
Pi's real screen, or pass `--headless` to measure without one.

### Cheats

**Options → Arcade Options → Cheats Menu** (devmode only) puts a **Cheats** entry on the main menu for
players: god mode, all weapons and keys, no clipping, exit level, and **Show Coordinates**. It ships
off, in which case the entry is operator-only and reachable just in a `-devmode` session.

Cheats are single-player only, and using any of them — from the menu, the console, or a typed
IDDQD/IDKFA/IDCLIP — voids that run's score. The HUD then shows `PLAYER CHEATED - UNRANKED` for the
rest of the run.

**Show Coordinates** is the exception. It draws position, angle, the sector you are in and the
linedef you are looking at, changes nothing in the simulation, and so does not void the run — the
same rule the typed IDDT/IDMYPOS/IDMUS already follow. It is there to report where a bug is on a
cabinet with no keyboard to read a console line from.

### Keeping the cabinet running

On a dedicated machine, launch the game from a wrapper script that restarts it in a loop, with an
escape hatch on a key that is **not** wired to any cabinet button. Players can then quit the game —
or it can crash — and the cabinet comes straight back up on the attract screen instead of dropping
someone to a desktop.

```bash
#!/bin/bash
cd /path/to/bin || exit 1
while :; do
    ./doomlegacyarcade
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
./doomlegacyarcade -clearhighscores
```

This clears both tables — `highscores.dat` and the `runs.dat` leaderboard with its initials — *and*
deletes the saved record demos. Deleting the files by hand is not enough: the tables are held in
memory while the game runs and get written back out.

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
| `-devmode` | Unlock menus, save settings, disable the ruleset (or press Scroll Lock at the attract screen) |
| `-clearhighscores` | Wipe scores and record demos at startup |
| `-clearaudit` | Reset the operator audit counters at startup |
| `-game <name>` | Start a specific game (`doomu`, `doom2`, `plutonia`, `tnt`) |
| `-warp <map>` | Jump straight to a map |
| `-file <wad>` | Load a wad at startup — a level pack, a soundtrack, a DEH/BEX patch |
| `-config <file>` | Use a different configuration file |
| `-v` | Verbose startup, showing which files were found |
| `-nonodebuild` | Don't rebuild the level's BSP nodes at load — the slime-trail fix, off |
| `-frameprofile` | Print a breakdown of where each frame's time actually goes |
| `-playdemo <file>` | Replay a record demo from `legacyhome/demos`, then quit |
| `-synclog` | While recording or replaying a demo, write one line of simulation state per tic to `synclog_rec.txt` / `synclog_play.txt`. Diff the two and the first differing line is where a demo went out of sync |
| `-noendtext` | Skip the exit text screen |
| `--version` | Print the version and what it is a fork of, and exit |

---

## Where your data lives

Everything is in `legacyhome/` beside the binary:

| | |
| --- | --- |
| `config.cfg` | All settings. Written only by a `-devmode` session. |
| `config8p.cfg`, `configgl.cfg`, `confign.cfg` | Video settings for each drawmode, applied after `config.cfg`. |
| `highscores.dat` | Best cumulative times per map, skill and category. Plain text, one record per line. |
| `runs.dat` | The run leaderboard — whole runs with their initials. Plain text. |
| `demos/` | Saved record demos, one per map/skill/category. |
| `levels/` | Level packs you've added. |
| `audit.dat` | Operator bookkeeping counters. Plain text. |
| `autoexec.cfg` | Optional. Console commands run at startup — where the `addfile` lines for soundtrack wads go. |

**Back up `highscores.dat`, `runs.dat` and `demos/`.** They are the only things here that can't be
recreated — your players' scores and the runs that set them. Everything else can be rebuilt from this
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
`t` the level clock (and, in Single Player, the `TT` run total above it), `b` the four ammo counts
broken out, single player only. The default is `kahmfeistb`.

**A new HUD element still doesn't appear after rebuilding.**
`config.cfg` overrides the compiled default, so an existing install keeps its old `overlay` line.
Add the letter by hand, or re-save from a `-devmode` session.

**Scores aren't being recorded.**
Check the HUD for `UNRANKED`. If it's there, either a gameplay setting differs from the standard
ruleset — the console log names which one — or somebody died. Returning to the attract screen
resets the ruleset automatically, so starting a fresh game normally clears it.

**The join screen never appears, or panels 3 and 4 have no settings pages.**
`Control Panels`, on the Options → Arcade Options → **Players & Views** page, is still at 1. Nothing
about the extra panels shows up until the cabinet is told how many it has. Check `Join Time` isn't 0
while you're there.

**Replacement music isn't playing.**
Check `music_source` is `Auto` rather than `MUS` — that alone disables it, and an older config may
still carry `MUS`. Then check the lump
names inside the wad against the list above; a track named after the *file* rather than the lump
simply never gets looked for. `-v` reports the wad being loaded at startup.

**A game is missing from the Select Game menu.**
Its IWAD wasn't found. Run with `-v` and check the search paths reported at startup.

**The game runs slowly, or the frame rate is choppy.**
Options → Video Options → **Performance Options**, with **Show Ticrate** on so you can see what each
change does. On a Pi or another low-power board: use the **software** drawmode, turn **Render
Threads** to `Auto`, turn **8bpp Draw** and **Row Padding** on, set the Pi's desktop to 1280x720,
and drop the resolution — on a Pi 3 Model B with a 1280x720 desktop, 640x480 or smaller should hold
60 FPS in play and 512x384 has room to spare. See [Performance](#performance) for measured numbers. On a desktop, use OpenGL and check
**Framerate Cap** matches the panel's refresh rate.

**My monitor's resolution isn't in the Video Modes list.**
Two things hide modes, and the page tells you about both. The `Aspect:` line at the top says how
many are filtered out — press **`A`** until it reads `All`. And the list **pages**: if it says
*Page 1 of 3*, press Left/Right. Only if it is still missing with `All` on every page is the mode
genuinely unavailable.

**The picture is fine but the motion looks different from what I remember.**
That will be **Framerate Cap**, which now draws frames between tics and interpolates them. Setting
it to `35` gives the exact old behaviour. It changes nothing about the simulation either way.

**The game crashed, and I want to report it usefully.**
The terminal log names the level. Every level load prints a line like
`Level: E1M7  skill 4  play  chasecam off  views 1`, and during the attract cycle it also names the
record being replayed — `demo E1M1  ITYTD  SPEED  1:11.05  AAA`. That line, plus the handful before
it, usually says what was on screen without anyone having to reproduce it.

For a backtrace as well, install the crash catcher once — `sudo apt install systemd-coredump gdb` on
Raspberry Pi OS, `sudo dnf install systemd-coredump gdb` on Fedora — and then after a crash run:

```
coredumpctl list
coredumpctl debug doomlegacyarcade --debugger=gdb \
    --debugger-arguments="-batch -ex 'thread apply all bt full'"
```

On a Pi, also run `sudo mkdir -p /var/log/journal && sudo systemctl restart systemd-journald`, or
the next reboot erases the logs.

**The game won't build.**
Almost always one of the three `make_options` edits above. `-march=i686` and the default `gnu23`
standard both produce errors that don't obviously point at the cause.

---

## Credits and licence

DoomLegacy is by Fabrice Denis, Boris Pereira and the DoomLegacy team, based on the original Doom
source released by id Software. This build tracks DoomLegacy 1.48.18 (SVN r1749) with local arcade
customisations.

Licensed under the **GNU General Public License**; see [`LICENSE`](LICENSE), and
[`svn1749/docs/LICENSE.txt`](svn1749/docs/LICENSE.txt) for the copy that came with the port. Doom,
Doom II, Final Doom and Heretic game data remain the property of their respective owners and are
not distributed here.
