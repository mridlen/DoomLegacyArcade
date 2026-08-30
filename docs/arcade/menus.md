# Menu lockdown, naming and the operator-only pages

*Part of the DoomLegacy arcade cabinet build. Read before adding, removing or reordering any menu row in `m_menu.c` — several menus are addressed by hardcoded position.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **Menu lockdown** (`m_menu.c`, in `M_Init` under `if( ! devmode )`). What a player can reach:

  ```
  Main:     New Game / Options / [End Game] / [Quit Game]
  New Game: Single Player / Single Level / Multiplayer
  Options:  Crosshair / Player >> / Game Options >> / Select Game >>
  Player:   Player1 config >> / Player2 config >>
  Player n: Your color / Crosshair / Control scheme
  ```

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
- **Menu naming**: the New Game page offers **Single Player** and **Multiplayer**, where
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
    `TwoPlayerMenu[4]`) still point at the right rows.

- **The operator page is named "Arcade Options"** — `OptionsMenu`'s row, and `MenuOptionsDef`'s own
  `menutitle`, which had been left as a copy-pasted "Effects". The array and the `menu_t` are still
  called `MenuOptionsMenu`/`MenuOptionsDef`, and the lockdown still hides the row by its hardcoded
  index (`OptionsMenu[9]`), so nothing else moved. The old name said where the page sat in the menu
  tree; the new one says what is on it — every row is a cabinet setting, none of them is about menus.

- **Attract Volume** — `cv_attractvolume`, appended to the end of `MenuOptionsMenu` like every other
  operator row. Written up in `attract.md`; noted here only because it is a row on this page.

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
