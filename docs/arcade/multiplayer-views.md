# Four local players, the 2x2 view grid and the join screen

*Part of the DoomLegacy arcade cabinet build. Read before touching `D_NumViews`/`D_NumLocalPlayers`, `localplayer*[]`, viewport geometry in `r_main.c`/`r_draw.c`/`hw_main.c`, the HUD grid in `st_stuff.c`, or the rankings overlay in `hu_stuff.c`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **Local players: up to four (Phase 1 — no viewports yet)**. A multicade panel may have three or
  four sets of controls. The engine was hardcoded to two *everywhere*: `d_net.h` even defined
  `MAXSPLITSCREENPLAYERS 2`, but **nothing referenced it** — every limit was its own literal. That
  constant now lives in `doomdef.h` beside `MAXPLAYERS`, is **4**, and is the real knob.
  - **`cv_localplayers`** ("localplayers", 1..4, default 1, `CV_SAVE`) is how many players join on
    this machine — an operator setting, and since it also decides whether two player play is
    offered at all it is the only one. It is **not** `cv_splitscreen`, which
    is only the two-view render toggle. `D_NumLocalPlayers()` clamps it.
  - **Players 3 and 4 played but were not drawn** *at this phase* — the renderer split into at
    most two stacked halves (`r_main.c` `rdraw_viewheight >>= 1`, and `r_draw.c` had exactly
    `ylookup1`/`ylookup2`, view 2 at `vid.height>>1`). Phases 2 and 3 below gave them viewports;
    this paragraph is the starting state, not the current one. A 2x2 split needs per-view *horizontal* offsets, which have no
    precedent in the code — that is Phase 2, along with re-deriving the HUD placement.
  - What Phase 1 changed, and why each was load-bearing:
    - `SV_commit_player` refused a third player outright (`if( pind > 1 ) return 255`).
    - **The protocol already anticipated this.** `clientcmd_pak_t` carries a `pind_mask` and is
      sized to "use only what is needed", so the send/receive paths generalised cleanly; only the
      hardcoded `0x03` and `cmd[1]` had to go.
    - **`pind == 2` was a sentinel meaning "the server"**, not a player index (`Send_NetXCmd_auto`
      routing, used by bots). A third panel would have been routed as the server. It is
      **`TEXTCMD_PIND_SERVER`** now, tied to `MAXSPLITSCREENPLAYERS` so the two cannot collide.
    - `gamecontrol` / `gamecontrol2` became one table, `gamecontrol_pl[pind]`. **The two old names
      survive as macros onto rows 0 and 1**, so all ~47 existing references across a dozen files
      keep working untouched. `setcontrol3`/`setcontrol4` bind the new panels and the config saves
      all four.
    - The whole per-player cvar family widened to `MAXSPLITSCREENPLAYERS`: name, color, skin,
      autoaim, weaponpref, originalweaponswitch, autorun, alwaysmlook, mousemove, crosshair,
      controlscheme, customcontrols. First two console names are unchanged, so **an existing
      `config.cfg` still loads exactly as before**; panels 3/4 add `name3`, `skin4` and so on.
    - **Registration is a loop now.** Only players 0 and 1 were registered; an unregistered
      `consvar_t` has a NULL string, which `Send_NameColor_pind` hands straight to the netxcmd
      packer — a third panel segfaulted the instant the server announced it.
    - `G_BuildTiccmd` took its player from **`displayplayer2_ptr`**, a *view* pointer that is NULL
      whenever the screen is not split. It reads `localplayer[pind]` now. **This is the general
      lesson: views and local players are different things**, and anything that conflates them
      breaks the moment a panel has a player but no viewport.
    - `scheme_keys[]` has no preset for panels 3/4 **on purpose** — the two presets are Dvorak
      characters chosen so one keyboard can drive two players, and there is no third set that
      would not collide. Panels 3/4 stay unbound until the guided setup fills
      `cv_customcontrols[pind]`.
    - `localplayer[]` **must keep a static initializer**: `M_LoadConfig` runs long before
      `D_Init_ClientServer`, and the config's name/skin lines reach `Send_NameColor2`, which tests
      `localplayer[1] < MAXPLAYERS`. Zeroed, that reads as "player 0" and sends a netxcmd with no
      server — an immediate startup segfault. A `typedef` guard fails the build if the initializer
      stops covering every slot.
  - Verified headless: `localplayers` 1/2/3/4 each joined exactly that many players, filling slots
    0..3 with real mobjs and surviving. Controls: `splitscreen 1` still gives the normal two player
    split (`multiplayer=1`), and the stock default is untouched at one player.
  - **Phase 2 (viewports) is done for both renderers** — the hardware one first, the software
    one in Phase 3 below. `D_NumViews()` returns 1, 2 or 4:
    one view is the whole screen, two are the stacked halves splitscreen always had, and three or
    four are a **2x2 grid** (three players leave one quadrant unused, rather than inventing a
    third layout). `pind` 0..3 reads left-to-right then top-to-bottom, so panel order matches
    screen order.
    - **A demo always plays full screen.** `D_NumViews()` returns 1 while `demoplayback`, whatever
      the cabinet's panel count says — otherwise the attract screen was carved into a 2x2 and the
      demo drew in the top-left quadrant. This is the same trap `cv_splitscreen` had, which
      `Command_ExitGame_f` clears on the way to the title; `cv_localplayers` is an operator setting
      and is *not* cleared, so the view count has to ignore it here. Covers the Single Level
      "watch run" replays too.
    - **Changing the view count needs `R_SetViewSize()`**, or the old geometry sticks: viewport
      sizes are only recomputed on request. `G_DoPlayDemo` and `G_StopDemo` both call it, as
      `D_Set_View_Cell` does. **This has now caught me twice** — the count was right and the
      rectangle was stale both times.
    - **The cabinet runs OpenGL** (`drawmode "OpenGL"` in the tracked config), which is why the
      grid went into `hw_main.c`: a viewport there is just a rectangle. `HWR_SetViewSize` halves
      `gr_viewheight` and `gr_viewwidth` on whichever axes the grid divides, and
      `HWR_RenderPlayerView` offsets `gr_viewwindowx/y` by the view's column and row. (Both read
      `D_View_Grid`/`D_View_Cell_Pos` now — see the side-by-side split at the end of this file.)
    - **The "does the view fill the screen" test had to become "does it fill its cell"**
      (`gr_viewwidth == view_span_w`, not `vid.width`). Left as it was, a half-width quadrant took
      the status-bar centering path and the top row came out at **y = -61**.
    - A quadrant is very nearly the screen's own aspect ratio, so it must **not** get the
      2-view projection squash, and the same for the weapon-sprite nudge that keys off fov 90.
      (`atransform.splitscreen` was `(D_NumViews() == 2)` at this phase; it is `D_View_Squash()`
      now, which also says *which way* a view is squashed — see the end of this file.)
    - `viewsv_need_sky[]` and `view_dynlights[]` were `[2]`, indexed by view number.
    - **Phase 3: the software renderer draws the 2x2 grid too.** It used to draw at most two
      views — `r_draw.c` had exactly `ylookup1`/`ylookup2` for stacked halves, and `D_Display`
      rendered view 0 plus, for the software path, a view 1 hardwired to the lower half. With
      four panels that produced **two half-screen views and four HUDs**, which is what a
      software-mode cabinet actually showed.
      - **The mechanism was already there and unused.** The software renderer has always had a
        horizontal window offset as well as a vertical one — `view_window_x`, folded into
        `columnofs[]` by `R_Init_ViewBuffer` — but nothing ever used it for a second *column*,
        because splitscreen only ever stacked. **`R_Set_View_Window(vind)`** (`r_draw.c`) now
        places both tables on any cell of the grid, and `D_Display` calls it per view. The two
        precomputed half-screen tables are **gone**: they can only describe stacked halves and
        nothing said so at the call site.
      - **Only the software path halves the width**, gated on `rendermode` by `soft_columns` in
        `R_ExecuteSetViewSize` (named `soft_grid` when it could only mean the 2x2). The hardware renderer places its views by GL viewport and reads
        `rdraw_viewwidth` only through **`vid.fit_width`/`fit_height`**, which feed
        `atransform.scalex/scaley`, `HWR_Init_TextureMapping`'s focal length and
        `gr_pspritescale_*` — halving those visibly widened the OpenGL field of view and
        resized the weapon sprite. **Anything that touches `rdraw_*` reaches the hardware
        renderer through `vid.fit_*`**; check there before assuming the two paths are separate.
      - `r_draw.c` keeps the two questions apart: `R_View_Cell_Size()` is the **view grid the
        player sees** (both renderers, used to black out an unclaimed cell), while
        `R_Draw_Cell_Size()`/`R_Draw_Column_Split()` describe the **software draw window**,
        which still spans the screen in hardware mode. Placing a view with the wrong one of the
        two puts `view_window_x` at a negative value in OpenGL.
      - **The border tests had to become cell-relative**, the same trap the hardware renderer hit
        with `gr_viewwidth == view_span_w`: `rdraw_scaledviewwidth == vid.width` reads a
        half-width cell as a reduced view window, so `D_Display`, `R_FillBackScreen` and
        `R_DrawViewBorder` would paint a border around every quadrant. They ask
        **`R_View_Fills_Cell()`** now.
      - **The freelook aiming shift asked `cv_splitscreen`** (`R_SetupFrame`'s
        `dy = cv_splitscreen.EV ? rdraw_viewheight*2 : ...`). The view height is halved by the
        *view count*, and `cv_localplayers` halves it with that cvar still off — the same
        views-versus-local-players conflation that broke `G_BuildTiccmd`. It asks `D_NumViews()`.
      - **So did the weapon sprite's base centre**, and that one was visible from across the
        room: `R_DrawPSprite` (`r_things.c`) sets
        `vis->texturemid = cv_splitscreen.EV ? 120 : BASEYCENTER`. The 120 is a hand tuning for
        the **two-view** split, where the weapon is drawn at the *full screen* y scale inside a
        half height view — `pspriteyscale` keeps `vid.height` while `rdraw_viewwidth` is not
        halved for two views — so it has to be shifted up to sit right. A 2x2 cell is the other
        case: `rdraw_viewwidth` is halved with it, the weapon is scaled proportionally, and it
        wants the ordinary `BASEYCENTER`. It asks `(D_NumViews() == 2)` now, which is exactly
        the distinction the hardware renderer already draws with `atransform.splitscreen`.
        - Keyed on the cvar it was wrong in both directions: `cv_localplayers` puts four views
          on screen with the cvar **off**, and the Multiplayer menu turns the cvar **on** for a
          game that then runs as a 2x2 — shifting the weapon up by 20 base units, about 38px of
          a 384px cell, leaving the wrist ending in mid-air above the bottom of the view.
        - **Which route started the game therefore decided whether it looked right**, and that
          is why it survived the first round of testing: a headless run sets `cv_localplayers`
          straight from the config and never touches the cvar, so it takes the correct branch.
          **To reproduce anything the Multiplayer menu can cause, set `splitscreen 1` as well** —
          `M_StartServer` does, and `D_NumLocalPlayers` treats it as "at least two players".
        - Verified by screenshot at 1280x800 (the cabinet's `viewfit` class, unlike a 4:3 test
          resolution): with the cvar on and off the 2x2 weapon is now identical, the classic
          two-view split is **0 pixels changed**, and single player is untouched.
      - **The crosshair is per view now** (`HU_Draw_Crosshair`). It drew exactly two, the second
        only when `cv_splitscreen` was set, and both at `vid.width>>1` — the middle of the
        *screen*. That is right for the stacked halves, which span the full width, but a 2x2
        cell's centre is not the screen's, so a quadrant's crosshair would have sat on the
        boundary between two players' views. It loops over views, places each at its own cell's
        centre, and reads `cv_crosshair[D_Panel_Of(vind)]` so the setting follows the player's
        panel. One and two views are unchanged (the two-view crosshair sites are byte-identical).
      - **But the real reason panels 3 and 4 had no crosshair was that the cvar was never
        registered.** `menu_command_cvar_list` (`m_menu.c`) stopped at Player2 for **six** of the
        widened arrays: `cv_autorun`, `cv_crosshair`, `cv_alwaysfreelook`, `cv_mouse_move`,
        `cv_controlscheme` and `cv_customcontrols`. That list's own comment says it: *any cv_
        with CV_SAVE needs to be registered, even if it is not used.* Unregistered, the name is
        an unknown command, so the setting cannot be held, cannot be set, and is never written
        out — `config.cfg` reported `Unknown command 'crosshair3'` on every boot.
        - **The expensive one there is `cv_customcontrols[2]`/`[3]`**, which is where the guided
          setup stores a panel's ten keys. Unregistered means **the guided setup for panels 3
          and 4 could never be saved** — rebind them, quit, and the bindings are gone. Same for
          `cv_controlscheme[2]`/`[3]`. Worth re-running the guided setup for those panels now.
        - The six arrays that *were* fully registered are the ones covered by the
          `for( pind=0; pind<MAXSPLITSCREENPLAYERS; pind++ )` loop in `d_netcmd.c` (name, color,
          skin, autoaim, weaponpref, originalweaponswitch). The gap was everything registered by
          hand-written list entry instead of by loop. **When widening a per-player cvar, grep for
          its `[1]` registration** — the declaration being `[MAXSPLITSCREENPLAYERS]` proves
          nothing.
        - `cv_usemouse` stays `[2]` deliberately (two mouse devices), so the panel 3/4 entries
          for `cv_alwaysfreelook`/`cv_mouse_move` are kept out of the mouse init blocks, where
          the order of `cv_usemouse` registration matters.
      - **Auditing this by autoexec gives a false all-clear.** Querying each name from
        `legacyhome/autoexec.cfg` reported no unknown commands at all — because without `-warp`
        that run never reached `D_DoomLoop` to execute the file. The reliable signal is the
        **config load**, which names the line: `config line 350: unknown setting "crosshair3"`.
        Check the output has the lines you expect before concluding anything from its silence.
      - **One `cv_splitscreen` read remains, deliberately.** `st_stuff.c` hides the K/I/S overlay
        elements on that cvar, so they show or hide depending on which menu started the game.
        Left alone on the user's call.
      - **Pixel comparison here has a noise floor**, so read a small diff before believing it:
        the screenshot lands on tic 70 or 71 depending on timing, which moves monsters by one
        animation frame and ticks the HUD clock — about 6000 pixels of 1024000 at 1280x800.
        A real difference in this area shows up in the *weapon*; if the diff mask is monsters
        and clock digits only, nothing changed. Render the mask and look at it.
      - **A cell with no player is filled black** (`D_Display`). Three players use the 2x2 and
        leave one quadrant unused, and *neither* renderer clears it — the hardware per-frame clear
        (`HWR_ClearView`) is depth only, and the software renderer draws straight into the screen
        buffer — so it keeps the last thing drawn there, frozen, which reads as a crashed renderer.
        - **The empty cell is found by elimination, after the render loop**, exactly as the spare
          quadrant's rankings are. It used to black out `D_View_Cell(vind)` for each *pind with no
          player*, which is only the empty cell when the panels joined in order.
          `D_Set_View_Cell` is called only for pinds that actually joined and the rest keep the
          identity default, so with panels **1+3+4** the table reads `[0,2,3,3]`: the unjoined
          pind 3 reports cell 3, which is the **third player's** cell. That blacked out a view
          that had just been rendered — the quadrant went black with only its HUD surviving on
          top, since `ST_Drawer` runs later — while the genuinely empty cell 1 was never cleared
          and flickered with stale frames. Only 1+2+3 ever worked.
        - Done **after** the render loop, not inside it, so a cell can never be cleared before the
          player who owns it has drawn into it.
      - Sprite, psprite and corona clipping already clamp to `rdraw_viewwidth`
        (`r_things.c:1919/2193/2910`), so nothing can bleed into the next cell horizontally; the
        `vid.width` tests near `r_things.c:3529` are coarse off-screen rejects, with the fine
        clip after them. The half-*height* case has relied on exactly this for years.
      - Verified first by printing each view's draw window through a temporary console command,
        with `soft_grid` temporarily forced true so the OpenGL harness computed the *software*
        numbers — software mode could not be run at all at that point (see the sprite crash
        below). At 1024x768, four views: cells 512x384 at `0,0` `512,0` `0,384` `512,384`, each
        view's `columnofs` and `ylookup` confined to its own cell, and the highest byte written
        exactly `width*height*bytepp` — the buffer end, no overrun. One and two views unchanged.
        Once the crash was fixed this was confirmed by **screenshot**: four quadrants each with
        their own view and HUD, and with three players a black fourth quadrant.
      - The OpenGL path was required to stay **byte-identical**, and is: screenshots at 1, 2 and
        4 players differ in 0 of 786432 pixels. Three players differ only inside the
        bottom-right quadrant, which is now pure black instead of stale image.
    - Verified numerically rather than by eye, printing each view rectangle at 1366x768:
      1 view `0,0 1366x768`; 2 views `0,0` and `0,384`, each 1366x384; 4 views `0,0` `683,0`
      `0,384` `683,384`, each 683x384 — tiling the screen exactly. Three players give the first
      three of those.
    - **The HUD follows the same grid** (`st_stuff.c`). `SCY(y, y0)` used to halve for
      `cv_splitscreen` and add a row offset; it and `SCX` now both take the view's origin and a
      per-axis divisor, so a quadrant halves *both* axes and shifts into its column.
      `ST_overlayDrawer` takes a view index instead of a 0/1 "status position", and `ST_Drawer`
      loops over the views, taking each player from `localplayer[]`.
    - **The HUD art shrinks for a quadrant, and can be**: it is 320x200 base art multiplied by
      `vid.dupx`/`dupy`, not fixed-size, so halving that scale gives a quarter-screen view the same
      HUD-to-view proportions the full screen has. **Round the halved floats rather than dividing
      the integers**: at 1366x768 dup is 4,3, and integer halving gives 2,1 — art twice as wide as
      tall. Rounding gives 2,2. The hardware renderer uses the floats either way.
    - **Halve the global `vid` scale, not `drawinfo`'s copy of it**, and restore it at the end of
      `ST_overlayDrawer` (which has a single exit). `ST_drawOverlayNum` and `V_DrawScalePic_Num`
      call `V_SetupDraw` *themselves*, which re-reads `vid.dupx/dupy`, and `ST_drawOverlayNum` also
      uses `vid.dupx` directly for the digit advance. Halving only `drawinfo` was silently undone
      for exactly **health, ammo and armor**, while the keys — drawn without a re-setup — did
      shrink. That split symptom is the giveaway: **if some overlay elements scale and others do
      not, the ones that do not are re-entering `V_SetupDraw`.**
    - With the global scale halved, positions shrink with the art, so `SCX`/`SCY` take divisor 1
      for the 2x2 case; only the two-view split still passes `ydiv = 2`.
    - Digit width measured: 14px base art draws at `wfv` 56 for one or two views (`dup` 4,3) and 28
      for four (`dup` 2,2) — half, matching the half-width cell.
    - **The deathmatch rankings are per view too** (`hu_stuff.c`). `playerdeadview` and
      `hu_showscores` were single globals, so one player dying painted the score table across
      *everyone's* screen at full size — including in an ordinary two player game.
      `HU_Draw_DeathmatchRankings` now takes a view index, halves the scale and draws in that
      view's cell, and `HU_Drawer` asks per view via `HU_Rankings_For_View`: that view's own
      player's death, or that panel's own `gamecontrol_pl[vind][gc_scores]` key.
    - **The spare quadrant shows the rankings permanently.** Three players use the 2x2 grid and
      leave one cell unclaimed, which `D_Display` fills black — dead space on a screen where the
      score is the one thing everybody keeps wanting, and the only way to see it was to hold your
      own scores key and lose your view while you did.
      - **The empty cell must be found by elimination, not by looking up the missing player.**
        `M_Join_Start` assigns cells by *panel* once three or more join
        (`D_Set_View_Cell(i, joined_panel[i])`), so which quadrant is spare depends on which panels
        joined: 1+2+3 leaves cell 3, 1+2+4 leaves cell 2, 1+3+4 leaves cell 1, 2+3+4 leaves cell 0.
      - **`localplayer_cell[]` is only assigned for pinds that actually joined**, and the rest keep
        the identity default — so with panels 1+2+4 it reads `[0,1,3,3]`, and pind 2 (the real
        third player) and the unused pind 3 *both* report cell 3. Drawing at the missing player's
        cell therefore painted over a live view and left the genuinely empty quadrant black. Three
        of the four combinations were wrong that way; only 1+2+3 happened to work, which is exactly
        the one a quick test would try. `HU_Drawer` marks each live player's cell in a bitmask and
        draws in the one left over.
      - `HU_Draw_Rankings_In_Cell` takes the **cell**, not a local player index — the empty
        quadrant has a cell but no player to derive one from. It builds the table from
        `playeringame[]` and highlights `consoleplayer`, so it never needs the view's player.
      - `HU_Drawer` runs *after* the black fill in `D_Display`, so this paints over it rather than
        being erased by it.
      - Deathmatch only, like the rest of the block. A three player coop game still gets a black
        quadrant; the frag table would say little there anyway.
    - **`V_CENTERHORZ` must be off for the multi-view case, and this is a trap worth knowing.**
      It puts its centering into `drawinfo.start_offset`, and in hardware mode **`V_DrawString`
      does not apply that** — it positions text as `x * drawfont.fdupx0` with no start offset,
      while `V_DrawScaledFill` and the patches do apply it. The two therefore disagree by exactly
      `start_offset`. At full scale that is 43px and nobody noticed; with a half-width block it
      becomes 363px, which threw the ranking text clean off the left of the screen while the
      colour bars stayed put. The block is placed by explicit coordinates instead, so text and
      fills share an origin. **Any overlay mixing `V_DrawString` with fills or patches under
      `V_CENTERHORZ` has this bug latent in it.**
    - **The float and integer scales must be pinned equal too.** Fills and patches position by the
      integer `dupx0`, `V_DrawString` by the float `fdupx0`; 2 versus 2.13 slid a name 35px off
      the colour bar it belongs on — more than half the bar's width. `vid.fdupx/fdupy` are set to
      `(float)vid.dupx/dupy` for this overlay; 6% of scale is the cheaper loss.
    - Offsets are derived from pixels, not as multiples of `BASEVIDWIDTH`/`BASEVIDHEIGHT`: with
      `dup` rounded to 2 a 200 unit block is 400px tall in a 384px cell, so stepping a row by
      `BASEVIDHEIGHT` would push the lower row off the screen bottom.
    - **`WI_Draw_Ranking` takes a `y_limit`.** It used to stop on `y >= BASEVIDHEIGHT` — "do not
      draw past the bottom of the screen" — which is right for the intermission but wrong for a
      block offset into a viewport cell: a bottom-row cell starts at base y 277, already past 200,
      so it drew **one row and broke out**. Sorted highest-first, that left only the leader
      showing, which reads as "the players on 0 points are missing". The overlay passes
      `offy + BASEVIDHEIGHT`; every other caller passes `BASEVIDHEIGHT` and is unchanged.
      **Any drawing offset into a cell has to re-examine limits expressed in base units.**
    - Verified numerically at 1366x768, with fill and text positions printed side by side and
      required to match. Two views: both at x 522, rows at y 194 and 578 (cells 0..384, 384..768).
      Four views: both at x 180 and 864 (cells 0..683, 683..1366), rows at y 170 and 554. Only the
      dead player's view draws: killing player 0 produced a table for view 0 alone.
    - Only the 2x2 grid rescales. **The two-view split still draws full size art at halved
      positions**, deliberately: that layout is measured and working, and rescaling it would move
      everything. Verified byte-identical — `lowerbar_y` is still exactly 319 for the upper half.
    - Verified numerically at 1366x768, four views: cells 683x384, `lowerbar_y` 350 (top row) and
      734 (bottom), health x at 106/789, ammo at 499/1182 — every element inside its own cell,
      with `dup=2,2`.
  - **There are no dep files for most objects** (`svn1749/dep` holds about 16), so **a header edit
    does not trigger a rebuild**. This bit during this work: `console.o` kept a stale
    `gamecontrol` symbol and failed at link. After editing any header, `make clean && make`.

- **Player config for panels 3 and 4** (`m_menu.c`). Options → Player now lists four, and Options →
  Setup Controls four guided setups. **No menu graphics were needed**: that operator route is plain
  text (`"Player3 config >>"`). The `M_SETUPA`/`M_SETUPB` patches ("SETUP PLAYER 1/2") belong to the
  upstream *Two Player Game* and *Multiplayer* screens, which the lockdown hides from players, and
  those were left alone.
  - Very little had to change, because `M_SetupMultiPlayer_pind(pind)` already repointed every
    per-player cvar from the arrays. Panels 3 and 4 needed their own thin entry points and entries
    in `M_SetupMultiPlayer[]`, `M_Setup_P_Controls[]` and the three label tables, all widened to
    `MAXSPLITSCREENPLAYERS`.
  - `M_Guided_Start` and `M_Setup_P*_Controls` took `(pind == 0) ? gamecontrol : gamecontrol2`;
    they index `gamecontrol_pl[pind]` now.
  - **`cv_usemouse` stays `[2]`** — there are two mouse devices, not four — so the three mouse rows
    (Use Mouse, Mouse Move, Always MouseLook) are **hidden for panels 3 and 4** rather than
    inventing a `use_mouse3`. An arcade panel has no mouse anyway. The compiler caught this as an
    out-of-bounds read; the other per-player cvars had already been widened.
  - **Options → Setup Controls carries both rows per panel**: *Guided setup Pn* and
    *Player n Controls >>*. The guided setup only teaches the ten controls a standard six button
    panel needs, so a panel with more buttons binds the rest on the full per-action page — which is
    the whole reason the second row has to exist for 3 and 4, not just 1 and 2.
  - **`MControlMenu` is now addressed by position**, which it explicitly was not before, so its
    indices are named in an enum (`mcontrol_*`) and the lockdown uses those. Keep the enum in step
    with the array.
  - **The binding page names the panel it is editing.** Its header was
    `controls_player ? "PLAYER2" : "PLAYER1"`, so panels 3 and 4 both announced themselves as
    PLAYER2 — exactly the confusion this screen must not create while an operator is assigning
    buttons. It prints the number now.
  - Entries for panels that do not exist are hidden in **`M_Configure`**, keyed on
    `cv_localplayers`, for the usual reason: config.cfg is not loaded when `M_Init` runs.

- **Join screen** (`m_menu.c`, `M_Join_*`, `JoinDef`). After the skill is chosen and before the
  game starts, each control panel presses fire to be counted in. Laid out as the view grid it is
  about to become, so a player presses and watches **their own cell** claim itself.
  - **`cv_jointime`** ("jointime", default 20s, `CV_SAVE`) is the countdown, and
    **`cv_localplayers`** the panel count — both operator settings under **Options → Arcade
    Options**, so only a `-devmode` session writes them. `jointime 0` or a single panel
    skips the page entirely and the game starts exactly as it always did.
  - **A player's identity follows their panel too.** `cv_playername[N]`, `cv_playercolor[N]` and
    `cv_skin[N]` (`name`/`name2`..`name4`, `color`..`color4`, `skin`..`skin4`) are **panel N's**
    name, colour and skin, which is what Options → Player → "Player N config" has always edited.
    `Send_NameColor_pind` reads them through `D_Panel_Of(pind)`.
    - It used to index them by pind. `SV_commit_player` hands out pind sequentially, so with
      panels **2+3+4** joining, the player standing at panel 2 became pind 0 and was announced as
      "P1 Doomguy" — panel 1's identity at the second station. On a cabinet where each panel is a
      fixed station, and especially one painted to match its player's colour, identity has to
      follow the panel.
    - **The OnChange callbacks had to be inverted with it.** `cv_playername[1]`'s callback means
      "panel 2 changed", and the local player at panel 2 is not pind 1 once the panels join out of
      order. `Send_NameColor_panel` maps through **`D_Pind_Of_Panel`**, the inverse of
      `D_Panel_Of`, which skips slots with no player — `localplayer_panel[]` keeps its identity
      default for pinds that never joined, so with 2+3+4 it reads `[1,2,3,3]` and the unjoined
      pind 3 would otherwise answer for panel 4 instead of pind 2, who is actually playing there.
    - It also makes the config load safer than before: those callbacks fire long before any game
      exists, and `localplayer[]` starts `{255,255,255,255}`, so `D_Pind_Of_Panel` finds nobody and
      no netxcmd is sent with no server to receive it.
    - Verified by simulating the join assignment for all fifteen panel combinations: every live
      player takes their own panel's identity, the panel→pind round trip agrees, and no two
      players ever claim the same one.
  - **Which panel drives a player is a different question from which cell they occupy**, and
    conflating the two was a bug. `D_View_Cell(pind)` is the screen cell; `D_Panel_Of(pind)` is the
    physical panel, and `G_BuildTiccmd` indexes `gamecontrol_pl[]` by the latter. A lone player who
    joined at panel 3 gets the whole screen (cell 0) but must still be driven by panel 3's buttons
    — with one table the compact layout overwrote the panel with the cell, so joining at panel 3
    handed you player 1's controls and the panel you were standing at did nothing.
  - **Single Player starts on the first press** (`join_first_press_starts`), with no countdown to
    sit through: there is nobody else to wait for, and the page's value on that route is letting a
    lone player claim the panel they are standing at. Multiplayer still waits.
  - **One or two players get the big layout** — the whole screen, or the stacked halves — however
    far apart their panels are; only three or more use the 2x2. Keeping a player in their own
    panel's cell only earns its keep once the grid is in use anyway: with two players top and
    bottom there is nothing to be confused about, and a quarter screen each is a poor trade.
    `M_Join_Start` assigns cells by join order when `joined <= 2` and by panel otherwise.
  - **Both routes into a game must be hooked.** The cabinet's New Game menu offers *Single Player*,
    which ends at `M_ChooseSkill` → `G_DeferedInitNew`, and *Two Player Game → Start Game*, which
    goes through `M_StartServer` and issues its own command sequence. Hooking only the first left
    the two player route skipping the page entirely. `M_Join_Open` therefore takes a **completion
    callback** rather than game parameters, and each route supplies its own starter
    (`M_NewGame_Go`, `M_StartServer_Go`).
  - Nobody pressing starts panel 1 alone rather than dead-ending on the page. Use/open from a panel
    that is already in starts immediately, so a ready group need not sit out the countdown.
  - **The page does not appear until `cv_localplayers` is raised**, which is correct but reads as
    the feature being broken: it ships at 1, and one panel has nothing to ask. Set *Control Panels*
    under Options → Arcade Options in a `-devmode` session, which is the only session that saves.
  - **Keys are read before `M_Cabinet_Menu_Key`**, in `M_Responder`'s `ev_keydown`. That
    translation turns every panel's buttons into the same cursor keys, which is exactly the
    identity this page needs; taken afterwards, every panel would look alike.
  - **The page owns a `join_active` flag rather than testing `currentMenu`.** `M_Clear_Menus`
    leaves `currentMenu` pointing at the page it closed, so a `currentMenu` test kept firing the
    countdown every tic *after* the game had started — `G_DeferedInitNew` once per tic, for ever.
  - Single Level sets `D_Set_Join_Count(1)` and resets the cells: it is a scored single player mode
    and must not inherit a join from an earlier multiplayer game.
  - **Clamp the cell to 0 when only one view is drawn.** A lone player at panel 2 holds cell 1, and
    unclamped that still set `row = 1`, offsetting the HUD into a half of the screen that is not
    being drawn.
  - Verified headless by driving the page through temporary console hooks: panels 1+3 give 2 views
    (stacked halves, cells 0/1); panel 2 alone gives 1 view full screen; panels 1,2,4 give 4 views
    at cells 0,1,3 with the bottom-left empty; all four give cells 0..3. Nobody pressing yields one
    player; `localplayers 1` and `jointime 0` both skip the page.

- **Two player mode is folded into the panel count.** There used to be a separate on/off operator
  setting, `cv_twoplayer` ("twoplayer", default On), beside `cv_localplayers` — and the two said the
  same thing twice. **A cabinet with one set of controls is exactly a cabinet with two player mode
  off**, and because they were independent they could contradict each other: `twoplayer "Off"` with
  `localplayers "4"` hid the Multiplayer entry on a cabinet with four working panels, and nothing
  reconciled them. `cv_twoplayer` is **removed**; the test is now `cv_localplayers.EV < 2`, and
  "Control Panels" is the single row that decides it.
  - When it reads 1, a player sees neither *Multiplayer* on the New Game menu nor *Player2 config*
    under Options → Player — exactly what `twoplayer "Off"` did.
  - Applied in **`M_Configure`, not `M_Init`** — `config.cfg` is not loaded until well after
    `M_Init` runs, so the value there would always still be the compiled default. Same rule as the
    game selector and the "Read This!" hiding.
  - Hiding *Multiplayer* removes the only player-reachable route to `TwoPlayerDef`; the other entry
    point, `MultiPlayerMenu[0]`, is already behind the hidden Networked Multiplayer item.
  - **Old configs carry a stale `twoplayer "On"` line.** With the cvar gone `M_Verify_Config` names
    it as an unknown setting at startup and moves on — harmless, and it disappears the next time a
    `-devmode` session saves the config. The tracked `cabinet/legacyhome/config.cfg` has had the
    line removed already.
  - Verified headless by reading `SingleMulti_Menu[singlemulti_multi].status` back through a
    temporary console command: `18` (`IT_PATCH | IT_CALL`, shown) with `localplayers "4"`, `144`
    (`IT_HIDDEN`) with `localplayers "1"`.

- **Two players can be split side by side instead of stacked** (`cv_splitvertical`, "2 Player
  Split" on the Arcade Options page, `Top/Bottom` or `Side by Side`, default `Top/Bottom`). Two
  players only: three or four are the 2x2 grid either way, and one player has the whole screen.
  - **Why it is worth a setting.** The stacked halves are 1366x384 each on this cabinet — a
    letterbox slot that crops away what is above and below, which is where the things shooting at
    you are. Side by side gives each player 683x768: the full height of the corridor, cropped left
    and right instead. Which reads better depends on the monitor the cabinet was built around, so
    it is an operator setting rather than a change of default.
  - **"Two views" no longer says which way they are cut**, and that is the whole shape of the
    change. Every placement decision used to be written inline, once per call site, as some
    variant of `col = (num_views >= 4) ? (cell & 1) : 0; row = (num_views >= 4) ? (cell >> 1) :
    cell`. There were nine of those. They all moved behind four helpers in `d_clisrv.c`:

    | helper | answers |
    | --- | --- |
    | `D_View_Grid(&cols, &rows)` | how many columns and rows the screen is carved into |
    | `D_Cell_Pos(cell, &col, &row)` | where a given cell sits in that grid |
    | `D_View_Cell_Pos(vind, &col, &row)` | where a *view* is drawn — the panel's cell, not the join order, clamped to 0,0 for a single view |
    | `D_View_Squash()` | 0 none, 1 half height, 2 half width |

    A cell is `vid.width / cols` by `vid.height / rows`, and that is the only sizing rule left.
    The callers are both renderers' viewports, the software draw tables (`R_Set_View_Window`), the
    HUD overlay, the deathmatch rankings, the crosshair, the black fill of an unclaimed cell in
    `D_Display`, and the join screen's boxes. **Add a layout and you change `D_View_Grid`, not
    nine call sites** — which is the reason to do it this way even though only one new layout
    exists.
  - **The projection is the part that is not just geometry, and getting it wrong is not subtle.**
    A 2x2 cell keeps the screen's own aspect ratio, so halving both axes leaves the field of view
    unchanged and simply draws it smaller. **A side-by-side cell is not that case**: it is full
    height. It has to keep the *full width* projection and crop what it shows to left and right,
    exactly the way the stacked halves keep the full height projection and crop above and below.
    - Halved instead — the obvious reading of "it is half the screen, so halve it" — the world
      zooms out to half scale and the vertical field of view opens from 58 degrees to about 96.
      Undistorted, and completely wrong: sprites at half size and a fish-eye.
    - Software: `vid.fit_width` is taken from `fit_ref_width`, which **undoes** the halving for
      this layout only. Written as an undo rather than as `vid.width` so a single view at a
      reduced `cv_viewsize` is untouched. `case 2` of the `viewfit` switch has to use the same
      width, or the two axes disagree and the image stretches.
    - Hardware: a third `gluPerspective` case in `SetTransform` (`r_opengl.c`). The stacked half
      is `gluPerspective(53.13, 2*ASPECT_RATIO)` — half the vertical fov (53.13 = 2*atan(0.5), whose
      tangent is exactly half of 90's) at double the aspect, so the horizontal fov is unchanged.
      Side by side is the mirror: `gluPerspective(fovxangle, ASPECT_RATIO/2)`. It needs no
      `fov == 90` guard, unlike the stacked case whose 53.13 is hand fitted to that one fov.
    - `FTransform_t.splitscreen` **carries a value now, not a flag** (0/1/2, from
      `D_View_Squash`). The old `atransform.splitscreen = (D_NumViews() == 2)` could not say
      *which* way. Only `r_opengl` reads it; the dead Glide/D3D/miniGL backends never did.
    - The **weapon sprite's nudge is stacked-only** in both renderers, for the same reason — it
      compensates for a weapon drawn at the full screen height inside a half height view. Keyed on
      `D_View_Squash() == 1`, replacing `D_NumViews() == 2` in `r_things.c` (`vis->texturemid`) and
      `hw_main.c` (`ty -= 20`).
    - **A side-by-side view needed the opposite correction, and did not have one.** `pspriteyscale`
      is derived from the view *width* — `(vid.height * rdraw_viewwidth / vid.width) /
      BASEVIDHEIGHT` (`r_main.c`) — which is right only while the view's height is proportional to
      its width. A full screen and a 2x2 cell are; a side-by-side view is **half width and full
      height**, so the weapon is drawn at half scale while `centerypsp` still sits at half the
      *full* height, and it floats a quarter of the view above the floor. Software renderer,
      weapon top..bottom against the bottom of the view:

      | layout | view | before | after |
      | --- | --- | --- | --- |
      | single | 320x200 | 138..**200** | unchanged |
      | 2x2 | 160x100 | 69..**100** | unchanged |
      | side by side | 160x200 | 119..**150** (50px short) | 169..**200** |
      | stacked | 320x100 | 68..130 (overhangs) | unchanged |

      Fixed in `R_DrawPlayerSprites` by anchoring `centery` to the bottom of the view rather than
      its middle — `rdraw_viewheight - BASEYCENTER * pspriteyscale` — which is arithmetically the
      value `centerypsp` already holds for a full screen and for a 2x2, so it is applied only to
      `D_View_Squash() == 2` and the working cases are not touched. It is proportional, so it was
      wrong at **every** resolution, not just the low ones: verified landing exactly on the view
      bottom at 320x200, 512x384, 640x480, 800x600 and 1024x768.
    - **The stacked halves are deliberately left on their hand-tuned 120.** They are the opposite
      error — full scale in a half height view, so the weapon overhangs the bottom by 30px and is
      clipped. The same anchor would seat them properly (`centery` 0, weapon 38..100 of 100), but
      it would also show the *whole* weapon in a half height view where a cropped one has been
      shipping. That is a change to how the cabinet looks rather than a fix, so it needs a decision
      rather than a commit.
    - **The hardware renderer is a different mechanism and is believed unaffected.**
      `HWR_DrawPSprite` builds its quad in 320x200 **base** coordinates (`BASECENTER_Y - ty`) and
      lets the transform map them onto the view, rather than positioning in pixels against a
      width-derived scale — which is consistent with it needing the same stacked-only nudge and no
      side-by-side one. Not measured; confirm on the cabinet before assuming.
  - **The HUD overlay needed its art scale separated from its layout scale.** They had always been
    the same thing: `ST_overlayDrawer` halves the global `vid.dupx/fdupx` for a quarter-screen
    view, and `SCX`/`SCY` read those same globals for positions. That works while every cell is
    either the full screen or half of it in *both* axes. A side-by-side cell is half width and
    full height, and no single scale can be both.
    - `SCX`/`SCY` now take **a scale, not a divisor**: `x0 + (int)(x * xsc)`, with
      `xsc = fdupx / cols` and `ysc = fdupy / rows`, taken before the art scale is halved. For the
      three layouts that already existed this is the same arithmetic to the pixel — checked at
      1366x768 for y = 1, 4, 11, 21 and 198, where the old truncate-then-divide and the new
      divide-then-truncate agree.
    - The art halving is keyed on the cell being half the screen **wide** (`cols >= 2`), because
      that is what the overlay runs out of room in: the 320 unit layout has to fit across the
      cell. A stacked half is full width and keeps full size art, as it always has.
  - **The rankings block is centred vertically in its cell now**, clamped at zero. A cell can be
    taller than the block for the first time: 200 base units at `dupy` 2 is 400px, which
    overflows a 384px stacked half or quadrant (so the clamp keeps those exactly where they were)
    but leaves the block floating in the top half of a 768px side-by-side cell.
  - **The join screen lays its boxes out the same way**, so the page says which half is yours
    before the game starts.
  - Verified headless under `SDL_VIDEODRIVER=offscreen` at 1024x768, by screenshot, in **both**
    renderers — the geometry is renderer-specific, so one of them proves nothing about the other:
    - Side by side draws two full-height views, correct undistorted geometry (ceiling and floor
      both in frame, which is the visible difference from the stacked half), a complete half-scale
      HUD inside each half, and the weapon at the bottom centre of each.
    - **One view and the 2x2 grid are pixel-identical with the setting on and off** (`magick
      compare -metric AE` → 0), which is the check that the setting cannot reach the layouts it
      is not meant to touch.
    - Against a binary built from the previous commit: one view and software stacked are
      pixel-identical; GL stacked and the 2x2 differ **only** in the level clock digits and one
      moving monster — the same difference two runs of the *same* binary produce, which is the
      control that says it is run variance and not the change.
    - Deathmatch rankings, the K/I/S block and the ammo breakdown all sit inside their own half.
    - `splitvertical 1` typed at the console mid-game re-carves the screen immediately, which is
      the `CV_CALL` → `R_SetViewSize` path the menu row uses; and a `-devmode` session writes
      `splitvertical "Side by Side"` back to `config.cfg`.
