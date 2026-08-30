# The attract cycle, the idle timeout and arcade death

*Part of the DoomLegacy arcade cabinet build. Read before touching `D_AdvanceDemo`/`D_DoAdvanceDemo`/`D_PageDrawer`, `G_Idle_Timeout_Check`, or `G_Arcade_Death_Check`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **A menu over the attract screen gets a black backdrop, and the cycle holds while it is open**
  (`d_main.c`, `D_AdvanceDemo` and `D_Display`).
  - Upstream refuses to advance the sequence while `menuactive` (*"do not start a demo when a menu
    or console is open"*). That was **briefly removed** so the cabinet would keep advertising
    itself behind an open menu, and it had to go straight back: **starting a demo tears the menu
    down.** `D_DoAdvanceDemo` → `G_DoPlayDemo` loads a level, and the player reading a page is
    thrown out of it — every time a page elapses, which on a cabinet is constant. A held backdrop
    is a far smaller cost than a menu that will not stay open.
  - So the backdrop is **painted black instead** (`D_Menu_Over_Attract`, filled just before
    `CON_Drawer`/`M_Drawer`). Without it the player reads the menu over a frozen title page, or
    over the last frame of a demo that has since ended — both of which look like the machine has
    locked up. Painted *over the top* rather than by suppressing the draws beneath, because the
    attract content comes from three different places (`D_PageDrawer`, the score table, and the
    `GS_LEVEL` view render) and covering all three at one point is much harder to get wrong.
  - **The test is `demo_ctrl & DEMO_seq_disabled`, not `demoplayback || gamestate ==
    GS_DEMOSCREEN`.** That flag is the engine's own "a real game is running" marker —
    `D_DisableDemo` sets it from `G_DeferedInitNew` and load-game, `D_StartTitle` clears it. It is
    what covers the awkward in-between state: when an attract demo *ends* while a menu is open its
    `D_AdvanceDemo` is discarded, so `demoplayback` is already false while `gamestate` is still
    `GS_LEVEL`, and the naive pair would have let the demo's last frame sit frozen behind the menu.
  - **A blocked advance is deferred, never dropped** (`demo_advance_deferred`,
    `D_Demo_Advance_Retry`, called every tic from `G_Ticker`). Dropping it was fine while the
    request came from `D_PageTicker` — the page stayed up, `pagetic` ran negative, and the tic
    after the menu closed advanced it. But a demo *ending* asks through `G_CheckDemoStatus`, and by
    then the engine has already left the demo: **gamestate is `GS_NULL`, not `GS_DEMOSCREEN`**.
    With the request thrown away nothing ever moved it on — no page, no demo, and `G_Ticker`'s
    switch has no case for `GS_NULL`, so `G_Idle_Timeout_Check` stopped being called and the open
    menu could never time out. The cabinet sat there until someone touched it.
    - The retry runs **before** the gamestate switch precisely so it works in `GS_NULL`.
    - `D_DisableDemo` clears the flag: a real game outranks a pending attract advance.
    - **`G_Ticker`'s `default` case now runs the idle check when a menu is open**, as a backstop.
      The deferral stops the cabinet reaching that state at all; this makes sure no future route
      can strand an open menu the same way. Verified: with the demo ended under an open menu at
      `GS_NULL`, the menu closed at exactly the configured timeout and the attract screen came
      back (`gs` 0 -> 4, `menuactive` 1 -> 0).
  - **The stall is hard, not a pause, and that is what makes the resume instant.** `D_PageTicker`
    decrements `pagetic` every tic regardless of what is on screen, so once it passes zero it calls
    `D_AdvanceDemo` on *every* tic and every one is discarded — `pagetic` runs deeply negative. The
    moment the menu closes, the very next tic advances the page.
  - **The idle timeout is what closes an abandoned menu**, so the cabinet returns to the attract
    cycle on its own. Verified end to end at `idletimeout 10`: `demosequence` sat at 0 with
    `pagetic` falling 170 → -180 for the whole 350 tics the menu was up, then at the timeout
    `menuactive` went 0, `demosequence` stepped to 1 with a fresh `pagetic` of 167, and the cycle
    carried on to 2 normally.
  - **`console_open` is deliberately kept** in the same test. Someone typing at the console is
    working on the machine, not watching it.
  - Note that "the Options screen doesn't go back to attract mode" was **this**, not the timeout.
    The idle timeout itself was always working: instrumented at the title with the Options page up
    it fired at exactly `cv_idletimeout * TICRATE` (700 at 20s, 2100 at the cabinet's 60s). Do not
    go looking for a second bug there.

- **A death ends the run, arcade style** (`g_game.c`, `G_Arcade_Death_Check`). Vanilla Doom retries
  by reloading the map, which on a scored cabinet is a free second attempt at a run
  `HS_Player_Died` has already voided — an unranked do-over with nothing to say why. The player now
  watches the corpse, signs the board if they placed, and the cabinet returns to the attract screen.
  - **`G_Player_Death_Settled()` is the "hit the ground" test**, and there is one of it because two
    things wait on it: the initials page (a prompt over a corpse still falling looks like it fired
    at the wrong moment) and the teardown. `P_DeathThink` lowers `viewheight` by one unit a tic to
    `6*FRACUNIT`, so from a standing 41 that is about a second. Measured: the page opens **35 tics**
    after the death, at `viewheight` 6.
  - Order after landing: the initials page gets its turn first (`HS_Initials_Pending()` /
    `M_Initials_Active()` hold the teardown off), then **`DEATH_LINGER_TICS`** (2s) on the ground.
    With no placement that is death → attract in about **3 seconds**; with one, it leaves as soon as
    the initials are in.
  - **The teardown is deferred through `gameaction = ga_worlddone`**, consumed by `G_DoWorldDone`,
    not called inline — the same route Single Level takes, and for the same reason: `G_Ticker`'s
    reborn loop is above the point where tearing the level down is safe. `death_ended_run`
    distinguishes it there from an ordinary level completion, which is what everything below that
    hook is about.
  - **`G_DoReborn` takes the same exit**, so the "use" press that used to retry leaves early
    instead of reloading — *unless* the initials page is pending or up, in which case it puts the
    player back to `PST_DEAD` and lets the death play out. Without that, a use press in the second
    before the page opens tore the game down and the player met the prompt back at the title, which
    is the disjointedness this was meant to remove.
  - **Skipped in devmode**, like the idle timeout: `-devmode` keeps stock behaviour, and an operator
    testing a level does not want to be thrown to the title on every death.
  - Single Level mode keeps its own route (`single_level_mode` → `M_SingleLevel_Finished`), which
    returns to its menu rather than the attract screen.

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
  - **The menu is closed before anything else happens, on every path.** `D_StartTitle` restarts
    the attract cycle through `D_AdvanceDemo`, and that is discarded while a menu is open — so
    ending a game underneath an open menu left `gamestate` at `GS_LEVEL` with no game running, no
    title, and the menu still up. Only the menu-only path re-arms `last_input_tic`, so the check
    then re-fired *every tic* for ever without ever getting out. **That is what "the menus do not
    time out" was**: not the timeout failing to fire, but firing repeatedly with nothing to show
    for it. Reproduced at `idletimeout 2` with a menu opened during a `-warp` game — `closed=0`,
    `gamestate` still `GS_LEVEL` after ten seconds of firing.
  - **The menu-only path kicks `D_AdvanceDemo()` when `gamestate != GS_DEMOSCREEN`.** When an
    attract demo *ends* behind an open menu its own `D_AdvanceDemo` is discarded, leaving
    `GS_LEVEL` with nothing playing and no page pending; `D_PageTicker` only runs in
    `GS_DEMOSCREEN`, so nothing else would ever restart the cycle. Harmless in the ordinary case,
    where `pagetic` is already deeply negative and the next tic would advance anyway.
  - **`GS_DEMOSCREEN` is not enough on its own, and neither is `demoplayback`.** While an attract
    demo plays behind the menu the gamestate is `GS_LEVEL`, so the `GS_LEVEL` call has to
    distinguish "menu over the attract screen" from "menu over a real game". It used to pass
    `demoplayback && menuactive`, which is only *most* of the attract case — when the demo ends
    behind the menu, `demoplayback` goes false while `gamestate` stays `GS_LEVEL`, and the timeout
    then treated the attract screen as a game to tear down. It passes **`D_Menu_Over_Attract()`**
    now (`d_main.c`), which asks the engine's own `DEMO_seq_disabled` "a real game is running"
    flag and covers both. A real game with the menu open has that flag set and still
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

  **Verified across every menu, not by spot check.** A temporary console harness
  (`tmpopen <n>` / `tmpstat <n>` over a table of every `menu_t` in `m_menu.c`) walks the list at
  `idletimeout 2`, opening each page, idling past the timeout and reporting whether it closed.
  All 37 testable menus close over the attract screen, and a representative in-game spread (Main,
  New Game, Single Level, Cheats, Options, Game Options, Start Game) closes *and* lands back at
  `GS_DEMOSCREEN`. Four menus cannot be pushed raw by such a harness and segfault it —
  `SetupMultiPlayerDef`, `ControlDef`, `LoadDef`, `SaveDef` — because they need state their real
  entry points set up first; that is a harness limitation, not a timeout failure, and they are
  reached in play through those entry points.

  It runs in **`GS_LEVEL`, `GS_INTERMISSION` and `GS_FINALE`**, not just during play — both of the
  other two wait *indefinitely* for a keypress the walk-away player never gives, so covering only
  `GS_LEVEL` left the cabinet hung. The intermission stalls at `sp_state == 10` (`wi_stuff.c`)
  once the counters finish; the finale stalls in `F_Ticker`'s `finalestage 0` (the Doom 2 text
  screens need `keypressed`) and again in the cast call, which loops forever. `D_Display` calls
  `HU_Drawer` for `GS_LEVEL` **only**, so the countdown would not have been visible in the other
  two states — `HU_Draw_Tip` was un-`static`ed (declared in `hu_stuff.h`) and is called directly
  after `WI_Drawer`/`F_Drawer`. **Anything drawn by `HU_Drawer` has this same limitation.**

- **The attract cycle plays at a reduced volume** (`cv_attractvolume`, "attractvolume", default
  **50** percent, `CV_SAVE`), under **Options → Arcade Options**. An arcade cabinet advertises itself
  with sound, but a machine living in a house cannot do that at playing volume all day. `0` is a
  silent attract screen, `100` is the stock behaviour. Sound and music are scaled together: the
  question being asked is "how loud is the cabinet when nobody is playing", and that is one
  question.
  - **It has to be applied in `S_UpdateSounds` (`s_sound.c`), and nowhere else.** That function is
    the single place the mixer volume is reconciled with the cvars, it runs every frame from
    `D_DoomLoop` whatever the gamestate, and it re-asserts `cv_soundvolume`/`cv_musicvolume` on any
    mismatch. So setting the mixer directly from anywhere else — at the start of a demo, say — is
    silently undone on the next frame. `S_Attract_Scaled()` changes the *target* that comparison
    comes from, which is why coming out of attract into a game restores full volume by itself,
    within a frame, with no second hook.
  - **A menu open over the attract screen counts as "in use", not as attract.** The moment somebody
    presses a key the cabinet is being used, even though the attract cycle is technically still what
    is on screen — any keypress over a demo raises the menu — and the menu's own sounds were being
    played at advertising volume, so the first thing a player heard after touching the machine came
    out quieter than the demo that drew them to it. `S_Attract_Scaled` therefore returns the
    unscaled volume when `D_Menu_Over_Attract()`, reusing the predicate the menu backdrop already
    defines rather than growing a second opinion about what counts as the attract screen.
    - Backing out of the menu without starting anything drops the volume again by itself, because
      this is reconciled every frame — there is no state to leave the cabinet loud all night, and
      that is worth checking rather than assuming. Verified across the whole round trip in one
      process with temporary instrumentation on the mixer change, at `attractvolume 25` with both
      volumes 20: attract **5**, keypress → **20** one tic later (`attract=1 menuover=1`), Escape →
      back to **5** one tic later. The pre-existing path still behaves too — `map map01` → **20**
      (`attract=0`), `exitgame` → **5** — which are the same numbers this file recorded before.
  - **`D_Attract_Running()`** (`d_main.c`) is the predicate: `!(demo_ctrl & DEMO_seq_disabled)`,
    the engine's own "a real game is running" marker. `D_Menu_Over_Attract` was rewritten in terms
    of it, so both readers share one definition. `(demoplayback || gamestate == GS_DEMOSCREEN)`
    would have been wrong here for the same reason it is wrong for the menu backdrop above.
  - **Not skipped in devmode**, unlike most cabinet behaviour: an operator changing this setting
    needs to hear what it does, and there is no lockdown reason to suppress it.
  - Appended to the end of `MenuOptionsMenu`, like every other operator row, because the lockdown
    addresses menu items by hardcoded index.
  - Verified headlessly with temporary instrumentation on the mixer-volume change, across the full
    cycle in one process: at `attractvolume 25` with both volumes 20, the attract screen ran at
    **5**, `map map01` restored **20**, and `exitgame` returned it to **5**. Separately,
    `attractvolume 0` gave a genuinely silent attract (**0**) that still restored to 20 in game, and
    `50` gave 10 as expected. `s_sound.c` is one of the **ISO-8859 files grep skips silently** — the
    first search for `S_SetSfxVolume` returned nothing at all, which reads as the function not
    existing. Use `grep -a`.

## GAME OVER

- **The cabinet names the end of a run.** A death is how nearly every cabinet session finishes, and
  the player was being returned to the attract screen with nothing said — the corpse simply stopped
  being theirs. `HU_Draw_GameOver` (`hu_stuff.c`) puts a **GAME OVER** card over the death view for
  the length of the dwell.
- **Drawn from the `M_GAMOVR` patch in `legacy.wad`** (125x17, added for this), centred on the
  patch's own width and placed by its own height — `(BASEVIDWIDTH - p->width)/2`,
  `(BASEVIDHEIGHT - ST_HEIGHT - p->height)/2` — so replacing the artwork with a different size
  needs no code change.
  - **It falls back to `V_DrawString("GAME OVER")` when the lump is missing**, so the build still
    works against a stock `legacy.wad`, and picked the artwork up with no code change when it
    arrived. Worth keeping: it is what let the timing be built and tested before the art existed.
  - The lump is looked up **once per level**, not per frame — `W_CheckNumForName` walks the whole
    directory and this is a drawer.
- **Timing is `G_Arcade_Death_Showing()` (`g_game.c`)**, not a timer of the drawer's own. True from
  the moment the body settles until the game is torn down, so the card and the corpse share one
  dwell: **`DEATH_LINGER_TICS`, raised from 2 to 3 seconds** so the card reads as deliberate rather
  than a flicker on the way out. One constant moves both.
  - **It is suppressed while the initials page is up or pending.** That page is its own moment and
    would otherwise have GAME OVER painted across it. The linger clock is already paused for the
    same reason, so the card simply reappears for its full dwell once the player has signed.
- **Single Level never shows it, and that is structural rather than a test result.**
  `death_settled_tic` is the flag the card keys off, and the only assignment that makes it non-zero
  sits *downstream* of `G_Arcade_Death_Check`'s early return on `single_level_mode` — so in Single
  Level the flag cannot be non-zero and the card cannot appear. The same guard covers `devmode`,
  demo playback and every multiplayer game. Nothing extra was added for any of them.
  - This is the same guard that already keeps a Single Level death from ending the run, so the two
    behaviours cannot drift apart.
- **It bled into the next game, for one frame.** Reported as a flash of GAME OVER at the start of a
  new Single Player session. `death_settled_tic` was left set by the run that ended, and the one
  place that clears it is guarded by `playerstate != PST_DEAD` — which is *still false* after the
  teardown, because nothing resets `playerstate` on the way back to the title. So the stale tic
  survived the attract screen and the card painted over the new game's first frame, until the new
  player's first tic cleared it. Textbook case of the CLAUDE.md rule that returning to the title
  resets very little.
  - Fixed twice over, deliberately. **`G_Reset_Arcade_Death()`** clears the flag from
    `Command_ExitGame_f`, the single funnel every route back to the title passes through — that is
    the direct fix. And **`G_Arcade_Death_Showing()` now ends in `G_Player_Death_Settled()`**, so
    there must be a body on the ground *right now*: that makes the bad state unreachable rather
    than merely reset, and the card cannot be drawn over a living player whatever the flag says.
  - Measured with a temporary print in the drawer, counting draws either side of the teardown:
    **1 draw after teardown before the fix, 0 after** — and the leaked frame logged
    `playerstate=0` (alive), which is exactly what the new test rejects. Total draws in a run:
    **105, exactly `DEATH_LINGER_TICS`**, so the dwell is the full 3 seconds and not a frame more.
- Verified headlessly under `SDL_VIDEODRIVER=offscreen`: `kill` in a `-warp 1` game, screenshot
  during the dwell. The patch draws centred (measured: spans 310..710 of 1024, centre 510 against a
  screen centre of 512).

---

## The chase camera on record demos

- **Every third record demo in the attract rotation is shown from the chase camera**, captioned
  with a blinking `CHASE CAM`. A record demo is somebody's actual run, and from behind it reads as
  a *person playing* rather than as a first person view a passer-by can mistake for the attract
  screen having frozen.
  - **Only record demos, never the stock IWAD demos** — the chase camera is for the record holders,
    and `demo1`/`demo2`/`demo3` are nobody's record. The decision is taken in `D_DoAdvanceDemo`
    (`attract_demo`) **before** the stock fallback overwrites `demo_name`, since by then the two
    are indistinguishable.
  - **Every third, not every one.** One record demo plays per attract cycle, so `1` would make the
    chase camera the normal way the cabinet looks, and the front view is what an onlooker
    recognises as Doom. `ATTRACT_CHASECAM_EVERY` (`d_main.c`) is the one constant to change.
  - The counter **counts up and resets rather than taking a modulo**: it is a byte, and `n % 3`
    would put two chase cam demos back to back every time it wrapped at 255.
- **`cv_chasecamdemo`** ("chasecamdemo", default **On**, `CV_SAVE`), under **Options → Arcade
  Options** as "Chase Cam Demo". Operator setting like the rest of that page. It costs nothing on a
  cabinet with no records — there is then no record demo to apply it to.
- **The engine's own `cv_chasecam` is what gets switched**, and it is switched back in two places,
  because a camera left following the player into a real game would be a serious bug:
  - `D_Clear_Attract_ChaseCam()` at the top of `D_DoAdvanceDemo`, so the next attract page is
    normal again.
  - the same call at the top of **`D_DisableDemo`**, which is the funnel every real game start
    passes through (`SV_SpawnServer`). Verified: with the camera engaged on a record demo, a `map`
    command cleared it back to `cv_chasecam=0`.
  - Both are guarded on the module's own `attract_chasecam` flag, so they can only undo a change
    *this code* made — `cv_chasecam` is an ordinary cvar a `-devmode` session may have set by hand
    at the title screen, and clearing that unasked would look like the console command not working.
- **It does not desync the record demos, and this was measured rather than reasoned.** The camera
  is a view, not part of the simulation: `MT_CHASECAM` is
  `MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY|MF_FLOAT` (`info.c`), so it is in neither the blockmap
  nor any sector list and nothing in the playsim can see it. Proven by playing the same record demo
  with `chasecam 0` and `chasecam 1` and fingerprinting the player every 35 tics — position,
  height, angle and health were **identical at all 43 samples across 1925 tics**. Worth keeping in
  mind before putting anything *else* in front of a record demo.
- The caption is drawn in `HU_Drawer` (`hu_stuff.c`) inside the existing `demoplayback` block,
  directly under the demo label: that sits at y 8 and `hu_font` glyphs are 7 tall, so **y 18**
  clears it and stays well above the status bar. Centred, so its width needs no measuring. It
  blinks on `gametic & 16`, the same cadence as `PRESS FIRE TO START` and the intermission's
  `NEW RECORD`, so the attract screen has one heartbeat rather than three.

### The chase camera gets stuck, and how it is unstuck

**`MT_CHASECAM` collides with the world.** It is `MF_NOBLOCKMAP|MF_NOSECTOR`, so nothing can collide
with *it* — that is what keeps it out of the playsim — but it is moved by momentum through the
ordinary mobj thinker, so `P_TryMove` stops it against walls like anything else. In doorways and
around corners it wedges, and since `P_MoveChaseCamera` only nudges it `cv_cam_speed` (0.25) of the
way toward the ideal spot each tic, it stays wedged while the player walks off — leaving the attract
screen pointed at an empty room while the run continues somewhere else.

The half-finished intent is still visible in the source: `P_MoveChaseCamera` carries a commented-out
`P_PathTraverse(...PT_ADDLINES, PTR_UseTraverse)` and a disabled `PTR_FindCameraPoint`, i.e. the
original authors meant to *raycast* the camera into place and never finished it.

**Measured before fixing anything**, on the `doom2_ep1_sk3_speed` record demo, logging the
camera-to-player distance every tic:

| | before | after |
| --- | --- | --- |
| median distance | 185 | **129** (the hover distance) |
| p95 | 752 | **200** |
| max | 832 | **277** |
| share of run beyond 288 | 35.8% | **0.0%** |
| worst stuck episode | **823 tics — 23 seconds** | 68 tics, and within normal lag |

The 35.8% is not constant trouble; it is five excursions, one of them catastrophic. That mattered
for the design: an early worry that a distance trigger would fire constantly was **wrong**, and only
looking at the time series rather than the percentile table showed it. In the fixed build the
trigger fires **6 times in ~136 seconds**.

**The fix is a distance trigger, and the recovery is `P_ResetCamera`.** That routine puts the camera
on the player's *own position*, which is always valid — the player is standing in it — so unlike
snapping to the ideal spot behind the player it can never drop the camera inside a wall. The follow
code immediately eases it back out, and it is the same movement a teleport already produces, so the
visual is one the cabinet has always shown.

**The threshold has to clear the legitimate follow lag, which is not small.** The camera closes only
`cv_cam_speed` of the gap per tic, so a player moving *v* units per tic settles about *v*/0.25 = 4*v*
units behind the ideal spot. The demo peaked at 20.3 units/tic — ~81 units of lag on top of the 128
hover distance, ~209 in all — and `P_AproxDistance` overestimates a true distance by up to 12%, so
that reads as ~234. **`CAM_SNAP_SLACK` is 160**, giving 288 at the default `cv_cam_dist` of 128:
clear of legitimate lag, far below every stuck excursion measured. Deriving it from `cv_cam_dist`
means it tracks an operator who changes the hover distance.

- **No demo desync**, re-verified after the change with the same fingerprint method — the record
  demos are what the chase camera is mostly used on, so this has to hold. See the desync note above.
- **Still not a true fix.** The camera is unstuck rather than prevented from sticking; a raycast
  placement (pull the camera in to the first wall between it and the player) is the fuller answer and
  is what the dead code was reaching for. The trigger is cheap, safe and reuses tested code, which
  the raycast would not.

### The chase camera crashed the cabinet, hours in

Symptom: the cabinet, left running unattended, would occasionally segfault once the chase camera
demos were in the rotation. Never on a short test.

**Found from core dumps, not from new logging.** Fedora's `systemd-coredump` had already kept every
crash — `coredumpctl list` showed three of the cabinet binary, and all three had the identical stack:

```
#0  R_SetupFrame        #1  HWR_RenderPlayerView        #2  D_Display        #3  D_DoomLoop
```

faulting on `mov (%rax),%rax` with `rax` holding garbage (`0x36fa318c3f8ff790`) — not NULL, so a
**stale** pointer rather than a missing one. The instruction loaded a pointer at offset `0xb8`, then
read two ints at `0x84`/`0x88`, which matches `r_main.c`'s
`viewer_sector = viewmobj->subsector->sector;` followed by `viewer_sector->modelsec` and `->model`.

**The bug was in `P_SetupLevel` (`p_setup.c`), and it is pre-existing engine code:**

```c
    if (camera.chase)        // <-- the guard
        camera.mo = NULL;
```

The camera mobj is freed with every other thinker a few lines below, so the pointer must be dropped
*whatever* the camera's state — but the guard only dropped it while a chase was active. The fatal
order is:

1. chase camera on — a camera mobj is spawned
2. chase camera off — `R_SetupFrame` sets `camera.chase = NULL`
3. **a level load** — the guard is now false, so the mobj is freed and `camera.mo` still points at it
4. chase camera on again — `P_ResetCamera` takes its "already have one" branch and writes *through
   the dangling pointer*, keeping it; `R_SetupFrame` then reads a `subsector` belonging to a level
   that no longer exists

**Nothing performed that sequence until the attract cycle started showing every third record demo in
chase view**, which does exactly on/off/level-load/on, over and over, all day.

**Why it was occasional:** it is a use-after-free, so it only faults once something else reuses that
memory. Freed zone memory usually still holds plausible values for a while, which is why a
deliberate reproduction of the sequence ran to completion without crashing. Proving it therefore
meant instrumenting the condition rather than waiting for a fault:

```
CAMLEAK level load: chase=(nil) mo=0x7fa4f53b4d20 -> mo=0x7fa4f53b4d20 *** DANGLING ***
CAMLEAK P_ResetCamera reusing mo=0x7fa4f53b4d20 (subsector=0x7fa4f51c5e28)
```

and after making the clear unconditional, the same script gives `-> mo=(nil) cleared` and the
`P_ResetCamera reusing` line **disappears entirely** — a fresh camera is spawned instead.

- **A crash that will not reproduce on demand is not thereby unproven.** Instrument the *condition*
  (here: a pointer surviving the free of what it points to) and show it before and after. Waiting
  for the fault would have proved nothing either way.
- **Guarded cleanup is where dangling pointers come from.** `if (still_in_use) drop_the_pointer()`
  is backwards: the pointer must be dropped because the *target* is going away, which has nothing to
  do with whether the subsystem is currently active.
- The cabinet binary is built **without `-g`**, so these backtraces have function names but no line
  numbers. Adding `-g` to `ENV_CFLAGS` in `make_options` costs nothing at runtime and would make the
  next one far quicker to read.
