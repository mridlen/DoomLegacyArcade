# The status bar overlay elements

*Part of the DoomLegacy arcade cabinet build. Read before changing `ST_overlayDrawer` or the `overlay` cvar element codes. Note a saved `config.cfg` overrides the compiled default string.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **Kills/items/secrets on the HUD** (`st_stuff.c`, `ST_overlayDrawer`). The engine's status-bar
  overlay is driven by the `overlay` cvar, a **string of one-letter element codes** — stock
  `"kahmf"` is keys/ammo/health/armor/frags. Upstream already had `e` (kills) and `s` (secrets)
  but neither was in the default and there was **no items element**; `i` is new. The three are
  stacked top-right at `SCY(1/11/21)` with `K`/`I`/`S` labels. This pairs with the high-score
  **max** category, which needs 100% kills and secrets, so the player can see whether the run is
  still eligible. The compiled default is now **`"kahmfeist"`** — `eis` for these three plus `t`
  for the level clock below.
  - The overlay only draws when **`st_overlay_on`**, which `R_SetViewSize` (`r_main.c`) sets from
    `cv_viewsize.value == 11` — the largest view size, no status bar. At any smaller viewsize the
    classic status bar draws instead and none of this appears. The cabinet's `config.cfg` is at
    `viewsize 11`.
  - Skipped in splitscreen, like the upstream `e`/`s` cases: `killcount` is per player while
    `totalkills` is the map's, so one corner cannot speak for both. High scores are single player
    anyway.
  - **Skipped during `demoplayback` too.** The three exist so a *player* can see whether their own
    run is still eligible for the max category; on the attract screen they are somebody else's
    counters cluttering the corner of a screen that is meant to look inviting. The condition is
    shared as **`ST_KIS_ON`** so the three rows cannot drift apart — they are one block and must
    appear and disappear together. Like the gameplay-message suppression in `console.c` this covers
    the Single Level "watch run" replays, which are the same thing: a recording, not your run. The
    level clock `t` deliberately stays, since it reads as part of the demo.
  - **`config.cfg` overrides the compiled default**, and only devmode rewrites it, so changing the
    default in `st_stuff.c` does nothing on a machine with an existing config — the saved
    `overlay` line has to be edited (or re-saved from a `-devmode` session) as well.

- **Small status numbers when the view is too small for the tall ones** (`ST_overlayDrawer`,
  `compact_hud`). Element *positions* come from the layout scale `xdiv`, which a view grid divides
  by the number of columns. The digits are drawn at the *art* scale `vid.dupx`, which is
  `vid.width / 320` — an **integer**, and it floors at 1. Above 640x480 the two move together; below
  it they part company, and in a 2x2 the layout keeps shrinking while the 14x16 `STTNUM` digits do
  not. Measured, three-digit values, top-left cell:

  | mode | health | ammo | armor | |
  | --- | --- | --- | --- | --- |
  | 320x200 2x2 | **-17**..25 | 75..117 | 108..150 | 9px overlap, health off the left edge |
  | 400x300 2x2 | **-11**..31 | 104..146 | 145..187 | 1px overlap |
  | 512x384 2x2 | -2..40 | 145..187 | 198..240 | 11px clear |
  | 640x480 2x2 | 8..50 | 192..234 | 258..300 | the layout as designed |

  The tightest pair is **ammo against armor** — 66 base units apart with three digits to fit
  between — so that is the test, and it lands exactly where the overlap starts. `STYSNUM` is
  **4x6** against `STTNUM`'s **14x16**, is already cached for the classic status bar, and the same
  `ST_drawOverlayNum` takes it, so three digits become 12 pixels instead of 42.
  - **The icons go with the tall digits.** `SBOHEALT` and friends are 16 wide and cannot be drawn
    smaller either; three of them is a fifth of a 160 pixel cell, and the armour one lands past the
    right edge and into the next player's view. Position carries the meaning instead, as it does in
    the stock bar — health left, ammo middle, armour right. The blue-armour cue is lost in a
    compact view; there is nowhere to put it.
  - **A full screen view is never compact, at any resolution.** There `xdiv` and `vid.dupx` agree
    and the layout is the 320x200 one it was drawn for — so single player at 320x200 keeps the tall
    digits and the icons, which is the vanilla HUD and fits by construction. Verified unchanged at
    every resolution tested, and unchanged for 2x2 at 640x480 and above.
  - The K/I/S block, the ammo breakdown and the run total are `ST_SOLO_HUD` already, so they never
    appear in a split view and needed nothing. The **level clock** did benefit for free: at
    `CLK_DY` 9 below `lowerbar_y` it was inside the 16-tall digits in a 2x2 at 320x200 and now
    clears them.
  - Verified by measurement, not by eye: a probe printing each element's span at 320x200, 400x300,
    512x384 and 640x480 for one, two and four views, then a screenshot diff against the previous
    build. The status row differs by 59% of its pixels while **both 3D view areas are pixel
    identical**, which is also what proves the run pair was deterministic enough for the comparison
    to mean anything.

- **Key icons never step by less than they are wide** (`ST_drawOverlayKeys`). Same cause, separate
  symptom: the step is `(ST_KEY_WIDTH + 1) * vid.fdupx` — the *layout* scale — while each key is
  drawn at the art scale. In a 2x2 at 320x200 that is a **3 pixel step for a 7x5 patch**, so the
  three keys are drawn on top of each other. Floored at the patch size, taken from the patch rather
  than assumed: `ST_KEY_WIDTH` is 6 but `STKEYS0` is **7** wide. Floored at the width and not
  width+1 deliberately, so the normal scales are untouched — there the step already equals it
  (7*fdupx against a 7 wide patch, 14 against 14 at 640x480).

- **Blue armour gets its own overlay icon** — **`SBOARMBL`**, added to `legacy.wad`, drawn by the
  `m` element in place of `SBOARMOR` when `armortype >= 2`. Green absorbs a third and blue a half,
  so 100 green points are worth much less than 100 blue ones and the bare number could not say
  which. `p_inter.c` tests `armortype == 1` for the weaker case, so `>= 2` is blue and also picks
  up the megasphere.
  - **The lump is a Doom patch, while every stock `SBOxxxx` icon is a `pic_t`**, and the two need
    different drawers — `V_DrawScaledPatch_Num` versus `V_DrawScalePic_Num`. Rather than require
    the artwork be converted, the format is detected once at load: a `pic_t` always has **byte 2 of
    its header zero** (`r_defs.h` calls that field out as exactly this autodetection hook), while in
    a `patch_t` those bytes are the height, never 0 for a real icon. Read with `W_ReadLumpHeader`,
    not `W_CacheLumpNum`, so the test stays clear of whatever caching the patch path then does.
  - `W_CheckNumForName`, not `W_GetNumForName`: a `legacy.wad` without the lump keeps one armour
    icon instead of failing to start.
- **Gameplay messages are off the HUD when the screen is shared, or during a demo** (`console.c`,
  the `gameplay_msg` block). A pickup line belongs to whoever triggered it but is painted across
  the top of the *whole* screen: on a splitscreen or 2x2 cabinet it covers someone else's view, and
  on the attract screen it is somebody else's pickups scrolling over a shop window. Forced to
  `viewnum = 5` for `D_NumViews() > 1` or `demoplayback` — the console-only path the
  `cv_showmessages` test already used, so nothing new had to be invented and the messages are still
  in the console and the log. The demo case covers the Single Level "watch run" replays too.
  - `HS_DemoLabel` still draws on the **second** text line even though the first is now free
    during playback; left there so the caption does not move.
  - Single player is untouched, and this is separate from `cv_showmessages`, which still works
    normally for one player.
  - **`HU_SetTip` is a different path and unaffected**, which matters: the idle-timeout countdown
    uses it and must keep showing.
  - Verified by counting suppressions: with a cheat generating messages, one player suppressed 0,
    four players 3, splitscreen 4; and an attract cycle suppressed 20 demo messages while a normal
    single player game suppressed none.
- **…and off the HUD entirely by default, full screen included** — **Arcade Options → Messages +
  Banners → Singleplayer Messages / Multiplayer Messages** (`cv_msg_singleplayer` /
  `cv_msg_multiplayer`, `msg_singleplayer` / `msg_multiplayer`, `CV_SAVE`, both default **Off**,
  defined in `hu_stuff.c`). Nobody at a cabinet reads a pickup line or a deathmatch kill line; they
  only cover the view. Same block in `console.c`, same `viewnum = 5`, so they still reach the
  console and the log.
  - **Which switch applies is `HS_Scored_Game()`**, deliberately the scoring's own definition of
    "single player": Campaign and Single Level solo (bots included) read the single player switch;
    deathmatch, co-op and anything with more than one person reads the multiplayer one. Two
    definitions would drift.
  - **Only `EMSG_playmsg`/`EMSG_playmsg2`.** Pickups, locked doors (`PD_BLUEK` etc., msglevel 31)
    and the obituaries in `p_inter.c` all arrive that way. `EMSG_hud` — pauses, players joining or
    leaving — is not touched. Note the locked-door line goes too; the *oof* sound stays.
  - **Layered on the rules above, not replacing them**: with Multiplayer Messages On, a split
    screen or a demo still hides them. On matters for a single view in a multiplayer game — a bot
    game, or a linked cabinet with one player.
  - **Not gameplay**: routing only, no `PP_Random`, not a `NETVAR`, no demo header byte.
  - `cv_showmessages` still applies underneath (the cabinet config has it at `Verbose`).
  - Verified headlessly with temporary routing output and a `kill` from `autoexec.cfg` (the
    obituary is an `EMSG_playmsg`), solo and `-deathmatch`, each switch alone: every case went to
    the HUD only when its own switch was On. **The scratch config needs `localplayers "1"`** — the
    cabinet's is `"4"`, which makes it a shared screen and hides everything, and the first pass of
    this test read as "the switch does nothing".

- **Ammo breakdown on the HUD** — element code **`b`**, new, so the compiled default is now
  **`"kahmfeistb"`**. All four ammo types with their maximum, in the small font up the right hand
  side above the keys: `BULL 200/400`, `SHEL`, `RCKT`, `CELL`. The stock `a` element shows only the
  *ready weapon's* count in the big status numbers, which says nothing about what is worth picking
  up — that was checked before writing a new one.
  - Row order is the panel's, not the enum's. `ammotype_t` runs clip, shell, **cell, misl**, so the
    rows are listed explicitly rather than looped over the enum.
  - **Drawn at half scale**, the same size the 2x2 HUD uses, by the same mechanism and with the
    same trap avoided: halve the **global** `vid` scale (not `drawinfo`'s copy, which the draw
    calls re-read), halve the floats and round the integers to them so a 4,3 dup gives 2,2 rather
    than 2,1, re-issue the element loop's `V_SetupDraw`, and restore immediately after so the
    elements that follow are untouched.
    - **The column positions are computed from `SCX(318)` taken at the *outer* scale.** The block
      still sits against the right edge of the screen; only the glyphs shrink. Computing them
      after the halving would have put `SCX(318)` at half the screen width.
  - **Four fixed columns** — label, current, `/`, maximum — rather than one right-justified
    string, so the numbers line up down the block instead of the labels going ragged. The current
    count is zero padded *and* right aligned on its column: `hu_font` digits are **not** fixed
    width (`1` is 5px against `0` at 8), so padding alone does not align them.
  - Column widths in base units, measured against the real `STCFN` lumps: widest label 32
    (`BULL`/`RCKT`/`CELL`), three digits at their widest 24, `/` is 7 — 99 across with the gaps.
    Verified at 1366x768, 1280x800, 1024x768 and 640x400: the block clears the right edge by
    4..9px, clears the keys by 8..16px, and starts far below the K/I/S corner.
  - Shares **`ST_SOLO_HUD`** with the K/I/S block (renamed from `ST_KIS_ON` now that two things use
    it): solo, full screen, not a demo. All of these describe *your* run and say nothing useful
    about somebody else's recording on the attract screen.
  - Same `config.cfg` caveat as the rest: the saved `overlay` line overrides the compiled default,
    so an existing install needs the `b` added by hand or re-saved from a `-devmode` session. The
    tracked `cabinet/legacyhome/config.cfg` has it; a live `svn1749/bin/legacyhome/config.cfg`
    does not, because `make` stages that with `cp -n`.
  - **`ST_Check_Overlay_Elements()` now says so at startup**, because this caveat had already
    caught the level clock and then caught the ammo breakdown the same way — both times presenting
    as the feature being broken rather than unconfigured, which is slow to work out from outside.
    It compares the running string against `cv_stbaroverlay.defaultvalue` (not a hardcoded list, so
    the next element is covered for free) and names the missing letters and the value to set.
    Called from `D_DoomLoop` beside `M_Verify_Config`, for the same reason: at config load time the
    cvar has not settled. **Note `M_Verify_Config` cannot catch this** — the `overlay` line *did*
    take effect, it is simply short.

- **Level clock on the HUD** — element code **`t`**, new, so the default is now `"kahmfeist"`.
  Counts *down* the time remaining when a time limit is set and counts elapsed time *up* when one
  is not, so it serves both a deathmatch round and a speed run. Format `T 4:59`.
  - Drawn **low and left of centre on the status number row** — `SCX(CLK_CX - V_StringWidth/2)` at
    `lowerbar_y + CLK_DY*sf_dupy`, with `CLK_CX` 104 and `CLK_DY` 9. **Two thirds of the screen are
    unusable here, both learned by putting it there first:**
    - The **top** is covered by `HU_Drawer`'s pickup messages at y=0 — the same reason
      `HS_DemoLabel` sits at y=8. A top-left clock is invisible in play.
    - **Dead centre** is where the **weapon sprite** draws, in every mode. `CLK_CX` is 160 minus
      about nine characters (~6px average glyph width) to get out from behind it.
    - Along `lowerbar_y` the free span is **x 68..192**: health's number is right-justified ending
      at 50 with its 16px `SBOHEALT` icon at 52, and ammo's is right-justified at 234 with at most
      three 14px `STTNUM` digits, so it starts at 192. The widest string is `T 12:34` at 44px, so
      centred on 104 it spans 82..126 and clears health by 14px.
    - **`CLK_DY` is capped by splitscreen, not by the full screen.** `hu_font` glyphs are **7** tall
      (not 8 — measured). In the upper half `lowerbar_y` is 319 and the half ends at row 383, so
      +9 leaves the text bottom at 379 with 4px spare, while a full character down (+12) would
      bleed into player 2's view. Single player has more room but uses the same offset so the modes
      agree. Offsets scale by `sf_dupy` and are *not* halved for splitscreen, matching how
      `lowerbar_y` itself offsets from `SCY(198,y0)` — `SCY` already halves and adds `y0`, so each
      half gets its own copy.
    - Verified at 1366x768, text bottom vs limit: single 759/767, upper half 379/383, lower half
      763/767; x spans 350..537 against a health edge at 290 and ammo at 819.
  - **Not skipped in splitscreen**, unlike `e`/`i`/`s` — the clock belongs to the level rather than
    to one player, so it is correct in both halves, and two player deathmatch is exactly the case
    that wants it. `y0` already offsets it per half.
  - Reads `timelimit_tics` (`g_game.c`, externed in `d_netcmd.h`), which `TimeLimit_OnChange`
    derives from `cv_timelimit`; do not recompute from the cvar.
  - Same `config.cfg` caveat as above. This one **did** bite: the clock did not appear at all until
    the `t` was added to the saved `overlay` line, because the config value overrides the compiled
    default and only a devmode session rewrites it. The cabinet's line is now `"kahmfeist"`, but
    any *other* install still needs the letter added by hand.

- **Run total beside the level clock** — `T` is this level, **`TT`** the whole run so far, drawn as
  two stacked rows in two columns:

  ```
  TT 12:34
  T   4:59
  ```

  - **Folded into the existing `t` element rather than given a letter of its own.** A new element
    code would need adding to every saved `overlay` line by hand, and that caveat has already bitten
    twice in this file — the level clock itself did not appear at all until `t` was added to the
    cabinet's saved config. Sharing `t` means TT appears the moment the build is installed.
  - **Single Player only, on three separate grounds.** `HS_Scored_Game()` is the *meaning*: TT is
    the run's total, and a run only exists in the scored modes — in a deathmatch there is nothing
    for it to total. **`! HS_Single_Level_Run()`** narrows that from "scored" to Single Player: a one
    map run's total is always identical to the level clock directly beneath it, and two rows showing
    the same number is worse than one row.
    - **Testing `single_level_mode` directly was not enough**, and this shipped wrong: that flag
      belongs to a *live* game, set from the Single Level menu, and a demo replay never sets it — so
      TT appeared over the attract cycle's single level record demos, beside a level clock showing
      the very same number. `HS_Single_Level_Run()` answers for the replayed case too: during
      playback it reports "one map" unless the demo is a **Survival** record, which is the only kind
      that spans levels. A stock IWAD demo is one map as well and falls out correctly. `D_NumViews() == 1` is
    the *layout*: a split view has no room for a second row, which is the same limit that caps
    `CLK_DY` at 9 rather than a full character.
    - Verified by flipping `single_level_mode` at runtime through a temporary console command (the
      Single Level menu is the only normal way in and cannot be driven headlessly): `show_total`
      went 1 → 0 → 1 as the flag went 0 → 1 → 0, with `scored` and the view count unchanged
      throughout, so the new term is what moved it.
  - **TT goes above T, on `lowerbar_y` itself, because there is nothing below.** `lowerbar_y` is 16
    base units above y 198 and `CLK_DY` is 9, so the T row already ends at 198. The row above is
    free over the same x span: health and ammo bound this block *horizontally* (68..192), not
    vertically.
  - **Two columns: labels left aligned, times right aligned on a shared edge**, so the seconds line
    up under each other even when the minutes differ in width — which is the entire point of
    stacking them. Widths come from `V_StringWidth` at draw time, never assumed: `hu_font` is
    proportional and its digits are **not** fixed width.
  - **With no TT row the block is exactly the old string centred on `CLK_CX`**, because the label
    column is sized to the label actually in use (`"T "` vs `"TT "`). The splitscreen and deathmatch
    layout verified above is therefore untouched.
  - Measured rather than eyeballed, with temporary instrumentation reporting the computed spans and
    the real `V_StringWidth` values:

    | string | width |
    | --- | --- |
    | `TT ` | 20 |
    | `9:59` | 27 |
    | `99:59` | 35 |
    | `199:59` | 40 |
    | `999:59` | 43 |

    Worst realistic case is a run past an hour and a half: block width 20+40 = **60**, centred on
    `CLK_CX` 104 giving **74..134** — inside the free span 68..192, clearing health by 6px and ammo
    by 58px. Vertically at `sf_dupy` 3: TT spans 712..733 and T 739..760 against a 768 limit, a 6px
    gap between the rows and 8px below.
  - `HS_Cumulative_Tics()` is the run's time *before* the current level — `HS_LevelExit` folds
    `leveltime` in at the exit — so the HUD adds the live `leveltime` to it. It is maintained during
    demo playback too, so a record demo's replay shows its own running total.

## The status digits overlapped in OpenGL, and only in OpenGL

Reported as "in 1366x768 in OpenGL, the health bar digits overlap each other sometimes". They did,
by 30% of a digit, and the "sometimes" was simply one digit versus three.

`ST_drawOverlayNum` steps from digit to digit by `wf * vid.dupx` and then draws each one with
`V_DrawScaledPatch`. **Those are two different scales, and which one they are depends on the
renderer.** `V_SetupDraw` puts `vid.dupx` — the whole number — into `drawinfo` for
`V_SCALEPATCH`, and the software drawers use it; but `V_DrawScaledPatch` in hardware mode hands
straight off to `HWR_DrawPatch` (`hardware/hw_draw.c`), which scales the quad by
`drawinfo.fdupx` — the **exact fraction**. So the advance was the whole number while OpenGL drew
the fraction, and the digits ran into each other by the difference.

Software was right the whole time, by accident: there the two numbers are the same one.

| mode | `dupx` | `fdupx` | STTNUM advance | STTNUM drawn | overlap in GL |
| --- | --- | --- | --- | --- | --- |
| 320x200 | 1 | 1.00 | 14 | 14.0 | none |
| 640x480 | 2 | 2.00 | 28 | 28.0 | none |
| 1024x768 | 3 | 3.20 | 42 | 44.8 | 2.8px |
| **1366x768** | **3** | **4.27** | **42** | **59.7** | **17.7px, 30% of a digit** |
| 1920x1080 | 6 | 6.00 | 84 | 84.0 | none |

It was always slightly wrong wherever `fdupx` had a fractional part, and it became gross when
*Choose the two 2D whole-number scales together* took `vid.dupx` at 1366x768 from 4 to 3 — which
is also why that commit's note that "the hardware renderer is untouched: it scales by the exact
fdupx/fdupy and never had this" was wrong. It never had the *software* symptom. It had this one,
and halving the whole number doubled it.

- **Fixed by advancing at the scale the digit is actually drawn at** — `vid.dupx` in software,
  `vid.fdupx` in hardware, the same `sf_dupx` split `ST_overlayDrawer` already uses for text a few
  lines above. Software is bit identical: there the expression evaluates to `wf * vid.dupx` as
  before.
- The minus sign steps by `8 * art scale`, and 8 is right — `STTMINUS` is measured **8x6**.
- The pickup flash behind the number (`V_DrawVidFill`) was sized `wfv*3` by `hf*vid.dupy` and had
  the same split: `V_DrawVidFill` takes raw pixels in both renderers, so its height was the
  software one on a hardware screen. It follows the digits now.
- **Widening the digits is the risk this fix carries**, since they grow leftwards into the icon
  beside them. Measured rather than assumed, from the real lumps — `STTNUM` 14x16, `STYSNUM` 4x6,
  `SBOHEALT`/`SBOARMOR`/`SBOFRAGS`/`SBOARMBL` all 16x16. The tight pair is the ammo icon (ending at
  base 252) against the armor digits (ending at base 300): at 1366x768 in GL the icon ends at
  1075px and three armor digits now start at 1101px, 26px clear. Checked at every resolution in
  the table below.

### `tools/hudtext-test.py`

Neither this nor the centring bug below is visible to a headless run, and nothing else measures
either. The tool lifts `ST_drawOverlayNum`, the `HU_*` placement helpers and `V_Setup_VideoDraw`'s
scale derivation **verbatim out of the source by brace matching**, stubs `V_DrawScaledPatch` to
record where each patch lands and how wide it is drawn, and drives them over 16 resolutions in
both renderers. It runs in under a second and checks that

- consecutive digits abut exactly — no overlap, no gap — for both `STTNUM` and `STYSNUM`,
- the pickup flash matches the digits it sits behind,
- the digits clear the icon to their left and stay on the screen,
- a centred string is centred **on the screen**, and a layout row lands where the screen puts it.

`--selfcheck` reinstates each of the six real bugs and reports whether the check goes red. That is
not decoration: it caught a check of its own that could not fail. "PRESS FIRE TO START clears the
status bar" passes happily on the *old* code, because the old code drew the text far too **high** —
`160*dupy` down a screen that is `200*fdupy` tall. The property worth testing was that the row lands
where the layout puts it, not that it stays above something. **A clean result from a check never
shown to fail is not evidence.** → `screen-fill.md`

## The WINNING banner

`HU_Draw_Winning` (`hu_stuff.c`), called from `HU_Drawer`. Deathmatch only, and only while
`cv_winningbanner` (`winningbanner`, **Arcade Options → Messages + Banners → Winning Banner**, `CV_SAVE`, default
Rainbow) is not Off. Not a NETVAR: it only draws.

The setting has three values, `CV_WinningBanner[]` in `hu_stuff.c`: **Off**, **Rainbow** (the
per-letter ripple, what the banner has always done) and **Cycle** (the whole word one colour at a
time).

**No entry in a `PossibleValue` list may repeat a value, and getting this wrong breaks the menu
rather than the cvar.** This first shipped with a fourth `{1,"On"}` entry, so that a `config.cfg`
written while this was an on/off cvar would still load rather than be refused by name.
`CV_set_str_value` handled it perfectly and `CV_get_possiblevalue_string` still displayed
"Rainbow" — every check run at the time passed. But `CV_ValueIncDec` (`command.c`) finds the
current entry by scanning the list for a matching value and keeping the **last** match, and says
so in a comment: *"this code do not support more than same value for differant PossibleValue"*.
So Rainbow resolved to index 3, not 1, and the arrow keys stepped from there — right to `Off`,
left to `Cycle`, and right from `Cycle` onto the duplicate `On`. Three values, six apparent
behaviours, and the cvar itself was never wrong.

The lesson is about *where* to test: loading a value and drawing it is not the same as **walking
the list with the arrow keys**, which is the only thing that reads the list positionally. An old
config saying `"On"` is now refused by name, which leaves the cvar at its registered default — 1,
Rainbow, exactly what `"On"` meant — at the cost of one `M_Verify_Config` complaint until that
machine next saves a `-devmode` session.

- **Who**: `HU_Winning_Leader` — the unique top of `ST_PlayerFrags` over the players in the game,
  or with `teamplay` on the unique top of `HU_Create_TeamFragTbl` (so the team number is the skin
  colour in colour teams, the skin in skin teams). A tie, or fewer than two players/teams, is
  nobody: nothing is drawn at 0-0.
- **Size**: the level clock's. `ST_overlayDrawer` divides the global `vid.dupx/dupy/fdupx/fdupy`
  by the column count before drawing, so in a 2x2 (the cabinet's layout) or side by side the clock
  is half size; the banner makes the same change and restores it on its single exit. The first
  version drew at the full scale, which matched the clock in a stacked split (both 7 x 3.84 = 27px
  in GL at 1024x768) and was twice its height in the 2x2. **Measure glyph heights with a lenient
  colour test**: the font darkens towards the bottom, and a "bright red" filter clipped the clock
  to 19px against the banner's 27 and made two equal sizes look different.
- **Where**: centred in each winning view's own cell (`D_View_Grid` + `D_Cell_Pos`), in pixels
  under `V_NOSCALE` as the clock is, one text line down (`8 * ` the *full* art scale, since the
  pickup messages above it do not shrink with the grid) — the first line is where pickup messages
  print, the same reason `HS_DemoLabel` sits at y 8. A view showing the rankings (its player is dead, or holding scores) is skipped, since
  the rankings cover it anyway. If the team string is wider than the cell (three or four columns)
  it drops to the bare `WINNING`: the colour still names the team.
- **Team play ignores the setting.** Cycle applies to the plain-deathmatch banner only; with
  `teamplay` on, the colour is what says *which team* is winning, which is the point of the
  banner, so it is not the setting's to repaint.
- **Individual colour cycle** (Rainbow): each letter is drawn separately through one of six colormaps that
  map the font's red ramp 176..191 onto a palette hue ramp — red, orange, yellow, green, blue,
  magenta, read out of PLAYPAL, with each ramp's near-white start skipped. The letter's colour is
  `(n - gametic/3) mod 6`, so the colours march along the word. Fixed per-colour buffers, because
  the OpenGL patch cache is keyed by colormap pointer.
- **Whole-word colour cycle** (Cycle): `HU_Draw_Cycle_String`, the same six ramps in the same
  order, but one map for the entire string, `(gametic / HU_CYCLE_TICS) mod 6`. **The rate had to
  differ from the rainbow's.** The rainbow steps every 3 tics and reads as motion because adjacent
  letters already differ; a word that changes colour *all at once* every 3 tics is a 12Hz flash
  across the top of the view. `HU_CYCLE_TICS` is 10 — 3.5 changes a second, slow enough to read
  the word as a colour rather than as flicker. Both drawers fall back to `V_WHITEMAP` on a
  non-Doom palette, where `HU_Rainbow_Map` returns NULL.
- **Team colour** comes from `M_Skin_Font_Map` (`m_menu.c`, made public for this), the join
  screen's map: font red onto the sprite green ramp, then through the skin translation, so the
  text is the exact shades the team's sprites are drawn in. Skin teams (`teamplay 2`) have no
  colour and draw grey.
- Both maps return NULL outside the Doom palette (Heretic), and the text falls back to grey.
- Drawing only; nothing here touches game state, so demos are unaffected.

**Testing it headlessly** needs someone to be ahead, which `kill` provides: it credits the victim
with a self-frag. `tools/shotsheet.py --args "-deathmatch -splitscreen" --exec kill --exec "wait 40"`
leaves player 1 on -1 with the rankings up and player 2 leading with the banner. Add
`--cvar color=3 --cvar color2=8 --exec "teamplay 1"` (before the `kill`) for the team version.
`--args` and `--exec` were added to `shotsheet.py` for this. For the cabinet's 2x2, add
`--cvar localplayers=4 --cvar split4=Grid` and colours 3/8/3/8 for two teams of two; for a
free-for-all at half size use `--cvar "splitvertical=Side by Side"` (four players with one
self-kill is a three-way tie, so no banner). `--cvar winningbanner=Off` checks the switch.

### The crash it caused: team names that were never made

The first cabinet Team Deathmatch with the banner segfaulted at once in `get_team_name`
(`g_game.c`), called through `HU_Winning_Leader` → `HU_Create_TeamFragTbl`. Two faults, both
upstream, that the banner exposed by asking for team names **every frame from the first**:

- The bound was `team_num <= num_teams`. With no team created, team 0 — **Green** — read
  `team_info[0]`, which is NULL.
- No team had been created. Names are made only by `TeamPlay_OnChange`, and the crash dump had
  `teamplay` 1 ("Color") with `num_teams` 0, so on the menu's route it never ran. `M_Arcade_MP_Go`
  sets `netgame`/`server` before queuing `teamplay 1`, and a server's NETVAR change skips its own
  OnChange (`CV_Set`, `call_enable` 0) and relies on the broadcast. Not reproduced headlessly
  through the command line: `-teamplay`, and `teamplay 1` from the title under `-server`, both
  named the teams.

`get_team_name` now checks the bound and makes a missing name on demand from the same source
the OnChange uses. The old dead-player team rankings had the same exposure: "Unknown team" for
every team, and a crash if Green was playing.

**Reproduced deterministically under gdb** instead: a wrapper passed to `shotsheet.py --binary`
copies the real binary into the scratch directory and runs it under
`gdb -batch` with a breakpoint on `TeamPlay_OnChange` whose commands are `return` / `continue`,
so the teams are never named. With `-deathmatch -teamplay -splitscreen`, `color 0`, `color2 8`
the old binary crashes with the cabinet's exact backtrace; the fixed one draws GREEN TEAM /
BLUE TEAM and the banner. The wrapper needs a `legacyhome` beside it (a symlink will do),
because `shotsheet.py` takes the home from the binary's directory.
