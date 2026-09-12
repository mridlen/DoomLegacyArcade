# Menu lockdown, naming and the operator-only pages

*Part of the DoomLegacy arcade cabinet build. Read before adding, removing or reordering any menu row in `m_menu.c` — several menus are addressed by hardcoded position.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **A menu can silently run off the bottom of the screen, and nothing tells you.** The screen is
  200 lines; `M_DrawGenericMenu` starts at `menu_t.y` and advances `STRINGHEIGHT` (10) per ordinary
  row. A row placed past y=200 is simply not drawn — no clipping mark, no scroll, no warning — so
  the page looks complete and the cursor moves onto items nobody can see.

  Video Options had been in that state for some time: 17 rows from y=40 put the last one at exactly
  y=200 and the OpenGL link was invisible. It now starts at **y=24** (the `M_OPTTTL` title patch is
  15 tall drawn at y=2, so that clears it by 7) and has room for **one more row**.

  **That headroom is gone, and there was less of it than the note claimed.** 17 rows of
  `STRINGHEIGHT` from y=24 end at y=191; an 18th starts at y=194 and runs to 201, off the
  200-line screen, and the title patch above leaves nowhere to start higher.

  So the performance settings moved off instead. **Video Options -> Performance Options >>**
  (`PerformanceMenu` / `PerformanceDef`) holds *Framerate Cap*, *Render Threads* and *Show
  Ticrate* — everything that trades picture for speed. (*8bpp Draw* and *Row Padding* were
  added later; the page is five `STRINGHEIGHT` rows at y=48..88, the last glyph ending at y=95. *Row Padding* went in after
  *8bpp Draw* as `PERF_rowpad`, and only `PERF_threads` and `PERF_rowpad` are indexed, both by
  `M_Draw_Performance`, which greys them outside the software renderer. The page uses its own
  drawer, so `tools/menufit-test.py` does not measure it.) Two rows left Video Options and one link
  arrived, so that page is **16 rows now, ending at y=181**: shorter than it was, with room again.

  `VO_gamma` is unaffected — it is index 4 and both rows that left were below it. Check that
  when moving anything on this page; the gamma triple is the only positional dependency and it
  is easy to shift by accident.

  **`VO_gamma` no longer exists, and Video Options no longer has a positional dependency at
  all.** *Gamma Function*, *Gamma*, *Black level* and *Brightness* moved to their own page,
  **Video Options → Gamma Options >>** (`GammaOptionsMenu` / `GammaOptionsDef`). They belong
  together — the gamma function chooses the curve and the other three are its parameters, which
  is exactly why `MenuGammaFunc_dependencies` greys them as a group — and taking four rows off
  Video Options is what made room for *Keep aspect* (`software-fullscreen.md`).

  The indices are `GO_gammafunc`/`GO_gamma`/`GO_black`/`GO_bright`, and
  `MenuGammaFunc_dependencies` now writes `GammaOptionsMenu[GO_gamma .. GO_gamma+2]`. **The
  move also removed the `__DJGPP__` conditional from the calculation**: `VO_gamma` was 3 or 4
  depending on whether the *Fullscreen* row was compiled in, on a page where that row sits above
  it. The new page has nothing conditional above the gamma rows, so the enum is the same
  everywhere.

  **F11 opens Gamma Options directly.** That key has always been "the gamma key" and used to
  land on Video Options only because that is where these rows lived.

  Video Options is **14 rows now, ending at y=161**: 16, minus the four gamma rows, plus the
  Gamma Options link, plus *Keep aspect*. Room for three more. Still measure before adding one —
  nothing warns when a row falls off the bottom, which is the whole point of this section.

  *Keep aspect* is greyed (`IT_DISABLED`) outside the software drawmode, the same choice and the
  same reason as *Render Threads*: in OpenGL the black bars come from the monitor's own scaler
  and the engine is not placing the picture at all. It is found **by its cvar pointer, not by
  index**, in `M_Draw_VideoOptions` — a hardcoded index there would put straight back the
  positional dependency this move removed, on a page that still has two conditionally compiled
  rows.

  *Render Threads* is shown **greyed** (`IT_DISABLED`) outside the software drawmode rather than
  hidden, so it stays discoverable and it is obvious why it is unavailable — hiding it is what
  made it impossible to find the first time. → `render-threads.md`

  **Measure before adding a row to a long page.** The trap when measuring by hand: `IT_CV_SLIDER`
  rows advance by `STRINGHEIGHT` like any other — the `y+=16` in that drawer belongs to the
  `IT_CV_STRING` text-entry branch, not the slider. Reading it the other way makes a page with four
  sliders come out 24 px taller than it is. → `uncapped-framerate.md`

---

- **Menu lockdown** (`m_menu.c`, in `M_Init` under `if( ! devmode )`). What a player can reach:

  ```
  Main:     New Game / Options / [End Game] / [Quit Game]
  New Game: Campaign / [Deathmatch] / Single Level / [Multiplayer]
  Options:  Player >> / Game Options >> / Select Game >>
  Player:   Player1 config >> / Player2 config >>
  Player n: Your color / Crosshair / Control scheme
  ```

  Deathmatch and Multiplayer are in brackets because `M_Configure` hides both on a one panel
  cabinet — see "Campaign and Deathmatch" below. **Multiplayer and Game Options can also be taken
  away on any cabinet**, by the operator switches `cv_multiplayermenu` / `cv_gameoptionsmenu` — see
  "Multiplayer Menu / Game Options" below. So this tree is the *most* a player can be given, not a
  fixed shape.

  On **Multiplayer → Options** (the Net Options page) only the deathmatch ruleset a player might
  reasonably choose is left: Allow exitlevel, Teamplay, TeamDamage, Fraglimit, Timelimit,
  Deathmatch Type, Frag's Weapon Falling, and the Game Options link. Allow Jump, Allow Rocket Jump,
  Allow autoaim, Allow turbo, Allow join player and Maxplayers are hidden — server and network
  plumbing that means nothing on a cabinet. `NetOptionsMenu` is addressed by position for this, so
  its indices are named (`netoption_*`); keep the enum in step with the array.

  Quit Game is in brackets because it is now an operator setting and hidden by default — see
  "Quit Game entry" below. End Game is in brackets because it appears only while a game is
  actually being played — see "End Game" below.

  Hidden: Networked Multiplayer (both entry points), Load/Save on the main menu, most of Options (Messages,
  Always Run, Effects/Connect/Network/Server/Arcade Options, Sound Volume, Video Options, Setup
  Controls), Network Options again where Game Options nests it, several Start Game server options,
  Always Run/Autoaim/mouse/weaponpref/rebinding on the player config screen, and name/skin on the
  Setup Player screens. Uses **`IT_HIDDEN`**, a locally added
  `IT_DISPLAY` value — unlike stock `IT_DISABLED` (grayed but still occupying a row) the generic
  drawer skips it without advancing `y`, so entries vanish and the list closes up. Items are hidden
  *in place*, never removed from the arrays, because several menus are indexed by hardcoded position
  elsewhere. `M_DrawSetupMultiPlayerMenu` paints the name box and skin string outside the item loop,
  so those are suppressed separately. Each affected menu's `lastOn` is moved to the first item still
  shown, or the cursor starts on an invisible row (`M_SetupMenu` only walks *down* past hidden
  items, so it cannot recover when index 0 is hidden).
- **The devmode hotkey** (`gc_devmode`, `M_Devmode_Hotkey` in `m_menu.c`, hooked into
  `D_Process_Events` in `d_main.c`). An operator key that restarts the cabinet into `-devmode`, and
  another press restarts it back out.

  **It is an assignable game control, not a cvar.** It appears on `ControlMenu3` as *Devmode
  Restart*, beside Screenshot and above the "Joystick and Mouse Only" heading — above it on purpose,
  since it is a keyboard key by nature and by default. That makes it per panel, and
  `M_Devmode_Hotkey` accepts **any** of the four panels' bindings: which page an operator happened
  to set it on says nothing about who is pressing it. `G_Controldefault` gives panel 1 **Scroll
  Lock** (no panel can produce it, and nothing else in the engine wants it); panels 2-4 start
  unbound.
  - **`gc_devmode` must stay last in `gamecontrols_e`**, immediately before `num_gamecontrols`.
    `gamecontrolname[]` is indexed by that enum and `config.cfg` stores control *names*, so an entry
    inserted anywhere else renames every control after it and shifts the bindings in an existing
    config — which is exactly the `gc_comehere` bug written up in `input.md`. Appended at the end,
    nothing moves: verified by re-saving the cabinet's own `config.cfg` through the new build and
    diffing the `setcontrol` block, which came back identical apart from the one added
    `setcontrol "devmode" "scroll lock"` line. No config migration is needed, and an existing config
    with no such line simply keeps the default.
  - Being assignable, it **can** be put on a panel button. Nothing prevents that and nothing should
    — but it makes the attract-screen gate below the only thing standing between a cabinet button
    and a restart, which is worth saying out loud in the operator docs.

  **It is a restart and not a live toggle, because `devmode` cannot be flipped in a running
  session.** All three of its jobs are applied once at startup and none of them is reversible in
  place:

  - the **menu lockdown** above is one way. `M_Init` overwrites each locked item's `.status` with
    `IT_HIDDEN` and keeps no record of what it was, and `M_Configure` adds a second batch. Undoing
    it would mean restoring roughly forty statuses nobody saved, in the arrays that are addressed by
    hardcoded position — the exact place this file warns about.
  - the **ranked ruleset** (`HS_Apply_Ranked_Ruleset`, called from `D_DoomMain` when `! devmode`)
    has already overwritten a set of `CV_NETVAR`s.
  - **config writing** is the one job that does read `devmode` live, in `M_Save_Config`.

  Re-execing sidesteps all of it: the new session runs the whole startup and is indistinguishable
  from one launched with or without the flag by hand. `M_Restart_Program` already existed for the
  game selector, so the hotkey only added a `want_devmode` argument — it strips `-devmode` from the
  copied argv unconditionally and re-adds it if the new session wants it, which is what makes one
  code path work in both directions. Its two existing callers pass the current `devmode` to leave
  the mode alone.

  **The config save falls out for free, and that is the point of the round trip.** Restarting *into*
  devmode writes nothing, because a player session never saves. Restarting *out* of it runs
  `D_Quit_Save` while still in devmode, which is exactly the devmode config write — so an operator's
  changes are saved on the way back to the locked cabinet, with no separate Quit. Verified by
  restarting out of devmode headlessly and watching `config.cfg`'s mtime move and
  `setcontrol "devmode" "scroll lock"` appear in it.

  **Attract screen only** (`gamestate == GS_DEMOSCREEN` and `! M_Initials_Active()`). A restart
  throws away whatever is running, so a stray press during a game would take a paying player's run —
  or, during initials entry, a record earned but not yet committed.

  **A refused press is not consumed**, and that is not a detail. This runs ahead of every other
  responder, so returning true on the refusal path would have swallowed the key for the whole
  session everywhere but the attract screen — and the stock cabinet binds **screenshot to both F12
  and Print**, two of the five settings offered. Returning false leaves the key its ordinary job and
  costs nothing. For the same reason the note is `GenPrintf(EMSG_dev, ...)` and not `CONS_Printf`: a
  console line can reach the HUD, and a player must neither be shown that the key exists nor get a
  line per press on a key they use.

  **The hook goes in `D_Process_Events` after `M_Responder` and `CON_Responder`, and that ordering is
  load-bearing.** It ran first at one point, which reads as the obvious choice — but `gc_devmode` is
  an assignable control, and the menu's "press a key" capture never sees a key this consumes, so the
  operator could not re-assign it (and could not type it into the console). Behind those two and
  ahead of `G_Responder`, which is what pops the menu up on any key at the attract screen, it still
  wins in the one state where it acts. Cost is a keydown-only loop over four panels.
  - The consequence is that a key the menu already claims cannot be used: the screenshot handler in
    `M_Responder` consumes F12 and Print, and the `!menuactive` switch consumes F1-F11. Those keys
    can be *bound* to `gc_devmode` and will simply keep doing their old job. Scroll Lock and Pause
    are clear.

  Verified headlessly with a temporary console command that fires a synthetic keydown, built from
  the live `gamecontrol_pl[0][gc_devmode][0]`, through `M_Devmode_Hotkey`: from the attract screen
  the process re-execs and comes back with `devmode` set (and back again with it clear, rewriting
  `config.cfg` on that leg), while the same command at tic 105 of a loaded level produces no second
  startup at all. That the synthetic key worked at all is also the proof that the default binding
  is applied — an unbound control reads as `KEY_NULL` and is rejected before the gate.
- **Campaign and Deathmatch** (`m_menu.c`, `M_CampaignNewGame` / `M_DeathmatchNewGame` /
  `M_Arcade_MP_Go`; `SingleMulti_Menu`). The New Game page is now four rows —
  **Campaign**, **Deathmatch**, **Single Level**, **Multiplayer** — where the first two start the
  two games somebody standing at the cabinet actually asks for, with no settings page in between.

  New graphics `M_CAMPGN` (108x17) and `M_DEATHM` (135x17) in `legacy.wad`. `M_SINGLE` reads
  "SINGLE PLAYER" and is now unused, alongside `M_2PLAYR`.

  - **Campaign is the old Single Player route, renamed, with the mode decided at the join screen
    instead of in advance.** Episode where the game has one, then skill, then the join screen; one
    panel checking in is the solo run it has always been, two or more start the same game as local
    coop. Nobody picks "coop" anywhere — the people at the cabinet answer it by pressing fire,
    which is the whole UX argument for doing it this way. `M_NewGame_Go` branches on the count:
    `D_Num_Joined_Players() > 1` goes to `M_Arcade_MP_Go( DMM_coop, 1, 0 )`, one player falls
    through to the unchanged `HS_NewGame()` / `G_DeferedInitNew()` pair.
    - **The name change is not cosmetic.** "Single Player" would now be a lie on the row that
      starts a four player coop game; "Campaign" says what the row does rather than how many people
      are expected to do it.
    - **Not `D_NumLocalPlayers()`, which was the first thing tried.** That one bumps its answer to
      2 whenever `cv_splitscreen` is set — a fudge that exists for the old Two Player menu, which
      sets the render split and nothing else. A menu deciding *what kind of game to start* has to
      go on what was just answered on the join screen, not on a split a previous game may have left
      on, or a one player campaign silently becomes a two player one. Hence
      `D_Num_Joined_Players()` (`d_clisrv.c`), which is the same arithmetic without the bump;
      `D_NumLocalPlayers` now calls it and adds the bump back.
    - **No `HS_NewGame()` on the coop branch**, deliberately. `HS_Scored_Game` already excludes
      anything with a second person in it, so calling it would spend the record demo buffer on a
      run that can never be saved.

  - **Deathmatch is the Multiplayer page's game with its ruleset pinned**: `DM_both` (3), monsters
    off, bots 0, `cv_dm_timelimit` on the clock. Episode page where the game has episodes, and
    nothing else: `M_Episode` checks `newgame_route` and calls `M_Deathmatch_Start` instead of
    pushing `NewDef`.
    - **No skill page.** With no monsters the skill only decides how much ammo and armour the map
      hands out, so it is a page nobody has a reason to think about standing between pressing the
      row and playing the game. `sk_medium`, fixed.
    - **`cv_bots` is the one setting that has to be written rather than passed.** The `map`
      command has switches for skill and monsters but not bots — `G_InitNew` hands `cv_bots`
      straight to `B_Regulate_Bots` — so `M_Deathmatch_Go` does a `CV_SetValue`. It is `CV_HIDEN`
      and not saved, so nothing persists past the boot, and the ranked ruleset pins it to 0 anyway.

  - **`M_Arcade_MP_Go` is deliberately *not* a refactor of `M_StartServer_Go`.** That one belongs
    to the page a player tweaks by hand and reads its settings off that page's cvars; it is left
    byte for byte as it was, because the brief was to leave Multiplayer alone. The new one writes
    **no cvar at all** (bar `cv_bots`), so playing a Campaign or a Deathmatch does not quietly
    rewrite what Multiplayer → Start Game offers next time — which is what setting
    `cv_deathmatch_menu` / `cv_monsters` / `cv_nextmap` and calling the existing function would
    have done.
    - The order in it is load bearing and copied from the proven one:
      `server`/`netgame`/`multiplayer` before `D_WaitPlayer_Setup()`, and every setting into the
      command buffer *ahead* of the `map` command, which does not run until the buffer drains. See
      the `G_DeferedInitNew` note in `CLAUDE.md` — nothing there may be set as a cvar and expected
      to be in force when the level loads.
    - It clears `fraglimit` the way `G_DeferedInitNew` does and `M_StartServer_Go` does not, and
      takes fast monsters / monster respawn from `cv_fastmonsters_menu` /
      `cv_respawnmonsters_menu` — the player's own choice — rather than from whatever the last
      game left set.
    - `DMM_coop` is **0x10**, not 0. In `deathmatch_cons_t` 0 is `Coop_weapons`, a different game.
    - **`cv_wait_players` has to be set to the joined count, or a one player Deathmatch hangs the
      cabinet.** Setting `netgame` arms `D_WaitPlayer_Setup`'s wait, and it defaults to **2** with
      `cv_wait_timeout` 0 — no timeout — so a Deathmatch that exactly one person pressed fire for
      sits on "waiting for players" for ever. That is not an exotic case: one curious player
      pressing the row is the obvious way to meet it. **Multiplayer → Start Game only escapes it by
      accident**, because it always issues `splitscreen 1` and the `cv_splitscreen` bump inside
      `D_NumLocalPlayers` then reports two players whether or not two joined — which is also why
      taking that bump out of the *decision* (above) has to be paired with this.
      `M_Arcade_MP_Go` sets the cvar, calls `D_WaitPlayer_Setup`, and puts it straight back: the
      value is copied into `wait_netplayer` there and read nowhere else, so restoring it keeps the
      Multiplayer page's "Wait Players" exactly as the operator set it.

  - **Both new rows are hidden on a one panel cabinet**, in `M_Configure` beside Multiplayer
    (not `M_Init` — `cv_localplayers` comes from `config.cfg`, which is not loaded yet). Campaign
    stays: on one panel it is the solo run it has always been.

  - **Geometry.** Five rows from `SingleMultiDef.y` 64, `IT_PATCH` stepping `LINEHEIGHT` 16, so
    64/80/96/112 with the plain-text Networked row at 128. Measured from the lumps rather than
    their bounding boxes: the new art is 17 tall in the box but **15 rows of ink** (rows 1..15),
    exactly like `M_SINLVL`, so ink runs 64..78, 80..94, 96..110, 112..126 against a text row
    starting at 128 — no overlap anywhere, and the widest (`M_SINLVL`) ends at x 244 of 320.
    `tools/menufit-test.py` reports the page at `5 rows, y 64..135, room for 6 more`.

  - **`newgame_route` existed** because `EpiDef` is shared and knows nothing about either route,
    while what happens *after* it differs. It is **gone**: Deathmatch no longer passes through the
    episode page at all (see the map selector below), so `M_Episode` belongs to Campaign alone and
    has nothing to remember across it. `EpiDef` now has one entry point again.

- **The Deathmatch map selector** (`m_menu.c`, `DeathmatchLevelDef` / `DeathmatchLevelMenu`;
  `cv_dm_nextmap` / `cv_dm_nextepmap`). Deathmatch asks **which map**, on a page of its own, where
  it used to show the shared "Which Episode?" page.

  - **The episode page was answering the wrong question.** It could only ever pick which `E?M1` to
    start on, and on a flat `MAPxx` game there was nothing to ask, so `M_DeathmatchNewGame` skipped
    it and **every Doom 2 deathmatch started on MAP01** — one arena out of thirty-two, chosen by the
    engine. The people at the panels have a favourite map, which is exactly the thing the old page
    could not be told.

  - **Two rows: the map, and Start.** `DML_map` is swapped by gamemode in `M_Configure`
    (`DeathmatchLevelMenu_Map` / `_EpisodeMap`), the same line and the same
    `gamemode==doom2_commercial` test `SingleLevelMenu[SL_map]` uses directly above it — that page
    is the map selector a player on this cabinet has already learned, so this one is built from it.
    Done in `M_Configure` for the same reason: the New Game row reaches the page by
    `Push_Setup_Menu`, which has no handler to hook, and `gamemode` is not known at `M_Init`.
    - **Still no skill page**, unchanged and for the unchanged reason: with no monsters the skill
      only decides how much ammo and armour the map hands out.
    - Geometry: `tools/menufit-test.py` reports `2 rows, y 40..77, room for 12 more`. Start carries
      `IT_YOFFSET` 30 so it sits a blank row below the map — a row that starts the game should not
      be one cursor step from a row the player is still scrolling.
    - The title patch is **`M_DEATHM`**, the New Game row's own graphic, the same trick
      `SingleLevelDef` plays with `M_SINLVL`: the page is named by the row that reached it. 135
      wide at the drawer's fixed title x of 94, so it ends at 229 of 320.

  - **Its own cvars, not `cv_nextmap`/`cv_nextepmap`.** Those are shared by Single Level *and*
    Multiplayer → Start Game, and a deathmatch map choice writing through to both is the quiet
    cross-talk `M_Arcade_MP_Go` was written to avoid ("writes **no cvar at all**", above). The new
    pair points at the **same `PossibleValue` tables**, so `M_Configure`'s trim of `exmy_cons_t`
    down to the episodes actually present covers them for free, and they are `CV_HIDEN` and unsaved
    like the pair they copy — every boot starts the page at MAP01 / E1M1. They are registered
    through `menu_init_cvar_list`; a cvar left out of it has no `.value` at all.

  - `M_Deathmatch_Start` now takes its map from `M_Deathmatch_MapName()` instead of
    `G_BuildMapName(epi+1,1)`, and nothing on this route sets `epi` any more.

- **Menu naming**: the New Game page offers **Campaign** and **Multiplayer**, where
  "Multiplayer" is *local* play on this cabinet (the old "Two Player Game" — no longer two player
  only) and uses the **`M_MULTI`** graphic, which reads "MULTIPLAYER". `M_2PLAYR` literally reads
  "TWO PLAYER GAME" and is now unused. The engine's networked server menu is renamed
  **"Networked Multiplayer >>"** and drawn as **plain text** rather than the `M_MULTI` graphic, so
  it cannot be mistaken for the line above. It was already devmode-only — the lockdown hides both
  of its entry points — and stays that way. **Not because it is known broken: it has never been
  exercised in this build**, since cabinet-to-cabinet play needs two cabinets and there is not yet
  one. Nothing was removed, so treat it as untested rather than unsupported, and do not "fix"
  anything there speculatively.
  - The `TwoPlayerDef` page uses `M_MULTI` as its title graphic for the same reason.
  - Its two `SETUP PLAYER` rows (`M_SETUPA`/`M_SETUPB`) are replaced by **four**
    "Player n config >>" text entries, matching Options → Player, so panels 3 and 4 needed no
    artwork. They open `PlayerOptionsDef` **without** `Pop_Menu()`, unlike `M_PlayerDirectorChoice`,
    so backing out returns to this page instead of skipping past it.
  - **`TwoPlayerMenu` is addressed by position** and the rows moved, so its indices are named
    (`twoplayer_*`) and the lockdown uses those — the networked row went from 4 to 6.
  - Rows for panels the cabinet does not have are hidden, as on the Player page.
  - **The three mouse rows are devmode-only** (`M_SetupMultiPlayer_pind`): a player has no use for
    mouse settings on a cabinet, and they were three rows of clutter on the page reached most
    often. Already hidden for panels 3 and 4, which have no mouse hardware at all.
  - **No menu indices moved**, so the lockdown's hardcoded positions (`SingleMulti_Menu[2]`,
    `TwoPlayerMenu[4]`) still point at the right rows. (`SingleMulti_Menu`'s *did* move later,
    when Campaign and Deathmatch went in — which is what the named `singlemulti_*` enum is for.)

- **The operator page is named "Arcade Options"** — `OptionsMenu`'s row, and `MenuOptionsDef`'s own
  `menutitle`, which had been left as a copy-pasted "Effects". The array and the `menu_t` are still
  called `MenuOptionsMenu`/`MenuOptionsDef`, and the lockdown still hides the row by its hardcoded
  index (`OptionsMenu[9]`), so nothing else moved. The old name said where the page sat in the menu
  tree; the new one says what is on it — every row is a cabinet setting, none of them is about menus.

- **Multiplayer Menu / Game Options** — `cv_multiplayermenu` / `cv_gameoptionsmenu`, two operator
  switches for how much menu a player is given, inserted after "Quit Menu" because they say the
  same kind of thing it and "Cheats Menu" do. Both `CV_SAVE`, both applied in `M_Configure` under
  `! devmode` — the usual reason, `config.cfg` is not loaded when `M_Init` runs.

  - **Both default On, and that is the rule rather than a preference**: a switch added so somebody
    *can* change something defaults to what the machine already did. See the `CLAUDE.md` note on a
    new cvar having no config line, so its compiled default is what every cabinet runs.

  - **Multiplayer Menu hides `SingleMulti_Menu[singlemulti_multi]` and nothing else.** Deathmatch is
    deliberately untouched: it is its own row starting its own game, not a way into the settings
    page this takes away. (The one-panel block above hides both, for the different reason that one
    person cannot have a deathmatch.) Two blocks can hide the same row; the second assignment is a
    no-op.

  - **Game Options has to hide two rows, not one.** The page is reachable from `OptionsMenu`
    (`OPT_gameoptions`, index 5) *and* from `NetOptionsMenu[netoption_gameoptions]`, which a player
    reaches through Multiplayer → Options. Hiding only the first leaves the page reachable and the
    setting looking broken. `MPOptionMenu` has a third copy, but that page lives inside Networked
    Multiplayer, which the lockdown hides entirely. **Grep `M_GameOption` before trusting this
    list** — four call sites, and the fourth is `AdvOption2Menu`, which is *inside* Game Options.
    - `OPT_gameoptions` is a hardcoded 5, counted from the top of `OptionsMenu`. The `#if` in the
      middle of that array yields exactly one item either way, so it does not move with the build
      options — unlike `GameOptionsMenu`'s own last row, which the lockdown indexes from the end
      for exactly that reason.

  - Geometry: 14 rows now, `tools/menufit-test.py` reports `y 40..177, room for 2 more`. Measured
    against the real `STCFN` lumps, "Game Options" the *label* is the wider new one at **125**
    ("Multiplayer Menu" is 122, against "Initials Timeout" at 108 already on the page), so it runs
    60..185 while an `Off` value (24 wide, right-justified to 260) starts at 236 — a 51px gap. (The
    4px squeeze written up under "2 Player Split" below belongs to the Players & Views page now,
    which that row moved to; nothing on this page comes near it.)

- **Attract Volume** — `cv_attractvolume`, appended to the end of `MenuOptionsMenu` like every other
  operator row. Written up in `attract.md`; noted here only because it is a row on this page.

- **Idle Timeout / Idle Warning** — `cv_idletimeout` / `cv_idlewarntime`, inserted after "Initials
  Timeout" so the three timeouts sit together. Written up in `attract.md`; noted here for the
  geometry and for why inserting rather than appending was safe.
  - **`MenuOptionsMenu` is the one arcade page nothing indexes by position.** The lockdown hides
    its whole entry in the parent (`OptionsMenu[9]`) rather than touching rows inside it, and
    `grep -n "MenuOptionsMenu\[" m_menu.c` is empty. So rows can go anywhere in it — which is not
    true of `ServerMenu`, `MainMenu`, `TwoPlayerMenu` or `SetupMultiPlayerMenu`, and the check
    costs one grep. The comment in the array still says "appended, the lockdown addresses menu
    items by hardcoded index"; that is about the *parent* page, and is why these two went in
    without renumbering anything.
  - Geometry: `MenuOptionsDef` is at `y` 40 with `M_DrawGenericMenu`, and every row is `IT_STRING`
    at `STRINGHEIGHT` 10. Thirteen rows put the last one at `y` 160, glyphs 7 tall, so the page
    ends at 167 of 200 — room to spare. Both labels are shorter than "Initials Timeout" which
    already sits above them, and the values (`Off`, `900`) are far shorter than the game names
    "Boot Game" renders on the same page, so the right-justified value column cannot collide.

- **2 Player Split** — `cv_splitvertical`, inserted directly after "Control Panels" because it
  qualifies it: the panel count decides how many views there are, this decides the shape of two of
  them. Written up in `multiplayer-views.md`; noted here for the geometry and for the insert.
  - Inserting mid-array was safe for the reason above — nothing indexes `MenuOptionsMenu` by
    position. Re-checked with `grep -n "MenuOptionsMenu\[" m_menu.c`, still empty.
  - Geometry: fourteen rows now, so the last one ("Audit >>") sits at `y` 170 and the page ends at
    177 of 200 — room to spare.
  - **The label was cut from "Two Player Split" to "2 Player Split" by measurement, not by eye.**
    `MenuOptionsDef.x` is 60 and `M_DrawGenericMenu` right-justifies the value at
    `BASEVIDWIDTH - x`, so the label runs from 60 and the value column ends at 260. Measured
    against the real `STCFN` lumps, "Two Player Split" is 116 wide — the widest label on the page,
    past "Initials Timeout" and "Attract Volume" at 108 — and the wider of the two values,
    "Top/Bottom", is 80 and so starts at 180. That is a **4px** end-to-start gap, the tightest row
    on a page where the next tightest has 66. "2 Player Split" is 99 wide and leaves 21.

- **Screen Order** — `cv_panelorder`, inserted directly after "2 Player Split" for the same reason
  that one follows "Control Panels": it qualifies the row above it. Which quadrant of the 2x2 each
  panel drives; written up in `multiplayer-views.md`, noted here for the geometry and the insert.
  - Inserting mid-array was safe for the reason above — nothing indexes `MenuOptionsMenu` by
    position, and `numitems` is `sizeof(...)/sizeof(menuitem_t)`. Re-checked with
    `grep -n "MenuOptionsMenu" m_menu.c`: the array, the `menu_t`, and nothing else.
  - **The values are the layout, not a word for it.** `1 3 / 2 4` and `1 2 / 3 4` draw the grid
    itself, which reads correctly whichever way round the operator is thinking about it; "Columns"
    and "Rows" both need a moment's translation and one of them is always ambiguous.
  - Geometry, measured against the real `STCFN` lumps: "Screen Order" is 91 wide, so the label runs
    60..151, and either value is 51 wide, so it starts at `260 - 51` = 209. A **58px** end-to-start
    gap — comfortable on a page whose tightest row ("2 Player Split") has 4. The measurement
    reproduces this file's published widths for "2 Player Split" (99) and "Top/Bottom" (80), which
    is how the script was checked before its new numbers were believed.
  - Fifteen rows now: `MenuOptionsDef.y` is 40 and `IT_CVAR` rows advance by `STRINGHEIGHT` 10, so
    the last one ("Audit >>") sits at 180 and the page ends at 187 of 200. Still room, but that is
    the row after which this page needs a second column or a submenu.

- **Boot game** — `cv_defaultgame` ("defaultgame", default `None`, `CV_SAVE`), under
  **Options → Arcade Options** as "Boot Game" beside the other operator rows, so it is
  operator-only. Picks
  which game the cabinet starts in instead of whichever IWAD the search finds first.
  - **It cannot be read as a cvar.** `IdentifyVersion()` chooses the IWAD at `d_main.c:3030`;
    `M_LoadConfig` does not run until **3216**. So `D_Read_Default_Game()` parses the single
    `defaultgame "..."` line straight out of `config.cfg` beforehand, called next to `HS_Init` —
    after `legacyhome`/`configfile_main` are resolved, before `IdentifyVersion`. A targeted parse
    was chosen over moving the config load earlier, which would reorder startup for everything.
  - The cvar's `PossibleValue` strings are **the `game_desc_table` idstrs themselves** (`doomu`,
    `doom2`, `plutonia`, `tnt`) rather than pretty labels, because config stores a cvar's *label*
    and that hand-parse needs the stored text usable as-is. They are also exactly what `-game`
    accepts, which makes the setting self-documenting.
  - Validated **before** entering the `-game` block in `IdentifyVersion`, not inside it: that
    block's `game_switch_found` label sits within its own braces and an unrecognized value there
    takes a fatal path. A boot game that is unrecognized, or whose IWAD has since been uninstalled
    (`D_Game_Available`), must never stop the cabinet booting — both cases warn and fall through to
    the normal search.
  - `-game` and `-iwad` on the command line both override it.
  - Verified headless across all six paths: unset → normal search; `doomu`/`tnt` → those games;
    `-game doom2` overriding a `tnt` default; `"banana"` → warns, normal search; and `plutonia`
    with the IWAD genuinely unreachable → warns, normal search. That last one needs `HOME` isolated
    as well as the wad removed from the run directory, or `~/games/doom` still satisfies the
    search and the test silently passes for the wrong reason.
- **Start Game carries a "Bot Options >>" link**, directly under the Bots count — the number that
  page sets is the only thing it otherwise says about them. Same entry point as the Game Options
  one (`M_BotOption`), so there is a single implementation. `ServerMenu` is addressed by position
  (by the lockdown, and by `M_StartServerMenu`'s per-gamemode map row swap) and inserting a row
  shifted five indices, so its indices are now named (`server_*`) like every other menu this file
  indexes. Geometry checked: `ServerDef.y` is 40 and an `IT_STRING` row is `STRINGHEIGHT` 10, so
  with every row shown Server Name occupies 130..140 and the `IT_YOFFSET` Start still sits at 150.
  - **An `IT_CALL` handler's `choice` argument *is* the item index** — `M_Responder` dispatches
    `routine(itemOn)` — so a menu's indices leak into its handlers as well as into any
    `Menu[i]` reference elsewhere. Grepping for `ServerMenu[` finds the array references and
    misses these entirely.
  - That is how inserting this row **crashed every Multiplayer game**. `M_StartServer_Go` tested
    `if( choice == 10 )` to recognise the Dedicated row; once "Bot Options >>" pushed *Start* to
    10, pressing Start started a **dedicated server** — no local player, and `I_ShutdownGraphics()`
    pulled the video out from under a cabinet that then carried on running. It reads as an instant
    hard crash on a menu item that has nothing to do with bots or dedicated servers.
  - The enum therefore lives **above `M_StartServer_Go`**, not beside the array it describes,
    because that is where it is first used; the array carries a comment pointing back to it.
  - **When inserting a row into any menu in this file, grep for the handlers too**, not just for
    `<Name>Menu[`: `grep -n "choice ==" m_menu.c` lists every handler that reads its index. The
    other two live ones are `M_Episode` (episode number) and the file browser, neither of which is
    positional in this sense.

- **The Net Options page is at x=48, not the 60 the other option pages use.** `M_DrawGenericMenu`
  writes the label at `x` and right-justifies the value at `BASEVIDWIDTH - x`, so a row has to fit
  in `320 - 2x`. Measured against the real `STCFN` lumps: "Deathmatch Type" is **117px** and the
  widest value `deathmatch_cons_t` can show is "Coop_weapons" at **96** — 213px of the 200 that
  x=60 leaves, so the value ran **13px back over the label**. (Upstream bug; `cv_deathmatch` is the
  only row on the page whose value is a long word.) Widening the page fixes it for every value
  rather than for whichever happened to be selected: at 48 the label ends at 165 and the widest
  value starts at 176, while the page's longest label — "Frag's Weapon Falling" at 152 — ends at
  200 against an On/Off value starting at 248. The cursor is drawn at `x + SKULLXOFF` (-32), so 16,
  still on screen. **Shortening the label or the value strings was rejected**: `config.cfg` stores
  a cvar's *label*, so renaming those values would break existing configs.

- **Game selector** (`m_menu.c`, `M_SelectGame` / `GameSelectDef`, reached from Options). Lists the
  installed IWADs (Ultimate Doom, Doom II, Final Doom Plutonia and TNT) and then any level packs.

  **Switching IWAD restarts the program.** The startup sequence has to run again; the engine can do
  that (the Launcher's "Iwad" item reaches `goto restart_command` in `D_DoomMain`) but only *before*
  `D_DoomLoop`, which is a `while(1)` that never returns. So `M_Restart_Program(idstr)` shuts down
  cleanly and **re-execs** with a different `-game`. Passing `NULL` restarts as-is, which the idle
  timeout uses to discard a loaded level pack.
  - `-game` takes the short name from the `gamedesc` table in `d_main.c` (`doomu`, `doom2`,
    `plutonia`, `tnt`, …), so the engine locates the IWAD itself and no wad path is hardcoded.
    Adding another game is one entry in `gameselect_arg[]` plus a display name.
  - The rebuilt command line preserves existing arguments (so `-devmode` survives a switch) and
    strips any earlier `-game`/`-iwad`.
  - `QUIT_normal` is required for the shutdown — the other severities force a 3 second sleep in
    `D_Quit_Save` — and `cv_textout.EV` is zeroed first to skip the ENDOOM screen.
  - **A splash says what the black screen is** (`M_Draw_Restart_Splash`), drawn immediately before
    `D_Quit_Save` while the video device is still up: `SWITCHING GAME...` when a `game_idstr` was
    given, `RESTARTING...` for the pack-unload and idle-timeout paths, which are not the same thing
    to the person watching. Unexplained, the second or two of black reads as a crash — the player
    picked a game and the machine appeared to die — and it is the one transition the README has to
    describe as "this is normal".
    - **Painted twice, then held 700ms.** The display may be double buffered, so a single paint and
      flip leaves the message on one buffer and the previous frame on the other — the same stale
      buffer alternation the loading box used to produce. The hold matters because everything after
      this call is teardown; without it the message can be gone before a person registers it.
    - **Centred by measuring the string**, the way `hu_stuff.c` draws `PRESS FIRE TO START`, not
      with `V_CENTERHORZ` — `V_DrawString` ignores horizontal centring in hardware mode where fills
      and patches honour it.
    - Verified on the real GL path (`SDL_VIDEODRIVER=offscreen`) by capturing the two paints with a
      temporary `M_ScreenShot()` call and measuring the TGAs: both frames identical at 8108
      non-black pixels, bounding box x 329–690 of a 1024-wide render (centre 509.5 against 512, the
      half-pixel of an integer division) and y 368–394, which is base y 96 and a 7-pixel glyph at
      the 3.84 vertical scale. Drawing there is also the risk — it happens part way into a shutdown
      — and the exec completed cleanly through six consecutive restarts.
    - **A screenshot taken under `SDL_VIDEODRIVER=dummy` is entirely black**, so this cannot be
      checked with the usual dummy harness; see `CLAUDE.md`. The control shot is what caught it —
      a title screen captured the same way was equally black, which is the only reason the first
      all-black splash capture was not read as the text failing to draw.
  - Entries whose IWAD is missing are hidden, via `D_Game_Available()` (`d_main.c`), which tries
    each candidate filename from `game_desc_table` through the engine's own `Search_doomwaddir` —
    so the normal search paths and alternate names (`doomu.wad`/`doom_se.wad`/`doom.wad`) all
    count. The whole "Select Game" line is hidden when fewer than two choices exist.

  **Level packs are loaded, not launched.** Every `.wad` in `legacyhome/levels/` is listed below the
  games as `"<game> wad: <name>"`, with a leading `*` when loaded. Selecting one issues
  `addfile "<path>"`, which adds the PWAD to the running session; its maps then replace the IWAD's,
  so the ordinary One or Two Player flow plays it. Adding a PWAD at runtime is supported; swapping
  the IWAD is not. Selecting does **not** start a game — doing so would force the mode and the
  starting map, which suits a deathmatch set but not a single player overhaul.
  - The directory is deliberately **separate from the iwad search paths**, so no name filtering is
    needed and `legacy.wad` or an IWAD can never be listed as a pack. Created on startup if absent.
  - Packs are filtered by map style: `MAPxx` for `doom2_commercial`, `ExMy` otherwise. A mismatch
    fails to load (DWANGO5 under Ultimate Doom), so `M_LevelPack_MapStyle()` reads the wad's lump
    directory directly — loading the pack to discover whether it loads defeats the purpose. It
    returns a **bitmask**, because some packs (Maps of Chaos) ship `MAPxx` and `ExMy` versions of
    every level in one wad; stopping at the first map lump hid them under one of the two games.
  - **One pack at a time.** Selecting a different pack replaces the loaded one; selecting the loaded
    one unloads it. **The engine cannot remove a wad** — there is no `W_Unload` in `w_wad.c`, the
    lumps stay for the life of the process — so both restart, re-adding what should remain with
    `-file` (`M_Restart_Program(idstr, keep_packs)`). Only loading into an empty slot avoids a
    restart. Packs restored by `-file` are detected in `argv` during the scan so they come back
    marked, and the old `-file` list is always stripped when rebuilding so packs cannot accumulate.
  - Once a pack is loaded the attract screen is not trustworthy — the pack overrides the IWAD maps,
    so the built-in demos play against the wrong levels. `M_LevelPack_Loaded()` reports this, and
    both routes back to the attract screen (the idle timeout in `G_Ticker`, and
    `M_EndGameResponse`) restart the program instead of returning to title.
- **"Read This!" is hidden on the Doom 1 gamemodes** (`m_menu.c`, `M_Configure`). Doom 2 already
  overwrites that slot with Quit (`MainMenu[MM_readthis] = MainMenu[MM_quitdoom]`), which is why the
  entry only appeared under Ultimate Doom, where it is the help/order-form screens. This lives in
  `M_Configure` rather than the `M_Init` lockdown because **`gamemode` is not yet known at
  `M_Init`** — `IdentifyVersion()` runs later, as does the doomwaddir setup. Anything menu-related
  that depends on the game or on locating wads must go in `M_Configure`; the game selector's
  availability check is there for the same reason.
- **Chase Cam Demo** — **`cv_chasecamdemo`** ("chasecamdemo", default **On**, `CV_SAVE`), under
  **Options → Arcade Options**. Shows every third attract *record* demo from the chase camera with
  a blinking `CHASE CAM` caption. Appended to `MenuOptionsMenu` before the Audit link; that menu is
  not addressed by index (unlike most others here), so the insert is safe. Full write-up in
  `attract.md`.
- **Settings do not persist** (`m_misc.c`, `M_SaveAllConfig` returns early unless `devmode`).
  Anything a player changes lasts only for that session; every launch reloads the baseline from
  `config.cfg`. The operator sets that baseline by running with `-devmode`, which is the **only**
  way the config is written — including for settings not exposed in the menus, such as screen
  resolution. High scores and record demos are separate files and still persist.
- **Launcher bypass** (`d_main.c`, `#ifdef LAUNCHER` block in `D_DoomMain`). Upstream shows its
  built-in Launcher menu whenever `myargc < 2`; that condition is removed so it only appears after a
  genuine startup error.
- **No confirmation prompts** (`m_menu.c`). Quit, End Game, Nightmare skill, "already playing", and
  quicksave/quickload all take the "yes" path immediately. Only the savegame-slot `Delete Y/N?`
  survives, as it guards irreversible data loss.

- **Quit Game entry** — **`cv_quitmenu`** ("quitmenu", default **Off**, `CV_SAVE`), under
  **Options → Arcade Options** as "Quit Menu". An arcade cabinet has no Quit button: quitting drops
  the player onto a desktop they should never see, and on an unattended machine nothing brings the
  game back. Off by default, so a stock player session cannot reach it; a `-devmode` session always
  keeps the row whatever this says, so the operator is never locked in.
  - **The hiding must run *after* the gamemode `switch` in `M_Configure`, not with the rest of the
    lockdown.** Under Doom 2 that switch does
    `MainMenu[MM_readthis] = MainMenu[MM_quitdoom]; MainDef.numitems--` — a whole-struct copy,
    `status` included — so before it runs the Quit row is index `MM_quitdoom` and after it is index
    `MM_readthis`. Hiding the wrong one leaves Quit on the menu with nothing to show for the
    setting, and under Doom 1 hides Read This instead. The code picks the index off `gamemode` for
    exactly this reason.
  - Verified headless by reading `MainMenu[quitrow].status` back through a temporary console
    command: `144` (`IT_HIDDEN` = `IT_SPACE | IT_NODRAW`) with the default Off, `18`
    (`IT_PATCH | IT_CALL`) with `quitmenu "On"` in the config.

- **End Game did nothing in single player or Single Level, and the reason was the score recorder.**
  `M_EndGame` opened with the stock `if (demoplayback || demorecording) { S_StartSound(sfx_oof);
  return; }`, which is right for a demo the player asked to record — but **`HS_NewGame` starts a
  background record demo for every ranked run** (`hs_stuff.c`, `G_RecordDemo_maxsize("hs_background",
  …)`), so `demorecording` is true for the whole of a single player or Single Level game. End Game
  played the "oof" and returned. Multiplayer worked, which is what made it look like a mode
  problem rather than a recorder problem: multiplayer is never scored, so it never records.
  - The fix tests **`demo_scratch`** (`g_game.c`), which is exactly the flag distinguishing the two
    kinds: `G_RecordDemo` clears it for a `-record` session that will be saved, `HS_NewGame` sets it
    for the throwaway buffer. The condition is now
    `demoplayback || (demorecording && ! demo_scratch)`.
  - Verified headless: a temporary console command called `HS_NewGame` and then `M_EndGame`, with a
    print on each branch. With the fix the run reported `demorec=1 scratch=1` and **PROCEEDING**;
    with the old condition reinstated, the identical run reported **REFUSED (oof)**. The bug was
    reproduced before the clean result was believed.

- **End Game lives on the main menu and appears only while a game is running.** It used to sit at
  the bottom of the New Game page, which put it two presses away and read as a fourth way to
  *start* a game on a page whose other rows all do exactly that. It is now `MM_endgame`, MainMenu
  index **5**, in the bottom slot Quit used to occupy: Quit is hidden for players
  (`cv_quitmenu`), so for them End Game is the last row on the menu, and in `-devmode` it sits
  directly above Quit, which the main menu has the vertical room for.
  - **Inserted before Read This, not appended after Quit**, for the same reason as Cheats — the
    Doom 2 fixup copies Quit over the Read This slot and drops the last item, so a row past Quit
    would vanish under Doom 2. Measured for height: Doom 1 with every row showing is 8 rows from
    `MainDef.y` 64 at `LINEHEIGHT` 16, so the last patch spans 176..191 of 200; Doom 2 drops to 7
    rows and `y` 72, ending at 183.
  - **The show/hide cannot live in `M_Init` or `M_Configure`** — unlike every other row on this
    menu, the condition changes during play, long after both have run. `M_Update_EndGame_Row` is
    called from the main menu's drawer (`M_MainMenuDrawer`, and `HereticMainMenuDrawer` for
    Heretic), which is the single funnel every route onto the menu passes through: Escape, backing
    out of a submenu, and the reenter event. It only assigns a value computed from current state,
    so running once a frame is idempotent, as a drawer must be.
  - **The test is `Game_Playing() && ! demoplayback`**, which covers Single Player, Single Level
    and Multiplayer alike. The `demoplayback` half is not optional: the menu can be opened straight
    over the attract screen (`D_Menu_Over_Attract`), where `Game_Playing()` is perfectly true but
    what is running is a demo, not the player's game.
  - Hiding the row also moves the cursor off it (`MainDef.lastOn` and, when the main menu is the
    current one, `itemOn`). With the menu already open the attract cycle keeps advancing
    underneath it, so a demo starting is enough to hide the row out from under the cursor, and
    `M_SetupMenu` only walks *down* past hidden items — it cannot recover from index 5.
  - `M_EndGame` re-tests the same condition itself rather than trusting the row to be the only way
    in.
  - **The `MultiPlayerMenu` (Networked Multiplayer) page keeps its own separate End Game row**,
    untouched. That page is devmode-only and has never been exercised in this build, so it was
    left alone deliberately rather than swept up in the move.
- **Cheats menu** (`m_menu.c`, `CheatsMenu`/`CheatsDef`, from a main menu entry using the locally
  added **`M_CHEATS`** graphic). Operator convenience: God Mode (`god`), All Weapons and Keys
  (`gimme health ammo armor keys weapons`, i.e. IDKFA), No Clipping (`noclip`) and Exit Level
  (`exitlevel`). Each issues the ordinary console command through `COM_BufAddText` rather than
  touching `player_t` directly, so there is one implementation of each cheat.
  - **Inserted at MainMenu index 4, before Read This**, giving `MM_cheats = 4` — with End Game
    later taking index 5, `MM_readthis` is now 6 and `MM_quitdoom` 7. Before it, *not* after: the Doom 2 fixup
    `MainMenu[MM_readthis] = MainMenu[MM_quitdoom]; numitems--` copies Quit over the Read This slot
    and drops the last row, so anything appended past Quit would be cut off under Doom 2. End Game
    was later inserted at index 5 for the same reason, pushing Read This to 6 and Quit to 7. The
    lockdown's Load/Save hiding at indices 1,2 is unaffected. Those are the complete set of index
    references — see the `grep` list under Single Level mode, which uses the same discipline.
  - Devmode only by default, hidden by the usual `IT_HIDDEN` treatment with `MainDef.lastOn` moved
    off it — but an operator can leave it up for players with **`cv_cheatsmenu`** ("cheatsmenu",
    default Off, `CV_SAVE`), under **Options → Arcade Options**. A cabinet at a
    party is not the same machine as a cabinet keeping scores; cheating voids the run either way.
    - **The hiding therefore lives in `M_Configure`, not `M_Init`'s lockdown**, for the same reason
      as `cv_localplayers` and the game selector: `config.cfg` is not loaded until long after `M_Init`
      runs, so the cvar would still read as its compiled default there. The condition is
      `! devmode && ! cv_cheatsmenu.EV`.
    - Being an operator setting, only a `-devmode` session saves it — a player cannot switch it on
      for themselves.
  - **Single player only** (the user's requirement). `Command_CheatGod_f` and `Command_CheatGimme_f`
    already `return` when `multiplayer` is set, so the engine enforces it; `M_Cheats_Usable()`
    additionally greys the items out when there is no single player level running, rather than
    offering a row that silently does nothing. The page footer says which case it is.
    - **That made this the first menu with *no* selectable item, which hard-locked the program.**
      `M_Responder`'s KEY_UPARROW/KEY_DOWNARROW handlers step the cursor in a
      `do … while(status & IT_TYPE) == IT_SPACE` loop, searching for something selectable —
      unbounded, so with every row `IT_DISABLED` (which *is* an `IT_SPACE` type) it spins for ever
      inside the event handler. Opening Cheats from the attract screen and pressing down froze the
      cabinet with no way out. Both loops are now bounded by `numitems` and leave the cursor where
      it was when nothing is selectable. **This is upstream code and a latent trap for the whole
      lockdown**: `IT_HIDDEN` is `IT_SPACE` too, so any menu the lockdown hides entirely would have
      done the same. `M_SetupMenu`'s own walk was already bounded (`&& itemOn`) and is unaffected.
  - **God Mode and No Clipping show their state, and do not close the menu.** Both are *toggles* —
    `Command_CheatGod_f`/`Command_CheatNoClip_f` XOR `player->cheats` — so a row reading only "God
    Mode" gave no way to tell an armed cheat from a disarmed one. `M_Draw_Cheat_State` draws On/Off
    where `M_DrawGenericMenu` puts a cvar's value (right justified at `BASEVIDWIDTH - x`), reading
    `players[consoleplayer].cheats`, and only while a level is running — which is exactly when the
    rows are not greyed out. `M_Cheat_Apply` gained a **`close`** flag, false for those two: closing
    the menu on a toggle made the state it had just set unreadable without reopening the page. The
    two one-shot rows (All Weapons, Exit Level) still close it. `CheatsMenu` is now addressed by
    position, so its indices are named (`cheat_*`).
  - **Show Coordinates** — a continuous readout of **X / Y / Z**, the view **angle**, the
    **sector** you are standing in and the **linedef you are looking at**, drawn top-left
    (`HU_Draw_Coords`, `hu_stuff.c`; cvar **`cv_coords`**, "coords", default Off, `CV_SAVE`). Built
    for reporting where a rendering bug is: stock Doom has `IDMYPOS`, but it prints a single line
    to the console, which a cabinet with no keyboard and no visible console cannot use.
    - The linedef comes from a `P_PathTraverse` with `PT_ADDLINES` forward from the eye, taking
      the first line hit. It only reads — the traverser records a pointer and changes nothing.
    - **It does not void the run**, and so is deliberately *not* routed through `M_Cheat_Apply`.
      It shows information and changes nothing in the simulation, which is the same rule the typed
      `IDDT` / `IDMYPOS` / `IDMUS` already follow (`m_cheat.c`). It is a toggle, so it leaves the
      menu up for the On/Off to be read, like God Mode and No Clipping.
    - Appended to `CheatsMenu` so the four existing indices do not move; `cheat_coords` was added
      to the enum and `M_Draw_Cheats` shows its state.
  - **Using any cheat voids the run's score**, via `HS_Player_Cheated()` (`hs_stuff.c`), modelled
    exactly on `HS_Player_Died`: it latches `hs_run_cheated`, clears `hs_run_ranked` so the existing
    early return in `HS_LevelExit` stops all further scoring, and closes the background recorder
    with `G_CheckDemoStatus`. Guarded on `netgame || multiplayer || deathmatch`, which are not
    scored anyway. The HUD marker becomes **`PLAYER CHEATED - UNRANKED`** (`hu_stuff.c`). A death
    shows no marker at all (see `high-scores.md`), so when both happened the cheat is what is
    named — it is the thing the player chose to do, and it is the reason the marker still appears.
  - **The hook is in the cheat commands, not the menu**, so the console (`god`, `noclip`, `gimme`)
    and the **typed cheat codes** are covered too. For the typed codes the single hook point is
    `cht_Responder`'s closing `if (msg)` block — every cheat that changes the simulation reports
    through `msg`, and the three that do not (IDDT, IDMYPOS, IDMUS) do not affect play, so they
    still score. `M_Cheat_Apply` calls it as well because **`exitlevel` is not a cheat command** and
    would otherwise skip the rest of a map for free; the flag is latched, so the double call is
    free.
  - Verified headless with two otherwise identical `-warp 1` autoexec runs differing only by a
    `god` line: the control wrote `doom2 MAP01 2 104 speed`, the cheated run wrote nothing.
  - `hs_run_cheated` is reset in `HS_NewGame` beside `hs_run_died`.

- **Everything a player can set for themselves is on one page** — `SetupMultiPlayerDef`, one per
  panel: **Your color**, **Crosshair**, **Control scheme**. It used to take two pages and three
  levels of menu (Options → Player → *Player n config* → Crosshair, *Player setup >>* → colour and
  scheme), with the crosshair stranded on its own page away from the other two.
  - **Consolidated onto this page rather than onto `PlayerOptionsDef`**, and that direction is
    forced: the colour row is `IT_CV_NOPRINT`, meaning its "value" is not text but the animated
    player sprite this page's own drawer paints. No other page has that drawer, so colour cannot
    leave.
  - **`PlayerOptionsDef` is now the devmode-only remainder** — always run, autoaim, the mouse rows,
    the weapon preference — reached from the *Player config >>* row on this page, which the lockdown
    now hides from players. Without that hiding a player would follow a link to a page showing them
    nothing they could not already see.
  - **The Crosshair row on the Options page is hidden from players** (`OptionsMenu[2]`), now that
    the setting lives on the per-player page. A second copy asked the player to set the same thing
    in two places — and that one is `cv_crosshair[0]`, player 1 only, so on a cabinet with more than
    one panel it silently did not mean what it appeared to. `OptionsDef.lastOn` moves 2 → 3 with it,
    or the cursor would start on an invisible row. Options now shows **Player >>, Game Options >>,
    Select Game >>**; devmode still shows everything, verified in both modes.
  - **Options → Player always shows the per-panel list now** (`M_PlayerDirector`). It used to jump
    straight to player 1's own page unless `menu_multiplayer` was set — and that flag is only raised
    when the Multiplayer menu is opened (`M_Player2_MenuEnable`, from `SplitScreen_OnChange`). So the
    same menu item led to **two different screens depending on where the player had been**: player
    1's page on a fresh boot, the Player 1-4 list after somebody had visited New Game → Multiplayer
    and backed out. It was also the one route still landing on `PlayerOptionsDef`, so it missed the
    consolidated page entirely.
    - Safe to show unconditionally because the lockdown already hides the rows for panels the
      cabinet does not have. Verified on a fresh boot with no Multiplayer visit: with four panels
      configured all four rows show, with one panel only `Player1 config >>` does.
  - Both routes in were repointed: `M_PlayerDirectorChoice` (Options → Player) and
    `M_TwoPlayer_PlayerConfig` (Multiplayer → Player n config). Both go through
    `M_SetupMultiPlayer[]` rather than pushing the menu directly, which brings the rest of the
    per-panel setup with it: `setupm_player`, every repointed cvar, the config row's label, and the
    `numitems` that drops the Player2-only rows. `M_TwoPlayer_PlayerConfig` still does not
    `Pop_Menu()` first, and `M_SetupMultiPlayer[]` does not either, so backing out still returns to
    the Multiplayer page.
  - **The new row goes *before* `setupmultiplayer_options` in the array.** `M_SetupMultiPlayer1`
    truncates the page with `numitems = setupmultiplayer_options + 1` to drop the Player2-only rows,
    so anything placed after that index would silently vanish for Player 1.
  - **On a page with `IT_YOFFSET` rows, the array order *is* the selection order and the two have to
    be kept in step by hand.** The cursor steps by index — `M_MultiPlayer_Responder` here, and
    `M_Responder` generally — not by where a row lands on screen. The crosshair row was first placed
    after Control scheme in the array while drawing above it, and the cursor then went colour →
    Control scheme *at the bottom of the page* → back up to crosshair. The fix is simply that the y
    offsets must ascend with the index; there is nothing in the drawer that will do it for you, and
    nothing that warns.

  ### The preview box, measured

  Asked whether shrinking the player sprite or its box would free a row, the answer from measuring
  is **no, not vertically**: the sprite is **41 × 56** in a **64 × 72** box interior, and the sprite
  is drawn with its feet 8px above the interior bottom, so the usable vertical slack is 8px — less
  than the 10–14px this page steps by. Dropping `PLBOXH` from 9 to 8 would leave the sprite flush
  against the top of its box and still not buy a row.

  **The sprite is drawn at half scale**, and the box is sized to it: `PLBOXW` **4** and `PLBOXH`
  **5**, from 8 and 9. Full size the man dominated the page for no gain — he is a colour swatch,
  not a portrait.

  **Halve the patch scale only, never the start scale.** `drawinfo` keeps the two apart and so does
  every renderer: software draws the patch through `xbytes`/`ybytes` and positions it through
  `x0bytes`/`y0bytes`, and the OpenGL path (`HWR_DrawMappedPatch`, `hw_draw.c`) likewise takes its
  size from `drawinfo.fdupx` and its position from `drawinfo.fdupx0`. Touching only the patch fields
  shrinks the sprite **in place**, in both renderers, with no coordinate arithmetic — it contracts
  toward the point it is anchored at, which is its own feet.
  - This is why the scale is changed in `drawinfo` rather than by halving `vid.dupx` and re-issuing
    `V_SetupDraw`, the way the 2x2 HUD does it (`st_stuff.c`). That halves the start scale too, so
    every position on the page would have to be doubled to compensate — **exactly**, which is not
    possible when `vid.dupx` is odd. At the measured 3/3 it would have put the sprite at 4/3 of its
    intended offset.
  - The integers are still rounded to the halved floats (a 4,3 dup gives 2,2 not 2,1) and clamped
    to 1, the same rule the HUD block follows.
  - Verified: outer dup 3/3 → 2/2, sprite 41×56 → **20×28**, sitting in a 32×40 interior with
    margins **L 7, R 5, T 7, B 5**.

  Sizing the box from the measured half-scale sprite: it is centred on the interior and stands with
  its feet 8 above the interior floor, so margins are `4*PLBOXW - 9` left, `4*PLBOXW - 11.5` right
  and `8*PLBOXH - 33.5` top. `PLBOXW` 3 would leave 0.5 on the right and `PLBOXH` 4 would clip the
  head by 1.5.

  **The crosshair row then moved off the narrow column and onto a line of its own**, in the run of
  rows below the box beside Control scheme. It was only squeezed in next to the box because, at full
  sprite size, there was nowhere else for it — that was what forced the "labels wider than ~90 units
  do not fit here" constraint, and it no longer applies to this row. The freed lines were spent on
  exactly this.

  The page is now, in both index and screen order:

  | row | y | shown to |
  | --- | --- | --- |
  | Your name | 40 | devmode |
  | Your color | 56 | everyone |
  | Your skin | 112 | devmode |
  | **Crosshair** | **126** | everyone |
  | Control scheme | 140 | everyone |
  | Player *n* config >> | 150 | devmode |
  | Player2 Controls >> | 160 | devmode, Player 2 |
  | Second Mouse config >> | 170 | devmode, Player 2 |

  Devmode worst case ends at y **177** of 200; the player sees three rows ending at **147**.

  **`PLSKINNAMEY` moved 96 → 72 to follow it.** That constant is where the block below the box
  starts, and the box got 32 shorter; leaving it alone would have left a visible hole. The box now
  ends at y 104, so 72 puts the skin row at 112, eight below it. The devmode worst case fell from
  y 187 to **y 163 of 200**, and the rows still ascend with their indices.

  Before the sprite was rescaled, the box was merely narrowed to `PLBOXW` 7 on the full size sprite:

  **Measure across the whole idle animation, not one frame.** The frames differ: widths 41 / 37 / 40
  with left offsets 18 / 19 / 16, so the sprite's closest approach to the interior edges was
  **L=13 R=8**, not the 14/9 a single sample reported. The sprite is centred on the interior, so
  each unit removed takes 4px off *each* side:

  | `PLBOXW` | interior | worst-case margins |
  | --- | --- | --- |
  | 8 (was) | 64 | L 13, R 8 |
  | **7 (now)** | **56** | **L 9, R 4** |
  | 6 | 48 | L 5, **R 0** — sprite touching the frame |

  Confirmed after the change: interior x 125..181 and worst margins L=9 R=4, exactly as predicted,
  with the row layout untouched (bottom still y 187 of 200). Height is deliberately left alone —
  8px of slack against a 10–14px row pitch, as above.
  - The sprite is clipped at x 0..300 by `V_DrawMappedPatch_Box`, **not** to its own box, so an
    unusually wide skin already spilled past the frame before this change; narrowing moves that
    threshold 8px closer. Changing skins is devmode-only, so a player cannot reach it.

  Moving the box *right* instead would let `Control scheme` (label ending at x 135) sit beside it
  and group all three player rows together — at the risk of the name box, drawn from the same
  `PLBOXX` at `MAXPLAYERNAME` wide, running off the right edge. Not attempted.

  ### It costs no vertical space, which was the worry

  **The crosshair row sits beside the player preview box, not below the list.** Measured rather than
  assumed, by instrumenting the page's own drawer to report every row's y, every label's right edge,
  and the box's real extent:

  | | value |
  | --- | --- |
  | preview box | x **117..197**, y **48..136** |
  | `Crosshair` label at y 72 | ends at x **93** — clears the box by 24px |
  | its value (right-justified at x 293) | starts ~250 — clears the box by ~53px |
  | devmode worst case (Player 2, 8 rows) | bottom text ends at y **187** of 200 |
  | player view (3 rows shown) | bottom text ends at y **157** of 200 |

  187 is **exactly what the page measured before the row was added**: the left column beside the box
  was empty from y 48 to 136, so the row went into space that was already going to waste.
  - **`Control scheme` cannot go in that column**, and this was found by trying it: its label ends
    at x **135**, eighteen pixels past the box's left edge at 117. It stays below the box. Only
    labels shorter than ~90 units fit beside it — `Crosshair` (93) and `Your color` (103) do.


## Arcade Options had to be split

Sixteen rows from y=40, at the generic menu's spacing, reaches the bottom of a 200 unit screen with
nothing to spare — so the page was full and "4 Player Split" had nowhere to go. Control Panels,
2 Player Split, 4 Player Split, Screen Order and Join Time moved to a new **Players and Views** page
reached from it (`PlayerViewsMenu`/`PlayerViewsDef`), which groups better than the flat list did and
leaves Arcade Options four rows shorter.

Safe to reorder because **nothing indexes `MenuOptionsMenu`** — every row is `IT_CVAR` or
`IT_SUBMENU`, there is no `IT_CALL` handler taking a `choice`, and the array is only ever referenced
by name and `sizeof`. That is not the usual case in this file and it was checked rather than
assumed; the note in the source about appending rather than inserting is the general rule
(`grep -n "choice ==" m_menu.c`), not a fact about this array. The new page inherits the lockdown
the same way, by hanging off a page that is only reachable under `-devmode`.

Forward-declared as a tentative definition — `menu_t PlayerViewsDef;` up beside `AuditDef` — which
is how this file already handles a menu that has to be named before it is defined.

## The Video Modes page, and paging a list that used to be truncated

`M_DrawVideoMode` lays the mode list out in three columns, filling down each column in turn, and it
used to stop dead at `MAXMODEDESCS` entries — `MAXCOLUMNMODES * 3`, with `MAXCOLUMNMODES` at 8, so
24 modes. Past that the fill loop simply broke. On a display advertising a lot of modes the tail of
the list was unreachable, and the tail is where `VID_add_scaled_modes` appends the small software
sizes (`software-fullscreen.md`), so the modes most worth having were the ones that went missing.

The page now holds 30 and pages beyond that.

**The geometry is fixed by the font, and was measured rather than guessed.** `V_StringHeight`
returns `hu_font[0]->height`, and `hu_font[0]` is `STCFN033`, which measures 7 pixels tall in
DOOM2.WAD. Rows start at `MODES_Y` (44) and step by `MODES_Y_INC` (8); the instruction block starts
at `MODETXT_Y` (128):

| rows per column | last row y | bottom y | clear of instructions |
| --- | --- | --- | --- |
| 8 (old) | 100 | 107 | 21px |
| **10 (now)** | **116** | **123** | **5px** |
| 11 | 124 | 131 | **overlaps** |

So 10 is the most that fits — which is exactly the `//#define MAXCOLUMNMODES 10` upstream left
commented out beside the 8. Three columns is likewise the most that fits: the widest name the list
can produce is `win 1600x1200`, 92 pixels in `hu_font`, and the third column already starts at
x=224 and ends at 316 of a 320-wide base screen. A fourth column at any spacing runs off the edge.

**The list is sorted by size, largest first, in `vidm_sort_by_size()`.** Nothing used to order it.
The fullscreen half arrives in whatever order SDL reported the display's modes — largest first, as a
rule, but only as a rule — and `VID_add_scaled_modes` appends the small software sizes after all of
it, so those came out both last *and* out of sequence. The windowed half is `windowedModes[]` in
`i_video.c`, a static table that runs largest to smallest. Neither agreed with the other. On this
laptop the fullscreen list now reads 1366x768, 1280x720, 1024x768, 800x600, 640x480, 512x384,
400x300, 320x200.

Sorted in the menu rather than in `i_video.c` on purpose: it is presentation, it covers both lists in
one place, and it leaves the engine's mode *indices* alone — `vid.modenum`, `VID_GetModeForSize` and
the per-drawmode configs all keep meaning what they meant. Two things it must get right:

- **The order has to be total and repeatable.** A drawer runs 35 times a second and `vidm_current` is
  a position in the sorted order, so an unstable or input-order-dependent sort would move the cursor
  under the player's finger. Insertion sort, stable, comparing width then height.
- **`current_modedesc` is a pointer into the array being sorted**, so it is handed in and handed
  back rather than kept. Mode numbers are unique per entry, so it is found again by that — not by
  the description string, which two entries could share.
- **The sentinel key for a mode the engine cannot describe flips with the direction.** Those sort to
  the *end*, which descending means a key *below* every real size (0) and ascending would mean one
  *above* it (`MAXVIDWIDTH + 1`). This sorted ascending first, and reversing the comparison alone
  would have hauled the undescribable entries to the top of the list. Reversing a sort is two edits,
  not one.

**`vidm_current` indexes the whole list; the page drawn is the one it falls on.** There is no
separate page variable to keep in step. `vidm_set_page()` settles `vidm_page_first`,
`vidm_page_count` and `vidm_column_size` around it, the drawer calls that first, and the key handler
pages simply by moving `vidm_current` across a page boundary. Everything in the key handler works in
page-local coordinates and reassembles `vidm_current` at the end.

**Left and Right off the edge of a page move between pages**, because that is the only thing the
cabinet panel can do — four directions and a fire button, no PgUp. PgUp/PgDn work too, for a
keyboard. A page indicator is drawn at y=34 (between the `M_VIDEO` title, 168x15 at y=2, and the
first row at 44) and only when there is more than one page.

Two range checks that look redundant and are not:

- `vidm_set_page` resets `vidm_current` to 0 when it is outside the list. **The Drawmode page shares
  `vidm_current`, `vidm_nummodes` and `vidm_column_size` with this one**, and indexes
  `vidm_drawmode[]` — `MAXCOLUMNMODES+2` entries — with it. Coming back from a long mode list left it
  well past the end of that array. `M_Draw_drawmode` clamps for the same reason. This was an out of
  bounds read before the list got longer; it is worse now.
- The key handler floors `vidm_column_size` at 1 and `vidm_page_colsize` never returns 0. With an
  empty mode list, removing *either* guard is harmless and removing *both* is a SIGFPE on the first
  arrow key.

### The aspect ratio filter

The page also filters itself to one screen shape — `vid_aspect`, cycled with **A**, defaulting to
whatever shape the display is. It is what keeps this list to one page on most machines now that
ultrawide draw sizes have been added to it, and it is written up in **`ultrawide.md`**, not here,
along with the rest of the 21:9/32:9 work. Two things about it that belong to *this* page: it is not
a row on the Video Options menu (that page is full, and its rows are addressed by hardcoded index),
and it can leave the list **empty**, which nothing else here could — so `change_mode` returns early
rather than indexing `modedescs[]`, and the page says "No modes of this shape".

### Testing it without a screen

Nothing drives this menu headlessly, and the failures here — a cursor outside the drawn page, a mode
no key sequence can reach, a sort that quietly drops an entry — are invisible to a smoke run.
**`tools/vidmenu-navtest.py`** covers it, in two suites. It **extracts `vidm_page_modes`,
`vidm_page_colsize`, `vidm_set_page`, `M_VideoMode_key_handler` and `vidm_sort_by_size` verbatim
from `m_menu.c` by brace matching**, stubs the handful of things they touch (`S_StartSound`,
`key_handler2`, `Pop_Menu`, the `KEY_*` values, `VID_GetMode_Stat`) and drives them. It reads the
geometry constants out of `m_menu.c` too. Extracting rather than copying matters: a copied test
drifts, and this one tests the shipped text — run it after any change to that page.

For list sizes 0..128, from every starting position, it walks every state reachable under the arrow
keys and checks the cursor stays inside the drawn page, the row stays inside the geometry above, the
page keys land where they are supposed to, and **every mode is reachable using the four arrows
alone, in both directions**.

The sort suite checks, over the two real lists, an already-ascending one, a messy one with duplicate
and undescribable modes, and 400 random lists: the result is descending by width then height, it is a
*permutation* of the input with nothing lost or duplicated, equal sizes keep their input order, and
the current-mode pointer comes back pointing at the same mode. Nothing lost is the one that matters
— a resolution silently dropped is the bug this whole page exists to fix. The already-ascending list
is there so the sort is never handed input that is already close to what it must produce.

That last clause was learned the hard way. The first version only checked reachability *forwards*
from mode 0, and a mutation that broke Left-edge paging left every mode still reachable by going
right — the test stayed green on genuinely broken code. Mutation testing is what found that: each
bug the test claims to catch was reinstated, and two of five were not caught.

So the mutations are part of the tool. **`tools/vidmenu-navtest.py --selfcheck`** reinstates each
one and reports whether the checks go red:

```
navigation:
  11 rows per column (overlaps the instructions)      caught -> FAIL row overlaps instructions
  left edge stops instead of paging back              caught -> FAIL unreachable: from mode 30 ...
  right edge stops instead of paging forward          caught -> FAIL unreachable: from mode 0 ...
  page target not clamped to a short page             caught -> FAIL page forward went astray
  both divide-by-zero guards removed                  caught -> exit -8
sort:
  sorted ascending instead of descending              caught -> FAIL not descending by width...
  height tiebreak dropped                             caught -> FAIL not descending by width...
  unstable: identical sizes reordered                 caught -> FAIL equal sizes reordered
  current mode not found again after the sort         caught -> FAIL current mode lost
  sizeless modes sort to the front                    caught -> FAIL not descending by width...
```

The sort mutations make the same point a second time. The first "unstable" mutation loosened the
width comparison as well, which broke the *ordering* too — so the ordering check caught it and the
stability check was never exercised at all. Narrowing it to the height alone, where the sort order
stays perfectly valid, is what finally put the stability check on trial. **A mutation that trips a
different check than the one you meant to test has not tested anything.**

**A clean result from a check that has never been shown to fail is not evidence.** If a mutation
stops applying because the code moved, the tool says so rather than quietly testing nothing.

---

- **Page geometry is now measured, not calculated by hand: `tools/menufit-test.py`.** The trap this
  file opens with — a row past y=200 is simply not drawn, with no clipping mark, no scroll and no
  warning — was checked by arithmetic in a comment every time, and the arithmetic was sometimes
  wrong. This file's own note on Game Options put "Adv Options >>" at y=160; it is at **150**
  (eleven `STRINGHEIGHT` rows from y=40 end at 140, and its `IT_YOFFSET` is 110). Nothing came of
  that particular slip, because the page has no collision either way, which is the point: a wrong
  number that happens not to matter is indistinguishable from a right one until the day it does.

  The script lifts every `menuitem_t` array **verbatim out of `m_menu.c` by brace matching** and
  pairs it with the `menu_t` that draws it with `M_DrawGenericMenu`, the way
  `tools/vidmenu-navtest.py` and the other extracted tests lift the functions they test — a copied
  table drifts away from the source and then passes forever. It resolves the `#ifdef` rows against
  the real `doomdef.h`, replays the drawer's y advance per `IT_DISPLAY` value, and reports two
  things nothing else does: a row whose bottom runs past the 200-line screen, and **two rows sharing
  a y**, which is what an `IT_YOFFSET` link left behind by an inserted row above it does.

  ```
  tools/menufit-test.py                     # every generic page, one line each
  tools/menufit-test.py EffectsOption1Menu  # one page, with every row's y
  tools/menufit-test.py --selfcheck         # prove the two checks can go red
  ```

  All **29** generic pages currently fit. The tightest are Options and Game Options at y 40..187,
  with room for one more row each. Run it after inserting, removing or reordering any row.
  - **`--selfcheck` reinstates both bugs** — it drags a page's `IT_YOFFSET` row up onto an ordinary
    row, and appends twenty rows to run it off the bottom — and reports whether each check goes red.
    Worth using on any check added to it: a clean result from a check never shown to fail is not
    evidence, and two of `vidmenu-navtest.py`'s five checks were silently useless until it grew the
    same option.
  - It only measures **generic** pages. A page with its own drawer (`M_DrawSetupMultiPlayerMenu`,
    the Join screen, initials entry) places things outside the item loop and is skipped — the
    `menu_t` filter on `M_DrawGenericMenu` is what excludes them, so a page is either measured or
    not listed at all. A symbolic `IT_YOFFSET` (`PLSKINNAMEY+14`) is reported as unmeasured rather
    than guessed.
  - **Effects Options gained a row** — *Rocket Trails*, third, beside Translucency and Spectre Fuzz.
    Ordinary rows now run y=40..150 and `"Next"` keeps its `IT_YOFFSET` at 40+130=170, so the page
    ends at 177 with two rows to spare. → `gameplay-defaults.md`
