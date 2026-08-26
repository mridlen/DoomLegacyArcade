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
