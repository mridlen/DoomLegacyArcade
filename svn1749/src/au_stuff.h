// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// [Arcade] Operator audit counters -- the cabinet's bookkeeping.
//
// An arcade board keeps a bookkeeping page: how many games, how long they
// lasted, which maps get played.  This is that page.  It answers questions an
// operator would otherwise be guessing at -- is anyone using the four player
// mode, is the default skill too hard, does anybody get past MAP03, is the
// scoring actually working out there -- and none of it can be recovered after
// the fact, so it has to be counted as it happens.
//
// Counters live in legacyhome/audit.dat, plain text, one "key value" per line,
// like highscores.dat.  Read at startup, written when a game ends and at
// shutdown.  Nothing here affects play: no gameplay code should ever read
// these, and they are deliberately not part of the demo header or the ranked
// ruleset.
//
//-----------------------------------------------------------------------------

#ifndef AU_STUFF_H
#define AU_STUFF_H

#include "doomtype.h"

// Why a run stopped being scored.  Kept apart because they mean different
// things to an operator: a ruleset count means players are changing settings
// (or that the ruleset is wrong), a cheat count means the cheats menu is on
// and being used, and a death count is just how hard the cabinet is.
typedef enum {
    AU_UR_ruleset = 0,
    AU_UR_cheat,
    AU_UR_death,
    AU_NUMUNRANKED
} au_unranked_e;

void  AU_Init( void );          // load counters, count this boot
void  AU_Save( void );          // flush to disk
void  AU_Clear( void );         // reset every counter, and the "since" date

void  AU_Ticker( void );        // once per tic: uptime and play time

void  AU_Game_Started( void );      // a game began (not a demo)
void  AU_Level_Started( void );     // a level loaded (not a demo)
void  AU_Level_Completed( void );   // a level was exited (not a demo)
void  AU_Player_Death( void );      // a player died
void  AU_Unranked( au_unranked_e reason );   // a run stopped scoring
void  AU_Board_Placement( void );   // a run took a place on a board

// Draw the audit page body.  It owns its own layout in base (320x200)
// coordinates; the caller draws the title and has already set up the draw.
void  AU_Drawer( void );

#endif  // AU_STUFF_H
