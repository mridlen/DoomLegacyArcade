# Fork identity and versioning

*Part of the DoomLegacy arcade cabinet build. Read before changing `VERSION_BANNER`,
`FORK_NOTICE`, `DLA_VERSION`, the executable name, or any string that names the program.*

---

The goal is that nobody — a player, a bug reporter, or the Doom Legacy team — can mistake this for
stock Doom Legacy, **while upstream keeps every bit of the credit it is owed**. Those pull in the
same direction, not opposite ones: GPLv2 §2(a) requires a modified work to carry prominent notices
that it was changed, and §1 requires the copyright notices to stay. Scrubbing "Doom Legacy" out of
the tree would be the violation, not the fix.

So: the fork names *itself* everywhere a user looks, and states what it derives from right next to
it.

## What the program calls itself

| string | value | where it shows |
| --- | --- | --- |
| `VERSION_BANNER` (`d_main.c`) | `Doom Legacy Arcade <DLA_VERSION>` | startup title box, SDL window title, `version` command, `--version`, `--help`, debug file, crash log |
| `FORK_NOTICE` (`d_main.c`) | `Fork of Doom Legacy 1.48.18 (rev 1749), unaffiliated with the Doom Legacy team.` | printed under the title box, and by `--version` / `--help` |
| `sv_name` default (`mserv.c`) | `Doom Legacy Arcade server` | **outward facing** — what a master server lists |
| `config.cfg` header (`m_misc.c`) | `// Doom Legacy Arcade configuration file.` | top of every written config |
| audio client (`linux_x/`) | `DoomLegacyArcade` | the system mixer (JACK, PulseAudio, ALSA sequencer) |
| executable | `doomlegacyarcade` (`.exe` on Windows) | the filesystem |
| ENDOOM lump | the cabinet's own screen | on exit — see `endoom.md` |

### Two hard limits on `VERSION_BANNER`

**It must stay under about 56 characters.** `D_Make_legacytitle` centres it in an 80 column line
that *also* carries the build date at columns 1–11 and the build time at 71–78, both written
afterwards. A longer banner is silently overwritten at both ends. That is why the upstream credit
lives on `FORK_NOTICE` instead of being crammed into the banner.

**It must not overflow the buffer.** The centring loop computed
`(MAX_TITLE_LEN - strlen(s)) / 2`, and `strlen` returns `size_t` — so a banner wider than 80
characters wrapped that subtraction to a huge unsigned value and wrote off the end of an 81 byte
array. Harmless while the banner was a fixed compile-time string; not harmless now that it ends in
`DLA_VERSION`, which comes from `git describe` and can be long. It is clamped, and the clamp is
tested by building with a 72 character version and checking the line is still exactly 80 columns.

## Versioning: `DLA_VERSION`

The fork has its own version, **separate from the upstream `DL_VER_*` numbers**, and it comes from
the git tag — the same tag a GitHub release is built from, so what the binary prints and what the
release is called cannot drift apart.

Resolution order, highest first:

1. **`DLA_VERSION` in the environment or on the make command line.** `make DLA_VERSION=v1.0.2`. The
   Makefile uses `?=`, and `?=` does not override a variable that is already set — including one
   from the environment — so exporting it is enough. This is how CI passes the tag.
2. **`git describe --tags --always --dirty`**, run by the Makefile. On a tagged commit this is
   exactly the tag (`v1.0.1`); later it is `v1.0.1-7-g248d498`; with uncommitted changes it gains
   `-dirty`. This is what a normal local build gets, and the `-dirty` suffix is a feature — it says
   the binary does not correspond to any commit.
3. **`"unknown"`**, the `#define` in `doomdef.h`, when there is no git and no override.

### The rebuild stamp

`d_main.c` bakes `DLA_VERSION` into the banner, but tagging a release does not modify `d_main.c` —
and this tree has no dep files for most objects, so nothing would notice. `$(O)/version.stamp` is
rewritten only when the derived version actually changes, and `d_main.o` depends on it. One
recompile when it matters, none when it does not. Verified by building with a different
`DLA_VERSION` and confirming `d_main.c` recompiled.

### CI

`.github/workflows/build.yml` already settles a version in its `check-inputs` job — the tag for a
release, the short SHA otherwise — and now passes it to both build jobs as `DLA_VERSION`. This is
not optional politeness: **the CI checkout is shallow and has no tags**, so `git describe` inside it
would find nothing.

## What must NOT be renamed

These look like branding and are not. Changing any of them breaks compatibility or data:

- **`DL_VER_MAJ` / `DL_VER_MIN` / `DL_VER_REV`, and the `VERSION` / `REVISION` ints.** They go into
  demo headers and savegames. Change them and every record demo on the cabinet is rejected or
  desyncs. `DLA_VERSION` exists precisely so these can stay put.
- **`BDTC_legacy = "DoomLegacy"`** (`g_game.c`) — a DeHackEd compatibility tag.
- **`"Doom Legacy WAD V%d.%d"`** (`d_main.c`) — a string *parsed out of* `legacy.wad`, not printed.
- **`legacyhome`, the savegame prefix, `legacy.wad`.** Renaming these orphans the live config, high
  scores, `audit.dat` and record demos.
- **Install directories** (`~/games/doomlegacy`, `$PREFIX/share/games/doomlegacy`, the
  `~/.doomlegacy` fallback). Left deliberately: renaming them orphans an existing install, and the
  cabinet uses the portable `legacyhome` beside the binary anyway.
- **`doomlegacy.sourceforge.net`, `doomlegacy.dyndns.org`** — real hosts, upstream's.
- **Everything under `svn1749/docs/` and `svn1749/logs/`** — upstream's own documentation and
  changelogs. Rewriting them would misrepresent what upstream wrote.
- **The copyright headers in every source file.** Required to stay.

## Renaming the executable: what it breaks

`doomlegacy` → `doomlegacyarcade` touched the Makefile (`EXENAME`, three platform branches, plus the
`objdump` target), `CMakeLists.txt`, `tools/build.sh`, `tools/build.ps1`, `tools/smoke.sh`,
`.github/workflows/build.yml`, and the docs.

**Anything outside the repository has to be updated by hand** — a cabinet launch script, a systemd
unit, a desktop entry, a shell alias. The old binary is not deleted by the rename; a stale launcher
keeps starting the *old* build and looks exactly like "my changes did nothing".

## The smoke test trap this sprang

`tools/smoke.sh` decided "the engine quit cleanly" by grepping the log for **`READ THE DOCS`** — a
phrase from the *stock* ENDOOM art. The moment that art was replaced, three of the five checks went
red while the engine was quitting perfectly cleanly, and the failure message sent the reader off
after a missing `SDL_NO_SIGNAL_HANDLERS=1` that was never missing.

It now detects the screen **structurally**: `endtxt.c` emits `\033[0m` once per cell, so a printed
ENDOOM is 2000 resets. Measured 2026 on a clean quit against 0 on a run the timeout killed, and the
threshold is half a screen. Nothing about the art can break it.

The general rule: **never tie a test to the contents of editable art.**

## How this was verified

- Built clean; `DLA_VERSION` reached the compiler as
  `-DDLA_VERSION='"v1.0.1-7-g248d498-dirty"'`, derived from the real tag.
- `--version` prints the title box at exactly 80 columns and `FORK_NOTICE` at 79, so neither wraps
  on an 80 column terminal.
- Rebuilt with a 72 character `DLA_VERSION`: `d_main.o` recompiled (the stamp fired) and the title
  line was still exactly 80 columns — the clamp holds, no overflow.
- `make smoke`: 5 passed, 0 failed, including `opengl` on the real GPU.
- Proved the fixed smoke check can still fail: `SMOKE_TIMEOUT=1` turns `startup` and `warp` red
  again, so it is detecting the quit rather than passing unconditionally.
