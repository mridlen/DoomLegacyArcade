// [Arcade] Shared high scores: the merge every cabinet computes.
//
// Two cabinets each hold a split table (highscores.dat) and a run board
// (runs.dat).  When they sync, each sends the other what it holds and both run
// HSM_Merge over the two -- the same pure function on the same inputs -- so
// both arrive at the same tables without either being "the copy".  That only
// converges if the merge is commutative, idempotent and associative, which in
// turn needs every comparison to be a total order: two records with equal
// times from two cabinets must not compare equal, or each cabinet keeps its
// own for ever.  See docs/arcade/cabinet-link.md, "Shared high scores".
//
// Deliberately no engine includes: tools/hsmerge-test.py compiles this file
// on its own and drives it exhaustively, and hs_stuff.c uses the same code for
// its own board ranking so a local insert and a merge can never disagree.

#ifndef HS_MERGE_H
#define HS_MERGE_H

#include <stdint.h>

#define HSM_GAME_LEN    40    // HS_GAMEID_LEN: "doom2", "doomu+pack-sl"
#define HSM_MAP_LEN      9    // "MAP01", "E1M1" and the NUL
#define HSM_INI_LEN      4    // HS_INITIALS_LEN
#define HSM_CAB_LEN     10    // LK_ID_SHORT_LEN: "7F3A-91C2"
#define HSM_SHA_LEN     32
#define HSM_NUMCAT       4    // HS_NUMCAT: speed, max, pacifist, tyson
#define HSM_NUMSKILLS    6    // HS_NUMSKILLS: five skills and No Monsters

#define HSM_DEPTH_SURVIVAL  1   // HS_BOARD_DEPTH_RUN
#define HSM_DEPTH_SINGLE    3   // HS_BOARD_DEPTH_SL

// set_time: Unix seconds on the cabinet that set the record, or 0 when
// unknown -- a record from before Phase 2, or a clock that had not been set
// (a Pi with no real-time clock boots in 1970).  0 counts as the *oldest*
// time, so an old record keeps its place on a tie, which is the arcade rule a
// cabinet has always applied locally (the first to reach a time keeps it).
// Anything before 2026-01-01 is stored as 0.
#define HSM_TIME_VALID_FROM  1767225600u

// cab: the short id of the cabinet that set it.  Empty means unknown: a line
// written before shared scores, or with the link never switched on.  It stays
// unknown everywhere, so the same old record held by two cabinets is one
// record, not two.

// One per-map record: the best time for (game, map, category, skill), and the
// demo that set it.
typedef struct
{
    char      game[HSM_GAME_LEN];
    char      map[HSM_MAP_LEN];
    uint8_t   skill, cat;
    uint32_t  tics;
    char      startmap[HSM_MAP_LEN];   // empty when unknown
    uint32_t  set_time;
    char      cab[HSM_CAB_LEN];
    uint8_t   sha[HSM_SHA_LEN];        // the demo's SHA-256; all zero = none known
} hsm_split_t;

// One run board entry.  Survival boards (a campaign game id) are one deep per
// (game, episode, skill, category) and carry that board's demo; single level
// boards (a "-sl" game id) are three deep per (game, map, skill, category)
// and have no demo of their own -- the split record holds it.
typedef struct
{
    char      game[HSM_GAME_LEN];
    char      startmap[HSM_MAP_LEN];
    char      endmap[HSM_MAP_LEN];
    uint8_t   skill, cat;
    uint32_t  tics;
    char      initials[HSM_INI_LEN];   // empty = nobody entered them
    uint32_t  set_time;
    char      cab[HSM_CAB_LEN];
    uint8_t   sha[HSM_SHA_LEN];
} hsm_run_t;

// A whole set of scores: what one cabinet holds, or the result of a merge.
// The arrays belong to the caller; max_* is their capacity.
typedef struct
{
    // When the scores were last cleared on the master (Unix seconds), 0 for
    // never.  Records a set holds under an older epoch survive a merge only if
    // they were set after that clear: a member switched off during a clear
    // loses what it had, but keeps a record played on it since.
    uint32_t       epoch;
    hsm_split_t *  splits;
    int            nsplits, max_splits;
    hsm_run_t *    runs;
    int            nruns, max_runs;
} hsm_set_t;

// Map progression order: Doom 2's MAP31/32 sit between MAP15 and MAP16,
// Doom 1 is episode then map.  Unrecognised names go last.
int   HSM_Map_Order( const char * mapname );
// Episode a map belongs to; 1 for the MAPxx games.
int   HSM_Episode_Of( const char * mapname );
// A single level game id ends in "-sl".
int   HSM_Id_Is_Single( const char * game );
int   HSM_Board_Depth( int single );

// Do two entries compete for the same board places?
int   HSM_Same_Board( const hsm_run_t * a, const hsm_run_t * b );
// Rank within one board.  Negative when a outranks b: further first (Survival),
// then faster, then set earlier, then by cabinet, then by the rest of the
// entry -- zero only for entries that are identical but for their initials.
int   HSM_Run_Rank_Cmp( const hsm_run_t * a, const hsm_run_t * b );
// The same rule for split records of one key.  Zero only when identical.
int   HSM_Split_Cmp( const hsm_split_t * a, const hsm_split_t * b );

// out = merge(a, b).  If the epochs differ, the set with the lower one only
// contributes records set at or after the higher epoch: a clear on the master
// spreads instead of being undone, and a record with no set time never
// outlives one.  Splits keep
// the best record per key, boards the union trimmed to depth, and an entry
// both sides hold keeps whichever initials were entered.  The result is in
// canonical order, so two cabinets holding the same scores write the same
// bytes.  out's arrays must not overlap a's or b's.  Returns 0, or -1 when out
// is too small (out is then unusable).
int   HSM_Merge( const hsm_set_t * a, const hsm_set_t * b, hsm_set_t * out );

// Put one set in canonical order and trim it, in place: merge(a, nothing).
// Returns 0, or -1 when scratch is too small (it needs a's counts).
int   HSM_Normalize( hsm_set_t * a, hsm_set_t * scratch );

// Same contents in the same order (used to decide whether a merge changed
// anything).
int   HSM_Equal( const hsm_set_t * a, const hsm_set_t * b );

#endif
