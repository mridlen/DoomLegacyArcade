# High scores, record demos and the run board

*Part of the DoomLegacy arcade cabinet build. Read before changing `hs_stuff.c`/`hs_stuff.h`, `HS_*` call sites, the intermission tables in `wi_stuff.c`, or anything that starts, ends or scores a run.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **High scores and record demos** (`hs_stuff.c`/`.h`, new). Tracks best cumulative time-to-exit per
  **(wad combination, map, skill, category)** for single player, shown on the intermission screen
  and as a page in the attract cycle.

  There are two **categories** (`HS_NUMCAT`, `HS_CAT_speed`/`HS_CAT_max`), timed identically and
  displayed as side-by-side time columns. A **max** run additionally requires 100% kills and 100%
  secrets on *every* level so far — items are deliberately not required. `WI_Init_Stats` computes
  that per level from `wb_plyr[me].skills`/`.ssecret` against `wbs->maxkills`/`->maxsecret` (a map
  with none of a category counts as satisfied, matching how the percentage display treats it) and
  passes it to `HS_LevelExit`. The run-level flag `hs_run_is_max` starts true in `HS_NewGame` and
  latches false on the first level exited short; from then on only the speed record can be beaten
  for the rest of that run. Each category keeps its **own** record demo.

  **The two categories end at different points, so each keeps its own frozen endpoint.** The speed
  run ends at the last scored level exit (`hs_run_endmap`/`hs_run_tics`); the max run ends at the
  last exit that was *still* 100%, which may be several levels earlier — `hs_max_endmap` /
  `hs_max_tics`, extended in `HS_LevelExit` only while `hs_run_endmap_max` holds, and used by
  `HS_Run_Finished` to commit the max entry.
  - Both used to be committed from the speed endpoint, gated on `hs_run_endmap_max`. So finishing
    a level short of max **erased the max progress already banked**: max MAP01, then finish MAP02
    without max, and the max board got *nothing at all*.
  - **The giveaway is that playing better scored worse.** A death leaves the frozen state
    untouched, so *dying* on MAP02 committed a max entry for MAP01 — while *surviving* MAP02
    committed none. The two paths differed only in whether the level exited, which is what made
    it look intermittent from the outside.
  - Two symptoms point here before the board does, and both are now consistent with it: the MAP01
    exit latches `hs_new_record[max]` so the intermission blinks **NEW RECORD**, and
    `HS_Snapshot_If_Leading` writes the `..._max.lmp` demo. Both promised a record that never
    landed. A max demo on disk with no board entry behind it is this bug.
  - An empty `hs_max_endmap` means the run was never max (the first level was already short), and
    no max entry is committed — which is the only case that should record nothing.
  - `HS_Run_Leads` needs no change and deliberately keeps its `hs_run_endmap_max` guard: it is
    only consulted while the max run is still alive, and at that point the two endpoints are
    identical. That guard is also what stops a *later* non-max exit re-snapshotting the max demo
    with a buffer that has run past the record.
  - Verified headlessly by calling the scoring entry points directly (a multi-level run cannot be
    played headlessly — the intermission will not advance), one scenario per skill so they do not
    compete on the same board: `1`→MAP01/MAP01, `10`→speed MAP02 **and max MAP01**, `110`→speed
    MAP03 and max MAP02, `0`→speed only, `11`→both MAP02. The pre-fix control wrote **no max line
    at all** for `10` and `110`, which is the reported bug and the proof the test can see it.

  **A death voids the rest of the run** (`HS_Player_Died`, called from `P_KillMobj` in `p_inter.c`
  right after `playerstate = PST_DEAD`). Levels already finished keep the records they earned —
  each was written to disk by its own `HS_LevelExit` before the fatal level was entered — but
  nothing after the death scores, and the background recording is closed with `G_CheckDemoStatus`
  since it can no longer produce anything. The HUD marker becomes **`PLAYER DIED - UNRANKED`**
  (`hu_stuff.c`); `UNRANKED` alone reads as a settings problem, and the player needs to know the
  retry they are about to play is not being scored.
  - Implemented by clearing `hs_run_ranked`, exactly as an altered ruleset does, so the "no further
    scores" half falls out of the existing early return in `HS_LevelExit`. The separate
    `hs_run_died` flag only selects the *reason* shown and logged — it is not a second gate.
  - Hooked at the moment of death rather than at `G_DoReborn`, so a player who dies and walks away
    still voids the run and releases the demo buffer.
  - **The demo is written when the death *lands*, not when the blow does.**
    `HS_Player_Died` runs from `P_KillMobj`, inside the tic the killing blow arrives on, so writing
    the snapshot there ends the recording on that tic: no death animation, no corpse, the replay
    simply cuts out mid-fight. It reads as a demo that stopped rather than a run that ended in a
    death. So `HS_Player_Died` only *decides* which demos the run earned (`hs_death_demo_pending`,
    plus the paths) and leaves the recorder running; `HS_Death_Demo_Finish` writes them and closes
    it, called from `G_Arcade_Death_Check` the moment `G_Player_Death_Settled()` is true.
    - **Deciding at the death is what makes it possible at all**: `HS_Run_Finished` runs there too
      and inserts the run into the board, after which `HS_Run_Leads` is false — it is no longer
      *beating* the entry, it *is* the entry — so nothing could be chosen afterwards.
    - `Command_ExitGame_f` calls it as a backstop, so a route that never reaches the settle (the
      player quitting during the death) cannot leave the recorder open.
    - **devmode keeps the old behaviour** — snapshot at the blow and close — because
      `G_Arcade_Death_Check` is skipped there, so nothing would ever finish the recording.
  - **It takes one last snapshot first, so the demo contains the death.** The snapshots taken at
    level exits each stop at that exit, so a Survival demo used to end at the last level completed
    and the run simply appeared to stop. Under Survival the death *is* the end of the run and the
    most interesting part of it. This costs the scoring nothing — the tables are written from the
    run state, and a snapshot is a copy of the recording buffer.
    - It must come **before** `HS_Run_Finished()`, which inserts this run into the board: after
      that `HS_Run_Leads` is false (it is no longer beating the entry, it *is* the entry) and
      nothing would be saved.
  - **Guarded on `demoplayback`** like `HS_LevelExit` is. Attract-mode record demos can contain a
    death, and replaying one must not void the cabinet's live run or close its recorder.
  - This closed a real hole rather than just adding a rule. Doom traditionally lets you retry a
    level, and the engine does that by *reloading* it — `G_DoReborn` → `G_DoLoadLevel(true)` →
    `P_SetupLevel`, which sets `leveltime = 0`. Since `HS_LevelExit` accumulates `leveltime`, the
    failed attempt used to cost the score **nothing at all**, making death a free reset button.
    Scores set before this change may include death-assisted runs; the cabinet's table was left in
    place deliberately (judged clean), so `clearhighscores` was **not** run.

  Both tables are laid out by hand against the surrounding graphics, so **the numbers matter**.
  `HS_Draw_IntermissionTable(x, y)` draws its column header 14 *above* the `y` it is passed, and
  right-justifies each category's times at `x + HS_COL_TIME + cat*HS_COL_STEP` (**108 and 72**), so
  the last column's right edge is `x + 180` and must stay inside `BASEVIDWIDTH` (320). On the single
  player intermission the free band is only what is left between the Secrets row and the Time row:
  rows start at `SP_STATSY` (50) and are `lh` apart, where `lh` is 1.5× the `WINUM0` patch height
  (12) = **18**, and the percent patches are 12 tall — so Secrets ends at 98 and `SP_TIMEY` is 168.
  The call site's `+ 12` centers header-plus-five-rows in that 70px gap. **If a row is ever added
  or the font changes, re-check both ends** — the drawer only bails at `BASEVIDHEIGHT`, so it will
  happily paint over the Time/Par row.

  A blinking **`NEW RECORD`** is drawn by the same function when the level just finished beat a
  time. `hs_new_record[HS_NUMCAT]` records which categories were beaten; it is cleared at the top
  of every `HS_LevelExit` — *before* the unranked/died early returns, so a voided exit cannot leave
  the previous level's marker on screen — and set alongside each `saved = true`. One marker covers
  both categories.
  - **It went in the horizontal space, not the vertical**, because the band above is already full:
    header-plus-rows spans `y-14 .. y+48` (102..164 at the call site's y of 116) inside a free band
    of only 98..168. The table itself occupies just `x .. x+180` (**138..318**), so everything left
    of `x` in that band is free. The text is centred there and on the table block's midpoint
    (`(y-14 + y+48)/2 = y+17`, glyphs 8 tall, so top edge `y+13`) — landing at x=30, y=129.
  - Width comes from `V_StringWidth` at runtime, not the measured 77px, so the centering follows
    the string if it is ever reworded. The 77px was read from the real `STCFN0xx` lumps, per the
    rule above about not eyeballing this font.
  - Blink is `gametic & 16`, matching the cadence `wi_stuff.c` uses for its own flashing "you are
    here" pointer. `gametic` (not `leveltime`) because only `gametic` advances during the
    intermission; `bcnt`, which `wi_stuff.c` blinks from, is static to that file.
  - Drawn with option **`0`, which is red** — it was white at first and disappeared against the
    grey intermission background. See the `V_DrawString` colour note below.

  **A blinking `MAX` sits beside the Kills and Secrets percentages** (`wi_stuff.c`,
  `WI_Draw_Stats`) when the level just finished satisfied the max category — 100% kills and 100%
  secrets. It is drawn from **`sp_maxed`**, which `WI_Init_Stats` sets from the very same
  expression it hands to `HS_LevelExit`, so the indicator and the scoring cannot disagree about
  what "max" means.
  - **Items deliberately get no indicator.** They are not part of the category, and the gap in the
    column is what says so.
  - Measured: the percentages are right-justified so their `WIPCNT` `%` patch (13 wide) sits at
    `BASEVIDWIDTH - SP_STATSX` = 270 and ends at 283. `MAX` is 26px against the real `STCFN`
    lumps, so `SP_MAXIND_X` 287 spans 287..313 of 320 — 4px clear of the `%`, 7px of right margin.
    The percent patches are 12 tall and `hu_font` glyphs 7, so `+2` centres the text on the row.
  - Blink is `gametic & 16`, the same cadence as `NEW RECORD` and as `wi_stuff.c`'s own "you are
    here" pointer. Option 0 is the font's native red, which reads on this screen's grey where
    `V_WHITEMAP` would not.

  **Cumulative run time** is shown under the intermission's Time row by `HS_Draw_TotalTime`, as
  `TOTAL` plus `hs_cumulative_time`. It already includes the level just finished — `WI_Init_Stats`
  calls `HS_LevelExit`, which adds that level's `leveltime`, before the intermission draws. Shown
  whatever the run's state: an unranked or death-voided run still has a meaningful elapsed time.
  - Layout, all measured: `WITIME` and the `WINUM` digits are both **12** tall and start at
    `SP_TIMEY` (168), so that row ends at 180; the call site's `+ 16` leaves a 4px gap, putting the
    text at 184 and — `hu_font` glyphs being 7 tall — its bottom at 191 of `BASEVIDHEIGHT` 200.
    The caption sits at `SP_TIMEX` (16) spanning 16..56, and the value is right-justified at
    `BASEVIDWIDTH/2 - SP_TIMEX` (144), at most 39px wide for a `123:45`, so it starts no further
    left than 105. Only the Par row shares that band and it is right of centre.
  - Drawn in **option 0 (red)**, not `V_WHITEMAP` — this screen's background is largely grey and
    white text vanishes into it, the same lesson as the `NEW RECORD` marker.
  - `hs_cumulative_time` is reset by `HS_NewGame`, which only the menu skill-select handlers call.
    A game started from the command line (`-warp`) never resets it, so the total can carry over
    from a previous run in that case. Not reachable from the cabinet's menus, but worth knowing
    when testing.

  The **wad combination** is `HS_GameId()`: the game's short name plus any loaded level pack, such
  as `doom2` or `doomu+mapsofchaos`. Both parts are needed — Doom 2, Plutonia and TNT all have a
  `MAP01` that is a different level, and a pack replaces the maps again. Record demos are named
  with the same key, and only the current combination's records and demos are shown or replayed
  (a demo from another combination would desync instantly). The key is recomputed per use, since a
  pack can be loaded mid-session, and sanitized to one filename-safe word because it is a space
  separated field in `highscores.dat` *and* part of the demo filename, while pack names come from
  arbitrary filenames. Renaming a wad therefore starts a fresh table — the name is the identity.

  Record demos are **captioned** during attract playback (`HS_DemoLabel()`, drawn by `HU_Drawer`)
  with the **map range**, skill, category and time — `E1M1-E1M5  ITYTD  MAX  4:32.17`.
  `HS_NextRecordDemoPath()` fills the label as a side effect of handing out a path, and
  `D_DoAdvanceDemo` clears it before every page, so a stock IWAD demo is never captioned with the
  previous record's text. Drawn on the **second** text line (y=8), because item pickups still print
  messages at y=0 during playback.
  - **The range is the point.** These times are cumulative from the start of a run, so the demo
    saved against the E1M5 record is a *five level* run. Captioned with the bare map name it was
    indistinguishable from a single level one, which badly undersold the longer runs. The range
    comes from `hs_maprecord_t.startmap[cat][skill]`, stored per record rather than inferred from
    the episode: every menu-started campaign does begin at map 1, but that is a property of the
    menus, not of the record.
  - A record whose `startmap` equals its map is captioned with the bare map name — no invented span.
  - **A campaign record with no stored `startmap` falls back to an inference** (`HS_Infer_StartMap`,
    used through `HS_Format_Range`): the first map of that map's own episode, so `MAP03` reads
    `MAP01-MAP03` and `E2M3` reads `E2M1-E2M3`. Episode-aware, not always `E1M1`.
    - This was deliberately *avoided* when the field was added, on the grounds that starting at map
      1 is a property of the menus rather than of the record. That was over-cautious for records
      written before the field existed: for those the choice is not between a fact and a guess but
      between a good guess and nothing, and a cumulative time at MAP03 can only have come from a
      run that began at MAP01, since that is the only way to accumulate time there.
    - **A stored start map always wins**, so this never overrides a recorded fact and quietly stops
      mattering as old records are beaten.
    - Single level records are excluded — one map by definition — so they stay bare.
    - Secret levels need no special case: reaching MAP31 or E1M9 still means a run from MAP01 or
      E1M1.
    - The one case it cannot describe is a run started mid-episode with `-warp`, which no route
      through the cabinet's menus produces.
  - **It still fits on one line.** Measured against the real `STCFN` lumps, the widest either form
    reaches is `SINGLE LEVEL: MAP01  ITYTD  SPEED  888:88.99` at **295px of 320** — a single level
    run is one map so it never carries a range, and a range costs less width than the
    `SINGLE LEVEL: ` prefix does.

  A blinking **`PRESS FIRE TO START`** is drawn over the same demos (`hu_stuff.c`, `HU_Drawer`) —
  the arcade "insert coin" on a machine that takes no coins. It is only a prompt: **`G_Responder`
  already pops up the menu on any keypress while a demo plays** (`g_game.c`, the "any other key
  pops up menu if in demos" branch), so fire has always started the way into a game and nothing on
  the attract screen said so.
  - Shown on `demoplayback && ! menuactive`, so it is not painted behind an open menu. That also
    means it appears over a Single Level **"watch run"** replay, which is harmless — fire ends the
    replay and opens the menu there too.
  - Blink is `gametic & 16`, the same cadence as the intermission's `NEW RECORD` marker.
  - **Placed clear of the status bar in either of its forms**, since a demo is drawn at whatever
    viewsize the cabinet is set to: the classic bar's top is `BASEVIDHEIGHT - ST_HEIGHT` = 168 and
    the overlay's lower row is at 182 (`st_stuff.c`'s `SCY(198) - 16`), so the text goes at
    **160**, spanning 160..167 for `hu_font`'s 7-tall glyphs. Width measured from the real `STCFN`
    lumps: **133px of 320**, centred by `V_StringWidth` so it follows any rewording.
  - It does cross the **weapon sprite**, which draws up the middle of the screen in every mode.
    Unavoidable for anything centred that low — the same constraint that pushed the HUD clock off
    centre — and the blink is what keeps it readable against the moving demo.
  - Only the demos carry it. The title and `CREDIT`/`CREDIT2` pages are `GS_DEMOSCREEN`, which
    `D_Display` does not call `HU_Drawer` for; they draw through `D_PageDrawer` and would need
    their own call.

  **The title track is a one-shot** — `S_ChangeMusic(..., false)` in the `attract_title` case,
  not `S_StartMusic`, which is `S_ChangeMusic(..., true)` and loops. Ultimate Doom's `D_INTRO` is
  short enough that the loop restarted it before the page was over, so the same sting played twice
  every time the title came round; Doom II's is long enough to hide it but is the same one-shot
  piece, and Final Doom is `doom2_commercial` and takes the Doom II track with it. The silence
  afterwards is deliberate — that is what a one-shot means, and it beats the repeat.
  - It still plays on **every** appearance of the title, despite `S_ChangeMusic`'s
    `mus_playing == music` early-out: the demo in between changes `mus_playing`, so the next call
    is not swallowed.

  **The attract cycle is a fixed four steps** (`d_main.c`, the `attract_*` enum): title → `CREDIT`
  → `CREDIT2` → one demo, repeating. It replaces the stock 6-step sequence (7 under the retail
  divisor), which showed `CREDIT` only once per three demos and filled the rest with the help and
  order pages. `demosequence` is read outside `D_DoAdvanceDemo` — `D_PageDrawer`'s Heretic raw-screen
  hack tests it — which is why the steps are named rather than numbered.
  - `CREDIT2` (in `legacy.wad`, 320x200 like the stock `CREDIT`) runs for `CREDIT2_SECS`. Its step
    is **skipped when the lump is absent**, so a `legacy.wad` without it still cycles cleanly.
  - The stock demo used when no record demo exists now **rotates** `demo1`/`demo2`/`demo3`, since
    the single demo step would otherwise always play `demo1`. `demo4` (retail only) is dropped.
  - The high score page is still interposed after each demo, on its own flag.
  - **Keep splash art at 320x200.** `D_PageDrawer` ends at `V_DrawScaledPatch_Name`, which
    multiplies by `vid.dupx/dupy` — a 960x720 patch would be drawn at 3840x2160 and only its
    top-left corner would be on screen.

  The attract page appears **after every demo**, skipped when the current combination has no times
  (`HS_Have_Records()`) so a fresh cabinet does not show an empty page repeatedly. It is interposed
  in `D_DoAdvanceDemo` via a flag rather than added as a `demosequence` case, because those cases
  are shared between game modes and the last is reachable only under the retail divisor.

  **Single Player scoring is Survival** (`HS_Survival_Entry`, `HS_Episode_Of`). A campaign run is
  scored on **how far it got in the episode, tie-broken by time** — not on per-map cumulative
  times, which was the confusing part of the older scheme and is gone entirely.

  - The key is **(game, episode, skill, category)** and the depth is **1**. `HS_Episode_Of` is what
    makes it an episode: the board used to be keyed by game alone, which let `HS_MapOrder` compare
    *across* episodes. On the cabinet's own table two one-minute E2M1 attempts (order 201)
    outranked two **completed** Episode 1 runs (E1M8, order 108). "Furthest" only means anything
    within one episode. Doom 2 and the other flat `MAPxx` games are one episode — the whole game
    is the run.
  - **No per-map campaign record is kept any more.** `HS_LevelExit` freezes the run's state and
    nothing else; the campaign half of `highscores.dat` is dead weight from older versions and is
    simply never written or read. Single Level times are untouched, including a campaign first
    level still competing there.
  - **The demo is snapshotted at each scored level exit while the run leads its board**
    (`HS_Snapshot_If_Leading`), not at the end of the run. It has to be: a **death closes the
    recorder** (`HS_Player_Died`), and under Survival a run that died still scores — on how far it
    got. Snapshotting as it goes means the file always holds the leading run up to its last scored
    exit. One demo per (game, episode, skill, category), named `..._ep<N>_sk<M>_<cat>.lmp`.
  - The intermission shows the **episode record and where this run stands against it**, replacing
    the old five-row per-map table. `NEW RECORD` can now blink *during* a run rather than only at
    the end, because under Survival being ahead is knowable: get past the holder's furthest map and
    you already lead.
    - **Both categories get a row, and the new record's *time* blinks.** The block is
      `SPEED` / `MAX` / `YOU`, one row each at `HS_IM_ROW` (12) pitch. It used to show the speed
      record alone, which said nothing to a player going for 100% — the two are independent
      records with independent holders. `hs_new_record[cat]` already tracked which category a run
      took, so the time on that row is simply left out on the blink-off frames; both blink when a
      run takes both. `NEW RECORD` stays, centred in the free space to the left and blinking in
      step, so the two read as one announcement — it says *that* a record fell, the blinking time
      says *which*.
      - `hs_new_record[]` is latched at the level exit, **before** the board is updated, so on a
        first-ever record the row still reads `NONE YET` and there is no old time to blink. The
        marker fires regardless, which is the right way round.
      - `HS_Intermission_Record()` picks the source: the map's own three deep board in
        `single_level_mode`, the episode's Survival board otherwise. `HS_Board_Entry` does not
        fill a map name — a single level run is one map by definition — so it is set from the exit.
    - **Every column of this block is measured at its widest glyphs, and twice now it was not.**
      `HS_Draw_IntermissionTable` lays out four columns left to right with an 8px gap between each:

      | column | widest | width | offset from `x` |
      | --- | --- | --- | --- |
      | label | `RECORD` | 48 | 0 |
      | map | `MAP01`/`MAP31` | 38 | `HS_IM_MAP_X` **56** |
      | time | `888:88.99` | 64 | right-justified at `HS_IM_TIME_R` **166** |
      | initials | `MMM`/`WWW` | 27 | `HS_IM_INI_X` **174** |

      201px in all, so the `wi_stuff.c` call site x must be ≤ 119; it passes **116**, leaving 3px
      at the right edge. `NEW RECORD` centres in the space to the left and fits at 19..96.
      - The **initials** were first measured with `AAA` — 8px a glyph, 24 in all, ending exactly on
        `BASEVIDWIDTH`. `M` and `W` are 9px, so real initials like `MLR` (25) or `MMM` (27) ran to
        323 and the last letter was cut off.
      - The **map and time columns** were then compared wrongly: the note claimed the widest time
        "starts no earlier than +86, clearing the map by 24", which compares the time's start
        against the map column's *start* (+64) instead of its *end* (+102). At the old constants
        they overlapped by up to 16px. It stayed hidden while times were short and map names
        narrow, and surfaced on **Doom II**, where the map name is 38px (`MAP01`) against Doom 1's
        27 (`E1M1`) and a run past ten minutes widens the time too: `MAP01` ended at +102 and a
        `10:20.55` began at +99, printing as **`MAP010:20`**.
      - **Measure a variable-width field at its widest glyphs, and check each gap end-to-start**,
        not start-to-start — the same rule as the rest of the `hu_font` layout work. The arithmetic
        is easy to get wrong by hand; derive it in a script against the real `STCFN` lumps.

  **The pages are enumerated, not numbered** (`HS_Build_Pages`, `hs_page_t`). Three kinds:

  | kind | one page per | content |
  | --- | --- | --- |
  | `HSPG_survival` | episode | the best run per skill and category, both categories side by side |
  | `HSPG_single` | (skill, category) | single level best times, whole map list, with initials |
  | `HSPG_slmap` | one slot | one map's single level top three, by difficulty |

  A page is only enumerated when it would have something on it. The list is rebuilt on every query
  because the tables grow during a session.

  - **Survival page layout**, measured against the real STCFN lumps. The skill label is written
    once and the two category blocks sit beside it: `"ITYTD"` 36px at 8, then blocks at 52 and 186,
    each `"E1M8 12:34.56 MLR"` at 122px — 308 of 320, and 316 with a three-digit-minute time. A
    per-episode board is what buys this; the old per-map page could not fit two categories at all.
    Rows are skills, so Doom 1 gets up to four pages and Doom 2 exactly one for the whole game,
    with no adaptive special-casing.
  - **An appearance shows `HS_PAGES_PER_CYCLE` (4) pages**, and the cursor is deliberately not
    reset between appearances. Even, so the single level speed/max pairs sit whole inside an
    appearance — but **even is not sufficient on its own**: a skill with a speed record and no max
    contributes a single page and shifts the parity of every pair after it. So
    `HS_Attract_Cycle_Pages` checks the boundary directly (`HS_Is_Pair_Tail`) and extends the
    appearance by one page rather than splitting a pair across a demo.
  - **`HSPG_slmap` is one slot, not one page per map.** A different map comes up each time it
    appears; enumerating a page per map would have put ten-plus near-identical pages in the
    rotation.
    - **The cursor is stepped by `HS_Attract_Advance_Page`, never by the drawer.** Advancing it in
      `HS_Draw_SL_Map_Page` made the page flicker through every map at frame rate, because
      **a drawer runs once per frame, not once per page** — anything it mutates changes 35 times a
      second. `HS_SL_Current_Map` resolves the cursor read-only for drawing. This is the general
      rule for these pages: the drawers must be idempotent.
  - **The page block steps once more on the way out** (`D_DoAdvanceDemo`), or the page an
    appearance ended on would be the first page of the next one.
  - **Demos are sorted by how long they run, not by which table their record lives in.** Anything
    under **`HS_LONG_DEMO_TICS`** (4 minutes) goes in the shuffled bag as ordinary filler between
    pages; anything over is dealt by **`HS_NextLongDemoPath`** when `HS_Attract_Rotation_Done()`
    reports a full pass of the score pages, so it appears roughly once every several cycles rather
    than parking the screen on one run.
    - It used to split on Single Level versus Survival, which is only a *proxy* for length and
      wrong in both directions: a Survival run that died two minutes in is short — and common, now
      that deaths are recorded — while a 100% run of one big map can easily pass four minutes.
      The record's own time *is* the demo's length, so ask that.
    - The two tables are laid end to end (`HS_Demo_Slot_Count` / `HS_Demo_Slot`) so one cursor
      walks both: per-map split records first, then one Survival record per episode.
      `HS_Demo_At` resolves a slot to a path, a length and a caption, and is the only place that
      knows the difference between the two.
  - **Captions carry the holder's initials.** The split table is anonymous — a time and nothing
    else — so they come from the board (`HS_Board_Entry`), which is where initials are stored.
    `---` when nobody claimed the entry.
    - The `SINGLE LEVEL: ` prefix is gone to pay for them: it cost 100px, the range already says
      whether the run was one map or several, and it was never quite true anyway — a campaign
      *first* level scores on that same table. Measured against the real `STCFN` lumps: the per-map
      form is at worst **283px** of 320 (narrower than the 295 it replaced) and the Survival form
      **304px**. The Survival caption also drops its `EP%d`, which the end map already implies.

  **The per-map single level page** (`HS_Draw_SL_Map_Page`) is laid out **difficulty x place**
  rather than a stacked block per difficulty, which reads in one glance: skill label at x=10, then
  three cells at 52, 139 and 226. Measured, a cell is `AAA 0:08.57` at 75px and at worst
  `AAA 29:59.99` at 83px for a slow max run of one map, so cells clear each other by 4px and the
  last ends at 309 of 320. The max block is drawn only when it has entries, or the page would be
  half a screen of blanks.

  - The skill is shown as the **New Game menu's own graphic** (`hs_skillpatch[]` —
    `M_JKILL`/`M_ROUGH`/`M_HURT`/`M_ULTRA`/`M_NMARE`, matching `ITYTD`/`HNTR`/`HMP`/`UV`/`NM`),
    not the short text name, so it reads from across a room. Falls back to the text if the lump is
    missing, since those are the Doom names and Heretic differs.
    - **Bottom aligned** on `HS_PG_SKILL_BOT` (**34**), not top aligned: the patches are 15..19
      tall (`M_NMARE` is the tall one), so aligning tops would make the baseline jump as pages
      cycle. Widths reach 248 (`M_ROUGH`), still centred in 320.
  - **`HS_PAGE_SECS` is 8.** Its cost is bounded now that an appearance shows at most three pages,
    so raising it is cheaper than it used to be.
  - **`HS_MapOrder()` sorts by the order a player actually reaches the maps, not by name.** Doom 2
    hides MAP31/32 behind MAP15's secret exit and returns to MAP16 afterwards, so the run is
    **15 -> 31 -> 32 -> 16**; plain numeric order would list them fifteen levels from where they
    are played. Doom 1 is episode major, map minor -- E?M9 is a secret level too but sits at the
    end of its episode, which is where numeric order already puts it.
  - Verified numerically rather than by eye: Ultimate Doom enumerates 36 maps E1M1..E4M9 in order,
    18 rows a column at step 8 ending at 189 of 200, columns 10..152 and 168..310, 14 pages at 3
    per cycle; Doom 2 enumerates 32 as MAP01..MAP15, **MAP31, MAP32**, MAP16..MAP30, 16 rows at
    step 9 ending at 188.

  Records demos in the background and saves the run that set each record. Hooks:
  `HS_Init` from `D_DoomMain` (after `legacyhome` is resolved — `M_Init` is too early),
  `HS_NewGame` from the menu skill-select handlers (**must** precede `G_DeferedInitNew`, see below),
  `HS_LevelExit` from `WI_Init_Stats` (already the single-player-only branch of `WI_Start`),
  `HS_Player_Died` from `P_KillMobj` (`p_inter.c`), and new
  cases in `D_DoAdvanceDemo`/`D_Display`. `G_SnapshotDemo` (`g_game.c`) copies the demo buffer
  without closing it, so live recording continues after a record is saved.

- **The run board** (`hs_stuff.c`, `hs_run_t`, `runs.dat`). The per-map table above holds *splits* —
  the best cumulative time anyone has reached a given map in, one deep and anonymous. The board is
  the separate table of whole **runs**, which is what a player competes on and puts initials
  against. **Two tables and two files, deliberately**: retrofitting depth and names into
  `hs_maprecord_t` would have multiplied a 64×2×5 array, while a list of runs is naturally bounded
  by pruning, and `highscores.dat` needs no migration at all.
  - **Campaign runs rank furthest-then-fastest**: `HS_MapOrder(endmap)` is the primary key and time
    only breaks ties. This is what gives the great majority of cabinet runs — the ones that end in
    a death partway — a board to land on; a completed episode tops it because nothing outranks it
    on progress. Ranking partial runs on time alone is *not possible*: 8:00 to E1M5 versus 12:00 to
    E1M7 has no answer without progress as a key. Single Level runs are all one map, so the same
    comparison reduces to time.
  - `HS_MapOrder` already encoded the real play order including Doom 2's 15→31→32→16 secret detour,
    so the progress metric was already written and verified.
  - Depths differ by board: **10** for campaign (`HS_BOARD_DEPTH_RUN`, per game/skill/category) and
    **3** for single level (`HS_BOARD_DEPTH_SL`, per game/map/skill/category).
  - **Depth is only shown where the player is looking at that one thing.** The three-deep single
    level board is drawn on the **Single Level menu page**, where the player has just highlighted a
    map and is about to play it. The attract cycle still shows one row per map. That is the rule
    that stops the page count exploding.
  - **The same rule puts the hovered skill's records under the New Game selector**
    (`HS_Draw_Skill_Records`, called from `M_DrawNewGame`), both categories, updating as the player
    scrolls — so a difficulty can be sized up before committing to it.

    ```
            LVL    TIME       WHO
    SPEED   E1M8    7:03.22   MLR
    MAX     E1M3    3:04.75   MLR
    ```

    - **The category is a row label under one shared header**, not a header block per category
      (`SPEED LVL / SPEED TIME / SPEED INITIALS`, then the same again for MAX). Measured, that
      header row is **256px** against 170 for a data row, so the columns would be sized by the
      headings and the numbers would sit in wide gaps; `SPEED` and `MAX` would each be written
      three times; and it needs five rows against three, which is the entire budget under the menu
      with nothing spare. It also matches the intermission block, which is already SPEED / MAX /
      YOU rows.
    - **`itemOn` *is* the skill.** `NewGameMenu`'s five rows are skills 0..4 in order, which is why
      `M_ChooseSkill` takes its `choice` as the skill directly.
    - `epi + 1` is the episode either way: the episode menu sets `epi` for the ExMy games, and
      `doom2_commercial` goes straight to the skill menu leaving it at 0, which is the one episode
      a flat `MAPxx` game has. It is the same expression `M_ChooseSkill` uses to build the
      starting map.
    - Measured: `NewDef` sits at y 63 and its five `IT_PATCH` rows step by `LINEHEIGHT` 16, so
      they occupy 63/79/95/111/127 — and `M_NMARE`, the tall one at 19, ends at **146**. Header
      plus two rows from 152 runs to 179 of `BASEVIDHEIGHT` 200. Columns keep 8px at their widest
      glyphs and the block ends at 240 of 320.
    - Headings and row labels are `V_WHITEMAP` (grey), values option 0 (red) — the two read
      backwards from their names; see the `V_DrawString` colour note.
  - **`hs_run_board_ok` is not `hs_run_ranked`.** A death clears `hs_run_ranked` (it gates the split
    records, where the free level reload would otherwise make dying a costless retry) but must not
    take the run off the board. So `hs_run_board_ok` tracks the ruleset and cheating **only**, and
    `HS_Player_Died` deliberately does not touch it. A cheat does clear it: dying is playing badly,
    cheating is not playing the same game.
  - Both are **initialised true**, matching each other. A game started from the command line
    (`-warp`) never runs `HS_NewGame`, so a board flag defaulting false would let such a game write
    split records while silently never reaching the board.
  - The run is **frozen at its last scored level exit** (`hs_run_startmap`/`endmap`/`tics`), inside
    the block already gated on `hs_run_ranked` — so it naturally stops at the last level completed
    under scoring. It is *not* `hs_cumulative_time`, which keeps counting after a death so the
    intermission can still show an honest elapsed time.
  - `HS_Run_Finished()` is hooked into **`Command_ExitGame_f`**, the single funnel every route back
    to the title passes through. Idempotent — it zeroes `hs_run_levels` — because some routes reach
    it twice.
  - **It is also called from `HS_Player_Died`, at the moment of death.** Under Survival a death is
    the end of the run in every sense that matters: nothing more can be scored, and the thing the
    board credits — how far it got — is already final. Leaving the commit to `Command_ExitGame_f`
    made the entry depend on how the session happened to unwind afterwards, and a player who died
    deep into an episode could find no board entry and no initials prompt for progress they had
    genuinely earned. Committing at the death makes the place safe the instant it is decided.
    - The idempotence is what makes this safe: the later `Command_ExitGame_f` call is a no-op, and
      a player who presses use and carries on playing unranked cannot commit twice.
    - It only *arms* the prompt. `M_Initials_Ticker` will not raise the page over a live level
      (`gamestate == GS_LEVEL && ! demoplayback`), so it still appears on the way out, which is
      where the player expects it.
  - **The three ways a run gets dropped are logged now**, instead of returning in silence: no level
    scored, `hs_run_board_ok` false (altered ruleset or a cheat), and placed nothing. "My run did
    not go on the board" was otherwise impossible to tell from "my run did not place", and neither
    is visible from the outside. Same reasoning as the `Run is unranked: "<cvar>"` line.
  - **A multi-level run cannot be driven headlessly**, which is worth knowing before trying: the
    intermission will not advance. Setting `accelerate_stage` twice from the console reaches
    `WI_Init_NoState` but the run still sits at `GS_INTERMISSION` on the same map, so a scripted
    second level exit never happens and `hs_run_levels` stays at 1. Single level runs, and a death
    on the level after the first, both drive fine.
  - **That funnel is not enough on its own: a completed episode never reaches it.** Finishing
    E?M8 (or Doom 2's MAP30) goes intermission → `G_NextLevel` → `F_StartFinale()`, and the player
    then presses through the ending or quits — neither of which passes through
    `Command_ExitGame_f`. The per-map splits recorded normally while the board entry was silently
    dropped, which is the exact opposite of what should happen: **a finished episode is the run
    that most deserves a place.** `HS_Run_Finished()` is therefore also called from `G_NextLevel`
    at the run-ending finales.
    - It looked intermittent because the **idle timeout does cover `GS_FINALE`** — waiting 60s on
      the ending screen committed the run, while pressing through it lost the run. Any "it worked
      that time" report about this is that difference.
    - **Only the run-ending finales, and `CL_Reset()` is what marks them.** Doom 2 reaches the same
      `F_StartFinale` for the *between levels* text screens after MAP06/11/20 (and 15/31 by the
      secret exit) and the campaign carries on afterwards, so committing there would end every
      Doom 2 run at map 6. The end-of-game cases are exactly the ones that call `CL_Reset()`
      ("end of game, disconnect from server"), so the two calls are kept side by side deliberately.
      There are three: `finale_after_intermission`, the UMAPINFO endbunny/endcast path, and
      `gamemap == 30`.
    - The run is committed **at the finale start**, not when the ending is over, so the place
      survives the player quitting during the credits. Only the *initials* are lost in that case,
      and the entry keeps the `---` placeholder.
    - **The prompt is raised over the ending, and `M_Initials_Ticker` deliberately does not
      exclude `GS_FINALE`.** It did at first, to avoid painting over the ending text, and that was
      wrong: a Doom 1 ending screen is *terminal* — you do not finish it, you leave it — so
      gamestate stayed `GS_FINALE` until the player quit and the entry kept its `---` placeholder.
      Only sitting out the whole 60s idle timeout on the ending screen ever produced a prompt,
      which is why this presented as "it skipped the initials again" even after the commit itself
      was fixed. Asking over the ending is also what an arcade machine does: finish the last
      level, sign the board, then watch the ending.
    - Two things make that safe, and both are worth knowing before putting any other page up
      during a finale: `M_Drawer` is called "even on top of everything" (`d_main.c`), so the page
      renders over it; and `M_Responder` runs **before** `G_Responder` in `D_ProcessEvents`, so
      the page takes the keys instead of the finale advancing underneath.
    - **Test the prompt inside a window shorter than `cv_idletimeout`.** The idle timeout covers
      `GS_FINALE` and reaches the prompt by its own route (`Command_ExitGame_f` → title), so a
      test longer than 60s passes whether or not this works. The verification run is 30s against
      the cabinet's 60s timeout.
    - Verified headless, driving the intermission with a temporary console accelerate: E1M8 gives
      `finale_pending=1` then a committed `doomu E1M8 E1M8 0 speed 104` entry with no
      `Command_ExitGame_f` involved at all, and `AAA` once the run is allowed to reach the title;
      Doom 2 MAP30 commits both categories; **Doom 2 MAP06 does not commit and writes no
      runs.dat**, which is the regression that matters.
    - **The intermission needs *two* accelerates to leave, not one** (`WI_update_Stats`): the first
      jumps `sp_state` to 10, the second takes the `sp_state == 10` branch to `WI_Init_NoState`.
      A headless test that sends one appears to prove the finale is never reached, when in fact
      the intermission simply never advanced — this cost a wrong diagnosis once already.
  - **Board entries have no demos of their own**, on purpose. The attract cycle keeps replaying the
    split-record demos exactly as before, so the demo count and the shuffle bag are unchanged; a
    three-deep board with its own demos would have tripled both.
  - `HS_Board_Entry` walks the stored order and hands back the nth match as the nth place, so
    **stored order must equal rank order**. Insertion maintains it; `HS_Runs_Load` re-sorts each
    board after reading, since `runs.dat` is plain text an operator may have edited.
  - Ties keep the **earlier** entry ahead (the comparison is strict), the arcade convention — and it
    matters now that times are kept to the tic.
  - Attract pages: one per (skill, category) that has anyone on it, appended **after** the split
    pages. Ten rows is the whole board, so a board never needs more than one page.
  - **One-time migration.** On the first boot with no `runs.dat`, the board is seeded from the
    existing *single level* split records — one of those is exactly one run, so the mapping is
    exact. Campaign splits cannot be converted (several come from one run and nothing says which),
    so the campaign board starts empty and fills as runs are played. Seeded entries get the `---`
    placeholder for initials.

- **Times are shown to hundredths where they are run times** (`HS_Format_Time_CS`). Whole seconds
  cannot separate two E1M1 runs. **This needed no format change and no migration**: `besttime` is
  `tic_t` and `highscores.dat`'s fourth field has always been a raw tic count — the precision was
  only being thrown away at the point of display.
  - Hundredths are `(tics % TICRATE) * 100 / TICRATE`. TICRATE is 35 and does not divide 100, so
    they are a scaled tic count rather than exact — which is the convention the wider Doom
    speedrunning world displays, so a cabinet time reads the same way as one from anywhere else.
  - **Where, and why not everywhere.** Run times get hundredths: the intermission TOTAL row, the
    intermission BEST table, the boards, the Single Level page and the demo captions. The
    **attract split page stays at `M:SS`**, and that is a geometry limit, not a preference: its two
    map columns are 152px each, and at `HS_PG_SPEED` 84 a `12:34.57` (51px) would start at 33 and
    collide with the 38px map name. Two hundredths-wide time columns need ~182px, which does not
    fit — and one column of 24 rows does not fit vertically either (`58 + 23*10` exceeds
    `BASEVIDHEIGHT` 200). Nobody compares E1M4 splits to the hundredth.
  - The intermission BEST table **was re-laid out** to make room: `HS_COL_TIME` 90→**108**,
    `HS_COL_STEP` 62→**72**, and the `wi_stuff.c` call site x 156→**138**. Measured: "ITYTD" 36px
    ends at 174, the widest time `888:88.99` 64px starts at 182, the last column's right edge is
    318 of 320. Done rather than left alone because BEST sits directly above TOTAL and the player
    compares the two — mismatched precision there is worse than the re-measure.

- **Initials entry** (`m_menu.c`, `M_Initials_*`, `InitialsDef`). Raised once when a run that took a
  board place has ended, and stamped onto every entry that run placed.
  - **Once per run, not per record.** A campaign run sets a split record on every level it passes,
    so prompting per record would ask eight times for one run. Both categories can place at once —
    same run, same player — and `hs_run_placed[]` records which, so one entry covers them.
  - **`cv_initialstimeout`** ("initialstimeout", default **60s**, `CV_SAVE`), under
    **Options → Menu Options** with the other operator settings. Generous on purpose: nothing is
    waiting on it (the cabinet is already back on the attract screen behind the page), and a player
    who just earned a place should not be racing a timer. `0` disables the timeout, which leaves
    the page up until somebody presses fire — supervised machines only.
  - **The place is saved before the prompt**, so every exit is an accept: ESC accepts rather than
    abandons, and the timeout accepts the default `AAA`. Backing out would only throw the name away.
  - **The idle timeout is held off while the page is up** (`G_Idle_Timeout_Check` asks
    `M_Initials_Active()`), or a player part way through entering would be closed out from under
    them. The page's own countdown still clears an abandoned one, so the cabinet can never stick.
  - **Asked over the corpse on a death, not only once the level is left.** Under Survival the run
    is over the moment the player dies -- `HS_Player_Died` commits it to the board right there --
    but `M_Initials_Ticker` refused to open over `GS_LEVEL`, and on a *campaign* death gamestate
    does not leave `GS_LEVEL`: pressing use reloads the map and play continues unranked. So the
    page sat armed and then ambushed the player minutes later, at whatever unrelated moment they
    finally left the level -- in practice while starting their next game, which is the worst
    possible time for a modal page asking for initials. The gate now also lets it through when
    `players[consoleplayer].playerstate == PST_DEAD`.
    - Safe over a live level because `M_Responder` runs before `G_Responder`: the page takes the
      keys, so the use press that would respawn cannot fire underneath it. Once the initials are
      in, the page closes and use works again.
    - Multiplayer cannot reach this: `HS_Player_Died` returns early there, so the prompt is never
      armed in the first place.
  - **Raised from `M_Ticker`, not from the run-end path.** `Command_ExitGame_f` only *arms* it
    (`HS_Initials_Pending`). Opening from the ticker keeps it independent of the order things
    happen in on the way back to the title — `M_SingleLevel_Finished` calls `Command_ExitGame_f`
    and *then* pushes its own page, so a prompt opened inline would be buried by it. From the
    ticker it lands on top of whatever settled, and backing out returns there.
  - **Driven from the translated keys**, unlike the join screen: it is taken *after*
    `M_Cabinet_Menu_Key`, because stick up/down to cycle a letter is exactly what that produces,
    and this page does not care which panel is entering. The join screen needs the opposite.
  - Character set is A-Z then 0-9, no blank — three characters always filled keeps every board row
    the same width. Cells are fixed pitch (20px) so the row does not shift as glyph widths change.

- **The episode's last level gets an intermission** (`g_game.c` `G_Start_Intermission` /
  `G_NextLevel`, `wi_stuff.c`). Vanilla skips it on those maps and jumps straight to the finale, so
  **E1M8 / E2M8 / E3M8 / E4M8 showed no summary page and never reached `HS_LevelExit`** — a time on
  an episode-ending map was impossible to record. Doom II already did it the other way round: MAP30
  gets its intermission and the finale is started afterwards from `G_NextLevel`. The Doom 1,
  Heretic and Chex maps now follow that same order, which is why one flag was enough to get both
  the stats page and the scoring.
  - **`finale_after_intermission`** (`g_game.c`, declared in `g_game.h`) carries the decision.
    `G_Start_Intermission` clears it at the top and sets it in place of the old
    `CL_Reset(); F_StartFinale(); return;`; `G_NextLevel` does the `CL_Reset` and the
    `F_StartFinale`. Decided in one place rather than recomputed from `gamemode`/`gamemap` at each
    site, because **UMAPINFO can override the ending** — its block runs first and `goto`s past the
    map-8 case entirely, so a pack that gives E1M8 a next map keeps a real next map.
  - Only the **non-deathmatch** path changes. In deathmatch map 8 already wrapped to `lev_next = 0`
    and went through the intermission.
  - **`wi_stuff.c` must skip `ShowNextLoc`** for these maps, in both `WI_update_Stats` and
    `WI_update_NetgameStats`. There is no next location, so the "Entering ..." screen would point
    at E?M1. `WI_Init_NoState()` instead — exactly what `doom2_commercial` already does there.
  - **Doom II needed no change at all**: MAP30 (and the 6/11/20 text maps) already showed the
    summary and scored. Confirmed by test, not assumed.
  - Verified headless, `-warp` plus `wait`/`exitlevel`, with the intermission's keypress forced:
    E1M8/E2M8/E3M8/E4M8 each went stats → NoState → `F_StartFinale` and wrote a record
    (`doomu E4M8 2 98 speed`); E1M1 and Doom II MAP01/MAP30 were unchanged controls
    (`finale_pending=0`, `ShowNextLoc` still shown for E1M1). A **single level** run of E4M8 wrote
    `doomu-sl E4M8 3 31 speed` with its demo and returned to the menu with the finale skipped.

- **The ranked ruleset** (`hs_stuff.c`, `hs_ranked_rules[]`). **Game options are not multiplayer
  only.** DoomLegacy has no separate single-player path — solo play runs the same client/server
  simulation — and every gameplay setting is a single global `CV_NETVAR`, so anything reachable
  under Options applies to a scored single-player run. The two-player Options screen edits
  single-player behavior for the same reason.

  The baseline is **vanilla difficulty knobs with Boom/MBF engine behavior left at its defaults**
  (roughly complevel 11): gravity, monster/item respawn, monster health and pickup multipliers,
  dogs, voodoo mode, insta-death, weapon recoil, jumping, tired run/drown, monster vary and the
  rest are pinned to vanilla, while the Boom/MBF AI and physics fixes stay on so Boom-format level
  packs still work. `HS_Apply_Ranked_Ruleset()` runs after the config load (`d_main.c`) and again
  in `Command_ExitGame_f`, so **one player's tinkering cannot leave the next player unable to
  score**; settings are not saved outside devmode, so this only ever undoes a within-session change.

  Rather than hide the menus, an altered ruleset **plays on but records nothing**:
  `HS_Ruleset_Is_Ranked()` is checked in `HS_NewGame` (which then skips starting the background
  recording at all), again at every `HS_LevelExit`, and — since the marker was reported as not
  appearing — **live, from `HS_Run_Is_Ranked()`**, latching `hs_run_ranked` false so a change made
  *mid-run* voids it rather than only the levels after. The player is told twice — a warning
  under the item list on the Game Options and Adv Options screens (`M_Draw_Unranked_Warning`,
  called from `M_Drawer` so all three screens are covered in one place), and an `UNRANKED` marker
  at the top of the HUD during play.
  - **The HUD marker reads a latched flag, so whatever sets that flag decides when the player is
    told.** `HS_Run_Is_Ranked()` (its only caller is `HU_Drawer`) returned `hs_run_ranked`, which
    was recomputed *only* at `HS_NewGame` and `HS_LevelExit` — so changing any Game Option in the
    middle of a level voided the run correctly but said nothing until the level ended. This
    affected **every** cvar in the table, not just the one it was reported against
    (Fast Monsters), and it is the setting that most needs immediate feedback: it changes the
    simulation underneath a demo that is being recorded.
    - The check-and-latch is now one function, **`HS_Void_If_Ruleset_Changed()`**, called from both
      `HS_Run_Is_Ranked()` and `HS_LevelExit` so the two cannot disagree about what voids a run.
    - **It must not run in a multiplayer game, and neither may the marker.** Starting one moves
      several cvars that are in `hs_ranked_rules[]` — `M_StartServer_Go` issues a deathmatch mode
      and `Deathmatch_OnChange` derives `cv_itemrespawn` from it — so the live check latched the
      run unranked within a frame of the game starting and painted **UNRANKED across a two or four
      player game that was never being scored at all.** Nothing had noticed before, because every
      *writing* path already returned early on the same test and only the display reached it.
    - That test is now **`HS_Scored_Game()`** (`hs_stuff.c`, declared in the header):
      `!(netgame || multiplayer || deathmatch)`, which `HS_LevelExit` and `HS_Player_Died` had
      always opened with by hand. It is a function because the *display* has to ask it too, and the
      same question written out in four places is how they drift apart. **Local splitscreen sets
      `netgame`**, so a two or four player game on one cabinet is excluded by either half.
    - The HUD marker asks it as well, not just the live check: `hs_run_ranked` is **not** reset when
      a multiplayer game starts (`HS_NewGame` runs only on the Single Player and Single Level
      routes), so a flag left false by an earlier solo run would otherwise paint the marker over a
      deathmatch.
    - **A multiplayer game cannot be exercised headlessly.** The server floods
      `Network: HSendPacket, network unreachable` in a sandbox with no networking and never reaches
      `GS_LEVEL`, so `HU_Drawer` never runs — millions of log lines and no useful output, with or
      without `internetserver 0`. Verify the single player side headlessly (it does still latch:
      `scored=1 ranked=1` clean, then `ranked=0 reason=fastmonstersopt` after a mid-run toggle) and
      take the multiplayer side to the cabinet.
    - **Safe to call from a drawer** — which is what `HU_Drawer` does, once a frame — because the
      latch is monotonic within a run: both flags only ever go false there, and only `HS_NewGame`
      sets them true again. That satisfies the "drawers must be idempotent" rule the attract score
      pages are subject to. The table walk is ~55 pointer compares and short-circuits once both
      flags are already false.
    - It deliberately does **not** early-return when `hs_run_ranked` is already false. A run that
      has *died* is unranked but still on the board (`hs_run_board_ok`), so altering the ruleset
      after dying has to take it off the board too — which is exactly what a "nothing left to do"
      early return would quietly get wrong.
    - Guarded on `demoplayback` at the call site: a replay is not the cabinet's run.
  - **The *engine* cvars `cv_respawnmonsters` and `cv_fastmonsters` are deliberately absent from
    the table.** `G_InitNew` turns both on for `sk_nightmare` (`g_game.c`, `CV_SetParam`), so their
    value belongs to the skill, not the player. **This bit once already** — `cv_fastmonsters` was
    left in the table by mistake, and every Nightmare run played MAP01 normally, then showed
    `UNRANKED` from MAP02 with no score for MAP01. That is the signature of a rule the *engine*
    changes after `HS_NewGame`: the run passes the check at menu time and fails it at the first
    level exit.
  - **What is in the table instead is the player-facing pair**, `cv_fastmonsters_menu` /
    `cv_respawnmonsters_menu` (`m_menu.c`, console names `fastmonstersopt` / `respawnmonstersopt`).
    Nothing in the engine ever writes those, so Nightmare cannot trip the check, while a player who
    turns either on from Game Options does — which is what has to happen, since both change the
    simulation and a mid-run change would desync the demo the run is being recorded into.

    This is the **third** instance of the same split in this file, after `cv_deathmatch_menu` and
    `cv_dm_timelimit`, and for the same reason: the engine cvar is written from under the player.
    Here it happens twice over —
    - `G_DeferedInitNew`'s game-start command line reset both to 0 (`fastmonsters 0;respawnmonsters
      0`), so **anything chosen in Game Options before a run was wiped before the first tic**. The
      setting only appeared to work if it was toggled *during* a run, which is what made it look
      broken rather than merely ineffective. That line now seeds the engine pair **from the menu
      pair**, which both honours the choice and still clears whatever the last game left set.
    - `sk_nightmare` forces both on, so after one Nightmare game they read as on for everything
      afterwards.

    The menu cvars are `CV_CALL` and mirror into the engine pair through `CV_Set_by_OnChange`, so
    toggling mid-game still takes effect immediately the way it always did. They are **not**
    `CV_SAVE`: `HS_Apply_Ranked_Ruleset` pins them to 0 at every route back to the title, which is
    also what now undoes Nightmare's pollution of the engine pair.
    - **Nightmare still gets fast monsters and respawn.** Order at a menu start: the command line
      sets the engine pair from the menu pair, *then* `map ... -skill 5` runs `G_InitNew`, which
      forces both to 1. `CV_SetParam` writes `.EV` directly and calls `CV_cvar_call`
      unconditionally — it has no unchanged-value early return — so `FastMonster_OnChange` always
      fires and the skill is unaffected by whatever the player chose.
  - When a run is voided, `HS_LevelExit` logs `Run is unranked: "<cvar>" differs...` once via
    `GenPrintf(EMSG_info, ...)`. A run that silently scores nothing is near-impossible to diagnose
    from the outside; check the console/log first if scores stop appearing.
  - **`config.cfg` legitimately disagrees with the ruleset, and that is not a bug.** `-devmode`
    skips `HS_Apply_Ranked_Ruleset()` entirely, so an operator session keeps whatever the config
    holds and writes it straight back; a player session loads the same file and then overrides the
    relevant cvars in memory, never saving. On the current cabinet config exactly three saved
    values differ — `monsterfriction "Momentum"` (2 vs 0), `monstergravity "On"` (2 vs 0) and
    `voodoo_mode "Auto"` (3 vs 0), all DoomLegacy defaults the ruleset pins to vanilla. Everything
    else in the table already matches. **Do not "fix" the config to match**, and do not compare the
    two by eye: config stores the *label* (`"On"`, `"Boom"`, `"x3"`, `"50"`) while the table stores
    the number, so most apparent mismatches are the same value. Resolve labels through the cvar's
    `PossibleValue` table before concluding anything.
  - `HS_Apply_Ranked_Ruleset` **self-checks** and warns via `GenPrintf(EMSG_warn, ...)`. If a value
    in the table is not one of a cvar's `PossibleValue`s, `CV_Set` rejects it silently and the
    cabinet would record nothing forever — this turns that into a visible message. All current
    values were verified against their `*_cons_t` tables and by a headless run.
  - Values are in menu units; `hs_rule_expected` scales `CV_FLOAT` cvars (gravity) by `FRACUNIT`,
    and `hs_rule_current` reads `.value` for `CV_FLOAT`/`CV_VALUE` cvars and the `.EV` byte
    otherwise — mirroring `command.c`'s own split. **`.EV` is a byte**, so a fixed-point cvar's
    `.EV` is its low 8 bits and useless for comparison.
  - The demo header records nearly all of these (38 bytes at `G_BeginRecording`, plus the 6 added
    for the Legacy extras), so pinning values is demo-safe. **`cv_gravity`, `cv_predictingmonsters`
    and `cv_blockmap_gen` are the exceptions — not recorded anywhere**, so changing one would
    desync a record demo. All three are pinned by the ruleset, which is now the only thing keeping
    them consistent. Worth remembering if a demo ever desyncs mysteriously.
