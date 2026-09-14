// [Arcade] Cabinet Link, Phase 2: shared high scores and demos.
//
// Linked cabinets hold one set of high scores.  Each offers the other what it
// holds for the game it is running; the other merges it with its own using
// the same pure function (hs_merge.c), fetches the record demos it is missing,
// and applies the result when nobody is playing.  A member syncs with its
// master and the master with every member, so everything meets at the master.
// All on the game thread; the link (d_link.c) only carries the chunks.
//
// See docs/arcade/cabinet-link.md, "Shared high scores and demos".

#ifndef D_LINKSCORE_H
#define D_LINKSCORE_H

#include "doomtype.h"

// Called from LK_Ticker every pass while the link runs.
void  LKS_Ticker( void );

// One LINKSCORE status line per peer (-linkstatus), terminal only.
void  LKS_Status_Print( void );

// What the operator page shows about sharing with this peer (by full id):
// "SCORES SHARED", "SCORES: DIFFERENT BUILD", ...  Empty when nothing to say.
const char *  LKS_Peer_Status( const byte * fp );

#endif
