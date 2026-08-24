// [Arcade] Persistent per-map, per-skill best cumulative-time high scores,
// plus the always-on background demo recording tied to new records.
// Single-player only.

#include <unistd.h>     // access()
#include <sys/types.h>
#include <dirent.h>     // demo directory sweep in Command_ClearHighScores_f
#include <time.h>       // seed for the attract demo shuffle

#include "doomincl.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "m_misc.h"
#include "m_argv.h"
#include "i_system.h"
#include "v_video.h"
#include "screen.h"
#include "m_menu.h"     // M_LevelPack_LoadedName
#include "p_local.h"    // ranked ruleset cvars
#include "p_spec.h"     // cv_zerotags
#include "d_netcmd.h"   // cv_itemrespawn, cv_respawnmonsters
#include "s_sound.h"    // cv_rndsoundpitch
#include "z_zone.h"     // PU_CACHE, for the attract page skill graphic
#include "hs_stuff.h"

#define HS_MAX_MAPS      64

// Records are keyed by game as well as map: Doom 2, Plutonia and TNT all
// have a MAP01, and they are different levels.  The loaded level pack is
// part of that key too ("doomu+mapsofchaos"), since a pack replaces the
// maps -- the same game with and without a pack are different levels again.
#define HS_GAMEID_LEN  40

// Two categories of record, both timed the same way.  A "max" run is one
// that has taken 100% kills and 100% secrets on every level so far (items
// are not required); the first level exited short of that ends the max run,
// while the speed run continues.
#define HS_NUMCAT  2
enum { HS_CAT_speed = 0, HS_CAT_max = 1 };
static const char * hs_catname[HS_NUMCAT] = { "speed", "max" };

// Time columns are right-justified at x + HS_COL_TIME + cat*HS_COL_STEP.
// Sized so the skill label (up to "ITYTD") clears the first column and the
// last column's right edge still lands inside BASEVIDWIDTH (320).
//
// [Arcade] Widened when these times went to hundredths, and re-measured
// against the real STCFN lumps at the call site's new x of 138:
//   skill label   "ITYTD" 36px    138 .. 174
//   speed time    "888:88.99" 64px right-justified at 138+108 = 246,
//                                 so 182 .. 246, clearing the label by 8
//   max time      right-justified at 138+108+72 = 318, so 254 .. 318,
//                                 clearing the speed column by 8
// 318 is inside BASEVIDWIDTH 320, and the headers ("speed" 39px, "max" 26px)
// are narrower than the values they sit over.  The call site moved from 156
// to 138 to open the extra 40px these two columns needed; "NEW RECORD" is
// still centred in everything left of x, which at 77px lands at 30..107.
#define HS_COL_TIME  108
#define HS_COL_STEP   72

typedef struct
{
    char     game[HS_GAMEID_LEN];   // gamedesc idstr: doom2, plutonia, tnt...
    char     mapname[9];
    boolean  has_record[HS_NUMCAT][HS_NUMSKILLS];
    tic_t    besttime[HS_NUMCAT][HS_NUMSKILLS];
    // [Arcade] Map the run that set this record began on.  besttime is
    // cumulative from the start of a run, so a record at E1M5 is normally an
    // E1M1-E1M5 time -- worth saying out loud on the attract captions, where
    // a five level run was previously indistinguishable from a one level one.
    // Stored rather than inferred from the episode: every menu-started
    // campaign does begin at map 1, but that is a property of the menus, not
    // of the record, and anything that ever starts a run mid-episode would
    // silently make the inferred range a lie.
    char     startmap[HS_NUMCAT][HS_NUMSKILLS][9];
} hs_maprecord_t;

static hs_maprecord_t  hs_table[HS_MAX_MAPS];
static int              hs_table_count = 0;

// -------------------------------------------------------------------------
// [Arcade] The run board.  See the header for why this is a separate table
// and a separate file from the per-map splits above.
#define HS_MAX_RUNS  256

typedef struct
{
    char   game[HS_GAMEID_LEN];   // carries the "-sl" suffix for single level
    char   startmap[9];
    char   endmap[9];
    byte   skill;
    byte   cat;
    tic_t  tics;
    char   initials[HS_INITIALS_LEN];
} hs_run_t;

static hs_run_t  hs_runs[HS_MAX_RUNS];
static int       hs_runs_count = 0;
static char      hs_runfile[MAX_WADPATH];

// The run in progress, frozen at its last *scored* level exit.  Deliberately
// separate from hs_cumulative_time, which keeps accumulating after a death
// so the intermission can still show an honest elapsed time: the board entry
// must stop at the last level the run actually completed under scoring.
static char    hs_run_startmap[9] = "";
static char    hs_run_endmap[9]   = "";
// The run's own skill, *not* hs_last_exit_skill.  That one is set above the
// demoplayback guard in HS_LevelExit, so an attract demo playing behind the
// initials prompt -- which sits there for up to cv_initialstimeout seconds --
// would overwrite it, and the initials would then be looked up against the
// wrong board and silently never stamped.
static skill_e hs_run_skill       = sk_baby;
static tic_t   hs_run_tics        = 0;
static int     hs_run_levels      = 0;    // scored level exits this run
static boolean hs_run_endmap_max  = false;// was the max category still alive?
static char    hs_run_gameid[HS_GAMEID_LEN] = "";

// Ruleset-and-cheating only -- a *death does not clear this*, which is the
// whole point of the progress board.  hs_run_ranked still goes false on a
// death (it gates the split records, where a free level reload would
// otherwise make dying a costless retry); this stays true so the run can
// still take its place on the board for how far it actually got.
// Initialised true to match hs_run_ranked above, and for the same reason: a
// game started from the command line (-warp) never runs HS_NewGame, so a
// flag defaulting false would let such a game write split records while
// silently never reaching the board.  The two must agree.
static boolean hs_run_board_ok = true;

// Set by HS_Run_Finished when the finished run took a place; cleared by
// HS_Set_Initials.
static boolean hs_initials_pending = false;
// Board places this run took, so the initials can be stamped on all of them.
// A list of full keys rather than a list of categories, because one run can
// now place on *two different boards*: a campaign run that stops after its
// first level also competes on that map's single level board (see
// HS_Score_As_Single_Level), so it may hold a campaign entry and a single
// level entry at once, under different game ids and different maps.
typedef struct
{
    char   game[HS_GAMEID_LEN];
    char   endmap[9];
    byte   skill;
    byte   cat;
    tic_t  tics;
} hs_placement_t;

#define HS_MAX_PLACED  (HS_NUMCAT * 2)   // campaign + single level, both cats
static hs_placement_t hs_placed[HS_MAX_PLACED];
static int     hs_placed_n = 0;
static int     hs_run_best_place = 0;

// =========================================================================
//   Ranked ruleset
// =========================================================================
// The cabinet's competitive baseline.  DoomLegacy has no separate single
// player code path -- solo play runs the same client/server simulation, and
// every gameplay setting below is a single global CV_NETVAR -- so anything a
// player changes under Options applies to their scored run.  Rather than
// hide the menus, the ruleset is applied at startup and then *checked*: a
// session that no longer matches it still plays, but records nothing.
//
// The baseline is "vanilla difficulty knobs, Boom/MBF engine behavior":
// every setting that makes the game easier or harder is pinned to its
// vanilla value, while the Boom and MBF AI/physics fixes stay at their
// defaults so Boom-format level packs still work.  Roughly complevel 11.
//
// Values are in the units the menu shows; CV_FLOAT cvars (gravity) are
// scaled to fixed_t internally, exactly as CV_Set does.
typedef struct
{
    consvar_t *  cv;
    int          val;
} hs_rule_t;

static hs_rule_t  hs_ranked_rules[] =
{
    // --- vanilla difficulty knobs ---
    { &cv_gravity,               1 },   // fixed_t FRACUNIT internally
    { &cv_itemrespawn,           0 },
    { &cv_itemrespawntime,      30 },
    { &cv_respawnmonsterstime,  12 },
    { &cv_monbehavior,           0 },   // Vanilla
    { &cv_predictingmonsters,    0 },
    { &cv_solidcorpse,           0 },
    { &cv_monstergravity,        0 },   // Vanilla (see G_demo_defaults)
    { &cv_monsterfriction,       0 },   // Vanilla
    { &cv_voodoo_mode,           0 },   // VM_vanilla
    { &cv_instadeath,            0 },
    { &cv_weapon_recoil,         0 },
    { &cv_allowjump,             0 },   // vanilla Doom has no jumping
    { &cv_rndsoundpitch,         0 },   // consumes M_Random, perturbs the RNG
    { &cv_mbf_dogs,              0 },   // no helper dogs fighting for you
    // [Arcade] The player-facing pair, NOT the engine cv_fastmonsters /
    // cv_respawnmonsters -- see the note below the table.
    { &cv_fastmonsters_menu,     0 },
    { &cv_respawnmonsters_menu,  0 },
#ifdef DOORDELAY_CONTROL
    { &cv_doordelay,             1 },
#endif
#ifdef MAPTHING_ADJUST
    { &cv_monster_health,        0 },
    { &cv_health_pickup,         0 },
    { &cv_armor_pickup,          0 },
    { &cv_ammo_pickup,           0 },
#endif
#ifdef ENABLE_TIRED_RUN
    { &cv_tired_run,             0 },
    { &cv_drown,                 0 },
#endif
#ifdef MONSTER_VARY
    { &cv_monster_vary,          0 },
    { &cv_vary_percent,          5 },
    { &cv_vary_size,             3 },
#endif
#ifdef ENABLE_TELE_CONTROL
    { &cv_tele_control,          0 },
#endif
#ifdef ENABLE_SLOW_REACT
    { &cv_slow_react,            0 },
#endif

    // --- Boom/MBF behavior, pinned at its default so packs still work ---
    { &cv_monster_remember,             1 },
    { &cv_mbf_monster_avoid_hazard,     1 },
    { &cv_mbf_monster_backing,          0 },
    { &cv_mbf_pursuit,                  0 },
    { &cv_mbf_dropoff,                  1 },
    { &cv_mbf_staylift,                 1 },
    { &cv_mbf_help_friend,              1 },
    { &cv_mbf_distfriend,             128 },
    { &cv_mbf_monkeys,                  0 },
    { &cv_mbf_falloff,                  1 },
    { &cv_doorstuck,                    2 },
    { &cv_zerotags,                     1 },
#ifdef DOGS
    { &cv_mbf_dog_jumping,              1 },
#endif
#ifdef GENERATE_BLOCKMAP
    { &cv_blockmap_gen,                 3 },
#endif
};

#define HS_NUM_RULES  (sizeof(hs_ranked_rules)/sizeof(hs_ranked_rules[0]))

// Deliberately NOT in the table: the *engine* cvars cv_respawnmonsters and
// cv_fastmonsters.  G_InitNew turns both on for sk_nightmare (g_game.c,
// "skill == sk_nightmare" -> CV_SetParam), so their value is part of the
// skill rather than a player setting: checking them directly made every
// Nightmare run play MAP01 normally and then report UNRANKED from MAP02, the
// signature of a rule the *engine* changes after HS_NewGame.
//
// What is in the table instead is the player-facing pair,
// cv_fastmonsters_menu / cv_respawnmonsters_menu (m_menu.c).  Nothing in the
// engine ever writes those, so Nightmare no longer trips the check, while a
// player who turns either on from Game Options does -- which is what has to
// happen: both change the simulation, and a mid-run change would desync the
// record demo the run is being recorded into.  The engine pair is still
// recorded in the demo header, so saved records replay correctly.

// What the simulation is actually running with.  command.c stores wide and
// float values in .value and everything else in the .EV byte (which is also
// what demo playback overwrites), so mirror that same test.
static int  hs_rule_current( consvar_t * cv )
{
    return ( cv->flags & (CV_FLOAT | CV_VALUE) ) ? cv->value : (int) cv->EV;
}

static int  hs_rule_expected( const hs_rule_t * r )
{
    return ( r->cv->flags & CV_FLOAT ) ? (r->val * FRACUNIT) : r->val;
}

void HS_Apply_Ranked_Ruleset( void )
{
    unsigned int i;
    const char * bad;

    for( i=0; i<HS_NUM_RULES; i++ )
        CV_SetValue( hs_ranked_rules[i].cv, hs_ranked_rules[i].val );

    // Self-check.  If a value here is not one of a cvar's PossibleValues,
    // CV_Set rejects it and leaves the cvar alone -- which would leave the
    // cabinet permanently "unranked" and silently recording nothing at all.
    // Say so loudly instead; this is a build error, not a runtime condition.
    bad = HS_Unranked_Reason();
    if( bad )
    {
        GenPrintf( EMSG_warn,
          "Ranked ruleset did not apply: \"%s\" would not take its value.\n"
          "No high scores or record demos will be saved.\n", bad );
    }
}

const char *  HS_Unranked_Reason( void )
{
    unsigned int i;
    for( i=0; i<HS_NUM_RULES; i++ )
    {
        const hs_rule_t * r = &hs_ranked_rules[i];
        if( hs_rule_current(r->cv) != hs_rule_expected(r) )
            return r->cv->name;
    }
    return NULL;
}

boolean  HS_Ruleset_Is_Ranked( void )
{
    return ( HS_Unranked_Reason() == NULL );
}


static tic_t   hs_cumulative_time = 0;
static boolean hs_run_is_max = true;   // every level maxed so far this run
// Latched false the moment the ruleset does not match the ranked baseline,
// so changing a setting mid-run voids it rather than only the levels after.
static boolean hs_run_ranked = true;
// Latched true by HS_Player_Died.  Only distinguishes the *reason* the run
// went unranked, for the HUD marker and the log line; the voiding itself is
// done by clearing hs_run_ranked, exactly as an altered ruleset does.
static boolean hs_run_died = false;
// Latched by HS_Player_Cheated, the same way and for the same reason.
static boolean hs_run_cheated = false;

// [Arcade] Void the run the moment the ruleset stops matching, and say which
// cvar did it.
//
// This used to live only in HS_LevelExit, which meant a Game Option changed in
// the middle of a level voided the run correctly but *said nothing* until the
// level ended -- so a player carried on believing they were still on the
// board, and the one setting most likely to be poked mid-run (Fast Monsters)
// was the one that gave no feedback at all.  It is also the change that most
// needs to be visible immediately: it alters the simulation underneath a demo
// that is being recorded.
//
// Safe to call from a drawer, which is how HU_Drawer reaches it once a frame:
// the latch is monotonic within a run -- both flags only ever go false here,
// and only HS_NewGame sets them true again -- so calling it any number of
// times is the same as calling it once.
//
// Deliberately does NOT return early when hs_run_ranked is already false: a
// run that has died is unranked but still on the board (hs_run_board_ok), and
// altering the ruleset after dying has to take it off the board too.  Skipping
// that case is what a "nothing left to do" early return would quietly get
// wrong.
static void  HS_Void_If_Ruleset_Changed( void )
{
    // Both already false: this can have no further effect (the log below is
    // gated on hs_run_ranked), so skip the table walk.
    if( ! hs_run_ranked && ! hs_run_board_ok )  return;

    if( HS_Ruleset_Is_Ranked() )  return;

    // Name the cvar.  A run silently scoring nothing is very hard to diagnose
    // from the outside -- this is exactly how the Nightmare cv_fastmonsters
    // bug presented (played fine, then UNRANKED with no score for the level
    // just finished).  Once per run, on the transition.
    if( hs_run_ranked )
        GenPrintf( EMSG_info,
            "Run is unranked: \"%s\" differs from the ranked ruleset.\n",
            HS_Unranked_Reason() );

    hs_run_ranked   = false;
    hs_run_board_ok = false;   // [Arcade] off the board as well
}


// [Arcade] Is this a game the high score system has anything to say about?
//
// Scoring is single player only -- there is no board, no record demo and no
// ranked ruleset for a multiplayer game, and nothing in this file writes one.
// HS_LevelExit and HS_Player_Died have always opened with this exact test;
// it is a function now because the *display* needs to ask it too, and having
// the same question written out in four places is how they drift apart.
//
// Local splitscreen sets netgame as well as multiplayer, so a two or four
// player game on one cabinet is correctly excluded by either half.
boolean  HS_Scored_Game( void )
{
    return ! ( netgame || multiplayer || deathmatch );
}


boolean  HS_Run_Is_Ranked( void )
{
    // Re-checked here rather than only at the next level exit, so the HUD's
    // UNRANKED marker appears as soon as the setting changes.
    //
    // Not in a multiplayer game.  Starting one moves several cvars that are
    // in hs_ranked_rules[] -- M_StartServer_Go issues a deathmatch mode, and
    // Deathmatch_OnChange derives cv_itemrespawn from it -- so this would
    // latch the run unranked within a frame of the game starting and paint
    // UNRANKED across a game that was never being scored in the first place.
    // Nothing else noticed, because every *writing* path already returns
    // early on the same test; only the display reached this.
    if( ! demoplayback && HS_Scored_Game() )
        HS_Void_If_Ruleset_Changed();

    return hs_run_ranked;
}

boolean  HS_Run_Died( void )
{
    return hs_run_died;
}

boolean  HS_Run_Cheated( void )
{
    return hs_run_cheated;
}


// [Arcade] A cheat was used, so the run scores nothing from here.  Modelled
// exactly on HS_Player_Died: clearing hs_run_ranked does the voiding, and the
// separate flag only selects the reason shown and logged.
//
// No demoplayback guard is needed -- a demo cannot reach the cheat menu --
// but the netgame/multiplayer one is kept so this stays a single player
// concern, matching the cheats themselves, which refuse in multiplayer.
void  HS_Player_Cheated( void )
{
    if( netgame || multiplayer || deathmatch )  return;
    if( hs_run_cheated )  return;   // already latched

    hs_run_cheated = true;

    if( hs_run_ranked )
    {
        GenPrintf( EMSG_info, "Run is unranked: a cheat was used.\n" );
        hs_run_ranked = false;
    }

    // [Arcade] A cheat *does* take the run off the board, unlike a death:
    // dying is playing badly, cheating is not playing the same game.
    hs_run_board_ok = false;

    // Nothing more can be recorded, so stop spending the demo buffer on it.
    // Records earned before the cheat keep their own saved demos.
    if( demorecording )
        G_CheckDemoStatus();
}
static char    hs_last_exit_mapname[9] = "";
static skill_e hs_last_exit_skill = sk_baby;
// Which categories the level just exited actually beat, for the blinking
// intermission marker.  Recomputed by every scored HS_LevelExit, so it only
// ever describes the level whose intermission is on screen.
static boolean hs_new_record[HS_NUMCAT];

static char    hs_scorefile[MAX_WADPATH];
static char    hs_demodir[MAX_WADPATH];

static const char * hs_skillnames[HS_NUMSKILLS] = { "ITYTD", "HNTR", "HMP", "UV", "NM" };


// The key identifying what is being played, used in the score file and in
// record demo names: the game's short name, plus the loaded level pack.
// Recomputed each call because a pack can be loaded mid-session.
// [Arcade] "single" gives the single-level table its own key, which is all
// that is needed to keep those runs from mixing with campaign ones: records,
// record demo filenames and the attract page are all selected by this id.
// Both attract-screen consumers now take both ids: the score pages give
// campaign and single level times their own page families (HS_Build_Pages),
// and HS_NextRecordDemoPath replays demos from either.  This is *not* how it
// started -- the pages were campaign only, to stop single level times
// leaking on whenever single_level_mode happened to still be set, and to
// stop the page count doubling.  Bounding the cycle to a few pages per
// appearance removed the second objection, and asking for the mode
// explicitly rather than reading the flag removed the first.
static const char * HS_GameId_Mode( boolean single )
{
    static char  id[HS_GAMEID_LEN];
    const char * game = ( gamedesc.idstr && gamedesc.idstr[0] )
                        ? gamedesc.idstr : "game";
    const char * pack = M_LevelPack_LoadedName();
    char * p;

    if( pack )
        snprintf( id, sizeof(id), "%s+%s%s", game, pack, single? "-sl" : "" );
    else
        snprintf( id, sizeof(id), "%s%s", game, single? "-sl" : "" );

    // Keep it a single filename-safe word: this is a space separated field
    // in highscores.dat and part of the record demo filename, and pack names
    // come from arbitrary filenames.
    for( p = id; *p; p++ )
    {
        if( ! ( isalnum((unsigned char)*p)
                || *p=='-' || *p=='_' || *p=='.' || *p=='+' ) )
            *p = '_';
    }
    return id;
}


static const char * HS_GameId( void )
{
    return HS_GameId_Mode( single_level_mode );
}


// [Arcade] Where a *campaign* run holding this record must have begun, for
// records written before the start map was tracked.
//
// This is an inference and was deliberately avoided when the field was added
// -- "every menu-started campaign does begin at map 1, but that is a
// property of the menus, not of the record".  It is used only as a fallback
// for records that have no stored start map, because for those the choice is
// not between a fact and a guess but between a good guess and nothing: a
// cumulative time at MAP03 can only have come from a run that began at
// MAP01, since that is the only way to accumulate time there.  A stored
// start map always wins.
//
// Campaign only.  A single level record is one map by definition, and the
// secret levels need no special case: reaching MAP31 or E1M9 still means a
// run that started at MAP01 or E1M1.
//
// The one thing this cannot describe is a run started mid-episode with
// -warp, which no route through the cabinet's menus can produce.
static void HS_Infer_StartMap( const char * mapname, char * out, size_t outsize )
{
    int e, m;

    out[0] = 0;
    if( sscanf(mapname, "MAP%d", &m) == 1 )
    {
        dl_strncpy( out, "MAP01", outsize );
        return;
    }
    if( sscanf(mapname, "E%dM%d", &e, &m) == 2 )
        snprintf( out, outsize, "E%dM1", e );
}


// The span a record covers, as "E1M1-E1M5", or the bare map name for a run
// of one map.  start may be empty, in which case a campaign record falls
// back to the inference above and a single level one stays bare.
static void HS_Format_Range( const char * start, const char * mapname,
                             boolean single, char * out, size_t outsize )
{
    char inferred[9];

    if( (start == NULL || start[0] == 0) && ! single )
    {
        HS_Infer_StartMap( mapname, inferred, sizeof(inferred) );
        start = inferred;
    }

    if( start && start[0] && strncmp(start, mapname, 8) != 0 )
        snprintf( out, outsize, "%.8s-%.8s", start, mapname );
    else
        snprintf( out, outsize, "%.8s", mapname );
}


// [Arcade] The same time to hundredths.  Times have always been stored as
// tics -- highscores.dat's fourth field is a raw tic count -- so this needs
// no format change and no migration; whole seconds were simply thrown away
// at the point of display, which is far too coarse to separate two E1M1
// runs a handful of tics apart.
//
// TICRATE is 35, which does not divide 100, so the hundredths are a scaled
// tic count rather than exact: floor((tics % 35) * 100 / 35).  That is the
// convention the wider Doom speedrunning world displays, so a cabinet time
// reads the same way as one from anywhere else.
void  HS_Format_Time_CS( tic_t tics, char * buf, size_t bufsize )
{
    int seconds = tics / TICRATE;
    int minutes = seconds / 60;
    int secs    = seconds % 60;
    int cs      = (int)((tics % TICRATE) * 100 / TICRATE);
    snprintf(buf, bufsize, "%d:%02d.%02d", minutes, secs, cs);
}


static hs_maprecord_t * HS_FindOrAddRecord( const char * game, const char * mapname )
{
    int  i;

    for( i=0; i<hs_table_count; i++ )
    {
        if( strncmp(hs_table[i].mapname, mapname, 8) == 0
            && strncmp(hs_table[i].game, game, HS_GAMEID_LEN-1) == 0 )
            return &hs_table[i];
    }

    if( hs_table_count >= HS_MAX_MAPS )
        return NULL;

    hs_maprecord_t * rec = &hs_table[hs_table_count++];
    memset(rec, 0, sizeof(*rec));
    dl_strncpy(rec->game, game, HS_GAMEID_LEN-1);
    dl_strncpy(rec->mapname, mapname, 8);
    return rec;
}


// Demo files carry the game too: a Doom 2 MAP01 demo would replay against
// the wrong level under Plutonia or TNT.
static void HS_BuildDemoPath( char * dest, const char * game,
                              const char * mapname, skill_e skill, int cat )
{
    char relname[96];
    // Bound the parts explicitly; map name is at most 8 ("MAPxx"/"ExMy").
    // The game id can be long once a pack name is folded in, and truncating
    // it would let two packs share a demo file.
    snprintf(relname, sizeof(relname), "%.39s_%.8s_sk%d_%s.lmp",
             game, mapname, (int)skill, hs_catname[cat]);
    cat_filename(dest, hs_demodir, relname);
}


// [Arcade] A Survival demo is one per (game, episode, skill, category),
// matching the board it belongs to -- not one per map, which is what the
// per-map scheme produced.  "ep<N>" cannot collide with a map name because
// map names are MAPxx or ExMy.
static void HS_BuildSurvivalDemoPath( char * dest, const char * game,
                                      int episode, skill_e skill, int cat )
{
    char relname[96];
    snprintf(relname, sizeof(relname), "%.39s_ep%d_sk%d_%s.lmp",
             game, episode, (int)skill, hs_catname[cat]);
    cat_filename(dest, hs_demodir, relname);
}


static void HS_Load( void )
{
    FILE * fr;
    char   line[128];
    char   game[64];   // wider than HS_GAMEID_LEN; copy in is bounded
    char   mapname[16];
    char   catname[16];
    char   startmap[16];
    int    skillnum, cat, i;
    unsigned int  tics;
    int    old_format = 0;

    hs_table_count = 0;

    fr = fopen(hs_scorefile, "r");
    if( ! fr )  return;

    while( fgets(line, sizeof(line), fr) )
    {
        if( line[0] == '#' || line[0] == '\n' || line[0] == 0 )
            continue;
        // Fields are only ever *appended*, so an older short line still
        // loads: four fields is a pre-category speed record, five adds the
        // category, six adds the map the run started on.
        startmap[0] = 0;
        int nf = sscanf(line, "%63s %15s %d %u %15s %15s",
                        game, mapname, &skillnum, &tics, catname, startmap);
        if( nf < 4 )
        {
            // Records written before scores were tracked per game cannot be
            // attributed to one, so they are dropped rather than guessed at.
            old_format ++;
            continue;
        }
        if( skillnum < 0 || skillnum >= HS_NUMSKILLS )
            continue;

        cat = HS_CAT_speed;
        if( nf >= 5 )
        {
            for( i = 0; i < HS_NUMCAT; i++ )
            {
                if( strcasecmp(catname, hs_catname[i]) == 0 )  { cat = i; break; }
            }
        }

        hs_maprecord_t * rec = HS_FindOrAddRecord(game, mapname);
        if( ! rec )  continue;
        rec->has_record[cat][skillnum] = true;
        rec->besttime[cat][skillnum]   = (tic_t) tics;
        // No start map recorded -- either a line written before this field
        // existed, or one HS_Save wrote "-" into because it did not know.
        // *Both must read back as empty.*  Taking the "-" literally makes it
        // a map name that differs from this record's map, which is exactly
        // what the caption treats as a range: it produced "--E4M1" and
        // "SINGLE LEVEL: --E1M1".  The placeholder is written so the field
        // cannot shift on the next read (see HS_Save); it is not a value.
        if( strcmp(startmap, "-") == 0 )  startmap[0] = 0;
        dl_strncpy( rec->startmap[cat][skillnum], startmap, 8 );
    }

    fclose(fr);

    if( old_format )
        GenPrintf(EMSG_info,
                  "High scores: discarded %d record(s) from before per-game scoring.\n",
                  old_format);
}


static void HS_Save( void )
{
    FILE * fw;
    int    i, sk, cat;

    fw = fopen(hs_scorefile, "w");
    if( ! fw )
    {
        GenPrintf(EMSG_warn, "HS_Save: could not write %s\n", hs_scorefile);
        return;
    }

    fprintf(fw, "# DoomLegacy arcade high scores:"
                " wadcombo mapname skill cumulative_tics category startmap\n");
    for( i=0; i<hs_table_count; i++ )
    {
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            for( sk=0; sk<HS_NUMSKILLS; sk++ )
            {
                const char * sm;
                if( ! hs_table[i].has_record[cat][sk] )  continue;

                // Never write an empty field: it would shift every field
                // after it on the next read.  "-" reads back as "no start
                // map known", the same as a line from before this existed.
                sm = hs_table[i].startmap[cat][sk];
                if( sm[0] == 0 )  sm = "-";

                fprintf(fw, "%s %s %d %u %s %s\n",
                        hs_table[i].game, hs_table[i].mapname, sk,
                        (unsigned int) hs_table[i].besttime[cat][sk],
                        hs_catname[cat], sm);
            }
        }
    }

    fclose(fw);
}


// =========================================================================
//   Run board storage  [Arcade]
// =========================================================================

static int   HS_MapOrder( const char * mapname );   // defined with the pages
static void  HS_Board_Sort( const hs_run_t * key ); // defined below

// Is this game id a single level one?  The board rules differ: single level
// runs are all one map so they rank on time alone and sit three deep, while
// campaign runs rank on progress first and sit ten deep.
static boolean  HS_Id_Is_Single( const char * game )
{
    size_t n = strlen(game);
    return (n >= 3) && (strcmp(game + n - 3, "-sl") == 0);
}


int  HS_Board_Depth( boolean single )
{
    return single ? HS_BOARD_DEPTH_SL : HS_BOARD_DEPTH_RUN;
}


// [Arcade] Which episode a map belongs to.  Doom 1 is E<episode>M<map>; the
// flat MAPxx games are one episode -- there, the whole game is the run.
//
// This is the key Survival scoring turns on.  The campaign board used to be
// keyed by game alone, which let HS_MapOrder compare across episodes: on the
// cabinet's own table two one-minute E2M1 attempts (order 201) outranked two
// *completed* Episode 1 runs (E1M8, order 108).  "Furthest" only means
// anything within a single episode.
int  HS_Episode_Of( const char * mapname )
{
    int e, m;
    if( sscanf(mapname, "E%dM%d", &e, &m) == 2 )  return e;
    return 1;
}


// Do two runs compete for the same board places?
static boolean  HS_Same_Board( const hs_run_t * a, const hs_run_t * b )
{
    if( a->skill != b->skill || a->cat != b->cat )  return false;
    if( strncmp(a->game, b->game, HS_GAMEID_LEN-1) != 0 )  return false;

    // A single level board is per map.  A Survival board is per *episode*:
    // see HS_Episode_Of for why keying by game alone was wrong.
    if( HS_Id_Is_Single(a->game) )
        return (strncmp(a->endmap, b->endmap, 8) == 0);

    return (HS_Episode_Of(a->endmap) == HS_Episode_Of(b->endmap));
}


// Ranking within a board.  Negative when a outranks b.
//
// Campaign runs go furthest first and use time only to break ties, so a run
// that died on E1M6 still outranks one that died on E1M3 however fast the
// latter was, and a completed episode tops the board because nothing beats
// it on progress.  Single level runs are all the same map, so progress is
// constant and this reduces to the time comparison.
static int  HS_Run_Cmp( const hs_run_t * a, const hs_run_t * b )
{
    if( ! HS_Id_Is_Single(a->game) )
    {
        int pa = HS_MapOrder(a->endmap);
        int pb = HS_MapOrder(b->endmap);
        if( pa != pb )  return (pb - pa);   // further is better
    }
    if( a->tics != b->tics )  return (a->tics < b->tics) ? -1 : 1;
    return 0;
}


static void HS_Runs_Load( void )
{
    FILE * fr;
    char   line[160];
    char   game[64], startmap[16], endmap[16], catname[16], initials[16];
    int    skillnum, cat, i;
    unsigned int tics;

    hs_runs_count = 0;

    fr = fopen(hs_runfile, "r");
    if( ! fr )  return;

    while( fgets(line, sizeof(line), fr) )
    {
        if( line[0] == '#' || line[0] == '\n' || line[0] == 0 )  continue;

        initials[0] = 0;
        int nf = sscanf(line, "%63s %15s %15s %d %15s %u %15s",
                        game, startmap, endmap, &skillnum, catname,
                        &tics, initials);
        if( nf < 6 )  continue;
        if( skillnum < 0 || skillnum >= HS_NUMSKILLS )  continue;
        if( hs_runs_count >= HS_MAX_RUNS )  break;

        cat = HS_CAT_speed;
        for( i = 0; i < HS_NUMCAT; i++ )
        {
            if( strcasecmp(catname, hs_catname[i]) == 0 )  { cat = i; break; }
        }

        hs_run_t * r = &hs_runs[hs_runs_count++];
        memset(r, 0, sizeof(*r));
        dl_strncpy(r->game, game, HS_GAMEID_LEN-1);
        dl_strncpy(r->startmap, startmap, 8);
        dl_strncpy(r->endmap, endmap, 8);
        r->skill = (byte) skillnum;
        r->cat   = (byte) cat;
        r->tics  = (tic_t) tics;
        // "---" is the placeholder written for a run nobody claimed, and "-"
        // the one for an unknown start map; read both back as empty so one
        // code path covers them.  A placeholder taken literally becomes a
        // value, which is how "--E4M1" happened in the split table.
        if( strcmp(initials, "---") == 0 )  initials[0] = 0;
        if( strcmp(startmap, "-") == 0 )    startmap[0] = 0;
        dl_strncpy(r->initials, initials, HS_INITIALS_LEN);
    }

    fclose(fr);

    // Re-establish "stored order is rank order", which HS_Board_Entry walks
    // to hand out places.  Insertion maintains it, but runs.dat is a plain
    // text file an operator may have edited.  One pass, sorting each board
    // the first time an entry of it is seen.
    for( i=0; i<hs_runs_count; i++ )
    {
        int j;
        for( j=0; j<i; j++ )
        {
            if( HS_Same_Board(&hs_runs[j], &hs_runs[i]) )  break;
        }
        if( j == i )  HS_Board_Sort( &hs_runs[i] );
    }
}


static void HS_Runs_Save( void )
{
    FILE * fw;
    int    i;

    fw = fopen(hs_runfile, "w");
    if( ! fw )
    {
        GenPrintf(EMSG_warn, "HS_Runs_Save: could not write %s\n", hs_runfile);
        return;
    }

    fprintf(fw, "# DoomLegacy arcade run board:"
                " wadcombo startmap endmap skill category tics initials\n");
    for( i=0; i<hs_runs_count; i++ )
    {
        const char * ini = hs_runs[i].initials;
        const char * sm  = hs_runs[i].startmap;
        if( ini[0] == 0 )  ini = "---";
        // Never write an empty field, for the same reason as HS_Save: it
        // would shift every field after it on the next read.  A committed
        // run always has a start map, so this is belt and braces -- but an
        // empty one here would silently reparse the *end* map as the start
        // map, which is the kind of corruption that is very hard to see.
        if( sm[0] == 0 )  sm = "-";
        fprintf(fw, "%s %s %s %d %s %u %s\n",
                hs_runs[i].game, sm, hs_runs[i].endmap,
                (int) hs_runs[i].skill, hs_catname[hs_runs[i].cat],
                (unsigned int) hs_runs[i].tics, ini);
    }

    fclose(fw);
}


// Slots holding the entries of one board, in stored order.  Boards are
// interleaved in hs_runs[] -- entries are appended as runs finish, whatever
// game or skill they belong to -- so every board operation works through
// this gather rather than over a contiguous range.
static int  HS_Board_Slots( const hs_run_t * key, int * out, int out_max )
{
    int  i, n = 0;
    for( i=0; i<hs_runs_count && n<out_max; i++ )
    {
        if( HS_Same_Board(&hs_runs[i], key) )  out[n++] = i;
    }
    return n;
}


// Put one board's entries in rank order.  Selection sort over the gathered
// slots, so the entries move but the slots they occupy do not -- other
// boards interleaved between them are left untouched.
//
// The stored order being the rank order is an invariant the rest of this
// relies on: HS_Board_Entry walks the file order and hands back the nth
// match as the nth place.  Insertion maintains it, and this re-establishes
// it after a load, since runs.dat may have been hand-edited.
static void  HS_Board_Sort( const hs_run_t * key )
{
    int  slot[HS_MAX_RUNS];
    int  n = HS_Board_Slots( key, slot, HS_MAX_RUNS );
    int  a, b;

    for( a=0; a<n-1; a++ )
    {
        int best = a;
        for( b=a+1; b<n; b++ )
        {
            if( HS_Run_Cmp(&hs_runs[slot[b]], &hs_runs[slot[best]]) < 0 )
                best = b;
        }
        if( best != a )
        {
            hs_run_t tmp        = hs_runs[slot[a]];
            hs_runs[slot[a]]    = hs_runs[slot[best]];
            hs_runs[slot[best]] = tmp;
        }
    }
}


// Insert a finished run, keeping its board sorted and pruned to depth.
// Returns the 1-based place taken, or 0 if it did not make the board.
static int  HS_Board_Insert( const hs_run_t * run )
{
    int  slot[HS_MAX_RUNS];
    int  n, place, depth, at;

    depth = HS_Board_Depth( HS_Id_Is_Single(run->game) );
    n = HS_Board_Slots( run, slot, HS_MAX_RUNS );

    // Where it lands among the entries it competes with.  The comparison is
    // strict, so an exactly equal time stops *behind* the entry already
    // there: first to achieve a time keeps the higher place, which is the
    // arcade convention and matters now that times are kept to the tic.
    for( place = 0; place < n; place++ )
    {
        if( HS_Run_Cmp(run, &hs_runs[slot[place]]) < 0 )  break;
    }
    if( place >= depth )  return 0;   // off the bottom of the board

    if( hs_runs_count >= HS_MAX_RUNS )
    {
        // Pruning bounds the file, so this only happens if it was hand
        // edited, or if very many games and packs share one cabinet.
        // Refusing is better than silently dropping someone else's entry.
        GenPrintf(EMSG_warn, "Run board full (%d); entry not recorded.\n",
                  HS_MAX_RUNS);
        return 0;
    }

    // Open a slot at the right position.  Inserting *at* the slot currently
    // holding the entry it displaces keeps this board's stored order equal
    // to its rank order without disturbing any other.
    at = (place < n) ? slot[place]
                     : ((n > 0) ? slot[n-1] + 1 : hs_runs_count);
    memmove( &hs_runs[at+1], &hs_runs[at],
             (hs_runs_count - at) * sizeof(hs_run_t) );
    hs_runs[at] = *run;
    hs_runs_count++;

    // Drop whatever fell off the bottom.  At most one entry can, since the
    // board was already within depth before this insert.
    n = HS_Board_Slots( run, slot, HS_MAX_RUNS );
    if( n > depth )
    {
        int drop = slot[n-1];
        memmove( &hs_runs[drop], &hs_runs[drop+1],
                 (hs_runs_count - drop - 1) * sizeof(hs_run_t) );
        hs_runs_count--;
    }

    return place + 1;
}


boolean  HS_Board_Entry( boolean single, const char * mapname,
                         skill_e skill, int cat, int place,
                         char * out_initials, char * out_range,
                         tic_t * out_tics )
{
    char  wanted[HS_GAMEID_LEN];
    int   i, seen = 0;

    if( cat < 0 || cat >= HS_NUMCAT )  return false;
    if( skill < 0 || skill >= HS_NUMSKILLS )  return false;

    dl_strncpy( wanted, HS_GameId_Mode(single), HS_GAMEID_LEN-1 );

    for( i=0; i<hs_runs_count; i++ )
    {
        const hs_run_t * r = &hs_runs[i];
        if( r->skill != skill || r->cat != cat )  continue;
        if( strncmp(r->game, wanted, HS_GAMEID_LEN-1) != 0 )  continue;
        if( single && mapname
            && strncmp(r->endmap, mapname, 8) != 0 )  continue;

        if( seen++ != place )  continue;

        if( out_initials )
            dl_strncpy( out_initials,
                        r->initials[0] ? r->initials : "---",
                        HS_INITIALS_LEN );
        if( out_range )
        {
            // A one map run reads as the bare map name; only a run that
            // actually spans levels gets a range.  Board entries always
            // carry a real start map -- they are built from live runs, or
            // seeded from single level records -- so the inference inside
            // this helper is a fallback that should never fire here.
            HS_Format_Range( r->startmap, r->endmap,
                             HS_Id_Is_Single(r->game), out_range, 20 );
        }
        if( out_tics )  *out_tics = r->tics;
        return true;
    }

    return false;
}



// -------------------------------------------------------------------------
// The run in progress, and committing it when it ends.

static void  HS_Run_Reset( void )
{
    hs_run_startmap[0] = 0;
    hs_run_endmap[0]   = 0;
    hs_run_skill       = sk_baby;
    hs_run_tics        = 0;
    hs_run_levels      = 0;
    hs_run_endmap_max  = false;
    hs_run_gameid[0]   = 0;
    hs_initials_pending = false;
    hs_placed_n        = 0;
    hs_run_best_place  = 0;
}


// Build the run-so-far as a board entry, for comparison or insertion.
static void  HS_Run_As_Entry( hs_run_t * out, skill_e skill, int cat )
{
    memset( out, 0, sizeof(*out) );
    dl_strncpy( out->game, hs_run_gameid, HS_GAMEID_LEN-1 );
    dl_strncpy( out->startmap, hs_run_startmap, 8 );
    dl_strncpy( out->endmap, hs_run_endmap, 8 );
    out->skill = (byte) skill;
    out->cat   = (byte) cat;
    out->tics  = hs_run_tics;
}


// [Arcade] Would the run as it stands take the Survival board?  Used both
// for the intermission's "ahead of the record" marker and to decide whether
// this run's demo is worth keeping.  Depth is 1, so "leading" is simply
// "beats the one entry there, or there is none".
static boolean  HS_Run_Leads( skill_e skill, int cat )
{
    hs_run_t  run;
    int  slot[HS_MAX_RUNS];
    int  ns;

    if( hs_run_levels == 0 || ! hs_run_board_ok )  return false;
    if( cat == HS_CAT_max && ! hs_run_endmap_max )  return false;

    HS_Run_As_Entry( &run, skill, cat );
    ns = HS_Board_Slots( &run, slot, HS_MAX_RUNS );
    if( ns == 0 )  return true;                 // nothing to beat yet
    return (HS_Run_Cmp( &run, &hs_runs[slot[0]] ) < 0);
}


// [Arcade] Keep this run's demo while it leads its board.
//
// Taken at each scored level exit rather than at the end of the run, because
// a *death* closes the recorder (HS_Player_Died) and under Survival a run
// that died still scores -- on how far it got.  Snapshotting as we go means
// the file always holds the leading run up to its last scored exit.
static void  HS_Snapshot_If_Leading( skill_e skill )
{
    int cat;

    if( ! demorecording )  return;

    // [Arcade] A single level run's demo is per *map*, not per episode, and
    // it is written by HS_Score_As_Single_Level below -- which is also what
    // updates the split record the Single Level menu and the attract
    // rotation read.  Taking a Survival snapshot here as well would put a
    // second copy of the same one-map run at "<game>-sl_ep<N>_...", a name
    // nothing ever reads back.
    if( HS_Id_Is_Single( hs_run_gameid ) )  return;

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        char demopath[MAX_WADPATH];

        if( ! HS_Run_Leads( skill, cat ) )  continue;

        // One demo per (game, episode, skill, category), matching the board.
        HS_BuildSurvivalDemoPath( demopath, hs_run_gameid,
                                  HS_Episode_Of(hs_run_endmap), skill, cat );
        G_SnapshotDemo( demopath );
    }
}


// Put a run on its board and, if it placed, remember the key so the initials
// can be stamped on that exact entry once the player has entered them.
// Deliberately keyed rather than indexed: the array is re-sorted and pruned
// by later inserts, so a stored slot number would not survive.
static void  HS_Record_Placement( const hs_run_t * run )
{
    int place = HS_Board_Insert( run );

    if( place <= 0 )  return;

    if( hs_placed_n < HS_MAX_PLACED )
    {
        hs_placement_t * p = &hs_placed[ hs_placed_n++ ];
        dl_strncpy( p->game, run->game, HS_GAMEID_LEN-1 );
        dl_strncpy( p->endmap, run->endmap, 8 );
        p->skill = run->skill;
        p->cat   = run->cat;
        p->tics  = run->tics;
    }

    if( hs_run_best_place == 0 || place < hs_run_best_place )
        hs_run_best_place = place;
}


// [Arcade] A campaign run that has only finished its *first* level is
// directly comparable with a Single Level run of that map, so it competes on
// the same tables: both are pistol starts of one map, and at the first exit
// the run's cumulative time is exactly that level's time.
//
// **Only the first level.** A campaign MAP02 begins with whatever was
// carried out of MAP01, while a Single Level MAP02 is a pistol start, so
// merging those would compare two different things.  Later exits of a
// campaign run never reach here.
// add_to_board is false for a run that is *already* a Single Level run: it
// is committed to the same board by HS_Run_Finished when it ends, and doing
// it here as well would put two identical entries on a three deep board.
static void  HS_Score_As_Single_Level( const char * mapname, skill_e skill,
                                       tic_t tics, boolean add_to_board )
{
    const char *      sl_id = HS_GameId_Mode( true );
    char              gid[HS_GAMEID_LEN];
    hs_maprecord_t *  rec;
    int               cat;

    // HS_GameId_Mode hands back one static buffer, so copy before anything
    // else calls it.
    dl_strncpy( gid, sl_id, HS_GAMEID_LEN-1 );

    rec = HS_FindOrAddRecord( gid, mapname );
    if( rec == NULL )  return;   // table full

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        hs_run_t run;

        if( cat == HS_CAT_max && ! hs_run_is_max )  continue;

        if( ! rec->has_record[cat][skill]
            || tics < rec->besttime[cat][skill] )
        {
            rec->has_record[cat][skill] = true;
            rec->besttime[cat][skill]   = tics;
            // One map, so the range is the bare name either way.
            dl_strncpy( rec->startmap[cat][skill], mapname, 8 );

            if( demorecording )
            {
                // The buffer holds exactly this one level at this point, so
                // a snapshot of it *is* a valid single level demo.
                char demopath[MAX_WADPATH];
                HS_BuildDemoPath( demopath, gid, mapname, skill, cat );
                G_SnapshotDemo( demopath );
            }
        }

        // The single level board takes it too, so a campaign first level can
        // hold a top three place beside runs started from the Single Level
        // menu.  Recorded now, but the initials prompt is still only raised
        // when the whole run ends.
        if( ! add_to_board )  continue;

        memset( &run, 0, sizeof(run) );
        dl_strncpy( run.game, gid, HS_GAMEID_LEN-1 );
        dl_strncpy( run.startmap, mapname, 8 );
        dl_strncpy( run.endmap, mapname, 8 );
        run.skill = (byte) skill;
        run.cat   = (byte) cat;
        run.tics  = tics;
        HS_Record_Placement( &run );
    }

    HS_Save();
}


// [Arcade] Called from every route back to the title (Command_ExitGame_f),
// which is the point a run is definitively over: the player finished the
// episode, died and gave up, chose End Game, or walked away and let the idle
// timeout fire.
//
// Idempotent, because that funnel can be reached more than once -- notably
// M_SingleLevel_Finished calls it and then the menu it returns to may lead
// straight back out again.  Committing sets hs_run_levels to 0, so a second
// call finds nothing to do.
void  HS_Run_Finished( void )
{
    hs_run_t  run;
    int  cat;

    // [Arcade] Both of these used to return in silence, which made "my run
    // did not go on the board" impossible to tell apart from "my run did not
    // place".  Name the reason, the same way HS_LevelExit names the cvar that
    // made a run unranked.
    if( hs_run_levels == 0 )
    {
        // Not necessarily wrong: this is also the idempotent second call from
        // Command_ExitGame_f after a death already committed the run.
        GenPrintf( EMSG_debug,
                   "Run not committed: no level was scored.\n" );
        return;
    }

    if( ! hs_run_board_ok )
    {
        GenPrintf( EMSG_info,
            "Run not committed to the board: the ruleset was altered or a"
            " cheat was used (%s..%s, %d level(s)).\n",
            hs_run_startmap[0] ? hs_run_startmap : "?",
            hs_run_endmap[0] ? hs_run_endmap : "?", hs_run_levels );
        HS_Run_Reset();
        return;
    }

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        // The max board only takes runs that were still 100% at the point
        // they stopped; the speed board takes every scored run.
        if( cat == HS_CAT_max && ! hs_run_endmap_max )  continue;

        memset( &run, 0, sizeof(run) );
        dl_strncpy( run.game, hs_run_gameid, HS_GAMEID_LEN-1 );
        dl_strncpy( run.startmap, hs_run_startmap, 8 );
        dl_strncpy( run.endmap, hs_run_endmap, 8 );
        run.skill = (byte) hs_run_skill;
        run.cat   = (byte) cat;
        run.tics  = hs_run_tics;

        HS_Record_Placement( &run );
    }

    if( hs_placed_n == 0 )
    {
        GenPrintf( EMSG_info,
            "Run finished %s..%s (%d level(s)) but did not make the board.\n",
            hs_run_startmap[0] ? hs_run_startmap : "?",
            hs_run_endmap[0] ? hs_run_endmap : "?", hs_run_levels );
    }

    if( hs_placed_n > 0 )
    {
        HS_Runs_Save();          // the place is real even if nobody claims it
        // Armed only here, at the end of the run.  A placement can be
        // recorded much earlier -- the single level one is taken at the
        // first level exit -- and raising the prompt then would put it over
        // the intermission of a game still in progress.
        hs_initials_pending = true;
        GenPrintf( EMSG_info, "Run placed %d on the board (%s %s-%s).\n",
                   hs_run_best_place, hs_run_gameid,
                   hs_run_startmap, hs_run_endmap );
    }

    // Consumed: the run's own state is done with, but the placement is left
    // standing for HS_Set_Initials to find.
    hs_run_levels = 0;
}


boolean  HS_Initials_Pending( void )
{
    return hs_initials_pending;
}


int  HS_Run_Place( void )
{
    return hs_run_best_place;
}


void  HS_Set_Initials( const char * ini )
{
    char  clean[HS_INITIALS_LEN];
    int   i, n;

    if( ! hs_initials_pending )  return;

    // Keep it to the characters hu_font can actually draw, uppercased.
    memset( clean, 0, sizeof(clean) );
    if( ini )
    {
        for( i=0, n=0; ini[i] && n < HS_INITIALS_LEN-1; i++ )
        {
            unsigned char uc = (unsigned char) ini[i];
            if( uc <= ' ' )  continue;
            clean[n++] = toupper(uc);
        }
    }

    // Stamp every entry this run placed.  Both categories can place at once,
    // and they are the same run by the same player.
    for( i=0; i<hs_placed_n; i++ )
    {
        const hs_placement_t * pl = &hs_placed[i];
        int  slot[HS_MAX_RUNS];
        int  ns, s;
        hs_run_t key;

        memset( &key, 0, sizeof(key) );
        dl_strncpy( key.game, pl->game, HS_GAMEID_LEN-1 );
        dl_strncpy( key.endmap, pl->endmap, 8 );
        key.skill = pl->skill;
        key.cat   = pl->cat;

        ns = HS_Board_Slots( &key, slot, HS_MAX_RUNS );
        for( s=0; s<ns; s++ )
        {
            hs_run_t * r = &hs_runs[slot[s]];
            // The entry this placement made: same end map and same time, and
            // not already claimed.  Matching on the time as well as the map
            // is what keeps this off an older entry that happens to share
            // the end map.
            if( r->tics != pl->tics )  continue;
            if( strncmp(r->endmap, pl->endmap, 8) != 0 )  continue;
            if( r->initials[0] )  continue;
            dl_strncpy( r->initials, clean, HS_INITIALS_LEN );
            break;
        }
    }

    HS_Runs_Save();

    hs_initials_pending = false;
    hs_placed_n         = 0;
    hs_run_best_place   = 0;
}


static void  HS_Shuffle_Seed( void );   // attract demo replay order, below

// [Arcade] Give a cabinet that already has scores a populated single level
// board on the first run of this build, instead of a page of dashes beside
// times it can plainly see elsewhere.
//
// Only the single level records can be converted: one of those *is* one run,
// so the mapping is exact.  A campaign split cannot be -- several of them
// come from the same run and nothing in the old file says which -- so the
// campaign board deliberately starts empty and fills as runs are played.
// Seeded entries carry the "nobody claimed this" placeholder for initials.
static void  HS_Seed_Runs_From_Splits( void )
{
    int  i, sk, cat;

    for( i=0; i<hs_table_count; i++ )
    {
        if( ! HS_Id_Is_Single(hs_table[i].game) )  continue;

        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            for( sk=0; sk<HS_NUMSKILLS; sk++ )
            {
                hs_run_t  run;
                if( ! hs_table[i].has_record[cat][sk] )  continue;

                memset( &run, 0, sizeof(run) );
                dl_strncpy( run.game, hs_table[i].game, HS_GAMEID_LEN-1 );
                dl_strncpy( run.startmap, hs_table[i].mapname, 8 );
                dl_strncpy( run.endmap,   hs_table[i].mapname, 8 );
                run.skill = (byte) sk;
                run.cat   = (byte) cat;
                run.tics  = hs_table[i].besttime[cat][sk];
                HS_Board_Insert( &run );
            }
        }
    }

    if( hs_runs_count > 0 )
    {
        GenPrintf( EMSG_info,
                   "Run board seeded with %d single level time(s).\n",
                   hs_runs_count );
        HS_Runs_Save();
    }
}


void HS_Init( void )
{
    boolean  had_runfile;

    cat_filename( hs_scorefile, legacyhome, "highscores.dat" );
    cat_filename( hs_runfile,   legacyhome, "runs.dat" );
    cat_filename( hs_demodir,   legacyhome, "demos" );

    if( access(hs_demodir, R_OK) < 0 )
        I_mkdir( hs_demodir, 0700 );

    HS_Shuffle_Seed();   // [Arcade] attract demo replay order
    HS_Load();

    had_runfile = (access(hs_runfile, R_OK) == 0);
    HS_Runs_Load();
    if( ! had_runfile )
        HS_Seed_Runs_From_Splits();
}


// Must be called BEFORE G_DeferedInitNew, so that the netxcmds which
// create the player and load the first map are captured into the demo
// stream.  A DoomLegacy-native demo replays by executing those embedded
// commands (see G_DoPlayDemo: demoversion>=127 waits for the map cmd),
// so a recording started any later plays back with no player set up and
// segfaults in P_SetupPsprites on a NULL player->weaponinfo.
// This mirrors how the -record command-line option begins recording
// before any game has started.
// Console command: clear all recorded times and their record-holder demos.
// Clears the in-memory table too, so a later record cannot write the old
// entries back out -- which is why deleting highscores.dat by hand while
// the game is running does not work.
void Command_ClearHighScores_f( void )
{
    int  removed = 0;
    DIR * dp;

    // Sweep the whole demos directory rather than only the files the current
    // table references, so demos orphaned by a format change (or by editing
    // the score file) are cleaned up too.
    dp = opendir( hs_demodir );
    if( dp )
    {
        struct dirent * dent;
        while( (dent = readdir(dp)) != NULL )
        {
            char demopath[MAX_WADPATH];
            const char * extp = strrchr( dent->d_name, '.' );
            if( (extp == NULL) || (strcasecmp(extp, ".lmp") != 0) )  continue;

            cat_filename( demopath, hs_demodir, dent->d_name );
            if( remove(demopath) == 0 )  removed++;
        }
        closedir( dp );
    }

    hs_table_count = 0;
    memset(hs_table, 0, sizeof(hs_table));
    hs_cumulative_time = 0;
    hs_last_exit_mapname[0] = 0;

    // [Arcade] The run board is part of "the scores" and goes with them --
    // leaving it would put named board entries beside an empty split table.
    hs_runs_count = 0;
    memset(hs_runs, 0, sizeof(hs_runs));
    HS_Run_Reset();
    if( remove(hs_runfile) == 0 )
        GenPrintf(EMSG_info, "Run board cleared.\n");

    if( remove(hs_scorefile) != 0 )
    {
        // Not an error when nothing has been recorded yet.
        GenPrintf(EMSG_info, "High scores cleared (no score file present).\n");
    }
    else
    {
        GenPrintf(EMSG_info, "High scores cleared.\n");
    }
    GenPrintf(EMSG_info, "Removed %d record demo(s).\n", removed);
}


void HS_NewGame( void )
{
    hs_cumulative_time = 0;
    hs_run_is_max = true;   // still eligible until a level is exited short
    hs_run_died = false;
    hs_run_cheated = false;
    memset( hs_new_record, 0, sizeof(hs_new_record) );

    // [Arcade] Any placement from the previous run is finished with by now:
    // the prompt is raised on the way back to the title, which every route
    // into a new game passes through first.
    HS_Run_Reset();

    // An altered ruleset makes the run unscoreable, so do not spend the
    // demo buffer on it either -- nothing would ever be saved from it.
    hs_run_ranked = HS_Ruleset_Is_Ranked();
    // The board takes runs that ended in a death -- that is how nearly every
    // cabinet run ends, and ranking on progress first is what gives those a
    // place.  So this tracks the ruleset and cheating *only*, and is
    // deliberately not cleared by HS_Player_Died.
    hs_run_board_ok = hs_run_ranked;
    if( ! hs_run_ranked )
    {
        if( demorecording )  G_CheckDemoStatus();
        return;
    }

    // Do not fight an explicit -record: there is only one global demo
    // buffer, and that recording was asked for deliberately.
    if( M_CheckParm("-record") )  return;

    // Attract-mode playback shares demobuffer with recording, so stop it
    // before claiming that buffer.  D_DisableDemo (called soon after by
    // G_DeferedInitNew) then finds demoplayback already false.
    if( demoplayback )
        G_StopDemo();

    // Flush/close any previous background recording.
    if( demorecording )
        G_CheckDemoStatus();

    G_RecordDemo_maxsize( "hs_background", HS_DEMOBUFFER_SIZE );
    demo_scratch = true;   // only ever read via G_SnapshotDemo, never saved
    G_BeginRecording();
}


// [Arcade] Called the moment a player avatar is killed (P_KillMobj), not at
// respawn: someone who dies and walks away must void the run too.
//
// Doom traditionally lets you retry the level, and the engine does that by
// reloading it -- which resets leveltime (P_SetupLevel), so the failed
// attempt used to cost the score nothing at all.  A death now ends scoring
// for the rest of the run instead.  Levels already finished keep their
// records: each was written to disk by its own HS_LevelExit before this
// level was ever entered.
void HS_Player_Died( void )
{
    // Same guards as HS_LevelExit.  demoplayback matters most: attract-mode
    // record demos can contain a death, and replaying one must not void the
    // cabinet's live run or close its recorder.
    if( ! HS_Scored_Game() )  return;
    if( demoplayback )  return;
    if( hs_run_died )  return;   // already latched, nothing left to do

    hs_run_died = true;

    if( hs_run_ranked )
    {
        GenPrintf( EMSG_info,
                   "Run is unranked: the player died.\n" );
        hs_run_ranked = false;
    }

    // The run can no longer set a record, so stop spending the demo buffer
    // on it.  Records earned earlier are unaffected -- G_SnapshotDemo copied
    // each one to its own file at the level exit that earned it.
    if( demorecording )
        G_CheckDemoStatus();

    // [Arcade] Commit the run to the board *here*, at the death, rather than
    // waiting for the way back to the title.
    //
    // Under Survival a death is the end of the run in every sense that
    // matters: nothing more can be scored, and what the board credits -- how
    // far it got -- is already final.  Leaving the commit to
    // Command_ExitGame_f meant the entry depended on how the session happened
    // to unwind afterwards, and a player who died deep into an episode could
    // find no entry and no initials prompt for progress they had genuinely
    // earned.  Committing at the death makes the place safe the instant it is
    // decided.
    //
    // HS_Run_Finished is idempotent -- it zeroes hs_run_levels -- so the
    // Command_ExitGame_f call that follows later is a no-op, and a player who
    // presses use and carries on playing unranked cannot commit twice.
    //
    // It only *arms* the initials prompt; M_Initials_Ticker will not raise
    // the page over a live level, so it still appears on the way out, which
    // is where the player expects it.
    HS_Run_Finished();
}


// [Arcade] A demo replay is its own run as far as the intermission is
// concerned.  Called when playback starts (G_DoPlayDemo) so the running total
// counts up from zero across the demo's own levels rather than continuing
// whatever the last live game reached, and so no stale NEW RECORD or
// best-times table from that game shows on the replay's first intermission.
// Live games get the same reset from HS_NewGame.
void HS_Demo_Start( void )
{
    hs_cumulative_time = 0;
    hs_last_exit_mapname[0] = 0;
    memset( hs_new_record, 0, sizeof(hs_new_record) );
}


void HS_LevelExit( int episode, int map, skill_e skill, tic_t leveltime,
                   boolean maxed )
{
    if( ! HS_Scored_Game() )  return;
    if( skill < 0 || skill >= HS_NUMSKILLS )  return;

    // [Arcade] Everything the intermission draws is derived here, and it must
    // be updated for demo playback too.  A record demo spans several levels,
    // so replaying one passes through these same level exits -- and when this
    // block sat below the demoplayback guard the replay's intermission showed
    // the *previous live game's* state: its running total, its best-times
    // table, and a NEW RECORD that could still be latched from it.
    // HS_Demo_Start clears these when a demo begins, so a replay counts up
    // from zero across its own levels.
    //
    // Deliberately does not call HS_FindOrAddRecord: that would add a table
    // entry, and a replay must not touch the table at all.  Setting the map
    // name alone is safe -- HS_Draw_IntermissionTable simply finds no record
    // and draws nothing.
    memset( hs_new_record, 0, sizeof(hs_new_record) );
    hs_cumulative_time += leveltime;
    dl_strncpy( hs_last_exit_mapname, G_BuildMapName(episode, map), 8 );
    hs_last_exit_skill = skill;

    // Never score a replay.  Everything below this writes to the table.
    if( demoplayback )  return;

    // One level short of 100% ends the max run for the rest of the game;
    // the speed run is unaffected and keeps accumulating.
    if( ! maxed )
        hs_run_is_max = false;

    // Re-checked per level, not just at HS_NewGame: the Options menu is
    // reachable mid-game, so a run started under the ranked ruleset can be
    // altered part way through.  Once voided it stays voided for this run.
    // Shared with the live check in HS_Run_Is_Ranked so the two can never
    // disagree.  Unreachable during demoplayback -- that returned above.
    HS_Void_If_Ruleset_Changed();

    if( ! hs_run_ranked )
        return;

    const char * mapname = G_BuildMapName(episode, map);

    // [Arcade] Freeze the run's board state at this exit.  Everything below
    // this point is gated on hs_run_ranked, which a death clears -- so this
    // naturally stops at the last level the run actually completed under
    // scoring, which is exactly the progress the board should credit.  It
    // deliberately does not track hs_cumulative_time, which keeps counting
    // after a death so the intermission can still show elapsed time.
    if( hs_run_startmap[0] == 0 )
    {
        // The first level a run exits is the level it started on.
        dl_strncpy( hs_run_startmap, mapname, 8 );
        dl_strncpy( hs_run_gameid, HS_GameId(), HS_GAMEID_LEN-1 );
    }
    dl_strncpy( hs_run_endmap, mapname, 8 );
    hs_run_tics       = hs_cumulative_time;
    hs_run_skill      = skill;
    hs_run_endmap_max = hs_run_is_max;
    hs_run_levels++;

    // [Arcade] **No per-map campaign record is kept any more.**  Survival
    // scores a run on how far it got in the episode, tie-broken by time, so
    // "the best cumulative time to reach map N" no longer means anything --
    // it was the confusing part of the old scheme and it is gone.  The run's
    // frozen state above is the whole campaign score; it is committed to the
    // Survival board when the run ends (HS_Run_Finished).
    //
    // What still happens per level exit is the *demo*: the recorder holds
    // this run, and the run is only worth a demo while it leads its board,
    // so the snapshot is taken here whenever it does.  Taking it at run end
    // instead would be too late -- a death closes the recorder.
    HS_Snapshot_If_Leading( skill );

    // Latched for the intermission's marker: the run is ahead of the
    // episode record as of this exit.
    {
        int cat;
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            if( cat == HS_CAT_max && ! hs_run_is_max )  continue;
            hs_new_record[cat] = HS_Run_Leads( skill, cat );
        }
    }

    // [Arcade] A campaign run's *first* level is the same thing as a Single
    // Level run of that map -- pistol start, one map, and at this point the
    // run's cumulative time is exactly that level's time -- so it competes
    // on the single level tables too.  hs_run_levels was incremented above,
    // so 1 means "this is the first scored exit of the run".
    //
    // [Arcade] A run started from the Single Level menu is scored here as
    // well.  It used to be excluded, which left it writing *only* a runs.dat
    // board entry: the split record went unwritten, so the Single Level
    // menu's "Watch run" items and the attract rotation -- both of which key
    // off the split table and its per-map demo -- kept serving whatever
    // stale file predated the record, and a map whose only record was set
    // this way had no playable demo at all.
    if( hs_run_levels == 1 )
        HS_Score_As_Single_Level( mapname, skill, hs_cumulative_time,
                                  ! single_level_mode );
}


// [Arcade] Cumulative time for the run so far, drawn under the intermission's
// Time row.  It already includes the level just finished: WI_Init_Stats calls
// HS_LevelExit, which adds this level's leveltime, before the intermission
// draws.  Shown whatever the run's state -- an unranked or death-voided run
// still has a meaningful elapsed time, it just is not going to be recorded.
//
// label_x is the left edge of the caption and time_right_x the right edge of
// the value, so the caller can line both up with the Time row above.  Drawn
// in option 0, the font's native red: this screen's background is largely
// grey and V_WHITEMAP text disappears into it (see the V_DrawString colour
// note in CLAUDE.md).
void HS_Draw_TotalTime( int label_x, int time_right_x, int y )
{
    char timebuf[16];

    // [Arcade] To hundredths: this is a *run* time, the number the player is
    // trying to beat, and it must read at the same precision as the BEST
    // table beside it.  Measured at the call site's SP_TIMEX 16 and right
    // edge 144: the widest value "888:88.99" is 64px so it starts no further
    // left than 80, clear of the "TOTAL" caption which ends at 56.
    HS_Format_Time_CS( hs_cumulative_time, timebuf, sizeof(timebuf) );
    V_DrawString( label_x, y, 0, "TOTAL" );
    V_DrawString( time_right_x - V_StringWidth(timebuf), y, 0, timebuf );
}


// [Arcade] Path of the saved record demo for one map/skill/category, or false
// when there is none to play.  Used by the Single Level menu's replay items.
boolean  HS_Demo_Path_For( const char * mapname, skill_e skill, int cat,
                           boolean single, /*OUT*/ char * dest )
{
    char path[MAX_WADPATH];

    if( skill < 0 || skill >= HS_NUMSKILLS )  return false;
    if( cat < 0 || cat >= HS_NUMCAT )  return false;

    HS_BuildDemoPath( path, HS_GameId_Mode(single), mapname, skill, cat );
    if( access( path, R_OK ) != 0 )  return false;

    if( dest )  dl_strncpy( dest, path, MAX_WADPATH );
    return true;
}


// [Arcade] The Survival record for this episode, and where this run stands
// against it.  Replaces the old per-map "best cumulative time to reach this
// map" table, which Survival scoring made meaningless.
//
// Two lines, drawn at the x and y the caller already measured for the old
// block:
//   RECORD   E1M6  12:34.56  MLR
//   YOU      E1M4   8:12.30
//
// Widths measured against the real STCFN lumps, every column at its widest
// glyphs rather than at "A" (the rule the initials field already taught us):
//
//   RECORD    48    map  MAP01/MAP31  38    time  888:88.99  64    ini  MMM/WWW  27
//
// Laid out left to right with an 8px gap between every pair, which is what
// the earlier numbers were missing: they compared the widest time's *start*
// (+86) against the map column's *start* (+64) instead of its end (+102), so
// the two overlapped by 16px in the worst case and nobody noticed while the
// times were narrow.  It surfaced on Doom II, where the map name is 38px
// ("MAP01") against Doom 1's 27 ("E1M1") and a run past ten minutes widens
// the time as well -- at the old constants "MAP01" ended at +102 and a
// "10:20.55" began at +99, printing as "MAP010:20".
//
//   RECORD  0..48 | map  56..94 | time  ..166 | initials  174..201
//
// 201px in all, so the call site's x must be <= 119; wi_stuff.c passes 116,
// leaving 3px at the right edge.  A narrower time simply starts further
// right, so 8px is the minimum gap and not the typical one.
#define HS_IM_ROW      12   // row pitch; hu_font glyphs are 7 tall
#define HS_IM_MAP_X    56
#define HS_IM_TIME_R  166
#define HS_IM_INI_X   174

// Row labels for the table.  hs_catname[] is the on-disk spelling and is
// lower case; these are what the player reads.
static const char * hs_cat_label[HS_NUMCAT] = { "SPEED", "MAX" };

// The record this intermission holds the run up against, for one category.
//
// [Arcade] A Single Level attempt competes on that map's own three deep
// board, not on the episode's Survival board.  Showing the Survival record
// there held the run up against a different game entirely -- a whole-episode
// time it cannot ever beat on one map -- so the player had no idea what they
// were actually chasing.  HS_Board_Entry does not fill a map name (a single
// level run is one map by definition), so it is set from the exit.
static boolean  HS_Intermission_Record( int cat, char * out_map,
                                        char * out_ini, tic_t * out_tics )
{
    if( single_level_mode )
    {
        dl_strncpy( out_map, hs_last_exit_mapname, 8 );
        return HS_Board_Entry( true, hs_last_exit_mapname, hs_last_exit_skill,
                               cat, 0, out_ini, NULL, out_tics );
    }

    return HS_Survival_Entry( HS_Episode_Of(hs_last_exit_mapname),
                              hs_last_exit_skill, cat,
                              out_map, out_ini, out_tics );
}

void HS_Draw_IntermissionTable( int x, int y )
{
    char   mapname[9], ini[HS_INITIALS_LEN], timebuf[16];
    tic_t  tics;
    int    cat, row_y = y;
    boolean any_new = false;
    // gametic (not leveltime) because only gametic advances through the
    // intermission; & 16 matches the cadence wi_stuff.c blinks its own
    // "you are here" pointer at.
    boolean blink_on = (gametic & 16) != 0;

    if( hs_last_exit_mapname[0] == 0 )  return;

    // [Arcade] Both categories are shown, one row each.  It used to be the
    // speed record alone, which said nothing about the run a player going for
    // 100% is actually competing in -- and the two are independent records
    // with independent holders.
    for( cat = 0; cat < HS_NUMCAT; cat++, row_y += HS_IM_ROW )
    {
        boolean have = HS_Intermission_Record( cat, mapname, ini, &tics );

        // Latched at the level exit, before the board is updated -- so on a
        // *first* record `have` is false and there is no old time to blink;
        // the marker below still fires.
        if( hs_new_record[cat] )  any_new = true;

        V_DrawString( x, row_y, 0, (char*) hs_cat_label[cat] );

        if( ! have )
        {
            V_DrawString( x + HS_IM_MAP_X, row_y, 0, "NONE YET" );
            continue;
        }

        HS_Format_Time_CS( tics, timebuf, sizeof(timebuf) );
        V_DrawString( x + HS_IM_MAP_X, row_y, 0, mapname );
        V_DrawString( x + HS_IM_INI_X, row_y, 0, ini );

        // [Arcade] The *time* blinks on the category this run just took, so
        // the player can see which record they set rather than only that they
        // set one.  Both blink when a run takes both.
        if( hs_new_record[cat] && ! blink_on )
            continue;   // blink off: leave this time out this frame

        V_DrawString( x + HS_IM_TIME_R - V_StringWidth(timebuf), row_y, 0,
                      timebuf );
    }

    V_DrawString( x, row_y, 0, "YOU" );
    HS_Format_Time_CS( hs_cumulative_time, timebuf, sizeof(timebuf) );
    V_DrawString( x + HS_IM_MAP_X, row_y, 0, hs_last_exit_mapname );
    V_DrawString( x + HS_IM_TIME_R - V_StringWidth(timebuf), row_y, 0, timebuf );

    // Blinking marker, shown the moment the run is ahead of the record --
    // which under Survival is knowable *during* the run rather than only at
    // the end: get past the holder's furthest map and you are already ahead.
    // Which record it was is in the blinking time above; this just announces
    // that there was one, and blinks in step so the two read as one thing.
    //
    // Centred in the free space to the left of the block and on its middle
    // row.  Option 0 is the font's native red -- see the V_DrawString colour
    // note in CLAUDE.md; white vanishes into this screen's grey background.
    if( any_new && blink_on )
    {
        const char * msg = "NEW RECORD";
        V_DrawString( (x - V_StringWidth(msg)) / 2, y + HS_IM_ROW, 0,
                      (char*) msg );
    }
}

// [Arcade] Is there anything for the attract screen to show?  Asked by
// D_DoAdvanceDemo before it interposes the score pages, so that a fresh
// cabinet does not flash an empty page after every demo.  Answered by the
// page enumeration itself, which is the same thing the drawer will walk --
// campaign times, single level times and the run boards all count.
boolean  HS_Have_Records( void )
{
    return (HS_Attract_Page_Count() > 0);
}

// Pages are numbered linearly across (skill, chunk); the index is recomputed
// from the table each time rather than stored, since the table grows during
// a session as new maps are played.
// =========================================================================
//   Attract screen pages  [Arcade]
// =========================================================================
// Four kinds of page, enumerated into one linear list each time it is asked
// for, since the tables grow during a session:
//
//   campaign  best cumulative time per map, one page per (skill, category)
//   single    the same for single level runs
//   board     the run board, one page per (skill, category)
//   slmap     one map's single level top three, by difficulty
//
// The cycle shows only HS_PAGES_PER_CYCLE of them after each demo and picks
// up where it left off next time.  Showing all of them every cycle reached ~100
// seconds between demos once the single level pages were added, which is
// unusable on a machine that is meant to be advertising itself.

// The New Game menu's skill graphics, indexed by skill.  Doom names; Heretic
// differs, hence the VALID_LUMP check at the draw site.
static const char * hs_skillpatch[HS_NUMSKILLS] =
  { "M_JKILL", "M_ROUGH", "M_HURT", "M_ULTRA", "M_NMARE" };

// Bottom edge of the skill graphic.  The patches vary in height (15..19), so
// they are bottom aligned on this rather than top aligned, or the baseline
// would jump as the pages cycle.
#define HS_PG_SKILL_BOT  34


// Sort key putting maps in the order a player actually reaches them.
//
// Doom 2 hides MAP31/32 behind MAP15's secret exit and returns to MAP16
// afterwards, so plain numeric order would list them ten levels from where
// they are played: the run is 15 -> 31 -> 32 -> 16.  Doom 1 is episode
// major, map minor; E?M9 is a secret level too but sits at the end of its
// episode, which is where numeric order already puts it.
static int  HS_MapOrder( const char * mapname )
{
    int  e, m;

    if( sscanf(mapname, "MAP%d", &m) == 1 )
    {
        if( m <= 15 )  return m;          // 1..15
        if( m == 31 )  return 16;         // secret, straight after MAP15
        if( m == 32 )  return 17;         // secret, then back out to MAP16
        if( m <= 30 )  return m + 2;      // 16..30 shifted past the two above
        return m + 100;                   // MAP33+, if a pack has them
    }

    if( sscanf(mapname, "E%dM%d", &e, &m) == 2 )
        return (e * 100) + m;

    return 100000;   // unrecognized: park it at the end
}


// [Arcade] Every map this game actually has, in progression order.
//
// The best-times pages list *all* of them, not only the ones with a time, so
// that the columns line up with the episodes (E1-E2 left, E3-E4 right) and
// an unclaimed map is visible as such.  Presence is decided by whether the
// map lump exists, which is what makes this work for level packs too rather
// than assuming 32 maps or four episodes.
#define HS_MAX_PAGE_MAPS  64

static int  HS_Map_List( char out[][9], int out_max )
{
    int  n = 0, e, m, i, j;

    if( gamemode == doom2_commercial )
    {
        for( m = 1; m <= 32 && n < out_max; m++ )
        {
            const char * nm = G_BuildMapName(1, m);
            if( ! VALID_LUMP( W_CheckNumForName((char*)nm) ) )  continue;
            dl_strncpy( out[n++], nm, 9 );
        }
    }
    else
    {
        for( e = 1; e <= 4; e++ )
        {
            for( m = 1; m <= 9 && n < out_max; m++ )
            {
                const char * nm = G_BuildMapName(e, m);
                if( ! VALID_LUMP( W_CheckNumForName((char*)nm) ) )  continue;
                dl_strncpy( out[n++], nm, 9 );
            }
        }
    }

    // Progression order.  A no-op for Doom 1, where the loops above already
    // walk episode major; it is Doom 2's secret detour that needs it.
    for( i = 1; i < n; i++ )
    {
        char key[9];
        dl_strncpy( key, out[i], 9 );
        for( j = i; j > 0 && HS_MapOrder(out[j-1]) > HS_MapOrder(key); j-- )
            dl_strncpy( out[j], out[j-1], 9 );
        dl_strncpy( out[j], key, 9 );
    }

    return n;
}


// The split-table record for one map of one game id, or NULL.
static const hs_maprecord_t *  HS_Find_Record( const char * game,
                                               const char * mapname )
{
    int i;
    for( i=0; i<hs_table_count; i++ )
    {
        if( strncmp(hs_table[i].mapname, mapname, 8) != 0 )  continue;
        if( strncmp(hs_table[i].game, game, HS_GAMEID_LEN-1) != 0 )  continue;
        return &hs_table[i];
    }
    return NULL;
}


// Does this (mode, skill, category) have any time at all?  A page is only
// enumerated when it would have something on it.
static boolean  HS_Have_Times( boolean single, int sk, int cat )
{
    char gid[HS_GAMEID_LEN];
    int  i;

    dl_strncpy( gid, HS_GameId_Mode(single), HS_GAMEID_LEN-1 );
    for( i=0; i<hs_table_count; i++ )
    {
        if( strncmp(hs_table[i].game, gid, HS_GAMEID_LEN-1) != 0 )  continue;
        if( hs_table[i].has_record[cat][sk] )  return true;
    }
    return false;
}


// [Arcade] The Survival record for one (episode, skill, category): the
// furthest map reached, its time and who holds it.  Depth is 1, so there is
// at most one.  Any out pointer may be NULL.
boolean  HS_Survival_Entry( int episode, skill_e skill, int cat,
                            char * out_map, char * out_initials,
                            tic_t * out_tics )
{
    char gid[HS_GAMEID_LEN];
    int  i;

    if( cat < 0 || cat >= HS_NUMCAT )  return false;
    if( skill < 0 || skill >= HS_NUMSKILLS )  return false;

    dl_strncpy( gid, HS_GameId_Mode(false), HS_GAMEID_LEN-1 );

    for( i=0; i<hs_runs_count; i++ )
    {
        const hs_run_t * r = &hs_runs[i];
        if( r->skill != skill || r->cat != cat )  continue;
        if( strncmp(r->game, gid, HS_GAMEID_LEN-1) != 0 )  continue;
        if( HS_Episode_Of(r->endmap) != episode )  continue;

        if( out_map )       dl_strncpy( out_map, r->endmap, 9 );
        if( out_initials )  dl_strncpy( out_initials,
                                        r->initials[0] ? r->initials : "---",
                                        HS_INITIALS_LEN );
        if( out_tics )      *out_tics = r->tics;
        return true;
    }
    return false;
}


// Does this episode have any Survival record at all?  Decides whether its
// page is worth enumerating.
static boolean  HS_Episode_Has_Records( int episode )
{
    int sk, cat;
    for( sk=0; sk<HS_NUMSKILLS; sk++ )
      for( cat=0; cat<HS_NUMCAT; cat++ )
        if( HS_Survival_Entry(episode, (skill_e)sk, cat, NULL, NULL, NULL) )
            return true;
    return false;
}


// Episodes this game has: four for Doom 1, one for the flat MAPxx games,
// where the whole game is the run.
static int  HS_Num_Episodes( void )
{
    return (gamemode == doom2_commercial) ? 1 : 4;
}


// Any single level board entry for this map, at any skill or category?
// Decides whether the rotating per-map page has anything to show.
static boolean  HS_SL_Map_Has_Entries( const char * mapname )
{
    int sk, cat;
    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            if( HS_Board_Entry(true, mapname, (skill_e)sk, cat, 0,
                               NULL, NULL, NULL) )
                return true;
        }
    }
    return false;
}


// [Arcade] HSPG_campaign (best cumulative time per map) and HSPG_board (the
// ten deep run board) are both gone: Survival replaces them with one page
// per episode, showing the single best run per skill and category.
enum { HSPG_survival = 0, HSPG_single, HSPG_slmap };

typedef struct
{
    byte  kind;
    byte  skill;    // HSPG_single only
    byte  cat;      // HSPG_single only
    byte  ep;       // HSPG_survival only
} hs_page_t;

// campaign + single + board, each (skill x category), plus the one rotating
// single level map page.
#define HS_MAX_PAGES  ((HS_NUMSKILLS * HS_NUMCAT * 3) + 1)

static int  hs_attract_page = 0;   // linear index into the enumeration
// Which map the rotating single level page is showing.  Advanced when that
// page is *drawn*, so a different map comes up each time it appears rather
// than every cycle starting at E1M1.
static int  hs_slmap_cursor = 0;
// Set when the page cursor wraps, consumed by HS_Attract_Rotation_Done.
static boolean  hs_rotation_done = false;

// Defined with the drawers below; needed by HS_Attract_Advance_Page above
// them, which is what steps the cursor now.
static int  HS_SL_Current_Map( char maps[][9], int nm );


static int  HS_Build_Pages( hs_page_t * out, int out_max )
{
    int  sk, cat, n = 0;

    {
        int ep, neps = HS_Num_Episodes();
        for( ep=1; ep<=neps; ep++ )
            if( n < out_max && HS_Episode_Has_Records(ep) )
            {
                out[n].kind = HSPG_survival;  out[n].ep = (byte) ep;
                out[n].skill = 0;  out[n].cat = 0;  n++;
            }
    }

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
      for( cat=0; cat<HS_NUMCAT; cat++ )
        if( n < out_max && HS_Have_Times(true, sk, cat) )
        {
            out[n].kind = HSPG_single;  out[n].skill = sk;
            out[n].cat = cat;  out[n].ep = 0;  n++;
        }

    // One slot only, however many maps have single level times: it shows a
    // different map each time it comes round, which keeps the cycle short.
    {
        char maps[HS_MAX_PAGE_MAPS][9];
        int  nm = HS_Map_List( maps, HS_MAX_PAGE_MAPS );
        int  i;
        for( i=0; i<nm; i++ )
        {
            if( HS_SL_Map_Has_Entries(maps[i]) )
            {
                if( n < out_max )
                {
                    out[n].kind = HSPG_slmap;  out[n].skill = 0;  out[n].ep = 0;
                    out[n].cat = 0;  n++;
                }
                break;
            }
        }
    }

    return n;
}


int HS_Attract_Page_Count( void )
{
    hs_page_t  pages[HS_MAX_PAGES];
    return HS_Build_Pages( pages, HS_MAX_PAGES );
}


// [Arcade] How many pages this appearance shows.  The cycle no longer runs
// the whole set after every demo: with campaign, single level, board and
// per-map pages that reached roughly 100 seconds of score pages between
// demos.  A small window, continued next time, keeps each interruption
// short while still making everything reachable.
// Even, so the speed/max pairs of the single level pages sit whole inside an
// appearance rather than straddling a demo.  HS_Attract_Cycle_Pages also
// guards the boundary directly, since an odd page elsewhere in the list
// shifts the parity.
#define HS_PAGES_PER_CYCLE  4

// True when b is the max half of the speed/max pair that a is the speed half
// of.  The single level best-times pages are enumerated in that order, so
// the two halves of a skill are always adjacent.
static boolean  HS_Is_Pair_Tail( const hs_page_t * a, const hs_page_t * b )
{
    return ( a->kind == HSPG_single && b->kind == HSPG_single
             && a->skill == b->skill
             && a->cat == HS_CAT_speed && b->cat == HS_CAT_max );
}


int HS_Attract_Cycle_Pages( void )
{
    hs_page_t  pages[HS_MAX_PAGES];
    int  total = HS_Build_Pages( pages, HS_MAX_PAGES );
    int  want  = HS_PAGES_PER_CYCLE;
    int  last, next;

    if( total <= 0 )  return 0;
    if( want > total )  return total;

    // [Arcade] Never end an appearance between a skill's speed page and its
    // max page: the pair belongs together, and splitting it across a demo
    // means the two halves of one skill are minutes apart.
    //
    // An even HS_PAGES_PER_CYCLE is not enough on its own -- a skill with a
    // speed record but no max contributes a single page and shifts the
    // parity of every pair after it -- so the boundary is checked directly
    // and the appearance extended by one page when it would split a pair.
    last = (hs_attract_page + want - 1) % total;
    next = (last + 1) % total;
    if( next != hs_attract_page && HS_Is_Pair_Tail(&pages[last], &pages[next]) )
        want++;

    return (want > total) ? total : want;
}


void HS_Attract_Advance_Page( void )
{
    hs_page_t  pages[HS_MAX_PAGES];
    int  total = HS_Build_Pages( pages, HS_MAX_PAGES );

    if( total <= 0 )
    {
        hs_attract_page = 0;
        return;
    }

    // Step the rotating single level page onto its next map as we leave it.
    // This belongs here and not in the drawer: this runs once per page, the
    // drawer once per frame.
    if( hs_attract_page < total && pages[hs_attract_page].kind == HSPG_slmap )
    {
        char maps[HS_MAX_PAGE_MAPS][9];
        int  nm = HS_Map_List( maps, HS_MAX_PAGE_MAPS );
        int  cur = HS_SL_Current_Map( maps, nm );
        if( cur >= 0 )
            hs_slmap_cursor = cur + 1;
    }

    hs_attract_page++;
    if( hs_attract_page >= total )
    {
        hs_attract_page = 0;
        // [Arcade] A full pass of the score pages has finished.  The next
        // demo is a Survival record run rather than the usual short single
        // level one -- that is what makes the long, whole-episode demo an
        // occasional feature instead of the filler between pages.
        hs_rotation_done = true;
    }
}


boolean  HS_Attract_Rotation_Done( void )
{
    boolean r = hs_rotation_done;
    hs_rotation_done = false;   // consumed: it marks one demo, not a state
    return r;
}


// -------------------------------------------------------------------------
// Best times page: every map of the game, one time each, two columns.
//
// One page per (skill, category) rather than both categories side by side,
// which is forced by width: measured against the real STCFN lumps a row of
// "MAP01  12:34.57  12:34.57" is 156px against the 160 a column has, and a
// cumulative Doom 2 time reaching "123:45.67" makes it 164.  Splitting by
// category also frees the room for the initials, which this page never had.
//
// Column layout, relative to the column origin:
//   map name   +0    ("MAP01" 38px, "E1M1" 27px)
//   time       right-justified at +110, so the widest "888:88.99" (64px)
//              starts at +46 and clears the map name by 8
//   initials   +118 .. +142 ("AAA" 24px)
// Origins 10 and 168, so the columns span 10..152 and 168..310 of 320.
#define HS_BT_COL0     10
#define HS_BT_COL1    168
#define HS_BT_TIME_R  110
#define HS_BT_INI_X   118
#define HS_BT_ROW0     46
#define HS_BT_ROW_MAXY 182    // top edge of the last row that still fits

static void  HS_Draw_SkillGraphic( int sk )
{
    lumpnum_t  sklump = W_CheckNumForName( (char*) hs_skillpatch[sk] );

    if( VALID_LUMP( sklump ) )
    {
        patch_t * skp = W_CachePatchName( (char*) hs_skillpatch[sk], PU_CACHE );
        int  pw = V_patch(skp)->width;
        int  ph = V_patch(skp)->height;

        V_DrawScaledPatch( (BASEVIDWIDTH - pw)/2, HS_PG_SKILL_BOT - ph, skp );
    }
    else
    {
        V_DrawString( (BASEVIDWIDTH - V_StringWidth((char*)hs_skillnames[sk]))/2,
                      HS_PG_SKILL_BOT - 7, V_WHITEMAP, (char*) hs_skillnames[sk] );
    }
}


static void  HS_Draw_BestTimes( boolean single, int sk, int cat )
{
    char  maps[HS_MAX_PAGE_MAPS][9];
    char  gid[HS_GAMEID_LEN];
    char  buf[64], timebuf[16];
    int   nm, rows, step, i, col;
    const char * title = single ? "SINGLE LEVEL BEST TIMES"
                                : "SINGLE PLAYER BEST TIMES";

    dl_strncpy( gid, HS_GameId_Mode(single), HS_GAMEID_LEN-1 );
    nm = HS_Map_List( maps, HS_MAX_PAGE_MAPS );

    V_DrawString( (BASEVIDWIDTH - V_StringWidth((char*)title))/2, 8,
                  V_WHITEMAP, (char*) title );
    HS_Draw_SkillGraphic( sk );

    // Rows per column, and a step chosen to fill the band: 36 maps (Doom 1
    // with four episodes) gives 18 rows and a step of 8, Doom 2's 32 gives
    // 16 and a step of 9, and a short level pack gets the full 10.
    rows = (nm + 1) / 2;
    if( rows < 1 )  rows = 1;
    step = (rows > 1) ? ((HS_BT_ROW_MAXY - HS_BT_ROW0) / (rows - 1)) : 10;
    if( step > 10 )  step = 10;
    if( step < 6 )   step = 6;

    for( col=0; col<2; col++ )
    {
        int x = col ? HS_BT_COL1 : HS_BT_COL0;
        V_DrawString( x, 36, V_WHITEMAP, "MAP" );
        snprintf( buf, sizeof(buf), "%s", hs_catname[cat] );
        strupr( buf );
        V_DrawString( x + HS_BT_TIME_R - V_StringWidth(buf), 36,
                      V_WHITEMAP, buf );
        // Only the single level page can name a holder; see below.
        if( single )
            V_DrawString( x + HS_BT_INI_X, 36, V_WHITEMAP, "BY" );
    }

    for( i=0; i<nm; i++ )
    {
        const hs_maprecord_t * rec;
        char  ini[HS_INITIALS_LEN];
        int   x, y;
        boolean have_ini = false;

        col = (i < rows) ? 0 : 1;             // fill the left column first
        x   = col ? HS_BT_COL1 : HS_BT_COL0;
        y   = HS_BT_ROW0 + ((i % rows) * step);

        V_DrawString( x, y, 0, maps[i] );

        rec = HS_Find_Record( gid, maps[i] );
        if( rec && rec->has_record[cat][sk] )
        {
            HS_Format_Time_CS( rec->besttime[cat][sk], timebuf, sizeof(timebuf) );
            // Only a single level time has an identifiable holder.  Its
            // board is keyed per map, so the number one entry there is the
            // same run as this split by construction.  A *campaign* split is
            // genuinely anonymous: the split table records no owner, and a
            // campaign board entry is keyed by the run's END map, so nothing
            // attributes the MAP03 split of a run that carried on to MAP08.
            // Showing "---" against every row would be worse than showing
            // nothing, so the column is omitted entirely for campaign pages.
            if( single )
                have_ini = HS_Board_Entry( true, maps[i], (skill_e)sk, cat, 0,
                                           ini, NULL, NULL );
        }
        else
        {
            dl_strncpy( timebuf, "--:--.--", sizeof(timebuf) );
        }

        V_DrawString( x + HS_BT_TIME_R - V_StringWidth(timebuf), y, 0, timebuf );
        if( have_ini )
            V_DrawString( x + HS_BT_INI_X, y, 0, ini );
    }
}


// -------------------------------------------------------------------------
// One map's single level top three, laid out difficulty x place.
//
// Read in one glance, unlike stacking a block per difficulty.  Measured
// against the real STCFN lumps: a cell is "AAA 0:08.57" at 75px and at worst
// "AAA 29:59.99" at 83px for a slow max run of one map, so cells at 52, 139
// and 226 clear each other by 4px and the last ends at 309 of 320.  The
// skill label ("ITYTD" 36px) sits at 10..46, clearing the first cell by 6.
//
// Only a single *level* time appears here, so three digit minutes -- which
// would make a cell 92px and crowd the next -- cannot arise from real play.
#define HS_SLM_SKILL_X   10
#define HS_SLM_CELL0     52
#define HS_SLM_CELL_STEP 87
#define HS_SLM_ROW0      50
#define HS_SLM_ROWSTEP   10

static void  HS_Draw_SL_Map_Block( const char * mapname, int cat, int y0 )
{
    char buf[64], timebuf[16], ini[HS_INITIALS_LEN];
    tic_t t;
    int  sk, place;

    snprintf( buf, sizeof(buf), "%s", hs_catname[cat] );
    strupr( buf );
    V_DrawString( HS_SLM_SKILL_X, y0, V_WHITEMAP, buf );

    for( place=0; place<HS_BOARD_DEPTH_SL; place++ )
    {
        static const char * ord[3] = { "1st", "2nd", "3rd" };
        V_DrawString( HS_SLM_CELL0 + (place * HS_SLM_CELL_STEP), y0,
                      V_WHITEMAP, (char*) ord[place] );
    }

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        int y = y0 + 10 + (sk * HS_SLM_ROWSTEP);

        V_DrawString( HS_SLM_SKILL_X, y, 0, (char*) hs_skillnames[sk] );

        for( place=0; place<HS_BOARD_DEPTH_SL; place++ )
        {
            int cx = HS_SLM_CELL0 + (place * HS_SLM_CELL_STEP);

            if( ! HS_Board_Entry( true, mapname, (skill_e)sk, cat, place,
                                  ini, NULL, &t ) )
                continue;   // blank rather than a row of dashes

            HS_Format_Time_CS( t, timebuf, sizeof(timebuf) );
            snprintf( buf, sizeof(buf), "%s %s", ini, timebuf );
            V_DrawString( cx, y, 0, buf );
        }
    }
}


// Which map the rotating page is showing, resolved from the cursor without
// touching it.  **Read only, deliberately.**  Advancing here is what made
// this page flicker through every map at frame rate: a drawer runs once per
// *frame*, not once per page, so anything it mutates changes 35 times a
// second.  The cursor is stepped by HS_Attract_Advance_Page instead, which
// runs once per page.  Returns -1 when no map has entries.
static int  HS_SL_Current_Map( char maps[][9], int nm )
{
    int  cur = hs_slmap_cursor;
    int  tries;

    // Bounded by the map count, so a table with no single level entries at
    // all cannot spin here.
    for( tries=0; tries<nm; tries++ )
    {
        if( cur >= nm )  cur = 0;
        if( HS_SL_Map_Has_Entries( maps[cur] ) )  return cur;
        cur++;
    }
    return -1;
}


static void  HS_Draw_SL_Map_Page( void )
{
    char  maps[HS_MAX_PAGE_MAPS][9];
    char  buf[64];
    int   nm, i, has_max = 0, sk;

    nm = HS_Map_List( maps, HS_MAX_PAGE_MAPS );
    if( nm == 0 )  return;

    i = HS_SL_Current_Map( maps, nm );
    if( i < 0 )  return;

    snprintf( buf, sizeof(buf), "SINGLE LEVEL: %s", maps[i] );
    V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2, 12, V_WHITEMAP, buf );

    HS_Draw_SL_Map_Block( maps[i], HS_CAT_speed, HS_SLM_ROW0 );

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        if( HS_Board_Entry(true, maps[i], (skill_e)sk, HS_CAT_max, 0,
                           NULL, NULL, NULL) )
            has_max = 1;
    }
    // Only drawn when there is something in it; otherwise the page would be
    // half a screen of empty rows.
    if( has_max )
        HS_Draw_SL_Map_Block( maps[i], HS_CAT_max, HS_SLM_ROW0 + 68 );
}


// -------------------------------------------------------------------------
// The Survival page: one episode, rows are skills, columns are the two
// categories side by side.
//
// Measured against the real STCFN lumps.  With the skill label written once
// and the two category blocks beside it, a row is
//   skill  "ITYTD" 36px      at 8,  so 8..44
//   speed  "E1M8 12:34.56 MLR" 122px at 52,  so 52..174
//   max    the same             at 186, so 186..308
// which is 308 of 320.  A three digit minute time ("123:45.67", 59px) makes
// a block 130px and the row 316 -- still inside.  This is what a per-episode
// board buys: the old per-map page could not fit two categories at all.
#define HS_SV_SKILL_X    8
#define HS_SV_COL0      52
#define HS_SV_COL1     186
#define HS_SV_MAP_W     34     // map name column inside a block
#define HS_SV_TIME_R    96     // right edge of the time, from block origin
#define HS_SV_INI_X    100     // initials, from block origin
#define HS_SV_ROW0      64
#define HS_SV_ROWSTEP   14

static void  HS_Draw_SurvivalPage( int ep )
{
    char  buf[64], mapname[9], ini[HS_INITIALS_LEN], timebuf[16];
    tic_t tics;
    int   sk, cat;

    V_DrawString( (BASEVIDWIDTH - V_StringWidth("SINGLE PLAYER - SURVIVAL"))/2,
                  8, V_WHITEMAP, "SINGLE PLAYER - SURVIVAL" );

    // Doom 2 is one episode -- the whole game is the run -- so naming an
    // episode there would be noise.
    if( HS_Num_Episodes() > 1 )
        snprintf( buf, sizeof(buf), "EPISODE %d", ep );
    else
        snprintf( buf, sizeof(buf), "FURTHEST, THEN FASTEST" );
    V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2, 24, 0, buf );

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        int x = cat ? HS_SV_COL1 : HS_SV_COL0;
        snprintf( buf, sizeof(buf), "%s", hs_catname[cat] );
        strupr( buf );
        V_DrawString( x, 46, V_WHITEMAP, buf );
    }

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        int y = HS_SV_ROW0 + (sk * HS_SV_ROWSTEP);

        V_DrawString( HS_SV_SKILL_X, y, V_WHITEMAP,
                      (char*) hs_skillnames[sk] );

        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            int x = cat ? HS_SV_COL1 : HS_SV_COL0;

            if( ! HS_Survival_Entry( ep, (skill_e)sk, cat,
                                     mapname, ini, &tics ) )
            {
                // Blank rather than a row of dashes: with one entry per
                // board an unplayed cell is common, and dashes everywhere
                // read as clutter.
                continue;
            }

            HS_Format_Time_CS( tics, timebuf, sizeof(timebuf) );
            V_DrawString( x, y, 0, mapname );
            V_DrawString( x + HS_SV_TIME_R - V_StringWidth(timebuf), y, 0,
                          timebuf );
            V_DrawString( x + HS_SV_INI_X, y, 0, ini );
        }
    }
}


void HS_Draw_AttractTable( void )
{
    hs_page_t  pages[HS_MAX_PAGES];
    char  buf[64];
    int   total;

    // This is an attract-screen page like the ones D_PageDrawer handles, so
    // it must establish the same draw state and cover the whole screen.
    // Without this it painted over whatever the previous page or demo had
    // left in the buffer -- and with page flipping, over two different stale
    // frames alternately, which looked like flickering garbage.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );
    V_DrawScaledFill( 0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0 );  // black

    total = HS_Build_Pages( pages, HS_MAX_PAGES );
    if( total == 0 )
    {
        V_DrawString( (BASEVIDWIDTH - V_StringWidth("No times recorded yet"))/2,
                      90, 0, "No times recorded yet" );
        return;
    }

    if( hs_attract_page >= total )  hs_attract_page = 0;

    switch( pages[hs_attract_page].kind )
    {
     case HSPG_survival:
        HS_Draw_SurvivalPage( pages[hs_attract_page].ep );
        break;
     case HSPG_single:
        HS_Draw_BestTimes( true, pages[hs_attract_page].skill,
                                 pages[hs_attract_page].cat );
        break;
     case HSPG_slmap:
        HS_Draw_SL_Map_Page();
        break;
     default:
        break;
    }

    // Footer, so it is obvious that other pages follow.
    if( total > 1 )
    {
        snprintf( buf, sizeof(buf), "%d of %d", hs_attract_page + 1, total );
        V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2,
                      BASEVIDHEIGHT-10, 0, buf );
    }
}



// [Arcade] Describes the record demo HS_NextRecordDemoPath last handed out,
// so the attract screen can caption it.  Empty while a stock IWAD demo (or
// nothing) is playing -- D_DoAdvanceDemo clears it before every page.
static char hs_demo_label[64] = "";

void HS_Clear_DemoLabel( void )
{
    hs_demo_label[0] = 0;
}

const char * HS_DemoLabel( void )
{
    return hs_demo_label[0] ? hs_demo_label : NULL;
}


// [Arcade] Replay order.  A linear walk of the table played the same demos
// in the same order every cycle, which reads as monotonous on a machine that
// sits on the attract screen all day.  The slots are shuffled into a bag and
// dealt out instead, so every demo is shown once before any is repeated --
// plain random picking would happily show the same one three times running,
// which is the complaint rather than the cure.
//
// The bag holds packed (map, skill, category) slot numbers, the same encoding
// the old cursor used.
#define HS_BAG_MAX  (HS_MAX_MAPS * HS_NUMSKILLS * HS_NUMCAT)

static uint16_t  hs_bag[HS_BAG_MAX];
static int       hs_bag_count = 0;   // slots in the bag
static int       hs_bag_pos   = 0;   // next to deal
static int       hs_bag_built_for = -1;  // hs_table_count when built
static int       hs_bag_last = -1;   // last slot dealt, to avoid a repeat

// Self-contained PRNG, deliberately not one of the engine's.  P_Random is
// demo-sync critical; M_Random/N_Random index a shared 256 entry table that
// M_ClearRandom resets at every game start, so a bag shuffled from it would
// come out the same after every boot.  This one is seeded from the clock and
// touches no state anything else reads, so it cannot perturb a recording.
static uint32_t  hs_shuffle_rng = 0;

static uint32_t  HS_Shuffle_Rand( void )
{
    // xorshift32; any nonzero seed has a full 2**32-1 period.
    hs_shuffle_rng ^= hs_shuffle_rng << 13;
    hs_shuffle_rng ^= hs_shuffle_rng >> 17;
    hs_shuffle_rng ^= hs_shuffle_rng << 5;
    return hs_shuffle_rng;
}

static void  HS_Shuffle_Seed( void )
{
    hs_shuffle_rng = (uint32_t)time(NULL) ^ 0x9E3779B9u;
    if( hs_shuffle_rng == 0 )  hs_shuffle_rng = 1;  // xorshift sticks at 0
}


// Refill the bag with every slot that currently holds a record, in a random
// order.  Rebuilt when exhausted, and whenever the table has grown -- a
// record set during this session should join the rotation without a restart.
// A record added to a map *row* that already exists does not change
// hs_table_count and so waits for the next natural refill, which is at most
// one pass of the cycle away.
static void  HS_Refill_DemoBag( void )
{
    int  mi, sk, cat, i;

    hs_bag_count = 0;
    hs_bag_pos = 0;
    hs_bag_built_for = hs_table_count;

    for( mi=0; mi<hs_table_count; mi++ )
    {
        for( sk=0; sk<HS_NUMSKILLS; sk++ )
        {
            for( cat=0; cat<HS_NUMCAT; cat++ )
            {
                if( ! hs_table[mi].has_record[cat][sk] )  continue;
                if( hs_bag_count >= HS_BAG_MAX )  goto filled;
                hs_bag[hs_bag_count++] =
                    (uint16_t)((mi * HS_NUMSKILLS + sk) * HS_NUMCAT + cat);
            }
        }
    }

filled:
    // Fisher-Yates, walking down so each position draws from what is left.
    for( i = hs_bag_count - 1; i > 0; i-- )
    {
        int j = (int)(HS_Shuffle_Rand() % (uint32_t)(i + 1));
        uint16_t t = hs_bag[i];
        hs_bag[i] = hs_bag[j];
        hs_bag[j] = t;
    }

    // Do not open a new bag with the demo that closed the last one, which is
    // the one repeat a shuffle cannot rule out on its own.
    if( hs_bag_count > 1 && hs_bag[0] == hs_bag_last )
    {
        int j = 1 + (int)(HS_Shuffle_Rand() % (uint32_t)(hs_bag_count - 1));
        uint16_t t = hs_bag[0];
        hs_bag[0] = hs_bag[j];
        hs_bag[j] = t;
    }
}


const char * HS_NextRecordDemoPath( void )
{
    static char path[MAX_WADPATH];
    // HS_GameId_Mode returns a pointer to one static buffer, so both ids
    // have to be copied out before the second call overwrites the first.
    char single_id[HS_GAMEID_LEN];
    int  slot;
    int  tries;
    int  mi, sk, cat;
    boolean is_single;

    if( hs_table_count == 0 )  return NULL;

    dl_strncpy( single_id,   HS_GameId_Mode(true),  HS_GAMEID_LEN-1 );

    if( hs_bag_pos >= hs_bag_count || hs_bag_built_for != hs_table_count )
        HS_Refill_DemoBag();

    // Bounded by the bag size: a slot can still be unusable here (another
    // game's demo, or the file gone), and the search must end even if none
    // of them is playable.
    for( tries=0; tries<hs_bag_count; tries++ )
    {
        if( hs_bag_pos >= hs_bag_count )
            hs_bag_pos = 0;   // wrap within this pass; refilled on the next call

        slot = hs_bag[hs_bag_pos++];

        mi  = slot / (HS_NUMSKILLS * HS_NUMCAT);
        sk  = (slot / HS_NUMCAT) % HS_NUMSKILLS;
        cat = slot % HS_NUMCAT;
        // Only demos from the running game: the same map name is a
        // different level in Doom 2, Plutonia and TNT, so replaying another
        // game's demo would desync immediately.
        // [Arcade] **Single level demos only in the normal rotation.**  A
        // Survival demo is a whole episode -- ten or twenty minutes -- which
        // would park the attract screen on one run.  Those are handed out by
        // HS_NextSurvivalDemoPath instead, once per full pass of the score
        // pages, so the long run is an occasional feature rather than the
        // filler between them.
        if( strncmp(hs_table[mi].game, single_id, HS_GAMEID_LEN-1) != 0 )
            continue;
        is_single = true;

        if( hs_table[mi].has_record[cat][sk] )
        {
            HS_BuildDemoPath(path, hs_table[mi].game, hs_table[mi].mapname,
                             (skill_e)sk, cat);
            if( access(path, R_OK) == 0 )
            {
                char timebuf[16];
                char range[20];
                const char * sm = hs_table[mi].startmap[cat][sk];

                HS_Format_Time_CS(hs_table[mi].besttime[cat][sk],
                                  timebuf, sizeof(timebuf));

                // [Arcade] Name the whole span the run covered.  These times
                // are cumulative from the start of a run, so the demo for the
                // E1M5 record is a five level run -- captioned with the bare
                // map name it was indistinguishable from a single level one,
                // which badly undersold the longer runs.  A campaign record
                // with no stored start map (written before that was tracked)
                // falls back to the inference in HS_Format_Range.
                HS_Format_Range( sm, hs_table[mi].mapname, is_single,
                                 range, sizeof(range) );

                // e.g. "E1M1-E1M5  ITYTD  MAX  4:32.17", or
                // "SINGLE LEVEL: MAP01  ITYTD  SPEED  4:32.17".  Measured
                // against the real STCFN lumps, the widest either form can
                // reach is "SINGLE LEVEL: MAP01  ITYTD  SPEED  888:88.99" at
                // 295px of BASEVIDWIDTH 320 -- a single level run is one map
                // so it never carries a range, and a range costs less width
                // than the prefix does.  HU_Drawer centres on V_StringWidth,
                // so this follows any rewording.
                snprintf(hs_demo_label, sizeof(hs_demo_label),
                         "%s%s  %s  %s  %s",
                         is_single? "Single Level: " : "",
                         range,
                         hs_skillnames[sk], hs_catname[cat], timebuf);
                strupr(hs_demo_label);

                hs_bag_last = slot;
                return path;
            }
        }
    }

    return NULL;
}





// [Arcade] The Survival record demo to show at the end of a full pass of the
// score pages.  A whole-episode run is far too long to be the ordinary
// filler between attract pages, so it appears once per rotation instead --
// roughly once every several attract cycles.
//
// Walks (episode, skill, category) from a cursor that persists, so a
// different record run comes up each time rather than always the first.
const char * HS_NextSurvivalDemoPath( void )
{
    static char path[MAX_WADPATH];
    static int  cursor = 0;
    char  gid[HS_GAMEID_LEN];
    int   neps  = HS_Num_Episodes();
    int   total = neps * HS_NUMSKILLS * HS_NUMCAT;
    int   tries;

    if( total <= 0 )  return NULL;
    dl_strncpy( gid, HS_GameId_Mode(false), HS_GAMEID_LEN-1 );

    for( tries=0; tries<total; tries++ )
    {
        char  mapname[9], ini[HS_INITIALS_LEN], timebuf[16];
        tic_t tics;
        int   slot = cursor % total;
        int   ep   = (slot / (HS_NUMSKILLS * HS_NUMCAT)) + 1;
        int   sk   = (slot / HS_NUMCAT) % HS_NUMSKILLS;
        int   cat  = slot % HS_NUMCAT;

        cursor++;

        if( ! HS_Survival_Entry( ep, (skill_e)sk, cat, mapname, ini, &tics ) )
            continue;

        HS_BuildSurvivalDemoPath( path, gid, ep, (skill_e)sk, cat );
        if( access(path, R_OK) != 0 )  continue;   // record kept, demo gone

        // e.g. "SURVIVAL EP1  UV  SPEED  E1M8  7:03.22".  Measured against
        // the real STCFN lumps at 231px of 320 for the widest realistic
        // form; HU_Drawer centres on V_StringWidth so it follows a rewording.
        HS_Format_Time_CS( tics, timebuf, sizeof(timebuf) );
        if( neps > 1 )
            snprintf( hs_demo_label, sizeof(hs_demo_label),
                      "Survival EP%d  %s  %s  %s  %s",
                      ep, hs_skillnames[sk], hs_catname[cat], mapname, timebuf );
        else
            snprintf( hs_demo_label, sizeof(hs_demo_label),
                      "Survival  %s  %s  %s  %s",
                      hs_skillnames[sk], hs_catname[cat], mapname, timebuf );
        strupr( hs_demo_label );
        return path;
    }

    return NULL;
}

