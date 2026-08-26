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
