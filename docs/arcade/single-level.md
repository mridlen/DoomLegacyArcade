# Single Level mode

*Part of the DoomLegacy arcade cabinet build. Read before touching `SingleLevelMenu`/`M_SingleLevel_*`, `single_level_mode`, or the `-sl` game id.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

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
  - **No "Entering &lt;next map&gt;" page.** `WI_update_Stats` (and the netgame path beside it) send
    the intermission to `WI_Init_ShowNextLoc` unless the game is `doom2_commercial` or the map ends
    the episode; a Single Level run is neither, so it announced the level the player had explicitly
    chosen not to play on to. `single_level_mode` joins that condition and goes straight to
    `WI_Init_NoState`, exactly as the other two cases do. Only the ExMy games ever showed it —
    Doom II already took the `doom2_commercial` branch — which is why it reads as a Doom 1 bug.
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
  - The bag is refilled when exhausted, when `hs_table_count` changes, **and whenever a demo file
    is written** (`hs_demo_gen`, bumped at both `G_SnapshotDemo` sites), so a run recorded during
    the session joins the rotation without waiting.
    - `hs_table_count` alone was enough while the bag held only per-map records: a new map scoring
      adds a row and changes the count. Once Survival demos joined the same pool that stopped
      being true — a Survival record lives on the board, changes no table row, and so could not
      trigger a refill at all. The demo then waited for the bag to run dry, which on a cabinet
      with forty demos is half an hour of attract screen. It presented as the new demo simply
      never coming up.
    - A new record on a row that already exists does not change the count either, so the
      generation counter is what makes *that* case immediate too — it used to be "at most one pass
      of the cycle".
    - A refill mid-pass reshuffles, so the "each demo once before any repeats" guarantee restarts
      from there. That is the same thing a `hs_table_count` change always did, it is rare, and
      `hs_bag_last` still blocks an immediate repeat.
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
