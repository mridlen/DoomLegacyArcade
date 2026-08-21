// [Arcade] Persistent per-map, per-skill best cumulative-time high scores,
// plus the always-on background demo recording tied to new records.
// Single-player only.

#ifndef HS_STUFF_H
#define HS_STUFF_H

#include "doomtype.h"
#include "doomstat.h"     // skill_e

#define HS_NUMSKILLS     5     // sk_baby .. sk_nightmare (doomstat.h)

// [Arcade] Buffer size for the always-on background recording.
// ~5-9 bytes/tic at TICRATE=35 => ~175-280 B/s; 8MB gives well over an
// hour of continuous non-idle play, far beyond what the idle-to-title
// timeout (cv_idletimeout) will ever let a single session run.
#define HS_DEMOBUFFER_SIZE  (8*1024*1024)

// [Arcade] The competitive baseline: vanilla difficulty settings with the
// Boom/MBF engine behavior left at its defaults (roughly complevel 11).
// Applied at startup and again on the way back to the attract screen, so
// every player starts ranked; checked at each new game and level exit, so a
// session that changes any of it plays on but scores and records nothing.
void      HS_Apply_Ranked_Ruleset(void);
boolean   HS_Ruleset_Is_Ranked(void);   // do the cvars match right now?
boolean   HS_Run_Is_Ranked(void);       // has this run stayed ranked?
const char *  HS_Unranked_Reason(void); // name of first differing cvar, or NULL

void  HS_Init(void);
void  Command_ClearHighScores_f(void);   // console: clearhighscores
void  HS_NewGame(void);
// [Arcade] Reset the intermission's per-run display state when a demo starts.
// A replayed record demo spans several levels and passes through real level
// exits, so it needs its own running total rather than the last live game's.
void  HS_Demo_Start(void);
// [Arcade] A death voids the rest of the run: no further level exit scores,
// and the background recording is closed.  Levels finished before it keep
// the records they already earned.  Called from P_KillMobj.
void  HS_Player_Died(void);
boolean  HS_Run_Died(void);   // was this run voided by a death?

// [Arcade] A cheat was used; voids the run the same way a death does.
void  HS_Player_Cheated(void);
boolean  HS_Run_Cheated(void);
// maxed: this level was exited with 100% kills and 100% secrets (items are
// not required).  A run stays eligible for the "max" record only while every
// level of it has been maxed; the "speed" record ignores this entirely.
void  HS_LevelExit(int episode, int map, skill_e skill, tic_t leveltime,
                   boolean maxed);
void  HS_Draw_IntermissionTable(int x, int y);
// [Arcade] Single Level menu support.  "single" selects which table to read,
// explicitly rather than from single_level_mode, because the menu shows
// single-level times while the cabinet is still in campaign mode.
// cat: 0 = speed, 1 = max.
boolean  HS_Demo_Path_For(const char * mapname, skill_e skill, int cat,
                          boolean single, char * dest);
// M:SS.ss -- run times, where whole seconds are far too coarse to separate
// two E1M1 runs.  The stored value has always been tics; only the display
// was rounding.  Used everywhere a *run* time is shown: the intermission
// total and best table, the boards, and the record demo captions.
void     HS_Format_Time_CS(tic_t tics, char * buf, size_t bufsize);

// =========================================================================
//   Run leaderboard  [Arcade]
// =========================================================================
// The per-map table above holds *splits*: the best cumulative time anyone
// has reached a given map in, one deep and anonymous.  This is the separate
// board of whole *runs*, which is what a player actually competes on and
// puts their initials against.  Kept in its own file (runs.dat) so the
// existing highscores.dat needs no migration.
//
// Campaign runs are ranked furthest-then-fastest: progress is the primary
// key and time only breaks ties.  That is what lets the great majority of
// cabinet runs -- the ones that end in a death partway through -- land on a
// board at all, with a completed episode naturally sitting at the top
// because nothing outranks it on progress.  Single Level runs are all one
// map, so they rank on time alone.
#define HS_INITIALS_LEN     4     // three characters plus NUL
#define HS_BOARD_DEPTH_RUN  10    // campaign, per (game, skill, category)
#define HS_BOARD_DEPTH_SL    3    // single level, per (game, map, skill, cat)

// The run ended (any route back to the title).  Commits it to the board if
// it earned a place, and arms the initials prompt when it did.  Idempotent:
// Command_ExitGame_f can be reached more than once.
void     HS_Run_Finished(void);
boolean  HS_Initials_Pending(void);
// Write the player's initials onto whatever the finished run placed, and
// disarm.  NULL or empty stores the "nobody entered" placeholder.
void     HS_Set_Initials(const char * ini);
// Best place the finished run took, 1-based, or 0.  For the prompt's header.
int      HS_Run_Place(void);

int      HS_Board_Depth(boolean single);
// One line of a board, place 0-based.  mapname selects the map for a single
// level board and is ignored for the campaign one.  out_range receives the
// map range ("E1M1-E1M5", or a bare map name for a one map run) and must be
// at least 20 bytes; any out pointer may be NULL.
boolean  HS_Board_Entry(boolean single, const char * mapname,
                        skill_e skill, int cat, int place,
                        char * out_initials, char * out_range,
                        tic_t * out_tics);
// Cumulative run time under the intermission's Time row.  label_x is the left
// edge of the caption, time_right_x the right edge of the value.
void  HS_Draw_TotalTime(int label_x, int time_right_x, int y);
// Seconds each score page is on screen.
//
// 3 was left over from the original one-map-per-page scheme, where each page
// held a single line.  A page is now a whole game's map list, which nobody
// can read in three seconds -- this is the number to raise if it still feels
// rushed.  Its cost is bounded now: an appearance shows at most
// HS_Attract_Cycle_Pages() of them, not the whole set.
#define HS_PAGE_SECS  8

void  HS_Attract_Advance_Page(void);  // next page, wrapping
int   HS_Attract_Page_Count(void);    // pages the tables currently justify
// [Arcade] How many pages one appearance shows before handing back to the
// demo cycle.  The whole set is no longer run after every demo: campaign,
// single level, board and per-map pages together reached about 100 seconds
// between demos, which is unusable on a machine advertising itself.  The
// page cursor is deliberately *not* reset between appearances, so the rest
// of the set comes round on later cycles.
int   HS_Attract_Cycle_Pages(void);
void  HS_Draw_AttractTable(void);
boolean  HS_Have_Records(void);   // any times for the running game?
const char *  HS_NextRecordDemoPath(void);

// Caption for the record demo HS_NextRecordDemoPath last returned, e.g.
// "E1M1  ITYTD  MAX  4:32", or NULL when the demo playing is not one of
// ours.  D_DoAdvanceDemo clears it before every attract page, so it only
// describes the demo actually on screen.
void          HS_Clear_DemoLabel(void);
const char *  HS_DemoLabel(void);

#endif // HS_STUFF_H
