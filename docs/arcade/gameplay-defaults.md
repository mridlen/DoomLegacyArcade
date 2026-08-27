# Gameplay defaults that diverge from upstream

*Part of the DoomLegacy arcade cabinet build. Read before changing weapon switching, deathmatch defaults, or any gameplay cvar default — several are demo-header sensitive.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

- **Deathmatch defaults** — a DM round gets a time limit from **`cv_dm_timelimit`** ("dmtimelimit",
  default 5 minutes, `CV_SAVE`), and coop explicitly clears the limit, appended to the game-start
  command in `M_StartServer` (`m_menu.c`).
  - **It is cleared in `Command_ExitGame_f`**, beside `cv_splitscreen` and for the same reason.
    Nothing used to clear it, so after a deathmatch the HUD clock kept counting *down* over the
    attract demos — a five minute DM timer on a single player recording. It read as a broken clock;
    it was a limit left behind by a game that had already ended. Starting a single player game
    appeared to fix it, because `G_DeferedInitNew` issues `timelimit 0` in its own setup line,
    which is why it came and went. Setting the cvar is what matters: `TimeLimit_OnChange` derives
    `timelimit_tics`, and that is what `st_stuff.c` reads.
  - **`cv_timelimit` is the engine's limit and is rewritten at every game start**, so the Net
    Options menu row could never edit it usefully: whatever was typed there was overwritten before
    it was read, and the row then displayed the 0 that a coop start had left behind — "it says 0
    and does 5 minutes whatever you set". The row edits `cv_dm_timelimit` now, which is only ever
    *read*, so it keeps what the operator set. Same split as `cv_deathmatch_menu` versus
    `cv_deathmatch`, and the same reason.
  - The limit still cannot simply be `cv_timelimit`'s default: that would cut **single player**
    levels short too, which is why it is applied per game start. An unattended cabinet has no other way out of a DM stalemate: players cannot reach
  End Game, and the idle timeout only fires when *nobody* is touching the controls. Applied per
  game start rather than as the `cv_timelimit` default, which would also cut single player levels
  short. In `deathmatch_cons_t` the DM modes are values **1..4**; every coop variant is 0 or ≥0x10.
  - The mode itself already defaulted to what was wanted: `cv_deathmatch_menu` ("dmm") is `"3"` =
    **`DM_both`**, items respawn *and* placed weapons respawn immediately.
  - **`Deathmatch_OnChange` is now called unconditionally from `G_InitNew`** (`g_game.c`), which is
    what actually made respawn work. It is the function that derives `deathmatch`,
    `weapon_persist` and `cv_itemrespawn` from `cv_deathmatch` — but as an OnChange it only ran
    when the cvar *changed*, and **`CV_Set` returns early on an unchanged value**
    (`command.c:1582`). Meanwhile `HS_Apply_Ranked_Ruleset` pins `cv_itemrespawn` back to 0 on the
    way out to the attract screen (it is in `hs_ranked_rules[]`). So the **first** deathmatch after
    boot had item respawn and every **later** one at the same setting silently did not — which
    reads exactly like "the default is wrong". Verified headless: with `+deathmatch 3` already set
    and `+respawnitem 0` forced afterwards, `G_InitNew` now recovers `itemrespawn=1`,
    `weapon_persist=1`, while single player still reports `itemrespawn=0`.
  - **This is the general trap, not a one-off**: any cvar whose OnChange has side effects on *other*
    cvars will silently skip them when re-set to the value it already holds. If something else pins
    those other cvars in between (the ranked ruleset does), they stay pinned. Re-apply the mapping
    at the point of use rather than relying on the change notification.
- **Vanilla weapon switching is the default** — `cv_originalweaponswitch[]` ("originalweaponswitch",
  `originalweaponswitch2/3/4`) now defaults **On** for all four panels, and `cv_weaponpref[]`
  defaults to **`045628137`** instead of upstream's `014576328`.

  Upstream's default is a **power ranking** — ssg > rocket > plasma > chaingun > shotgun > bfg >
  chainsaw > pistol > fist. That is sensible for *pickups* (grab a BFG, wield it) and dangerous on
  *ammo-out*: running the chaingun dry handed the player a **rocket launcher**, which killed them
  at point blank more than once on the cabinet.
  - **Vanilla does not rank by power.** Its ammo-out chain (`p_pspr.c`, the `else` branch of
    `P_CheckAmmo`) is **plasma → SSG → chaingun → shotgun → pistol → chainsaw → rocket → BFG →
    fist** — rocket 7th and BFG 8th of 9, deliberately near the bottom so running dry never puts
    explosives in your hands. This is also Boom/PrBoom/DSDA's default `weapon_preferences`, i.e.
    the order speedrunners actually play.
  - **`weaponpref` is a lookup table, not an ordered list** — indexed by weapon, valued by
    priority, **higher wins** (`FindBestWeapon`, `p_inter.c:350`, keeps the max). Ports that
    express this as an ordered array of weapon ids cannot be transcribed position-for-position.
    The string looks scrambled because **`weapontype_t` puts the SSG last at index 8**, after
    chainsaw — not in the middle where its keyboard slot suggests. Order is
    fist, pistol, shotgun, chaingun, missile, plasma, bfg, chainsaw, ssg.
  - **One string serves every game.** `FindBestWeapon` carries the same gamemode guards vanilla
    does (`p_inter.c:343-348`): SSG is skipped for non-`doom2_commercial`, plasma and BFG in
    shareware. So `045628137` gives vanilla's order under Doom II, Plutonia and TNT *and* under
    Ultimate Doom with the SSG simply absent. No per-game value is needed, and there is nowhere to
    put one — the cvar is per **panel**, not per game.
  - **`originalweaponswitch` overrides `weaponpref` entirely.** With `GF_original_weapon` set,
    `favoritweapon[]` is never read — not by the ammo-out fallback (`p_pspr.c:329`) and not by
    either pickup path (`p_inter.c:619`, `655`). It is On by default, so the pref string is a
    fallback for an operator who turns it off, not the live setting.
  - **The two settings cannot both be satisfied by one string**, which is why upstream's default
    is the way it is: DoomLegacy collapses ammo-out fallback *and* pickup auto-switch into one
    ranking, while vanilla keeps them separate (pickup **always** switches, ammo-out uses the
    fixed chain). `originalweaponswitch` On restores both vanilla behaviours at once; the pref
    string can only have one or the other.
  - **Demo-safe, and this was checked rather than assumed.** None of these three are in the demo
    header — they ride in the tic-0 **`XD_WEAPONPREF` netxcmd** (origweaponswitch byte, 9-char
    pref string, autoaim byte — exactly `Send_WeaponPref_pind`'s buffer), written into the demo by
    `AddLmpExtradata` and applied on playback by `Got_NetXCmd_WeaponPref`. Playback therefore uses
    the **recorded** value and ignores the live cvar, so changing the defaults cannot break an
    existing demo. Verified two ways against `doomu_ep1_sk3_speed.lmp`: flipping all three cvars
    in `config.cfg` gave **0 differing tics over 2925** of `-synclog`, while flipping the same
    three bytes *inside the demo file* diverged at **tic 107** — the second run being the
    sensitivity control that proves the first was capable of detecting a change. Old demos also
    predate the change harmlessly, and `G_Downgrade` forces `GF_original_weapon` for
    `version <= 109` anyway, so stock IWAD attract demos were never affected either way.
  - Not in `hs_ranked_rules[]`, so changing it does not void a run's score — it is a control
    preference, not a difficulty knob.

- **"Original Weapon Switch" has a menu row now**, on Options → Player → *PlayerN config*
  (`PlayerOptionsMenu`), directly above the WeaponPref row it gates. The cvar existed but was
  **console-only** — there was no way to reach it from any menu, and no `extern` for it in
  `d_netcmd.h` either (added).
  - **Devmode-only, like the row it gates.** The lockdown already hid `playeroption_weaponpref`
    (a player on a joystick cannot type digits), and the new row is hidden beside it.
  - **`PlayerOptionsMenu` is addressed by position**, so the `playeroption_*` enum gained
    `playeroption_origweapon` in step with the array — same discipline as every other menu in this
    file. Its two `IT_CALL` handlers ignore their `choice` argument, so no handler needed updating.
  - **WeaponPref greys out while the switch is On**, since it then changes nothing.
    `M_Draw_PlayerOptions` wraps `M_DrawGenericMenu` and sets the row's status each frame, so it
    tracks the toggle **live** — the operator moves the row above and watches this one grey. Safe
    from a drawer because it is idempotent (same status recomputed from the same cvar every
    frame), the rule the attract score pages already follow. It reads the cvar back out of the
    row above's `itemaction` rather than indexing by panel, so it cannot drift out of step with
    `M_SetupMultiPlayer_pind`.
  - **`IT_DISABLED` is deliberately not used, and this is a trap worth knowing.** It is
    `(IT_SPACE | IT_GRAYPATCH)`, and the `IT_GRAYPATCH` drawer needs either `FontBBaseLump` —
    Heretic's `FONTB_S`, which **does not exist under Doom**, so the lump check yields 0 — or a
    `mip->patch`, and these small-font rows carry neither. On this cabinet it would draw
    **nothing at all** and leave a blank 16px gap. `IT_WHITESTRING` (type `IT_SPACE`, so still
    unselectable) is the grey small font and is what "greyed out" means on a `V_DrawString` page.
    **The existing `IT_DISABLED` rows elsewhere — the Single Level "Watch run" items — are blank
    gaps for exactly this reason**, which is fine there because the intent was only to hold the
    page height.
  - The cursor is moved off the row when it greys, since `M_Responder` only steps over `IT_SPACE`
    on a keypress and would otherwise leave the marker parked on a row that cannot be chosen.
  - Measured against the real `STCFN` lumps at the page's `x` of 27: `"Original Weapon Switch"` is
    **157px**, ending at 184, and the widest value `"Off"` is 24px right-justified at
    `BASEVIDWIDTH - x` = 293, so it starts at 269 — **85px clear**. The page ends at y 156 of 200
    with every row shown (`PlayerOptionsDef.y` 40, `STRINGHEIGHT` 10, and the `IT_CV_STRING`
    WeaponPref row costing 26 rather than 10).
  - **The usual `config.cfg` caveat applies and it is the whole point of the change**: the saved
    file overrides the compiled default, and only a `-devmode` session writes it. The tracked
    `cabinet/legacyhome/config.cfg` has all eight lines updated, but a live untracked
    `svn1749/bin/legacyhome/config.cfg` still carries the old values — `make` stages with `cp -n`
    — so it needs the lines edited by hand or a re-save from `-devmode`. Same trap as the
    `overlay` element letters, twice.

- **`cv_fragsweaponfalling` defaults On** (`p_inter.c`), so killing the player holding the rocket
  launcher leaves it for whoever did it — how modern arena shooters behave, and what makes a
  cabinet deathmatch flow. It only fires when a **player** dies (`target_player`), so single play is
  unaffected in practice: a death there ends the run and reloads the level.
  - **Added to `G_demo_defaults()`** as a result. It is not in the demo header and was not in that
    function either, so it is exactly the case the rule below warns about: stock IWAD demos contain
    player deaths — the Ultimate Doom E4M2 one does — and were recorded without weapon dropping, so
    replaying them with it on spawns a weapon the recording does not have and desyncs from there.
    Playback pins it off; live play uses the new default.
  - Not in `hs_ranked_rules[]`, so it does not affect whether a run scores. It is a deathmatch
    setting and the ruleset governs single player scoring.

- **Nightmare's fast monsters only did half its job: fast fireballs, ordinary-speed demons.**
  `FastMonster_OnChange` (`p_enemy.c`) has two halves, and only one of them was running.
  - The first half walks `MonsterMissileInfo[]` and rewrites `mobjinfo[].speed` for every monster
    projectile — imp/caco fireballs, revenant tracers, baron shots. It is unconditional and always
    worked. This is why the bug was easy to miss from the cabinet: nightmare *felt* faster, because
    everything being thrown at you was.
  - The second half is the classic Doom trick of **halving the state tics of `S_SARG_RUN1` ..
    `S_SARG_PAIN2`**, which is the only thing that makes demons and spectres move and bite at
    double rate. Sargs have no ranged attack, so this is their entire share of "fast monsters".
  - With `MBF21` defined (`doomdef.h`) that half is compiled as the MBF21 variant, which only
    touches states carrying **`FRF_SKILL5_FAST`** — and that flag was *only* ever set from a DEH
    `SKILL5FAST` patch (`dehacked.c`). Nothing preset it on the vanilla SARG states, so with a
    plain IWAD no state carried it and the loop ran over zero states. The function's own comment
    names the preset as its requirement; it had simply never been written.
  - Fixed by presetting the flag over that state range in **`P_PatchInfoTables`** (`infoext.c`),
    inside the existing `#ifdef MBF21` block. That is the right home for it: it runs after
    `P_clear_all_state_ext` has zeroed every `state_ext_id`, and **before dehacked is applied** —
    the file says so in as many words — so a WAD's own DEH frame flags still override it, which is
    what MBF21 requires (`dehacked.c` clears `FRF_SKILL5_FAST` before setting from the patch).
  - **A second bug sat behind the first.** The restore branch read
    `fast_active & (frf & FRF_SKILL5_MOD_APPLIED)` — a **bitwise** `&` between `fast_active`, which
    is 0 or 1, and `FRF_SKILL5_MOD_APPLIED`, which is `0x0002`. `1 & 2` is 0, so the restore never
    ran. Turning fast monsters back off would have left the tics halved and the applied flag set,
    so demons stayed fast for the rest of the session and could never be re-halved. It was
    invisible only because nothing ever set `FRF_SKILL5_FAST`, so the halving never happened
    either — fixing the preset alone would have shipped a permanent-fast-demons bug. Now `&&`.
  - Verified headless with a temporary `GenPrintf` in `FastMonster_OnChange` printing `set_fast`,
    `states[S_SARG_RUN1].tics`, `states[S_SARG_ATK1].tics` and the frame flags. On `-skill 5`:
    `set_fast=1 SARG_RUN1.tics=1 SARG_ATK1.tics=4 frf=0006 TROOPSHOT.speed=20` (was 2 / 8 / 0000 /
    10). An `autoexec.cfg` of `wait 70 / fastmonsters 0 / wait 35 / fastmonsters 1` round-trips
    2→1→2→1 cleanly, which is the `&&` fix. **The control matters**: rebuilding with the preset
    disabled reproduces the reported symptom exactly — `set_fast=1`, `TROOPSHOT.speed=20`,
    `SARG_RUN1.tics=2`. Fast fireballs, slow pinkies.
  - **No existing recording is invalidated — measured, after two wrong guesses.** The first
    instinct was "every nightmare demo now desyncs", and the second, after seeing stale values in
    the demo headers, was "no, the header pins `fastmonsters` to 0 so none of them do". Both were
    reasoning from the header without replaying anything, and both were wrong. What settles it is
    that **the fix only touches `S_SARG_*` state tics, so a demo desyncs only if it reaches a map
    that actually contains demons or spectres.**
  - Verified by A/B replay: a build with the fix reverted versus the current one, the same demo,
    `-synclog` on both (`g_game.c`, `G_Synclog_Tic`), diffing the two `synclog_play.txt`.
    - `doom2_MAP01_sk4_speed.lmp` — **bit-identical**, all 507 tics. MAP01 has 0 sargs on UV/NM.
    - `doomu-sl_E1M1_sk4_speed.lmp` — **bit-identical**, all 731 tics. E1M1 has 0.
    - `doomu_ep1_sk4_speed.lmp` — the **scored portion is bit-identical**. Its three level segments
      are 739 + 2770 + 968 tics, and 739 + 2770 = 3509, exactly the `doomu E1M1 E1M2 4 speed 3509`
      row on the run board. Divergence begins 94 tics into the *third* segment, E1M3 — the first
      episode-1 map with sargs (7 demons + 2 spectres). That tail is play past the recorded finish
      and scores nothing.
    - Counted straight out of the IWAD `THINGS` lumps with the skill-4/5 flag (`0x04`): E1M1 and
      E1M2 have none, E1M3 has 9, and E1M4-E1M9 have 11/26/42/8/28/29.
  - So the run board and high score table stand as they are, and nothing needs clearing. The
    general rule worth keeping: **`cv_fastmonsters` in a demo header proves nothing about whether
    this fix moved that demo — the map's sarg count does.**

- **`cv_tall_monsters` ("tallmonsters") restores vanilla infinitely tall things, and is the new
  default.** Game Options row **"Monster Height"**, values `Infinite` (1, default) and
  `Over-Under` (0).
  - **What it fixes.** Vanilla Doom things are infinitely tall: a thing blocks another thing
    whatever their relative heights, so you can never stand on a monster. Legacy adopted Heretic's
    over-under passing for the Doom player **unconditionally** — `MT_PLAYER` carries
    `MF2_PASSMOBJ` in its `info.c` entry and the check in `PIT_CheckThing` (`p_map.c`) was gated on
    nothing else. So the player could climb on top of monsters and end up somewhere vanilla cannot
    reach; the lift by the E1M2 exit is the reported case. There was no setting either way, in the
    menus or on the console — checked against all 235 `consvar_t` definitions in the build.
  - **Only the player was affected.** Of the 306 thing definitions in `info.c`, exactly two in the
    Doom table carry `MF2_PASSMOBJ` — `MT_PLAYER` and `MT_POD`. Everything else holding it is in
    the Heretic table. Doom monsters therefore never climbed each other; only you climbed them.
  - **There are TWO `MF2_PASSMOBJ` gates and both must be closed. Gating only one is the mistake
    that shipped first**, and the cabinet still got stuck on a monster on the E1M2 lift with
    `tallmonsters "Infinite"` correctly set in `config.cfg`:
    - `PIT_CheckThing` (`p_map.c`) decides whether a thing may **move into** another's space.
    - the `MF2_PASSMOBJ` branch of `P_MobjThinker` (`p_mobj.c`) decides whether a thing may **come
      to rest on top of** one. This is the branch that actually parks you there:
      `mobj->z = onmo->z + onmo->height; mobj->flags2 |= MF2_ONMOBJ;`
    Both now call **`P_Mobj_Pass_Over_Under()`** (`p_map.c`), the single answer, so they cannot
    drift apart again. Heretic is exempt inside that one function (`EN_heretic_hexen`), since
    over-under is core there — its imps and wizards fly, and the block below the first gate holds
    the "don't let imps/wizards fly over other imps/wizards" special cases.
  - **How the player got up there with horizontal blocking working, which is the part worth
    remembering.** A moving sector does **no thing-vs-thing collision at all**:
    `P_ThingHeightClip` (`p_map.c`) calls `P_CheckPosition` only to recompute floor and ceiling
    heights, **discards its return value**, and then assigns `thing->z = thing->floorz`. So a lift
    raises the player straight through a monster's z-range without any move ever being blocked.
    Once the two overlap, the `P_MobjThinker` branch sees a gap of ≤ 24 units and lifts the player
    onto the monster's head — it does not require the player to have arrived from above.
    - Vanilla behaves the same way up to that point: a moving sector *can* push things into
      overlap. What vanilla will not do is let you stand on one. Closing the second gate is
      therefore the complete vanilla behavior, not a patch over a symptom — the player ends up at
      the lift's floor height, overlapping the monster, and simply walks out.
  - **Why this is now unreachable rather than merely unlikely.** `p_mobj.c` line ~1993 is the
    **only** assignment in the entire tree that sets a thing's z from another thing's top (grep
    `onmo->z + onmo->height`), and it is inside the gated branch. The other "standing on a thing"
    mechanism, `tmr_floorthing`, is dead code for `demoversion >= 145` — `PIT_CheckThing` returns
    before reaching it. With Infinite there is no path left that can hold a player up on a monster.
  - Verified with a passive counter on the branch — no forcing the player anywhere, which would
    only have tested recovery rather than prevention. Replaying the *same* demo with only the
    header's monster-height byte flipped: **Over-Under enters the branch 29 times, Infinite enters
    it 0 times.**
  - **Confirmed on the cabinet**, on the E1M2 lift that produced the original report, after the
    second gate was closed. The first fix passed every headless check it was given and still failed
    on hardware, because those checks only ever exercised the gate it had closed — worth
    remembering before calling a collision change verified.
  - **In `hs_ranked_rules[]` pinned to 1**, beside the other vanilla difficulty knobs. It changes
    the simulation, so a run played with over-under is not comparable to one played without.
    Game Options is reachable by players on the locked-down cabinet (only its Network Options link
    is hidden), so a player *can* flip it — and doing so drops the run to UNRANKED and raises
    `M_Draw_Unranked_Warning`, which is the intended outcome. `HS_Apply_Ranked_Ruleset` forces it
    back to Infinite at boot for a non-`-devmode` session.
  - **Demo safe, and no version bump needed.** Recorded into the 64-byte option area of the
    demo144 header at **byte 44** (the first free slot after `cv_viewheight`), which was previously
    zero-filled. 0 reads as `Over-Under`, which is exactly how every demo recorded before the
    setting existed was played, so old demos need no version test — and `G_demo_defaults()` sets
    `cv_tall_monsters.EV = 0` for demos too old to reach the field at all. Verified across all 74
    demos in `legacyhome/demos`: every one reads byte 44 = 0, and the `0x55` sync mark still lands
    at option-area byte 64, proving the header length is unchanged.
  - **Menu geometry measured, not guessed.** `GameOptionDef` is x=60, y=40, `STRINGHEIGHT` 10, and
    `IT_CVAR` values are right-justified ending at `BASEVIDWIDTH - x` = 260. `"Monster Height"` is
    **104px** against the real `STCFN` lumps, ending at 164; the wider value `"Infinite"` is 52px
    so it starts at 208 — **44px clear**. The row goes after "Solid corpse", putting the last
    `IT_STRING` row (Drown) at y=140, still clear of the `IT_YOFFSET` "Adv Options >>" at 160.
    The lockdown indexes this array **from the end**
    (`GameOptionsMenu[ GameOptionDef.numitems - 1 ]`), so inserting a row in the middle is safe —
    and nothing else indexes it by number.
  - **Verified headlessly** with a temporary one-shot in `P_Ticker` that spawned a demon at the
    player's feet, put the player on its head, and called `P_CheckPosition` under both values:
    `Infinite=0 Over-Under=1` — the engine rejects the position under the new default and allows it
    under the old one. Header round-trip proven both directions: `G_BeginRecording` writes the live
    value into byte 44, and playing back demos hand-patched to 0 and to 1 read back
    `cvar=0` / `cvar=1`.
