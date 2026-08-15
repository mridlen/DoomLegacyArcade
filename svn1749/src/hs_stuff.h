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

void  HS_Init(void);
void  Command_ClearHighScores_f(void);   // console: clearhighscores
void  HS_NewGame(void);
// maxed: this level was exited with 100% kills and 100% secrets (items are
// not required).  A run stays eligible for the "max" record only while every
// level of it has been maxed; the "speed" record ignores this entirely.
void  HS_LevelExit(int episode, int map, skill_e skill, tic_t leveltime,
                   boolean maxed);
void  HS_Draw_IntermissionTable(int x, int y);
void  HS_Draw_AttractTable(void);
boolean  HS_Have_Records(void);   // any times for the running game?
const char *  HS_NextRecordDemoPath(void);

#endif // HS_STUFF_H
