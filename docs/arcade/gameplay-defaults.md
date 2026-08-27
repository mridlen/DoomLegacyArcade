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
  - **Existing nightmare recordings desync.** `cv_fastmonsters` is already in the demo header, so
    nothing needed adding there — but the *meaning* of the flag has changed, and `demoversion` is
    the engine `VERSION`, with no sub-version to bump. Demos recorded on nightmare before this fix
    played back with slow sargs and now play back with fast ones. Only skill 4 is affected; every
    other skill records `fastmonsters 0` and is untouched. The nightmare entries on the run board
    and high score table were also set against slow demons, so they are easier than the build now
    allows and are worth clearing along with their `.lmp` files.
