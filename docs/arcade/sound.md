# Sound effects: channels, and why a sound stops early

**Read this before touching** `S_get_channel`, `S_UpdateSounds`, `S_StopXYZSound`, the channel table
in `s_sound.c`, the mixer slot table in `sdl/i_sound.c`, or where `D_DoomLoop` calls
`S_UpdateSounds`.

For a cabinet that is *silent* rather than cutting sounds short, see `-volog` in `cabinet-link.md`.

## The report

"The plasma rifle sound cuts off when shooting it on E4M2. On E1M1 it does not." Nothing in the
engine said why, so the first thing built was a tracer, and it named two separate bugs.

## Two layers, five ways to stop early

A sound effect passes through two tables of 16:

- **Engine channels**, `channels[]` in `s_sound.c`, `snd_channels` long (the cabinet's is 16).
  `S_get_channel` picks one. It stops a playing sound here in two cases. The first is a new sound
  from the **same origin**, only when both carry `SFX_org_kill` (doors, lifts, the chainsaw). The
  second is when every channel is busy: it steals the **lowest priority**, and only one *strictly*
  lower than the newcomer's. Otherwise it refuses the new sound.
- **Mixer slots**, `mix_channel[]` in `sdl/i_sound.c`, a fixed 16. `I_StartSound` takes a free slot,
  or evicts the "oldest by age + priority". `SFX_single` sounds (pistol, chainsaw, pickups) kill
  their own previous copy.

Beyond those, `S_UpdateSounds` stops a sound that has moved **out of earshot** (1200 units), and
`S_StopXYZSound` stops `SFX_org_kill` sounds whose **source was removed**.

A channel whose sound has *finished* is only released by `S_UpdateSounds` noticing
`! I_SoundIsPlaying`. Nothing else frees it.

## `-sndlog`: one line per sound stopped early

```
./doomlegacyarcade -sndlog            # every sound, every cause
./doomlegacyarcade -sndlog plasma     # only lines naming that sfx, plus each START and END of it
```

Writes **`sndlog.txt`** beside the program (flushed per line, since Windows has no console), and prints
the same lines. The name is the lump name without `DS`: `plasma`, `pistol`, `shotgn`, `firxpl`,
`rlaunc`. Every line starts with the game tic `T` and the wall clock in tics `W`. They drift apart
when the game stalls, and a sound's length is wall-clock time.

| line | meaning |
| --- | --- |
| `CUT a by b: same origin` | `S_get_channel` reused a's channel for b, from the same source |
| `CUT a by b: all channels busy, lowest priority stolen` | every channel was busy and a was the lowest |
| `REFUSED a ... held by x/age ...` | every channel was busy at a's priority or higher; a never played. Lists each channel's sfx and age in tics, `*` = orphaned |
| `CUT a: out of earshot` | `S_UpdateSounds` found a too far away |
| `CUT a: its source was stopped or removed` | an `SFX_org_kill` sound's mobj or sector stopped it |
| `ORPHAN a` | a's mobj was removed while a played on (a missile hitting something) |
| `CUT a in mixer slot N by b` | all 16 mixer slots were busy and the mixer evicted a |
| `CUT a in mixer: single-copy sound restarted` | an `SFX_single` sound replaced its own earlier copy |
| `START` / `END` | filtered sfx only: started on a channel, and released after finishing |

Costs nothing without the switch: each hook is a test of `sndlog_on` first.

**Read `START`→`END` lifetimes against the sample's length.** DSPLASMA is 0.52 s, 18 tics. A
lifetime far longer than the sample means the channel is not being released, which is what the
first bug looked like.

## Bug 1: finished sounds were not released under a frame-rate cap

The cause of the report. `S_UpdateSounds` sat inside the draw branch of `D_DoomLoop`, gated on
`tic_advanced`, so it ran only on a pass that **both** advanced a tic **and** drew a frame. The frame
limiter (`uncapped-framerate.md`) sets `draw_now = false` on a tic's own pass when the next frame is
not yet due. The frame is then drawn on a later pass, where `tic_advanced` is already false, and that
tic's sound update is lost. When the 35 Hz tic clock and the 60 Hz frame clock fall into step, it is
lost for seconds at a time.

Measured by replaying `doomu_ep4_sk0_speed.lmp` (it fires the plasma rifle through E4M2) with
`-sndlog plasma`, at the cabinet's `framerate_cap "60"`:

| | before | after |
| --- | --- | --- |
| plasma shots | 396 | 396 |
| played | 281 | 396 |
| refused, silent | 115 | 0 |
| START→END lifetime | 20 to 200+ tics | 10–29 tics |
| channels released in one tic | up to 16 | 1 |

Before the fix, finished plasma copies sat on all 16 channels for up to 175 tics and were then
released **ten or sixteen in the same tic**, the one tic the update happened to run. Every shot in
between was refused, because the channels held sounds of the plasma's own priority (193) and
`S_get_channel` only steals a strictly lower one. It depends on the map only through timing: what the
renderer costs decides whether the two clocks lock.

**Fix:** `S_UpdateSounds` runs once per tic that ran (`gametic != sound_tic`), outside the draw
branch. It uses no random numbers, so moving it cannot desync a demo: `make demotest` reported 0
desynced across 125 demos.

**Rule:** anything tic-paced in `D_DoomLoop` must not be nested inside `draw_now`. Under a cap,
"this pass draws" and "this pass ran a tic" are independent.

## Bug 2: a missile's sound read its position from freed memory

A missile's firing sound (plasma, rocket, imp and baron fireballs) is the missile's `seesound`,
played **from the missile**, not the shooter. When the missile hits something, `P_RemoveMobj` calls
`S_StopObjSound`, which only stops `SFX_org_kill` sounds. The firing sound plays on, which is
Legacy's intent (vanilla cut it). But its channel's `origin` still pointed at the mobj, which was
then `Z_Free`d, and `S_UpdateSounds` read a position out of that memory every tic, from whatever
was allocated there next. Under the old batched updates this rarely mattered. With updates every tic
it would cut sounds as "out of earshot" whenever the memory was reused.

**Fix:** `S_StopXYZSound` copies the last position into the channel (`orphan_pos`) and points
`origin` there. The sound plays on from where the missile died. The `ORPHAN` line marks each one.
