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
// maxed: this level was exited with 100% kills and 100% secrets (items are
// not required).  A run stays eligible for the "max" record only while every
// level of it has been maxed; the "speed" record ignores this entirely.
void  HS_LevelExit(int episode, int map, skill_e skill, tic_t leveltime,
                   boolean maxed);
void  HS_Draw_IntermissionTable(int x, int y);
// Cumulative run time under the intermission's Time row.  label_x is the left
// edge of the caption, time_right_x the right edge of the value.
void  HS_Draw_TotalTime(int label_x, int time_right_x, int y);
// The attract page shows one map at a time and steps through every map that
// has a time before handing back to the demo cycle.  Seconds each map is on
// screen; the whole page lasts this times HS_Attract_Page_Count().
#define HS_PAGE_SECS  3

void  HS_Attract_Reset_Pages(void);   // back to the first map
void  HS_Attract_Advance_Page(void);  // next map, wrapping
int   HS_Attract_Page_Count(void);    // maps with at least one time
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
