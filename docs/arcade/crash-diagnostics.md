# Crash report and class-list black box

*Part of the DoomLegacy arcade cabinet build. Read before changing `d_crash.c`/`d_crash.h`, the
`D_Crash_*` call sites (`p_tick.c`, `p_enemy.c`, `g_game.c`, `d_main.c`), or anything that adds a
thinker to a class-list.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

## Why it exists

On 2026-09-22 the Raspberry Pi test cabinet segfaulted half an hour into an untouched attract cycle,
in the E1M5 ITYTD speed record demo:

```
#0  PIT_FindTarget (mo=0x7f9aa56250) at p_enemy.c:1728      t2->target == mo
#1  P_LookForMonsters (actor=<zombieman>, allaround=false)
#2  P_LookForTargets / A_Look / P_SetMobjState / P_RunThinkers / P_Ticker
```

The zombieman was walking the MBF **friends list** (a hostile monster looks there for something to
fight). The list's only entry was not an object at all: it was a **door** thinker
(`TFI_VerticalDoor`), the newest thinker in the level, whose memory had once belonged to something
else. Its `cnext`/`cprev` both pointed at the list head -- the signature of being *inserted into an
empty list*, after the door existed. Past the door's small struct the "mobj" fields were a cached
graphic (a column-offset table, then palette bytes like `0x6A6A6A6A`), and that value happens to
carry both `MF_FRIEND` and `MF_COUNTKILL`.

(**Solved 2026-09-24**: the door was filed there by `P_AddThinker` itself, at the moment it was
created, from its memory's previous contents -- see *The cause, found on 2026-09-24* below. "After
the door existed" in the paragraph above was the wrong inference.)

What the core could not say is **who inserted it**. Everything after that was reconstruction:

- The simulation was ruled out as the whole story: the demo replays identically on the Pi and on
  the laptop (`-synclog`, byte for byte), and in a replay the friends list stays empty for the
  entire level -- rendered, threaded, with and without the chase camera, ten runs on the Pi itself.
- The demo had to be recovered from its header bytes, because `demoname` is `DEMONAME_LEN` (32)
  characters and every record demo's path is longer: the core held
  `/home/doom/DoomLegacyArcade/svn`.
- The terminal that launched it was still open, and was no help: it shows which demos played and
  nothing about the list.

So this records what was missing, and prints it at the moment it matters.

## What it does

**Crash report** (Linux/glibc only; `D_Crash_Init`, called after `AU_Init` once `legacyhome` is
known). A handler for `SIGSEGV`, `SIGBUS`, `SIGILL`, `SIGFPE` and `SIGABRT` writes, to stderr and
appended to `legacyhome/crash.txt`:

- the version, the signal, and the unix time;
- `gametic`, `leveltime`, `gamestate`, episode, map, skill, `demoplayback`;
- the **full** path of the demo playing (`D_Crash_Set_Demo`, from `G_DoPlayDemo`);
- a backtrace (`backtrace_symbols_fd`);
- the black box below;
- then it re-raises. `SA_RESETHAND` has restored the default action, so the process dies of the
  original signal and `systemd-coredump` keeps the core exactly as before -- the report is in
  addition to the core, never instead of it.

The handler uses only async-signal-safe calls (`write`, `open`, `close`, `time`, `raise`,
`backtrace*`). `backtrace()` is called once at install so its first-use `dlopen` of libgcc (which
mallocs) does not happen inside the handler, and it runs on a 64K `sigaltstack` so a stack overflow
still reports. Numbers are formatted by hand, not with `printf`.

**Class-list black box.** Two things, both logging only:

- **Anomaly check.** `P_UpdateClassThink` and `P_MoveClassThink` (when linking into
  friends/enemies) and `PIT_FindTarget` check that the thinker is an object
  (`D_Crash_Not_Object`: `TFI_MobjThinker`, `TFI_MobjNullThinker`, `TFI_BlasterMobjThinker`).
  Anything else prints a `CLASS-LIST ANOMALY:` line with the caller, as `binary(+0xoffset)` or
  `binary() [0xaddress]`. For a thinker already removed (`TFI_RemoveThinker`) its memory is still
  intact, so the line adds the object's type and health. Capped at 20 lines per session; the
  crash report prints the total.
- **Friends-list ring.** The last 32 additions to the friends list -- gametic, leveltime, thinker,
  its function, type and health, and the caller. `P_MoveClassThink` records only a thinker that was
  in no list before the call, so the frequent move-to-end calls do not wash real additions out.
  Friends are rare in a single-player game (the player below half health, which MBF moves to the
  front of the list; MBF friendly monsters), so 32 reaches a long way back.

**`Demo file:` line.** `D_Crash_Set_Demo` also prints the demo's full path at `EMSG_errlog` (the
terminal and `-logfile`, not the in-game console), next to the existing `Level:` line.

Resolve any address with `addr2line -f -i -e doomlegacyarcade ADDR`: the `+0x` offset for a
position-independent binary (the Pi's), the bracketed address otherwise (the laptop's). The binary
must be the one that crashed, built with `-g`.

## Nothing here changes the game

Every check is a read and a log; nothing is skipped, repaired or reordered, and there is no
`P_Random` anywhere in it. `make demotest` against a fresh baseline from the previous `main`
binary: **121 compared, 0 desynced**. Cost is a switch on a byte per class-list operation.

This is deliberate. Repairing the list (dropping the bad entry) would stop the crash and destroy the
evidence, and "forcing the bad state and checking recovery is not a test" applies: the job here is
to name the code that got there.

## How it was verified

Each part was shown to work *and* to fire, not just to stay quiet:

- **Crash report on a real fault.** `kill` at the title screen segfaults in `P_KillMobj` (a known
  stock bug, CLAUDE.md). With `autoexec.cfg` of `wait 35` / `kill`, the run exits 139 (still a
  segfault, core kept), and `crash.txt` holds the report; `addr2line` on the second frame gives
  `P_KillMobj p_inter.c:2178`.
- **The black box, against the Pi's own failure.** A throwaway build forced every new door into the
  friends list (`P_UpdateClassThink(&door->thinker, TH_friends)` after each door's
  `P_AddThinker`, behind an environment variable, reverted before commit). Replaying the E1M5
  speed demo, the log named the insertion at leveltime 647 and its caller, resolving to
  `EV_VerticalDoor p_doors.c:716` -- the injection line -- then `PIT_FindTarget` being handed the
  door. A `SIGSEGV` sent later produced a report whose ring shows that addition.
- **False positives.** All 121 record demos replayed with the anomaly log on. Exactly one logged
  anything, and it was real (next section).

## What it found on its first sweep

`doomu_ep1_sk4_speed.lmp` (Nightmare), E1M2 leveltime 1441:

```
CLASS-LIST ANOMALY: PIT_FindTarget on thinker ... (function 17, not an object) ... called from P_HelpFriend
CLASS-LIST ANOMALY:   it is a removed object: type 2 health -20
```

`P_HelpFriend` (MBF, `p_enemy.c`) walks the actor's own side's list and, for an ally under half
health that was just hit, calls `PIT_FindTarget( ally->target )`. That `target` was a **shotgun guy
corpse removed by Nightmare respawn** (`P_NightmareRespawn` removes the old body). With
`REFERENCE_COUNTING` compiled out -- MBF and PrBoom keep a removed object alive while anything
points at it; this engine does not -- `target` can outlive the object it names.

Here the object had been removed but not yet freed, so it was intact, had health -20, and
`PIT_FindTarget` rejected it harmlessly. **Once the memory is freed and reused**, the same call
reads garbage, and garbage that passes `PIT_FindTarget`'s checks reaches
`P_MoveClassThink( garbage )`, which picks a list from the garbage `flags`. That is precisely the Pi
core: a door in the friends list. And whether the memory has been reused by then depends on
allocation history, including what the renderer happened to cache -- which is why the same demo
plays identically on both machines and faults on only one.

**Superseded (2026-09-24): this was not the Pi crash** -- see the next section. The stale `target`
is still real and still unfixed, but it did not put the door in the list. What follows is kept as it
was written. It was the leading explanation, **not a proven one**: E1M5 on ITYTD has no respawn,
and the E1M5 replays never took this path. If it happens again, the anomaly lines and the ring will say. A fix
belongs in its own change and has to keep demos in sync -- the candidates are turning
`REFERENCE_COUNTING` on (engine wide: every `target`/`tracer`/`lastenemy` write must go through
`SET_TARGET_REF`), or not following a `target` whose object is already removed, which is
behaviour-identical only while the removed object is still intact.

## The cause, found on 2026-09-24

The Pi crashed again, same place (`PIT_FindTarget`, `p_enemy.c:1735`, from `P_LookForMonsters`),
after hours of untouched attract, in `doomu-sl_E1M8_sk0_max.lmp`. This time the black box named the
culprit. The ring's last entry, one tic before the fault:

```
gametic 1670132 leveltime 3943 thinker 0x7f8d65bb54 function 2 type -336860181 health 8488
  from doomlegacyarcade(+0x9cacc)      -> EV_BuildStairs p_floor.c:935
```

`p_floor.c:935` is `P_AddThinker( &mfloor->thinker )` for a new **stair step**, and the thinker
`PIT_FindTarget` faulted on is that same address. `function 2` is `TFI_MobjThinker` -- recorded for
something that was about to become a floor mover.

**`P_AddThinker` classified the thinker by a field its caller had not written yet.** It ended with
`P_UpdateClassThink( thinker, TH_unknown )`, which reads `thinker->function`, and for
`TFI_MobjThinker` also `health`, `flags` and `type`. But `Z_Malloc` does not clear, and nearly every
caller -- all of `p_doors.c`, `p_floor.c`, `p_ceilng.c`, `p_plats.c`, `p_lights.c`, `p_genlin.c`,
`p_spec.c`, the FraggleScript movers, the savegame loader -- follows the vanilla pattern of
`Z_Malloc`, `P_AddThinker`, *then* `function = ...`. So classification read the memory's previous
owner. When that block had last been a live monster, the new door or stair step was filed into the
enemies list, or -- if the leftover `flags` carried `MF_FRIEND`, as the cached graphic did on
2026-09-22 -- the friends list. It stays there until it is removed, since nothing reclassifies a
sector thinker, and the first monster to look for a target reads it as a monster.

This is an upstream bug (MBF and PrBoom have the same `P_AddThinker`; PrBoom `memset`s its sector
thinkers, which hides it). The only callers that set `function` *before* `P_AddThinker` are the
three that add objects: `P_SpawnMobj`, `P_MorphMobj` (Heretic's chicken), and the savegame's mobj
restore.

**Fix:** `P_AddThinker` no longer classifies. It passes `TH_misc` (which is not a real list: it
just clears `cnext`/`cprev`), so nothing about the thinker's contents is read. The three object
callers call `P_UpdateClassThink( ..., TH_unknown )` themselves, right after `P_AddThinker`, once
`function` is set. For an object the result is identical -- the same list, appended at the same end
-- so demos cannot drift; for everything else it only changes the case that was reading garbage.

Why the Pi and never the laptop: it needs a freed monster's block reused for a sector thinker, with
leftovers that pass the `health > 0 && MF_COUNTKILL` test, which depends on allocation history over
a long run. Both Pi crashes came after 30 minutes to hours of continuous attract. None of the 123
record demos, each replayed in a fresh process on the laptop, misfiled anything even with the old
code (a per-tic walk of both lists, temporary instrumentation), and neither did a 15-minute
headless attract soak of the old code on the laptop (seven demos). So there is no laptop
reproduction; the evidence is the Pi's ring entry above, which can only have been written with
`function` still stale. The fix removes the read rather than handling its result, so the misfiling
is unreachable, not merely less likely.

Verified: `make demotest` against a baseline from the unfixed `main` binary, **123 compared, 0
desynced**. The "ended at a different tic" lines it prints are the load-dependent noise described in
`demo-desync.md`; the unfixed binary against its own baseline prints the same kind.

## A backtrace that stops at libc means a jump to address 0

If the backtrace is only the program's own `crash_handler` frame plus the libc signal-return frame
(`libc.so.6(+0x1a070)`), the crash happened *at* an address that has no code: almost always a
call through a NULL function pointer. `backtrace()` cannot unwind from PC 0, so it stops there. The
report is no help then. **Go to the core**: `coredumpctl list`, then
`coredumpctl debug <pid> --debugger=gdb --debugger-arguments="-batch -ex bt -ex 'frame 2' -ex 'p *ev'"`.
gdb unwinds past the bad frame because the return address is still on the stack. That is how the
Video Modes fire crash (`menus.md`) was found. Frame #1 was `M_VideoMode_key_handler` at the
unchecked `key_handler2` call, and the event in frame #2 was the `ev_textchar` that caused it.

## Not covered

- **Windows** gets the black box and the `Demo file:` line but no handler: MSYS2 has no
  `execinfo.h`. `SetUnhandledExceptionFilter` plus `CaptureStackBackTrace` would be the equivalent.
- The enemies list is not ringed: every monster spawn adds to it, so a ring would hold nothing but
  the last 32 spawns. The anomaly check covers it.
