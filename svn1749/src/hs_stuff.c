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
// HS_Set_Initials.  hs_run_placed[] indexes the entries to stamp -- one run
// can place in both categories at once.
static boolean hs_initials_pending = false;
static int     hs_run_placed[HS_NUMCAT];
static int     hs_run_placed_n = 0;
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

// Deliberately NOT in the table: cv_respawnmonsters and cv_fastmonsters.
// G_InitNew turns both on for sk_nightmare (g_game.c, "skill == sk_nightmare"
// -> CV_SetParam), so they are part of the skill rather than a player
// setting, and gameskill is still the *previous* game's value at HS_NewGame
// time -- checking them there flagged legitimate runs as unranked.  Leaving
// them out costs nothing: on Nightmare the engine overrides the player
// either way, and on every other skill both default to off and can only be
// switched on, which makes the game harder rather than easier.  Both are
// recorded in the demo header, so records still replay correctly.

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

boolean  HS_Run_Is_Ranked( void )
{
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
// The two attract-screen consumers deliberately disagree about it:
// HS_Entry_Eligible takes campaign only, so the score *pages* stay campaign
// times whatever mode the cabinet was last left in (and do not double in
// number), while HS_NextRecordDemoPath replays both -- a one map demo is
// just a demo, and captioning it costs the cycle nothing.
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


static void HS_FormatTime( tic_t tics, char * buf, size_t bufsize )
{
    int seconds = tics / TICRATE;
    int minutes = seconds / 60;
    int secs    = seconds % 60;
    snprintf(buf, bufsize, "%d:%02d", minutes, secs);
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
        // No start map recorded (a line written before this field existed):
        // leave it empty and let the caption fall back to the bare map name
        // rather than inventing a range that may not be true.
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


// Do two runs compete for the same board places?
static boolean  HS_Same_Board( const hs_run_t * a, const hs_run_t * b )
{
    if( a->skill != b->skill || a->cat != b->cat )  return false;
    if( strncmp(a->game, b->game, HS_GAMEID_LEN-1) != 0 )  return false;
    // A single level board is per map; a campaign board spans the whole game.
    if( HS_Id_Is_Single(a->game)
        && strncmp(a->endmap, b->endmap, 8) != 0 )  return false;
    return true;
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
        // "---" is the placeholder written for a run nobody claimed; read it
        // back as empty so one code path covers both.
        if( strcmp(initials, "---") == 0 )  initials[0] = 0;
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
        if( ini[0] == 0 )  ini = "---";
        fprintf(fw, "%s %s %s %d %s %u %s\n",
                hs_runs[i].game, hs_runs[i].startmap, hs_runs[i].endmap,
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
            // actually spans levels gets a range.
            if( r->startmap[0] && strncmp(r->startmap, r->endmap, 8) != 0 )
                snprintf( out_range, 20, "%.8s-%.8s", r->startmap, r->endmap );
            else
                snprintf( out_range, 20, "%.8s", r->endmap );
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
    hs_run_placed_n    = 0;
    hs_run_best_place  = 0;
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

    if( hs_run_levels == 0 )  return;   // nothing was ever scored
    if( ! hs_run_board_ok )             // altered ruleset, or a cheat
    {
        HS_Run_Reset();
        return;
    }

    hs_run_placed_n   = 0;
    hs_run_best_place = 0;

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        int place;

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

        place = HS_Board_Insert( &run );
        if( place > 0 )
        {
            // Remember where it went so the initials can be stamped on
            // exactly these entries once the player has entered them.
            hs_run_placed[hs_run_placed_n++] = cat;
            if( hs_run_best_place == 0 || place < hs_run_best_place )
                hs_run_best_place = place;
        }
    }

    if( hs_run_placed_n > 0 )
    {
        HS_Runs_Save();          // the place is real even if nobody claims it
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
    for( i=0; i<hs_run_placed_n; i++ )
    {
        int  slot[HS_MAX_RUNS];
        int  ns, s;
        hs_run_t key;

        memset( &key, 0, sizeof(key) );
        dl_strncpy( key.game, hs_run_gameid, HS_GAMEID_LEN-1 );
        dl_strncpy( key.endmap, hs_run_endmap, 8 );
        key.skill = (byte) hs_run_skill;
        key.cat   = (byte) hs_run_placed[i];

        ns = HS_Board_Slots( &key, slot, HS_MAX_RUNS );
        for( s=0; s<ns; s++ )
        {
            hs_run_t * r = &hs_runs[slot[s]];
            // The entry this run just made: same end map and same time, and
            // not already claimed.  Matching on the time as well as the map
            // is what keeps this off an older entry that happens to share
            // the end map.
            if( r->tics != hs_run_tics )  continue;
            if( strncmp(r->endmap, hs_run_endmap, 8) != 0 )  continue;
            if( r->initials[0] )  continue;
            dl_strncpy( r->initials, clean, HS_INITIALS_LEN );
            break;
        }
    }

    HS_Runs_Save();

    hs_initials_pending = false;
    hs_run_placed_n     = 0;
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
    if( netgame || multiplayer || deathmatch )  return;
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
    if( netgame || multiplayer || deathmatch )  return;
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
    if( ! HS_Ruleset_Is_Ranked() )
    {
        // Name the cvar.  A run silently scoring nothing is very hard to
        // diagnose from the outside -- this is exactly how the Nightmare
        // cv_fastmonsters bug presented (played fine, then UNRANKED with no
        // score for the level just finished).
        if( hs_run_ranked )
            GenPrintf( EMSG_info,
                "Run is unranked: \"%s\" differs from the ranked ruleset.\n",
                HS_Unranked_Reason() );
        hs_run_ranked = false;
        hs_run_board_ok = false;   // [Arcade] off the board as well
    }
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

    hs_maprecord_t * rec = HS_FindOrAddRecord(HS_GameId(), mapname);
    if( rec == NULL )  return;   // table full

    int cat;
    boolean saved = false;
    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        if( cat == HS_CAT_max && ! hs_run_is_max )  continue;

        if( rec->has_record[cat][skill]
            && hs_cumulative_time >= rec->besttime[cat][skill] )
            continue;

        rec->has_record[cat][skill] = true;
        rec->besttime[cat][skill]   = hs_cumulative_time;
        // [Arcade] Remember where the run holding this split began, so the
        // attract caption can say "E1M1-E1M5" rather than leaving a five
        // level time looking like a single level one.
        dl_strncpy( rec->startmap[cat][skill], hs_run_startmap, 8 );
        saved = true;
        hs_new_record[cat] = true;

        if( demorecording )
        {
            char demopath[MAX_WADPATH];
            HS_BuildDemoPath(demopath, HS_GameId(), mapname, skill, cat);
            G_SnapshotDemo(demopath);
        }
    }

    if( saved )
        HS_Save();
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


void HS_Draw_IntermissionTable( int x, int y )
{
    hs_maprecord_t * rec;
    char   timebuf[16];
    int    i, sk, cat;
    int    row_y = y;

    if( hs_last_exit_mapname[0] == 0 )  return;

    rec = NULL;
    for( i=0; i<hs_table_count; i++ )
    {
        if( strncmp(hs_table[i].mapname, hs_last_exit_mapname, 8) == 0
            && strncmp(hs_table[i].game, HS_GameId(), HS_GAMEID_LEN-1) == 0 )
        {
            rec = &hs_table[i];
            break;
        }
    }
    if( rec == NULL )  return;

    // Header: skill labels down the left, one time column per category.
    V_DrawString(x, row_y-14, 0, "BEST");
    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        const char * cn = hs_catname[cat];
        V_DrawString(x + HS_COL_TIME + cat*HS_COL_STEP - V_StringWidth(cn),
                     row_y-14, 0, cn);
    }

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        int option = (sk == hs_last_exit_skill) ? V_WHITEMAP : 0;

        V_DrawString(x, row_y, option, hs_skillnames[sk]);

        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            // [Arcade] Hundredths here, matching the TOTAL row below: this
            // is the number the player is comparing their run against, and
            // at whole seconds two E1M1 times are usually just equal.
            if( rec->has_record[cat][sk] )
                HS_Format_Time_CS(rec->besttime[cat][sk], timebuf, sizeof(timebuf));
            else
                snprintf(timebuf, sizeof(timebuf), "--:--.--");

            V_DrawString(x + HS_COL_TIME + cat*HS_COL_STEP
                           - V_StringWidth(timebuf),
                         row_y, option, timebuf);
        }

        row_y += 10;
        if( row_y >= BASEVIDHEIGHT )
            break;
    }

    // Blinking "NEW RECORD" for the level just finished.
    //
    // There is no vertical room left beside the table: the free band on the
    // single player intermission runs from the bottom of the Secrets row
    // (98) to SP_TIMEY (168), and the header plus five skill rows already
    // spans y-14 .. y+48, which is 102..164 at the call site's y of 116.
    // So this goes in the horizontal space instead -- the table itself only
    // occupies x .. x+180 (HS_COL_TIME + (HS_NUMCAT-1)*HS_COL_STEP), which
    // is 138..318, leaving everything left of x free in that band.
    //
    // Centred in that free region and vertically on the table block, whose
    // midpoint is ((y-14) + (y+48))/2 = y+17; the glyphs are 8 tall, so the
    // top edge is y+13.  Width is taken from V_StringWidth rather than the
    // measured 77px so the placement follows the string if it ever changes.
    boolean any_new = false;
    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        if( hs_new_record[cat] )  any_new = true;
    }

    // gametic advances through the intermission (d_clisrv.c), so it drives
    // the blink; & 16 matches the cadence wi_stuff.c uses for its own
    // flashing "you are here" pointer.
    if( any_new && (gametic & 16) )
    {
        // Option 0, not V_WHITEMAP: hu_font is already red in Doom (these
        // glyphs are palette 177..187, pure red fading dark), and V_WHITEMAP
        // is precisely what greys it out -- console.c builds that table by
        // remapping the font's reds 168..192 onto the greys 80..104.  There
        // is no V_REDMAP flag because red is the untranslated colour.
        const char * msg = "NEW RECORD";
        V_DrawString( (x - V_StringWidth(msg)) / 2, y + 13,
                      0, (char*) msg );
    }
}


// Does the running game have any recorded times?
// The attract screen uses this to skip the page when it would only say
// "No times recorded yet", which would otherwise show after every demo.
// Part of the running game, and holding at least one time.  A map with
// nothing recorded is skipped entirely rather than shown as a blank page.
static boolean  HS_Entry_Eligible( const hs_maprecord_t * rec )
{
    int  sk, cat;

    // [Arcade] HS_GameId_Mode(false), not HS_GameId(): this feeds the attract
    // page, which must always show campaign times no matter what mode the
    // cabinet was last left in.  Otherwise single-level times leak onto the
    // attract screen whenever single_level_mode happens to still be set.
    // Note HS_NextRecordDemoPath does NOT filter this way -- single-level
    // *demos* are replayed, only the score pages are campaign only.
    if( strncmp(rec->game, HS_GameId_Mode(false), HS_GAMEID_LEN-1) != 0 )
        return false;

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        for( sk=0; sk<HS_NUMSKILLS; sk++ )
        {
            if( rec->has_record[cat][sk] )  return true;
        }
    }
    return false;
}


static int  HS_Board_Page_Count( void );   // defined with the pages below

boolean  HS_Have_Records( void )
{
    int  i;

    for( i=0; i<hs_table_count; i++ )
    {
        if( HS_Entry_Eligible(&hs_table[i]) )  return true;
    }
    // [Arcade] A run board with entries is worth showing on its own, even
    // if nothing qualifies for a split page.
    return (HS_Board_Page_Count() > 0);
}


// The attract screen shows one *skill* per page, listing every map that has
// a time under it -- a 32 map table is then two pages instead of thirty-two.
// Pages are numbered linearly across (skill, chunk); the index is recomputed
// from the table each time rather than stored, since the table grows during
// a session as new maps are played.
#define HS_ROWS_PER_COL  12
#define HS_PAGE_COLS      2
#define HS_PER_PAGE      (HS_ROWS_PER_COL * HS_PAGE_COLS)

static int  hs_attract_page = 0;    // linear page index


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


// Table indices of the maps holding a time at this skill, in play order.
// Returns how many were found (bounded by out_max).
static int  HS_Collect_Skill( int sk, int * out, int out_max )
{
    int  i, j, n = 0;

    for( i=0; i<hs_table_count; i++ )
    {
        if( ! HS_Entry_Eligible(&hs_table[i]) )  continue;
        if( ! hs_table[i].has_record[HS_CAT_speed][sk]
            && ! hs_table[i].has_record[HS_CAT_max][sk] )  continue;
        if( n >= out_max )  break;

        // Insertion sort; the table is at most HS_MAX_MAPS entries.
        for( j = n; j > 0
             && HS_MapOrder(hs_table[out[j-1]].mapname) > HS_MapOrder(hs_table[i].mapname);
             j-- )
            out[j] = out[j-1];
        out[j] = i;
        n++;
    }
    return n;
}


// Pages this skill occupies (0 when it has no times at all).
static int  HS_Skill_Pages( int sk )
{
    int  idx[HS_MAX_MAPS];
    int  n = HS_Collect_Skill( sk, idx, HS_MAX_MAPS );
    return (n + HS_PER_PAGE - 1) / HS_PER_PAGE;
}


// Pages of per-map split times, which come first in the cycle.
static int  HS_Split_Page_Count( void )
{
    int  sk, total = 0;
    for( sk=0; sk<HS_NUMSKILLS; sk++ )
        total += HS_Skill_Pages(sk);
    return total;
}


// [Arcade] Entries on the campaign run board at this skill and category.
// Counted by walking places rather than the array, so it obeys the board
// depth and the current game id exactly as the drawer will.
static int  HS_Board_Rows( int sk, int cat )
{
    int  n = 0;
    while( n < HS_BOARD_DEPTH_RUN
           && HS_Board_Entry(false, NULL, (skill_e)sk, cat, n,
                             NULL, NULL, NULL) )
        n++;
    return n;
}


// One page per (skill, category) that has anyone on it.  A page is ten rows,
// which is the whole board, so a board never needs more than one -- unlike
// the split pages, which chunk a 32 map table.
static int  HS_Board_Page_Count( void )
{
    int  sk, cat, total = 0;
    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            if( HS_Board_Rows(sk, cat) > 0 )  total++;
        }
    }
    return total;
}


int HS_Attract_Page_Count( void )
{
    return HS_Split_Page_Count() + HS_Board_Page_Count();
}


// Resolve the linear page index to a skill and a chunk within it.
// False when there is nothing to show.
static boolean  HS_Resolve_Page( int page, int * out_sk, int * out_chunk )
{
    int  sk, acc = 0;

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        int np = HS_Skill_Pages(sk);
        if( np == 0 )  continue;
        if( page < acc + np )
        {
            *out_sk = sk;
            *out_chunk = page - acc;
            return true;
        }
        acc += np;
    }
    return false;
}


// Resolve a board page index (already relative to the first board page).
static boolean  HS_Resolve_Board_Page( int page, int * out_sk, int * out_cat )
{
    int  sk, cat, acc = 0;

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            if( HS_Board_Rows(sk, cat) == 0 )  continue;
            if( page == acc )
            {
                *out_sk  = sk;
                *out_cat = cat;
                return true;
            }
            acc++;
        }
    }
    return false;
}


void HS_Attract_Advance_Page( void )
{
    int  total = HS_Attract_Page_Count();
    hs_attract_page = (total > 0) ? ((hs_attract_page + 1) % total) : 0;
}


// Start the cycle over at the first page, so every appearance runs the same
// sequence from the top rather than resuming wherever the last one stopped.
void HS_Attract_Reset_Pages( void )
{
    hs_attract_page = 0;
}


// Compact two-column list.  Within a column: map name at x, then the two
// category times right-justified.  Two columns of HS_ROWS_PER_COL fit 24
// maps a page, so even a full 32 map wad is two pages per skill.
// Offsets measured against the real font: "MAP01" is 38px and a "12:34" time
// about 38px, so the speed column has to start past x+52 or the two touch.
#define HS_PG_COL0     14      // left column origin
#define HS_PG_COL1    166      // right column origin
#define HS_PG_SPEED    84      // right edge of the speed time, from origin
#define HS_PG_MAX     138      // right edge of the max time, from origin
#define HS_PG_ROW0     58      // first row baseline
#define HS_PG_ROWSTEP  10
// Bottom edge of the skill graphic.  The patches vary in height, so they are
// bottom aligned on this; 42 leaves 2px above the column headers at 44.
#define HS_PG_SKILL_BOT  42

// The New Game menu's skill graphics, indexed by skill.  Doom names; Heretic
// differs, hence the VALID_LUMP check at the draw site.
static const char * hs_skillpatch[HS_NUMSKILLS] =
  { "M_JKILL", "M_ROUGH", "M_HURT", "M_ULTRA", "M_NMARE" };

static void  HS_Draw_Row( int x, int y, const hs_maprecord_t * rec, int sk )
{
    char  timebuf[16];
    int   cat;
    static const int  right[HS_NUMCAT] = { HS_PG_SPEED, HS_PG_MAX };

    V_DrawString( x, y, 0, (char*) rec->mapname );

    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        if( rec->has_record[cat][sk] )
            HS_FormatTime( rec->besttime[cat][sk], timebuf, sizeof(timebuf) );
        else
            snprintf( timebuf, sizeof(timebuf), "--:--" );

        V_DrawString( x + right[cat] - V_StringWidth(timebuf), y, 0, timebuf );
    }
}


// [Arcade] The run board page.  One category of one skill, ten places deep,
// each naming the player, the span of levels the run covered and its time.
//
// Offsets measured against the real STCFN lumps:
//   rank    "10." 17px right-justified at 41, so 24..41 ("1." is 9px)
//   player  "AAA" 24px at 48, so 48..72
//   levels  "MAP01-MAP30" 85px at 110, so 110..195
//   time    "888:88.99" 64px right-justified at 296, so 232..296
// The headers ("PLAYER" 48, "LEVELS" 46, "TIME" 29) all sit inside their
// columns, and ten rows from 58 at HS_PG_ROWSTEP reach 148, leaving the
// footer's line at BASEVIDHEIGHT-14 clear.
#define HS_BD_RANK_R    41
#define HS_BD_INIT_X    48
#define HS_BD_RANGE_X  110
#define HS_BD_TIME_R   296
#define HS_BD_ROW0      58

// The skill a page is for -- the whole point of grouping this way.  Drawn as
// the New Game menu's own skill graphic rather than the short text name, so
// it reads at a glance from across a room.
//
// *Bottom* aligned on HS_PG_SKILL_BOT, not top aligned: the five patches are
// 15..19 tall (M_NMARE is the tall one), so aligning their tops would leave
// the baseline jumping as the page cycles.  The band is tight -- the title
// ends at 21 and the column headers start at 44 -- so the tallest lands at
// y=23 with 2px clear above and below.  Widths run to 248 (M_ROUGH), which
// still centres inside 320.
//
// Falls back to the text name if the lump is missing: these are the Doom
// names, and Heretic uses different ones.
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
                      28, V_WHITEMAP, (char*) hs_skillnames[sk] );
    }
}


static void  HS_Draw_BoardPage( int sk, int cat )
{
    char  buf[64], range[20], ini[HS_INITIALS_LEN], timebuf[16];
    tic_t tics;
    int   place;

    snprintf( buf, sizeof(buf), "BEST %s RUNS", hs_catname[cat] );
    strupr( buf );
    V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2, 14, V_WHITEMAP, buf );

    HS_Draw_SkillGraphic( sk );

    V_DrawString( HS_BD_INIT_X,  44, V_WHITEMAP, "PLAYER" );
    V_DrawString( HS_BD_RANGE_X, 44, V_WHITEMAP, "LEVELS" );
    V_DrawString( HS_BD_TIME_R - V_StringWidth("TIME"), 44, V_WHITEMAP, "TIME" );

    for( place = 0; place < HS_BOARD_DEPTH_RUN; place++ )
    {
        int y = HS_BD_ROW0 + (place * HS_PG_ROWSTEP);

        if( ! HS_Board_Entry( false, NULL, (skill_e)sk, cat, place,
                              ini, range, &tics ) )
            break;

        snprintf( buf, sizeof(buf), "%d.", place + 1 );
        V_DrawString( HS_BD_RANK_R - V_StringWidth(buf), y, 0, buf );
        V_DrawString( HS_BD_INIT_X,  y, 0, ini );
        V_DrawString( HS_BD_RANGE_X, y, 0, range );

        HS_Format_Time_CS( tics, timebuf, sizeof(timebuf) );
        V_DrawString( HS_BD_TIME_R - V_StringWidth(timebuf), y, 0, timebuf );
    }
}


void HS_Draw_AttractTable( void )
{
    int   idx[HS_MAX_MAPS];
    char  buf[64];
    int   sk = 0, chunk = 0;
    int   n, first, i, col, x, y;
    int   total;
    int   splitpages;

    // This is an attract-screen page like the ones D_PageDrawer handles, so
    // it must establish the same draw state and cover the whole screen.
    // Without this it painted over whatever the previous page or demo had
    // left in the buffer -- and with page flipping, over two different stale
    // frames alternately, which looked like flickering garbage.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );
    V_DrawScaledFill( 0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0 );  // black

    total = HS_Attract_Page_Count();

    // [Arcade] The run boards come after the per-map split pages, so the
    // cycle reads splits-then-boards.  Each board draws its own title, so
    // the "HIGH SCORES" heading belongs to the split pages only.
    splitpages = HS_Split_Page_Count();
    if( hs_attract_page >= splitpages )
    {
        int  bsk = 0, bcat = 0;
        if( HS_Resolve_Board_Page( hs_attract_page - splitpages, &bsk, &bcat ) )
        {
            HS_Draw_BoardPage( bsk, bcat );
            if( total > 1 )
            {
                snprintf( buf, sizeof(buf), "%d of %d",
                          hs_attract_page + 1, total );
                V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2,
                              BASEVIDHEIGHT-14, 0, buf );
            }
            return;
        }
        // Fell off the end (the table shrank between the count and the
        // draw); show the split page heading and the empty message below.
    }

    V_DrawString( (BASEVIDWIDTH - V_StringWidth("HIGH SCORES"))/2,
                  14, V_WHITEMAP, "HIGH SCORES" );

    if( ! HS_Resolve_Page( hs_attract_page, &sk, &chunk ) )
    {
        V_DrawString( (BASEVIDWIDTH - V_StringWidth("No times recorded yet"))/2,
                      90, 0, "No times recorded yet" );
        return;
    }

    HS_Draw_SkillGraphic( sk );

    // Column headers over both columns.  A "max" run additionally requires
    // 100% kills and secrets on every level, so its times are always >= speed.
    for( col=0; col<HS_PAGE_COLS; col++ )
    {
        x = col ? HS_PG_COL1 : HS_PG_COL0;
        V_DrawString( x, 44, V_WHITEMAP, "MAP" );
        V_DrawString( x + HS_PG_SPEED - V_StringWidth((char*)hs_catname[HS_CAT_speed]),
                      44, V_WHITEMAP, (char*) hs_catname[HS_CAT_speed] );
        V_DrawString( x + HS_PG_MAX - V_StringWidth((char*)hs_catname[HS_CAT_max]),
                      44, V_WHITEMAP, (char*) hs_catname[HS_CAT_max] );
    }

    n = HS_Collect_Skill( sk, idx, HS_MAX_MAPS );
    first = chunk * HS_PER_PAGE;

    for( i=0; i<HS_PER_PAGE; i++ )
    {
        int  e = first + i;
        if( e >= n )  break;

        col = i / HS_ROWS_PER_COL;          // fill the left column first
        x   = col ? HS_PG_COL1 : HS_PG_COL0;
        y   = HS_PG_ROW0 + ((i % HS_ROWS_PER_COL) * HS_PG_ROWSTEP);

        HS_Draw_Row( x, y, &hs_table[ idx[e] ], sk );
    }

    // Footer, so it is obvious that other pages follow.  (total was taken
    // at the top, before the board pages were dispatched.)
    if( total > 1 )
    {
        snprintf( buf, sizeof(buf), "%d of %d", hs_attract_page + 1, total );
        V_DrawString( (BASEVIDWIDTH - V_StringWidth(buf))/2,
                      BASEVIDHEIGHT-14, 0, buf );
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
    char campaign_id[HS_GAMEID_LEN];
    char single_id[HS_GAMEID_LEN];
    int  slot;
    int  tries;
    int  mi, sk, cat;
    boolean is_single;

    if( hs_table_count == 0 )  return NULL;

    dl_strncpy( campaign_id, HS_GameId_Mode(false), HS_GAMEID_LEN-1 );
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
        // [Arcade] Both modes are replayed.  A single-level demo is an
        // ordinary one map recording -- nothing about playing it back is
        // mode specific -- so it is captioned rather than skipped.  This
        // deliberately differs from HS_Entry_Eligible, which still keeps
        // single-level times off the score *pages*: a demo costs the cycle
        // nothing extra, while those entries would double the page count.
        if( strncmp(hs_table[mi].game, campaign_id, HS_GAMEID_LEN-1) == 0 )
            is_single = false;
        else if( strncmp(hs_table[mi].game, single_id, HS_GAMEID_LEN-1) == 0 )
            is_single = true;
        else
            continue;

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
                // which badly undersold the longer runs.  A record with no
                // start map (written before that was stored) falls back to
                // the bare name rather than inventing a span.
                if( sm[0] && strncmp(sm, hs_table[mi].mapname, 8) != 0 )
                    snprintf(range, sizeof(range), "%.8s-%.8s",
                             sm, hs_table[mi].mapname);
                else
                    snprintf(range, sizeof(range), "%.8s",
                             hs_table[mi].mapname);

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



