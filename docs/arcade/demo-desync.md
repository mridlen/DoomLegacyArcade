# Demo desync testing

Read this before changing anything that could affect gameplay, and before
changing `tools/demotest.sh`, `G_Synclog_Tic` or the `-synclog` switch.

The cabinet keeps 97 record demos. Every one of them is a player's high score,
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
- **A `Level:` line says `demo`.** Every level load prints one, and it names
  what is driving the level — a demo, or a live player. Without this, a run
  that fell through to the attract cycle and played *some other* demo would
  pass.
- **The levels it loaded are the ones it loaded last time.** The map and skill
  of every level load are recorded in the baseline (`<demo>.maps`) and compared,
  so a demo that starts loading a different map is caught.

That last one is deliberately a comparison and not a prediction, and the
first version of this got it wrong in an instructive way. It predicted the
start map from the filename — and was wrong for 18 of the 95 demos, because
**the map in the name is the map the record is _for_, not where the demo
begins**. `HS_BuildDemoPath` (`hs_stuff.c`) names a per-map split record after
the map the run *reached*, so `doomu_E2M7_sk0_speed.lmp` is a campaign run that
starts at E2M1. The other scheme, `HS_BuildSurvivalDemoPath`, writes `ep<N>`
in that field and names no map at all.

Reading the start map out of the demo header instead is no better. The
offsets are right there (`DEMOHDR_skill = 7`, `episode = 8`, `map = 9`,
matching `G_BeginRecording`), but the header is patched after recording by
`G_Update_Demo_Header`, and a number of the demos on the cabinet have bytes
there that disagree with what the engine actually loads from them — they
predate a header change. **Whether that is worth fixing is an open question
and has not been investigated**; it is noted here only as the reason the
harness does not trust those bytes.

The general lesson is worth keeping: when a check needs to know what the
answer should be, and the convention that decides it lives somewhere else and
has changed over time, record the answer instead of predicting it.

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

    doom2_MAP03_sk0_speed.lmp: DESYNC at log line 102 (leveltime 101), and the level sequence changed
    doomu_E1M1_sk0_speed.lmp: DESYNC at log line 102 (leveltime 101)

One tic after the injected draw, in both demos, exit code 1 — and on the
multi-level one the divergence also sent the run down a different path, which
the recorded level sequence caught independently. The mutation was then
reverted, the tree rebuilt, and the same command over four demos reported
`4 compared, 0 desynced`, exit 0.

Green, red at the right tic, green again. **This has been done four times**:
when the harness was first written, again after the map check was rewritten
from prediction to comparison, again after the switch to `-timedemo`, and
again after the comparison moved to the shared prefix — each
time because the change touched exactly the code the self-check exercises, and
a green result that has not been re-earned is not worth anything.

The last of those is the one to copy, because the suite is fast enough now to
do it over everything:

    97 compared, 97 desynced, 28 ended at a different tic, 83s
    doomu_ep1_sk3_speed.lmp: DESYNC at log line 102 (leveltime 101), and the level sequence changed
    doomu_E2M7_sk0_speed.lmp: DESYNC at log line 102 (leveltime 101), and the level sequence changed
    ...

Every demo in the corpus, all at leveltime 101, one tic after the injected
draw. Revert, rebuild, and it reports `97 compared, 0 desynced`.

Redo it whenever the harness changes. Insert the line above into
`P_MobjThinker`, `make`, run the harness, then revert and rebuild — about four
minutes, nearly all of it compiling. It is the only thing that distinguishes a
working check from a check that always passes.

## Cost, and why there is a `--quick`

`-playdemo` replays at wall-clock speed: 35 tics a second, exactly as if
somebody were playing. About 425,000 tics across the 95 demos is roughly 200
minutes of that, and even spread over every core it measured **48 minutes**,
with a floor of about 16 set by the single longest demo.

`-timedemo` replays flat out instead, and the harness uses it.

**It always could have.** `TryRunTics` (`d_clisrv.c`) has had
`if(singletics) realtics = 1;` all along — one tic per pass of the main loop,
whatever the clock says — and the frame limiter in `D_DoomLoop` is explicitly
skipped under `singletics` too. Nothing paced the playback. What made
`-timedemo` *look* slower than `-playdemo` was that it never ended: the
`timingdemo` branch of `G_CheckDemoStatus` printed its result and called
`D_AdvanceDemo()`, dropping into the attract cycle, so the demo was replayed
over and over with **real-time attract pages between the passes**. A fixed
timeout caught a couple of fast passes separated by long idle stretches, which
reads exactly like slow playback.

That is now fixed: a `-timedemo` named on the command line sets `singledemo`
and quits when the timing is printed, the way `-playdemo` does.

Note `I_GetTime` (`sdl/i_system.c`) genuinely has no `singletics` fast path,
unlike vanilla Doom — but that only affects what the timing *measurement*
reports, not how fast the demo runs, because the loop never consults it for
pacing. It was tempting to "fix" that and it would have been the wrong change.

**That the two modes agree is checked, not assumed.** The baseline was recorded
with `-playdemo`, over 47 minutes, and the whole suite then re-run with
`-timedemo`: **94 of 95 byte-identical, in 40 seconds.** The manifest records
which mode a baseline used and the comparison says when it crosses modes.
`--playdemo` forces the old path, both to re-check that equivalence and as an
escape hatch if it ever stops holding.

    -playdemo   2846s   (47 minutes)
    -timedemo     40s   (~70x)

The 95th demo is the subject of the next section.

It still saturates the CPU while it runs, but 40 seconds is short enough that
the suite is worth running on anything, rather than saved for big changes.

## Two things that look like desyncs and are not

Both were found the first time the suite ran after somebody actually played the
cabinet, and both would have been read as regressions.

### The demo file was replaced

The cabinet **rewrites a demo whenever that record is beaten**. Play a better
run and the `.lmp` under that name is a different file, so the baseline log
describes a demo that no longer exists — and the comparison fails on three
demos with no code change at all. That is what happened: three "desyncs", all
with mtimes minutes after the baseline was taken, from one play session.

It cuts the other way too, which is the dangerous direction: a replaced demo
could equally have *hidden* a real regression.

So the baseline stores the sha256 of each `.lmp` and the comparison checks it
first. A changed file is reported as its own thing, with the command to
re-record just those entries. **Before blaming a run for changed files, compare
mtimes against the run times** — the general rule is already in CLAUDE.md, and
this is it in a new place.

A filtered `--baseline` therefore had to stop wiping the whole directory, which
it used to do: re-recording the one demo whose record was beaten would have
thrown away the other ninety-odd baselines and left the suite testing nothing.

### The demo stopped at a different tic

Where a demo *stops* is not reproducible, so it is not a signal.

A demo whose input runs out while the player is standing still keeps being
simulated — monsters still think, `prnd` still advances — and the point at
which the engine finally quits moves with how loaded the machine is. Six runs
of one binary on one demo, alternating an idle machine with eight busy cores:

    idle    1570   1656   1555
    loaded  1214   1238   1365

The `-timedemo` timing line is never printed in **any** of them, so the demo-end
path is never reached at all; the run is ending by some other, wall-clock
dependent route. That is a pre-existing engine wart in the same never-quits
family as the `-timedemo` attract-cycle bug above, and it has not been chased
down.

So the comparison checks the tics both runs reached, and a length difference is
counted and listed but is not a failure. A difference *within* the shared
prefix is a real desync and is what the suite exists to catch.

**The cost is worth stating plainly:** a change that made a demo genuinely end
earlier would now show up as a length note rather than a failure. Nothing that
was previously reliable is lost — that signal was already noise, and it was
producing false failures — but it is not free either. If the engine's demo-end
path is ever fixed, this should be tightened back up.

## The quarantine list

`tools/demotest-ignore.txt` names demos that are not usable as fixtures. **It is
currently empty, which is where it should stay.**

`doomu_ep1_sk0_max.lmp` lived there for a while, for the variable-termination
behaviour described above, before the comparison learned to handle that
generically. Returning it to the suite restored 1720 tics of real coverage that
were being thrown away, which is the argument against quarantining anything
that can be handled instead.

The distinction that matters for the ignore list: **quarantine is for a demo
whose playback is not reproducible, not for a demo that desyncs against the run
it originally recorded.** This harness compares one playback against another
playback of the same file. A demo that no longer reproduces the run a player
actually had is still a perfectly good regression fixture — it only has to
replay the same way twice. Dropping demos because they "already desync" would
throw away coverage for nothing.

Every quarantined demo is one that has stopped watching for regressions, so the
list is printed on every run rather than hidden, and each entry carries its
reason.

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
