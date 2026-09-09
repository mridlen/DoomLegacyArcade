# Demo desync testing

Read this before changing anything that could affect gameplay, and before
changing `tools/demotest.sh`, `G_Synclog_Tic` or the `-synclog` switch.

The cabinet keeps 95 record demos. Every one of them is a player's high score,
and the engine refuses a demo whose recorded settings do not match what it is
about to replay it with — so a gameplay change does not merely alter them, it
**invalidates the board**. This is the check that says whether that happened.

    cd svn1749/src
    make demotest_baseline      # once, on known-good code
    make demotest               # after a change

Success is three lines. Failure names the demo and the tic.

---

## Why "the demos still play" is not a test

The trap is in CLAUDE.md already and it is worth repeating here, because it is
what makes a naive version of this harness worse than none at all:

> `-playdemo` plays an external *file*, never an internal lump, and a failed
> demo run looks like a passing test. Two runs that both failed to load compare
> 100% identical.

Any check built on "did it crash" or "did the pixels match" inherits that. Two
runs that both loaded nothing are in perfect agreement. So the harness compares
**simulation state**, and separately proves the demo actually drove a level.

## What is compared

`-synclog` (`G_Synclog_Tic`, `g_game.c`) writes one line per tic while a demo
is recorded or played back:

    # leveltime prnd x y angle momx momy fwd side aturn btn tflags
    1 51 69206016 -236978176 1073741824 0 0 0 0 16384 0 0
    2 51 69206016 -236978176 1073741824 0 0 0 0 16384 0 0

`prnd` is the shared random index, and it is the most valuable field on the
line. `m_random.c` implements `PP_Random(pr)` as `rndtable[++prndindex]` and
never looks at `pr`, so **every** draw shares one index. An extra draw anywhere
— a puff of smoke, a decorative spark — shifts every monster decision and damage
roll after it. `prnd` moves the instant that happens, one tic before the
position fields do.

Playback is deterministic: the same binary replaying the same demo twice
produces byte-identical logs. That was verified before the harness was built,
not assumed.

## What proves the demo really ran

Three things, and all three are needed:

- **`synclog_play.txt` exists and is non-empty.** A demo that did not load
  writes nothing.
- **The engine reported the map the filename claims**, from the `Level:` line
  that every level load prints. A demo replayed under the wrong IWAD loads a
  different map and this catches it.
- **That line says `demo`.** It names what is driving the level — demo, or a
  live player. Without this, a run that fell through to the attract cycle and
  played *some other* demo would pass.

The colour escapes are stripped before matching. ENDOOM interleaves them per
character, so a plain grep on the raw output finds nothing and hands back a
false pass — the same mistake that once kept a `SDL_QUIT` diagnosis pointing at
`-warp`.

Exit code is checked too, but it is the weakest of the four: a demo named with
`-playdemo` sets `singledemo`, so the engine calls `I_Quit()` when it ends and
a healthy run exits 0.

## The self-check

**A clean result from a check never shown to fail is not evidence.** This one
was shown to fail before it was trusted, and the record of that is here so
nobody has to take it on faith:

An extra `P_Random()` guarded by `leveltime == 100` was added to
`P_MobjThinker`, the tree rebuilt, and the harness run against the clean
baseline. It reported:

    doomu_E1M1_sk0_speed.lmp: DESYNC at log line 102 (leveltime 101)
    doom2_MAP01_sk0_speed.lmp: DESYNC at log line 102 (leveltime 101)

One tic after the injected draw, in both demos, exit code 1. The mutation was
then reverted, the tree rebuilt, and the same command reported 0 desynced.
Green, red at the right tic, green again.

Redo this whenever the harness changes. It takes about four minutes and it is
the only thing that distinguishes a working check from a check that always
passes.

## Cost, and why there is a `--quick`

Demos replay at wall-clock speed. `-timedemo` does **not** help: `I_GetTime`
(`sdl/i_system.c`) has no `singletics` fast path, so it measures real-time
playback rather than running flat out, and it never quits either — the
`timingdemo` branch of `G_CheckDemoStatus` calls `D_AdvanceDemo()` and falls
into the attract cycle before it ever reaches the `singledemo` test. Both are
worth fixing; neither is fixed here, because **the safety net must not depend
on changing the thing it exists to protect.** The harness needs no engine
changes at all.

So the cost is arithmetic: about 425,000 tics across the 95 demos, at 35 tics a
second, is roughly 200 minutes serially. The script runs `nproc` demos at once
in separate scratch directories, which brings it to around 25 minutes — with a
floor set by the single longest demo, `doomu_ep1_sk3_speed.lmp`, at about 16
minutes on its own. Demos are dealt longest-first over the slots so they all
finish together.

`--quick` caps each demo at 60 seconds and compares only the tics both runs
reached. It takes a few minutes and catches anything that goes wrong early,
which is most things — a shifted random index diverges immediately. It will not
see a divergence that only happens deep into a long run, so it is the check for
"did I break something obvious", not the one to trust before a release.

## What the harness needs to know about a demo

The filename is the high score game id (`HS_GameId`, `hs_stuff.c`) plus the
map, skill and category:

    <game>[+<pack>][-sl]_<map>_sk<n>_<cat>.lmp
    doomu_E1M1_sk0_speed.lmp
    doom2+dwango5_MAP01_sk3_max.lmp
    doomu+mapsofchaos-sl_E1M1_sk1_speed.lmp

`<game>` is the `-game` switch name. `<pack>` is a wad in
`legacyhome/levels`. The `-sl` suffix is Single Level, which is a scoring mode
and needs nothing on the command line. An `epN` map field means a whole-episode
run, which starts at `E<N>M1` — or `MAP01` for the games with one episode.

`tools/demotest.sh -l` prints the arguments derived for every demo and runs
nothing. **Use it whenever a new game or level pack appears on the cabinet**,
because an unrecognised id is reported as skipped rather than guessed at — a
guess would load the wrong IWAD, and that reads as a desync in the engine
rather than a gap in the script.

## Scratch directories

Never the cabinet's own `bin/`. Since the portable install landed, `legacyhome`
is found *next to the binary* and `$HOME` is not consulted, so running the
engine there reads and writes the live config, scores and demos. The harness
builds one scratch directory per parallel slot, each with a hard link to the
binary, a copy of the config and symlinks to the wads.

The config is copied from the cabinet's own `legacyhome`, because that is what
the demos were recorded under and what the cabinet will replay them under —
with `drawmode` forced to `Software 8bit`, since a GL config under the dummy
video driver does not fall back but segfaults in `HWR_DrawPic`. The
per-drawmode configs are deliberately not copied; they execute after
`config.cfg` and would put the drawmode back.

The config's hash goes in the baseline manifest. A config change does not
invalidate the baseline on its own, but it can, so the comparison says when one
has happened rather than leaving it to be discovered as a mystery desync.

## Reading a failure

    demotest: 95 compared, 3 desynced, 1487s
      baseline was commit 43efbb3, now 43efbb3-dirty
      doomu_E1M1_sk0_speed.lmp: DESYNC at log line 102 (leveltime 101)

The logs are kept: `.demotest/baseline/<demo>.log` against
`.demotest/current/<demo>.log`. Diff them at the reported line. The first
differing field says a great deal:

- **`prnd` alone moved** — something drew a random that did not before. This is
  the common case and the cause is usually nowhere near the symptom.
- **position or momentum moved, `prnd` did not** — physics or a movement
  constant changed.
- **the log is shorter than the baseline** — the demo ended early, which
  usually means the engine rejected it partway rather than desyncing.

`leveltime` restarts at 1 on each level of a multi-level demo, so the log line
number and the leveltime are both reported; they only agree on the first level.

## Related

- `docs/arcade/gameplay-defaults.md` — what counts as gameplay-affecting, the
  demo header, and the bias-by-one rule for new header bytes.
- `docs/arcade/high-scores.md` — where the demos come from and what they are for.
- `docs/arcade/gotchas.md` — the demo archaeology this grew out of.
