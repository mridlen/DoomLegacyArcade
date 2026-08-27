# Operator audit counters

*Part of the DoomLegacy arcade cabinet build. Read before changing `au_stuff.c`/`au_stuff.h`, the
`AU_*` call sites, or the Audit page in `m_menu.c`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **What it is** (`au_stuff.c`/`.h`, new). An arcade board keeps a bookkeeping page; this is that
  page. It answers the questions an operator is otherwise guessing at — is anyone using the four
  player mode, does anybody get past MAP03, is the scoring working out there, how much of the
  cabinet's running time is actually being played — and **none of it can be recovered after the
  fact**, which is the whole argument for counting it as it happens.
  - **Reached at Options → Menu Options → Audit**, and from the console with **`audit`**. Menu
    Options is hidden by the lockdown, so the page is operator-only without needing a guard of its
    own.
  - Reset with **`clearaudit`** at the console or **`-clearaudit`** on the command line, mirroring
    `clearhighscores`. Resetting re-stamps the "since" date, which is what the page leads with.
  - **Nothing here affects play.** No gameplay code reads these counters, and they are deliberately
    not in the demo header and not part of the ranked ruleset. A counter that could change the
    simulation would be a desync waiting to happen.

- **The file** is `legacyhome/audit.dat`, plain text, one `key value` per line, like
  `highscores.dat`. Per-map lines are `map <wadcombo> <mapname> <count>`.
  - **Unknown keys are skipped, not rejected**, so a file written by a later build still loads into
    an older one. Fields can be added without a format version.
  - Back it up with `highscores.dat` and `demos/` if the numbers matter to you; unlike those two it
    is only bookkeeping, so losing it costs history rather than anybody's score.

- **When it is written.** At startup (so the boot is recorded even if the machine never exits
  cleanly), at every `Command_ExitGame_f` — the same "the player is done" funnel the run board uses
  — and in `D_Quit_Save`.
  - **A cabinet is far likelier to be switched off at the wall than quit cleanly.** Saving only at
    shutdown would lose most of what was counted, which is why the per-game save exists. The
    per-game save also means a power cut costs at most the game in progress.
  - `AU_Ticker` deliberately does **not** ask for a save: that would be 35 writes a second. The two
    clocks ride along with whatever save the next event triggers.

- **What each counter means, and the traps in them.**
  - **`uptics` counts game tics, not wall-clock seconds.** It advances from `G_Ticker`, so it is
    time in the game loop and excludes startup. This looks like undercounting when you first check
    it — seven headless test boots totalled 1205 tics (~34s) against minutes of wall time, which is
    correct: each run only reached ~110-250 tics before quitting. It is the right number for a
    cabinet ("how long has it been running the game") and the wrong one for "how long has the
    process existed".
  - **`playtics` is a *real game* on screen, not merely `GS_LEVEL`.** The attract cycle plays
    record demos in `GS_LEVEL` all day; counting those would make an idle cabinet look busy. The
    test is `!D_Attract_Running()`, the engine's own "a real game is running" flag — the same one
    the attract volume and the menu backdrop ask, so all three share one definition.
  - **`players1..4` is how many local players the game created**, which on a menu-started game is
    the join screen's count and on a console-started one (`-warp`, `map`) is every configured
    panel. So a `-warp` test on a four-panel cabinet records `players4`, and that is not a bug —
    `D_NumLocalPlayers()` really does create four. It was misread as one during testing; the tell
    is that setting `localplayers 1` immediately produces `players1`.
  - **`levels` counts `G_DoCompleted`**, so it is levels *finished*, in any mode, and does not care
    whether the run was scored.
  - **`deaths` counts every player death**, including deathmatch, unlike `HS_Player_Died` beside it
    which is single-player scoring only. A deathmatch death is still the cabinet being played.
  - **The three `unranked_*` counters are one-per-run, not one-per-event**, because each increments
    inside the `if( hs_run_ranked )` transition that voids the run. A run that is cheated *and*
    then dies counts only the cheat. That is intentional: the question is "why did this run stop
    scoring", and it stopped once.
  - **Per-map counts are keyed by wad combination** (`AU_GameKey`), so Doom II's MAP01 and a level
    pack's MAP01 stay apart — the same reasoning as the high score key. It deliberately does *not*
    fold in `single_level_mode`, unlike `HS_GameId_Mode`: that is a scoring distinction, and a map
    played in Single Level is the same map to an operator asking which maps get played.
  - The map table is capped at `AU_MAXMAPS` (192). When full, new maps are **not counted** rather
    than evicting an older entry — incomplete and visibly so, instead of quietly wrong.

- **Where the hooks are.** `AU_Init` (`d_main.c`, beside `HS_Init`), `AU_Ticker` (`G_Ticker`),
  `AU_Game_Started` (end of `G_InitNew`), `AU_Level_Started` (`G_DoLoadLevel`),
  `AU_Level_Completed` (`G_DoCompleted`), `AU_Player_Death` (`P_KillMobj`, beside
  `HS_Player_Died`), `AU_Unranked` (the three void sites in `hs_stuff.c`), `AU_Board_Placement`
  (`HS_Run_Finished`, where the initials prompt is armed), `AU_Save` (`Command_ExitGame_f` and
  `D_Quit_Save`).
  - **`AU_Game_Started` is in `G_InitNew`, not in the menus**, because every route that starts a
    game passes through it — Single Player, Single Level, Multiplayer, the console `map` command —
    and hooking each menu would miss whichever route was added next.
  - Everything is guarded on `demoplayback` inside the `AU_*` functions rather than at the call
    sites, so an attract demo cannot inflate the numbers no matter which hook it reaches.

- **The page layout is two columns**, because one column of all of it ran to y=205 in a 200 unit
  screen. Glyphs are 7 tall so the pitch is 8; labels are kept to about 12 characters so the widest
  still clears the right-justified value in its column.
  - **Verified numerically against a real capture**, not by eye (the standing rule): a temporary
    console command opened the page and took a screenshot, and the TGA was measured — **zero lit
    pixels in the base x=156..166 column gap** (so the columns cannot be touching), none past base
    x=318, none past base y=196, none before base x=4, and rows landing on an exact 8-unit pitch
    with 7-tall glyphs. A control shot of the title screen in the same run proved the capture
    worked.
  - The capture is only possible under the **software** drawmode with the dummy driver, or under
    `offscreen` for the hardware path — see `CLAUDE.md`. A screenshot taken from a console command
    also needs the binary in the scratch directory to actually be the one just built; a stale copy
    there produced "no TGA at all" and looked like `M_ScreenShot` failing.

- **Verification.** Each counter was driven headlessly and read back out of `audit.dat`:
  `-warp` gave `games 1`, `map doom2 MAP01 1`; `localplayers 1` moved the headcount from `players4`
  to `players1`; `wait/exitlevel` gave `levels 1`; `kill` gave `deaths 1` and `unranked_death 1`;
  `god` gave `unranked_cheat 1`; a run through `exitgame` that took a board place gave
  `placements 1`; counters accumulated correctly across eleven boots; and `-clearaudit` zeroed
  everything and re-stamped the date. The `audit` console command prints the same numbers — its
  output goes to the in-game console, not stdout, so checking it headlessly needed the prints
  temporarily routed through `GenPrintf(EMSG_warn, ...)`.
