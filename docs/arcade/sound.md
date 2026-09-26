# Sound effects: channels, and why a sound stops early

**Read this before touching** `S_get_channel`, `S_UpdateSounds`, `S_StopXYZSound`, the channel table
in `s_sound.c`, the mixer slot table in `sdl/i_sound.c`, or where `D_DoomLoop` calls
`S_UpdateSounds`. Also for the PC speaker emulation (`cv_pcspeaker`, `S_PCSpeaker_*`) and OPL
music (`opl/`, `cv_opl_music`, the music functions in `sdl/i_sound.c`), at the end.

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

## PC speaker emulation

**Options → Sound Volume → PC speaker** (`pcspeaker`, default Off) plays the `DP*` lumps instead of
the `DS*` ones. Every Doom IWAD has a `DP` lump for each `DS` one (107 in DOOM2.WAD). Each is a list of
tones, one per 1/140 s: `uint16 0`, `uint16 count`, then `count` tone numbers, where 0 is silence.

**How it plays.** `S_PCSpeaker_Lump` renders the tones to an ordinary DMX sound at 22050 Hz: a
square wave, box-filtered from 8x oversampling, with its phase kept across tone changes. After
that it is a normal `sfx->data`, so the mixer and every backend play it with no special case. The
behaviour copies prboom-plus, which dsda-doom inherited and then dropped in v0.27:

- **Its tone table**, with one tone added. The stock lumps use tone 96 and the table stopped at 95,
  so that tone played as silence. The table rises a quarter tone per step, so 96 is 2716 Hz.
- **One voice.** A new sound stops every channel, and the newest always wins.
- **No distance, stereo or pitch.** The start takes the unattenuated volume, centred, at
  `NORM_PITCH`. `S_UpdateSounds` skips `I_UpdateSoundParams`, because the speaker had nothing to
  update. It still stops sounds that go out of earshot, as vanilla's sound code above the driver
  did.
- **Six sounds never play**: `posact`, `bgact`, `dmact`, `dmpain`, `popain`, `sawidl`. A missing
  `DP` lump is silent. It does not fall back to the sampled sound, and it cuts nothing off.
- **Heretic has no `DP` lumps.** `S_PCSpeaker_Active` checks for `dppistol`, and without it the
  option does nothing, so turning it on cannot mute the game.

**Amplitude is RMS-matched, not peak-matched.** A square wave's RMS is its amplitude. The median
RMS of DOOM2.WAD's `DS` sounds is 28.8, so the amplitude is 32 of 127, the same fraction as
prboom-plus's `0x2000`. By peak, the speaker measures about a tenth of normal play (645 against
6878 in `-volog`'s `sfxpeak`). That is because sampled sounds are spiky and up to 16 of them
overlap, not because the speaker is quiet.

**Switching swaps two caches rather than freeing one.** `CV_pcspeaker_OnChange` exchanges each
sound's `data`/`length`/`lumpnum` with a second set. The SDL mixer mixes from a private copy of the
channel table outside `mix_lock`, so data freed from a menu could still be read for one buffer.
`PU_SOUND` is never purged, and nothing else in the tree frees sound data at run time either.

**It must not move the random numbers.** The speaker block in `S_StartSoundAtVolume` sits after
the `M_Random` draws for random pitch. A demo's `-synclog` is byte-identical with the option Off,
On, and switched three times during playback.

**How it was verified**, headlessly and without listening. `SDL_AUDIODRIVER=disk` with
`SDL_DISKAUDIOFILE` writes the mixer's real output to a file. Under sdl2-compat that file is at
44100 Hz whatever `-volog` says the mixer got (22050), so measure it at 44100. Checks that tell
Off from On:

- With `-nomusic`, left equals right on every sample.
- Flat-topped square half-cycles: 1705 with the option on, 0 with it off.
- Sustained tones land on the table with a median error of 0.005%.
- A run switched from an autoexec shows squares, then none, then squares again, at the switch
  times.

Music has to be off for any of this, because it fills the file with stereo that is not a square.

## OPL music

**Options → Sound Volume → OPL music** (`opl_music`, default Off, SDL_mixer builds only; the
`OPL_MUSIC` macro in `s_sound.h`) plays MUS and MIDI through prboom-plus's OPL2 player instead
of SDL_mixer's MIDI synth.

**The code is vendored in `opl/`**: `oplplayer.c` (Chocolate Doom's DMX-style MIDI-on-OPL player,
via prboom-plus), `opl.c`/`opl_queue.c` (timing), `dbopl.c` (the DOSBox OPL emulator, converted to
C) and `midifile.c`. All are GPLv2 or later.
- Each file includes `opl/opl_compat.h` in place of prboom-plus's headers. It maps `lprintf`,
  `dboolean`, `doom_htows` and `PACKEDATTR` onto this tree's names, and sends every notice to
  `EMSG_ver`, because unknown-controller messages are noise on a cabinet.
- The only edit to their logic is in `LoadInstrumentTable`: it checks that `GENMIDI` exists and is
  long enough, because this tree's `W_CacheLumpName` aborts on a missing lump, and it holds the lump
  `PU_STATIC` until shutdown.
- They build from `OPLOBJS` in the Makefile, like `nodebuild/`. `OPLWARN` silences two warnings
  about code the player never calls, instead of patching it out.

**How it plays.**
- `I_RegisterSong` (`sdl/i_sound.c`) offers each MUS or MIDI song to `I_OPL_RegisterSong`. MUS goes
  through the same `qmus2mid` call the SDL_mixer path uses. Anything it declines (option off, OGG or
  MP3, no usable `GENMIDI`, a device that is not 16-bit stereo, a song that will not load) carries
  on to SDL_mixer.
- The synth plays through `Mix_HookMusic` in place of a `Mix_Music`. That means SDL_mixer's music
  volume, pause and fades never reach it, so `I_SetMusicVolume`, `I_PauseSong`, `I_StopSong` and
  `I_UnRegisterSong` each handle the OPL case themselves.
- The synth is brought up on first use, because `GENMIDI` comes from the loaded wads.
- `CV_opl_music_OnChange` restarts the current song, so a switch is heard immediately.

**Locking.** The hook runs on the audio thread inside SDL_mixer's device lock and takes `opl_lock`.
So the game thread never holds `opl_lock` across a `Mix_*` call, or each would wait on the other.
`I_UnRegisterSong` unhooks *before* freeing the song: once `Mix_HookMusic(NULL, NULL)` returns, the
hook is not running and cannot run again.

**Volume.** The player's own volume follows DMX's curve, where half the slider is about a seventh
of the level. At the cabinet's usual music volume (about 5 of 31) that is close to silent, and it
does not match what `Mix_VolumeMusic` does to MIDI. So the synth plays at its full volume
(`setvolume(15)`), and the hook scales its output linearly by the slider, times `OPL_GAIN_256`
(2×).
- Measured at full volume, prboom-plus's unity gain peaked at 17280 on DOOM2 MAP01 and sat 3.5–4×
  below SDL_mixer's MIDI, which itself clips at that setting.
- At 2× it is about half the MIDI level, and clips 6 samples in a million on MAP01 at volume 31.

**How it was verified**, headlessly: `SDL_AUDIODRIVER=disk` capture, as for the speaker above, with
`soundvolume "0"` so the file holds only music. (`-nosound` cannot be used: `I_StartupSound` turns
music off with it.) The synth writes the same sample to both channels, which is its signature:
- With OPL on, every sample has L equal to R.
- With it off (SDL_mixer MIDI), about 5% do.
- A run switched from an autoexec reads MIDI, then OPL, then MIDI, then OPL, second by second.
- With `Doom2OST.wad` and *Music src* `Auto`, the OGG tracks stay stereo with OPL on.
- A PWAD with a bad `GENMIDI` header falls back to MIDI with the warning.
- `-synclog` is byte-identical with OPL off, on and switched.

Not verified: Heretic, which has a `GENMIDI` but no IWAD on the test machine; the Windows build;
and how it sounds.
