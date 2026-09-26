// [Arcade] Maps the names prboom-plus's OPL music player uses onto DoomLegacy's.
//
// The files in this directory are vendored from prboom-plus (prboom2/src/MUSIC),
// which took them from Chocolate Doom; dbopl.c is the DOSBox OPL emulator.  All
// are GPLv2 or later.  Their own code is left as close to upstream as possible,
// so that a later update is a diff rather than a rewrite: each one includes
// this header in place of prboom-plus's, and the few edits made to them are
// marked [Arcade].

#ifndef OPL_COMPAT_H
#define OPL_COMPAT_H

#include "doomincl.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_swap.h"

// GENMIDI is little endian.
#define doom_htows(x)  LE_SWAP16(x)

typedef boolean dboolean;

// The GENMIDI records have 16-bit fields at odd offsets, so must be packed.
#ifndef PACKEDATTR
# ifdef __GNUC__
#  define PACKEDATTR __attribute__((packed))
# else
#  define PACKEDATTR
# endif
#endif

// prboom-plus log levels.  Everything the player reports is a malformed-MIDI
// or unknown-event notice, which is noise on a cabinet, so it all goes to the
// verbose channel (-v).
#define LO_INFO   EMSG_ver
#define LO_WARN   EMSG_ver
#define LO_ERROR  EMSG_ver
#define lprintf(lvl, ...)   GenPrintf( (lvl), __VA_ARGS__ )

// Output gain, percent of prboom-plus's default (its mus_opl_gain, 50).
#define mus_opl_gain  50

#endif
