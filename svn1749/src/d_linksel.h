// [Arcade] Cabinet Link: Select Game Sync.
//
// When the master has Select Game Sync on, a game chosen on the Select Game
// page of any linked cabinet -- an IWAD, or a level pack -- is followed by
// every other cabinet as soon as it is not in a game: attract screen or menus,
// exactly the states an invite may interrupt.  Without it, a linked game of
// anything but the cabinets' boot game meant walking to each cabinet first.
//
// The master holds the latest selection and tells each member once; a member
// that cannot follow (the IWAD or the pack is not installed there) says so,
// and the master's Cabinet Link page shows it under that cabinet.  All on the
// game thread; the link only carries the messages.
//
// See docs/arcade/cabinet-link.md, "Select Game Sync".

#ifndef D_LINKSEL_H
#define D_LINKSEL_H

#include "doomtype.h"
#include "d_link.h"

// Called from LK_Ticker every pass while the link runs.
void          LKSEL_Ticker( void );

// A game message of one of the LK_GM_GAME_* types (from LKG_Ticker's poll).
void          LKSEL_On_Event( const lk_event_t * ev );

// A player chose a game on this cabinet without a restart (a level pack added
// into an empty slot).  A selection that restarts carries -linkselected
// instead, which the new process reads.
void          LKSEL_Selected( void );

// For the operator page: why the cabinet with this full id did not follow the
// latest selection ("" when there is nothing to say), and the same for this
// cabinet itself.
const char *  LKSEL_Peer_Status( const byte * fp );
const char *  LKSEL_Self_Status( void );

// One LINKSEL status line (-linkstatus), terminal only.
void          LKSEL_Status_Print( void );

#endif
