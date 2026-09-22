// [Arcade] Crash report and black box.  See docs/arcade/crash-diagnostics.md.
//
// On a fatal signal, print a backtrace and the game state to stderr and append
// it to legacyhome/crash.txt, then die the ordinary way so the core dump is
// still kept.  The black box records the last friends-list additions, which is
// what a core dump cannot tell you after the fact: who put an entry there.

#ifndef D_CRASH_H
#define D_CRASH_H

typedef struct thinker_s  thinker_t;   // d_think.h

// Once legacyhome is known.  Installs the fatal signal handlers (Linux).
void  D_Crash_Init( void );

// The demo about to play, with its full path.  The engine's own 'demoname' is
// cut at 32 characters, which loses the file name of every record demo.
void  D_Crash_Set_Demo( const char * path );

// A thinker was added to the friends list (P_UpdateClassThink,
// P_MoveClassThink).  'caller' is the code that asked.
void  D_Crash_Friend_Added( thinker_t * th, void * caller );

// True when a thinker is not an object (mobj_t) of any kind.  The class-lists
// and the blockmap hold nothing else; the spirit and Heretic's blaster
// projectiles are objects with their own thinker functions.
int   D_Crash_Not_Object( thinker_t * th );

// A class-list operation was handed something that is not an object thinker.
// Logged, never acted on -- the caller carries on exactly as before.
void  D_Crash_Class_Anomaly( const char * what, thinker_t * th, void * caller );

#endif
