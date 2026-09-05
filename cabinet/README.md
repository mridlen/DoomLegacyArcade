# Cabinet configuration

`legacyhome/config.cfg` is the tracked baseline for the arcade build.

The engine looks for a `legacyhome` directory **next to the binary** before it
falls back to `~/.doomlegacy` (`d_main.c`), so a built tree runs from this
config with no command line arguments:

    cd svn1749/src && make        # stages legacyhome/ into ../bin/
    cd ../bin && ./doomlegacyarcade

`make` copies this config to `svn1749/bin/legacyhome/` only if none is there,
so rebuilding never resets a cabinet's live settings.

Because `bin/` is gitignored, changes an operator makes in a `-devmode`
session land in an untracked file. To bring them back for committing:

    cd svn1749/src && make cabinet_save
    git diff cabinet/legacyhome/config.cfg

Player data (`highscores.dat`, `demos/`) and content (`levels/`) live in the
staged home next to the binary and are deliberately not tracked -- they want
backups, not version history. `levels/` is tens of megabytes of wads.
