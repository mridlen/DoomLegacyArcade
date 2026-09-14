// [Arcade] Cabinet Link, Phase 3: invites and linked games.
//
// The join screen of one cabinet invites every idle cabinet running the same
// game; whoever presses fire there joins the same game over the network.  This
// file is the invite protocol and its state, all on the game thread; the link
// (d_link.c) only carries the messages and seals the game's UDP.  Built in
// every binary: without the link it never has a peer, so it never invites.
//
// See docs/arcade/cabinet-link.md, "Invites and networked games".

#ifndef D_LINKGAME_H
#define D_LINKGAME_H

#include "doomtype.h"

enum
{
    LKG_CAT_NONE = 0,
    LKG_CAT_DEATHMATCH,
    LKG_CAT_CAMPAIGN
};

// Called from LK_Ticker every pass while the link runs.
void         LKG_Ticker( void );
// tools/linktest.sh (-linktest -linkkeys): press buttons on the Cabinet Link
// page from a script.  Called by LK_Ticker whatever the link's role.
void         LKG_Test_Keys( void );

// --- The host: a cabinet whose own join screen is up ---

// Is there anyone to invite into this kind of game?  The join screen opens on
// a single panel cabinet only when there is.
boolean      LKG_Would_Invite( byte category );
// The join screen opened: invite the other cabinets.
void         LKG_Host_Begin( byte category, const char * map, byte skill, int secs );
// Every panel that pressed in on another cabinet has locked in (or none did).
boolean      LKG_Remotes_All_Locked( void );
// The join screen is starting the game: send START to the cabinets with
// players in and begin the sealed game channel.  Returns their player count.
int          LKG_Host_Start( void );
// Players on other cabinets in the game being started (0 when none).
int          LKG_Remote_Players( void );
// A remote cabinet has players in this join screen (presence: HOSTING).
boolean      LKG_Hosting_Remote( void );

// --- A cabinet in a linked game it joined ---

// The host's idle timeout and idle warning, in seconds (timeout 0 is Off),
// which a joined game runs on instead of this cabinet's own.  False, leaving
// both untouched, when this cabinet is not in a linked game it joined.
boolean      LKG_Host_Idle_Settings( int * timeout_secs, int * warn_secs );

// --- Either side ---

// The join screen was backed out of (Escape).
void         LKG_Join_Abandoned( void );
// One line for the join screen: who else is in, or whose game this is.
const char * LKG_Join_Line( void );
// For the status output.
int          LKG_Players_In_Game( void );
const char * LKG_Mode_Name( void );

#endif
