// [Arcade] Persistent per-map, per-skill best cumulative-time high scores,
// plus the always-on background demo recording tied to new records.
// Single-player only.

#include <unistd.h>     // access()
#include <sys/types.h>
#include <dirent.h>     // demo directory sweep in Command_ClearHighScores_f

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
#define HS_COL_TIME   90
#define HS_COL_STEP   62
// The attract page also carries a map-name column, so its times sit further
// right of the table origin (x=40 there, versus x=156 at the intermission).
#define HS_ATT_TIME   150

typedef struct
{
    char     game[HS_GAMEID_LEN];   // gamedesc idstr: doom2, plutonia, tnt...
    char     mapname[9];
    boolean  has_record[HS_NUMCAT][HS_NUMSKILLS];
    tic_t    besttime[HS_NUMCAT][HS_NUMSKILLS];
} hs_maprecord_t;

static hs_maprecord_t  hs_table[HS_MAX_MAPS];
static int              hs_table_count = 0;

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

boolean  HS_Run_Is_Ranked( void )
{
    return hs_run_ranked;
}
static char    hs_last_exit_mapname[9] = "";
static skill_e hs_last_exit_skill = sk_baby;

static char    hs_scorefile[MAX_WADPATH];
static char    hs_demodir[MAX_WADPATH];

static const char * hs_skillnames[HS_NUMSKILLS] = { "ITYTD", "HNTR", "HMP", "UV", "NM" };


// The key identifying what is being played, used in the score file and in
// record demo names: the game's short name, plus the loaded level pack.
// Recomputed each call because a pack can be loaded mid-session.
static const char * HS_GameId( void )
{
    static char  id[HS_GAMEID_LEN];
    const char * game = ( gamedesc.idstr && gamedesc.idstr[0] )
                        ? gamedesc.idstr : "game";
    const char * pack = M_LevelPack_LoadedName();
    char * p;

    if( pack )
        snprintf( id, sizeof(id), "%s+%s", game, pack );
    else
        snprintf( id, sizeof(id), "%s", game );

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


static void HS_FormatTime( tic_t tics, char * buf, size_t bufsize )
{
    int seconds = tics / TICRATE;
    int minutes = seconds / 60;
    int secs    = seconds % 60;
    snprintf(buf, bufsize, "%d:%02d", minutes, secs);
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
        // The category was added last and is written at the end, so a line
        // without it is a speed record from before the split.
        int nf = sscanf(line, "%63s %15s %d %u %15s",
                        game, mapname, &skillnum, &tics, catname);
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
                " wadcombo mapname skill cumulative_tics category\n");
    for( i=0; i<hs_table_count; i++ )
    {
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            for( sk=0; sk<HS_NUMSKILLS; sk++ )
            {
                if( hs_table[i].has_record[cat][sk] )
                    fprintf(fw, "%s %s %d %u %s\n",
                            hs_table[i].game, hs_table[i].mapname, sk,
                            (unsigned int) hs_table[i].besttime[cat][sk],
                            hs_catname[cat]);
            }
        }
    }

    fclose(fw);
}


void HS_Init( void )
{
    cat_filename( hs_scorefile, legacyhome, "highscores.dat" );
    cat_filename( hs_demodir,   legacyhome, "demos" );

    if( access(hs_demodir, R_OK) < 0 )
        I_mkdir( hs_demodir, 0700 );

    HS_Load();
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

    // An altered ruleset makes the run unscoreable, so do not spend the
    // demo buffer on it either -- nothing would ever be saved from it.
    hs_run_ranked = HS_Ruleset_Is_Ranked();
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


void HS_LevelExit( int episode, int map, skill_e skill, tic_t leveltime,
                   boolean maxed )
{
    if( netgame || multiplayer || deathmatch )  return;
    // Never score a replay: attract-mode demo playback re-runs level exits.
    if( demoplayback )  return;
    if( skill < 0 || skill >= HS_NUMSKILLS )  return;

    hs_cumulative_time += leveltime;

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
    }
    if( ! hs_run_ranked )
        return;

    const char * mapname = G_BuildMapName(episode, map);
    hs_maprecord_t * rec = HS_FindOrAddRecord(HS_GameId(), mapname);
    if( rec == NULL )  return;   // table full

    dl_strncpy(hs_last_exit_mapname, mapname, 8);
    hs_last_exit_skill = skill;

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
        saved = true;

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
            if( rec->has_record[cat][sk] )
                HS_FormatTime(rec->besttime[cat][sk], timebuf, sizeof(timebuf));
            else
                snprintf(timebuf, sizeof(timebuf), "--:--");

            V_DrawString(x + HS_COL_TIME + cat*HS_COL_STEP
                           - V_StringWidth(timebuf),
                         row_y, option, timebuf);
        }

        row_y += 10;
        if( row_y >= BASEVIDHEIGHT )
            break;
    }
}


// Does the running game have any recorded times?
// The attract screen uses this to skip the page when it would only say
// "No times recorded yet", which would otherwise show after every demo.
boolean  HS_Have_Records( void )
{
    int  i, sk, cat;

    for( i=0; i<hs_table_count; i++ )
    {
        if( strncmp(hs_table[i].game, HS_GameId(), HS_GAMEID_LEN-1) != 0 )
            continue;
        for( cat=0; cat<HS_NUMCAT; cat++ )
        {
            for( sk=0; sk<HS_NUMSKILLS; sk++ )
            {
                if( hs_table[i].has_record[cat][sk] )  return true;
            }
        }
    }
    return false;
}


void HS_Draw_AttractTable( void )
{
    char   timebuf[16];
    int    i, sk, cat;
    int    shown = 0;
    int    x = 40;
    int    y = 20;

    // This is an attract-screen page like the ones D_PageDrawer handles, so
    // it must establish the same draw state and cover the whole screen.
    // Without this it painted over whatever the previous page or demo had
    // left in the buffer -- and with page flipping, over two different stale
    // frames alternately, which looked like flickering garbage.
    V_SetupDraw( 0 | V_SCALESTART | V_SCALEPATCH | V_CENTERHORZ );
    V_DrawScaledFill( 0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 0 );  // black

    V_DrawString(x, y, V_WHITEMAP, "HIGH SCORES - BEST TIME TO EXIT");
    y += 12;

    // Column header.  "max" runs additionally require 100% kills and 100%
    // secrets on every level of the run, so its times are always >= speed.
    for( cat=0; cat<HS_NUMCAT; cat++ )
    {
        const char * cn = hs_catname[cat];
        V_DrawString(x + HS_ATT_TIME + cat*HS_COL_STEP - V_StringWidth(cn),
                     y, V_WHITEMAP, cn);
    }
    y += 12;

    // Only the running game's records: the attract screen advertises the
    // game that is about to be played, and Doom 2 / Plutonia / TNT all have
    // a MAP01 which would otherwise be indistinguishable in this list.
    for( i=0, shown=0; i<hs_table_count && y < BASEVIDHEIGHT-10; i++ )
    {
        if( strncmp(hs_table[i].game, HS_GameId(), HS_GAMEID_LEN-1) != 0 )
            continue;

        for( sk=0; sk<HS_NUMSKILLS; sk++ )
        {
            if( ! hs_table[i].has_record[HS_CAT_speed][sk]
                && ! hs_table[i].has_record[HS_CAT_max][sk] )  continue;
            shown ++;

            // Columns: map at x, skill at x+50 (up to 5 chars, "ITYTD"),
            // then one right-justified time column per category.
            V_DrawString(x, y, 0, hs_table[i].mapname);
            V_DrawString(x+50, y, 0, hs_skillnames[sk]);

            for( cat=0; cat<HS_NUMCAT; cat++ )
            {
                if( hs_table[i].has_record[cat][sk] )
                    HS_FormatTime(hs_table[i].besttime[cat][sk],
                                  timebuf, sizeof(timebuf));
                else
                    snprintf(timebuf, sizeof(timebuf), "--:--");

                V_DrawString(x + HS_ATT_TIME + cat*HS_COL_STEP
                               - V_StringWidth(timebuf),
                             y, 0, timebuf);
            }

            y += 10;
            if( y >= BASEVIDHEIGHT-10 )  break;
        }
    }

    if( shown == 0 )
        V_DrawString(x, y, 0, "No times recorded yet");
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


const char * HS_NextRecordDemoPath( void )
{
    static char path[MAX_WADPATH];
    static int  cursor = 0;
    int  total;
    int  tries;
    int  mi, sk, cat;

    if( hs_table_count == 0 )  return NULL;

    total = hs_table_count * HS_NUMSKILLS * HS_NUMCAT;

    for( tries=0; tries<total; tries++, cursor=(cursor+1)%total )
    {
        mi  = cursor / (HS_NUMSKILLS * HS_NUMCAT);
        sk  = (cursor / HS_NUMCAT) % HS_NUMSKILLS;
        cat = cursor % HS_NUMCAT;
        // Only demos from the running game: the same map name is a
        // different level in Doom 2, Plutonia and TNT, so replaying another
        // game's demo would desync immediately.
        if( strncmp(hs_table[mi].game, HS_GameId(), HS_GAMEID_LEN-1) != 0 )
            continue;
        if( hs_table[mi].has_record[cat][sk] )
        {
            HS_BuildDemoPath(path, hs_table[mi].game, hs_table[mi].mapname,
                             (skill_e)sk, cat);
            if( access(path, R_OK) == 0 )
            {
                char timebuf[16];
                HS_FormatTime(hs_table[mi].besttime[cat][sk],
                              timebuf, sizeof(timebuf));
                // e.g. "E1M1  ITYTD  MAX  4:32"
                snprintf(hs_demo_label, sizeof(hs_demo_label),
                         "%s  %s  %s  %s", hs_table[mi].mapname,
                         hs_skillnames[sk], hs_catname[cat], timebuf);
                strupr(hs_demo_label);

                cursor = (cursor+1) % total;
                return path;
            }
        }
    }

    return NULL;
}


