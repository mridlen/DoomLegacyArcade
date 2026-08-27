# Portable install, config.cfg handling and the command buffer

*Part of the DoomLegacy arcade cabinet build. Read before touching `legacyhome` resolution in `d_main.c`, `M_SaveConfig`/`M_Verify_Config` in `m_misc.c`, or the tracked `cabinet/legacyhome/config.cfg`.*

See `CLAUDE.md` for the build, headless verification and the cross-cutting rules index.

---

### Portable install (`legacyhome` next to the binary)

**A `legacyhome/` directory sitting beside the executable overrides `~/.doomlegacy` entirely** —
config, high scores, demos, level packs and savegames. This is what lets a checked-out tree run
with its own tracked `config.cfg` and **no command line arguments**, which is the point: the config
is otherwise unversioned state the build silently depends on.

- Implemented in `d_main.c`, just above the `if (userhome)` legacyhome search. The engine already
  had `progdir` and a `progdir/DEFHOME` fallback; that path was **dead code**, because `$HOME` is
  always set on Linux and was tested first. The change only reorders the priority.
- **Presence of the directory is the switch.** Without one, nothing changes and an existing
  `~/.doomlegacy` install behaves exactly as before. Verified both ways.
- `progdir` comes from `readlink("/proc/self/exe")` (`I_Get_Prog_Dir`, `sdl/i_system.c`), so it
  follows the **executable, not the working directory** — a menu entry or service unit finds the
  same files as a shell. Verified by running from `/` with an absolute path.
- **The trailing slash is required.** `savegamename` concatenates directly onto `legacyhome`, and
  `cat_filename` only separates *dir from name* — it does not append one at the end. So the new
  branch passes `DEFHOME SLASH`, matching the `DEFAULTDIR1 SLASH` the userhome branch uses. The
  pre-existing `progdir` fallback below does **not** do this, which is a latent bug in a path that
  is only reachable when `$HOME` is unset.
- Kept in a local (`portable_home`) rather than testing `legacyhome` directly, because
  `D_DoomMain` re-runs this block via the launcher restart path and `legacyhome` may still hold
  the previous pass's value.

**The command buffer sizes the config.** `exec` pushes a whole file into `com_text` in one go, and
`COM_BUF_SIZE` (`command.c`) caps it. At the stock **8192** the cabinet's config outgrew it — 8195
bytes — and `VS_Print` failed, dropping the remaining lines **in silence**. Those cvars kept their
compiled defaults, and the next `-devmode` session wrote the defaults back over the file: that is
how a hand-tuned config "blows away on its own". The four player work is what pushed it over
(`setcontrol3` and `setcontrol4` are 38 lines each). Raised to **65536**, and the overflow is now
an `EMSG_error` naming the consequence rather than a `CONS_Printf` that scrolls past. **A four
panel config is ~8.5K, so watch this if the config grows much further** — and if settings ever stop
sticking, look for "Command buffer full" first.

**Config lines that fail are reported.** `M_Verify_Config` (`m_misc.c`) runs right after the load
and again on demand from the **`cfgcheck`** console command. Config lines are handed to the console
as an `exec`, so by the time they run the line numbers are gone and a failure is *silent*: an
unrecognised setting name does nothing, and `CV_Set` rejects a value that is not one of a cvar's
`PossibleValue`s without a word. Either way the cvar keeps its compiled default, which is exactly
how a config appears to have been "half loaded". Rather than instrument the command buffer, the
check re-reads the file afterwards and verifies each `name "value"` line actually left that cvar
holding that value, naming the line number when it did not.

**Comparing by `cv->string` is wrong**, and produced a page of alarming false reports the first
time — `drawmode` "did not take" on a cabinet visibly running OpenGL. `cv->string` is only the text
last *set*; the value in force is `.EV`, or `.value` for `CV_VALUE`/`CV_FLOAT`. Config files also
store a cvar's PossibleValue **label** ("On", "OpenGL", "32 bits"), so the file's text must be
resolved through that table before comparing. This is the same trap as the `cv_fastmonsters`
investigation: **resolve labels and read `.EV`/.`value`, never compare the strings.**

**Timing matters as much as the comparison.** The check runs from `D_DoomLoop`, not from
`M_LoadConfig`: at load time the video mode is not set and several subsystems have not applied
their settings, so `scr_width`, `drawmode` and friends still read as defaults and every one is
reported as failed.

Two sources of *expected* reports remain, so read the output with them in mind:
- **`botrandom`** is a random seed and always differs.
- A clean cabinet config reports exactly **four**: `botrandom` plus the three ruleset cvars below.
  Anything else is worth investigating. (The video settings used to appear here too; that was the
  command buffer overflow above, not the dummy driver.)
- **The ranked ruleset legitimately overrides the config** in a player session (see
  `hs_ranked_rules[]`), so `monstergravity`, `monsterfriction` and `voodoo_mode` report as not
  taking. That is the ruleset working, and matches the three documented above.

**Every config save keeps one generation** as `config.cfg.bak`, written by `M_SaveConfig`
(`m_misc.c`) just before the new file. A cabinet's config is hand-tuned, only a `-devmode` session
writes it, and the settings exist nowhere else on the machine — so a bad write is rare but
expensive. There *is* a `config_loaded` guard above that code, but it does not prevent a session
which started without reading the file from writing a full set of defaults over it: verified, a
devmode run with no `config.cfg` present writes 345 lines of pure defaults. The `.bak` makes any
such loss one `cp` from recovery. **A defaults-write is recognisable** by `name` being your Unix
login and `name2` being the compiled default `"big b"`.

The tracked copy lives at **`cabinet/legacyhome/config.cfg`** (see `cabinet/README.md`). `make`
stages it into `svn1749/bin/legacyhome/` via the `cabinet_home` target, using `cp -n` so a rebuild
**never resets a running cabinet's settings**. Since `bin/` is gitignored, an operator's `-devmode`
edits land in an untracked file — **`make cabinet_save`** copies the live config back over the
tracked one so the change shows up as a reviewable `git diff`. Player data and `levels/` stay
untracked by design: high scores and demos churn on every record and want backups rather than
history, and the level packs are ~26MB of wads.

Runtime data lives in the active legacyhome — `~/.doomlegacy/` unless a portable one is found:
`config.cfg`, `highscores.dat` (plain text,
`<wadcombo> map skill tics <category> <startmap>`), `runs.dat`
(`<wadcombo> <startmap> <endmap> skill <category> tics <initials>`),
`demos/<wadcombo>_<map>_sk<N>_<category>.lmp`, and
`levels/` for selectable level packs. `<wadcombo>` is `HS_GameId()`, e.g. `doom2` or
`doomu+mapsofchaos`; `<category>` is `speed` or `max`. **Fields are only ever appended** to
`highscores.dat`, so an older short line still loads: four fields is a pre-category speed record,
five adds the category, six adds the start map. A record with no start map is never written as an
empty field — `-` stands in, or it would shift every field after it on the next read.
`runs.dat` writes `---` for initials nobody entered, and reads that back as empty.

**Every placeholder must be converted back on load, or it becomes a value.** This is written twice
in each file — `-` for an unknown start map, `---` for unentered initials — and the load side has
to undo both. Missing the `-` conversion in `HS_Load` made an unknown start map read as a map
literally named `-`, which differs from the record's own map and so satisfied exactly the test the
caption uses to decide it has a range: attract captions came out as **`--E4M1`** and
**`SINGLE LEVEL: --E1M1`**. Nothing was corrupted on disk — the file was always right and only the
read was wrong — which is why it showed up as a display oddity rather than lost data, and why no
migration was needed to fix it.

To reset the scores, use the **`clearhighscores`** console command or the **`-clearhighscores`**
command-line flag (which runs the same code right after `HS_Init`). Both clear the in-memory tables
as well as the files — `runs.dat` goes with `highscores.dat`, since leaving it would put named
board entries beside an empty split table. Prefer them over deleting the files by hand: the tables
are cached in memory while the game runs, so a later record writes the old entries straight back
out. Note that deleting **only** `runs.dat` is a supported way to re-run the one-time seed from the
single level splits.

- **A cvar loaded from `config.cfg` can be silently dropped if its `OnChange` needs a subsystem
  that does not exist yet.** The config is executed by `M_LoadConfig` at `d_main.c:3534`; the
  OpenGL driver's function table is not filled in until `SCR_SetMode(0)` at `d_main.c:3706` calls
  `I_Rendermode_setup`. Three GL cvars have `OnChange` handlers guarded by
  `if( HWD.pfnSetSpecialState )` — `gr_filtermode`, `gr_fogdensity` and `gr_polygonsmooth` —
  so at config time the pointer is NULL, the handler quietly does nothing, and **nothing ever
  re-applies the value**.
  - **It cost the cabinet its texture filtering for the life of the build.** `config.cfg` said
    `gr_filtermode "Nearest"` and the game rendered `Bilinear`, the compiled default, throughout.
    The setting saved correctly, reloaded correctly, and read correctly in the menu — it simply
    never reached the driver. Nothing logs a warning, and `M_Verify_Config`'s "did not apply"
    check does not catch it either, because the cvar *did* take the value.
  - **`HWR_Apply_Config_Settings()`** (`hw_main.c`) now pushes all three into the driver, called
    from `HWR_Startup_Render`, which runs after `I_Rendermode_setup`. Changing them from the menu
    later still goes through the `OnChange` handlers as before.
  - **The tell is that setting it from the console works and the config does not.** Force the
    OnChange late from a scratch `autoexec.cfg` and compare:
    ```
    gr_filtermode Bilinear
    gr_filtermode Nearest      # two lines: re-setting a cvar to the value it
    wait 60                    # already holds returns early and never fires
    screenshot
    ```
    If the late path looks right and the config path does not, it is this.
  - **Suspect the same shape for any `OnChange` that touches video, sound or the renderer.**
    The guard that makes the handler "safe" is exactly what makes the loss silent. This is the
    same family as the `M_Init`/`M_Configure` ordering rule in `menus.md`.

