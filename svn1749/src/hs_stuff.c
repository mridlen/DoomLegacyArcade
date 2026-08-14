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
#include "hs_stuff.h"

#define HS_MAX_MAPS      64

// Records are keyed by game as well as map: Doom 2, Plutonia and TNT all
// have a MAP01, and they are different levels.
#define HS_GAMEID_LEN  12

typedef struct
{
    char     game[HS_GAMEID_LEN];   // gamedesc idstr: doom2, plutonia, tnt...
    char     mapname[9];
    boolean  has_record[HS_NUMSKILLS];
    tic_t    besttime[HS_NUMSKILLS];
} hs_maprecord_t;

static hs_maprecord_t  hs_table[HS_MAX_MAPS];
static int              hs_table_count = 0;

static tic_t   hs_cumulative_time = 0;
static char    hs_last_exit_mapname[9] = "";
static skill_e hs_last_exit_skill = sk_baby;

static char    hs_scorefile[MAX_WADPATH];
static char    hs_demodir[MAX_WADPATH];

static const char * hs_skillnames[HS_NUMSKILLS] = { "ITYTD", "HNTR", "HMP", "UV", "NM" };


// The running game's short name, as used in the score file and demo names.
static const char * HS_GameId( void )
{
    return ( gamedesc.idstr && gamedesc.idstr[0] ) ? gamedesc.idstr : "game";
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
                              const char * mapname, skill_e skill )
{
    char relname[48];
    // Bound the parts explicitly; map name is at most 8 ("MAPxx"/"ExMy").
    snprintf(relname, sizeof(relname), "%.11s_%.8s_sk%d.lmp",
             game, mapname, (int)skill);
    cat_filename(dest, hs_demodir, relname);
}


static void HS_Load( void )
{
    FILE * fr;
    char   line[96];
    char   game[16];
    char   mapname[16];
    int    skillnum;
    unsigned int  tics;
    int    old_format = 0;

    hs_table_count = 0;

    fr = fopen(hs_scorefile, "r");
    if( ! fr )  return;

    while( fgets(line, sizeof(line), fr) )
    {
        if( line[0] == '#' || line[0] == '\n' || line[0] == 0 )
            continue;
        if( sscanf(line, "%15s %15s %d %u", game, mapname, &skillnum, &tics) != 4 )
        {
            // Records written before scores were tracked per game cannot be
            // attributed to one, so they are dropped rather than guessed at.
            old_format ++;
            continue;
        }
        if( skillnum < 0 || skillnum >= HS_NUMSKILLS )
            continue;

        hs_maprecord_t * rec = HS_FindOrAddRecord(game, mapname);
        if( ! rec )  continue;
        rec->has_record[skillnum] = true;
        rec->besttime[skillnum]   = (tic_t) tics;
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
    int    i, sk;

    fw = fopen(hs_scorefile, "w");
    if( ! fw )
    {
        GenPrintf(EMSG_warn, "HS_Save: could not write %s\n", hs_scorefile);
        return;
    }

    fprintf(fw, "# DoomLegacy arcade high scores: game mapname skill cumulative_tics\n");
    for( i=0; i<hs_table_count; i++ )
    {
        for( sk=0; sk<HS_NUMSKILLS; sk++ )
        {
            if( hs_table[i].has_record[sk] )
                fprintf(fw, "%s %s %d %u\n",
                        hs_table[i].game, hs_table[i].mapname, sk,
                        (unsigned int) hs_table[i].besttime[sk]);
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
    G_BeginRecording();
}


void HS_LevelExit( int episode, int map, skill_e skill, tic_t leveltime )
{
    if( netgame || multiplayer || deathmatch )  return;
    // Never score a replay: attract-mode demo playback re-runs level exits.
    if( demoplayback )  return;
    if( skill < 0 || skill >= HS_NUMSKILLS )  return;

    hs_cumulative_time += leveltime;

    const char * mapname = G_BuildMapName(episode, map);
    hs_maprecord_t * rec = HS_FindOrAddRecord(HS_GameId(), mapname);
    if( rec == NULL )  return;   // table full

    dl_strncpy(hs_last_exit_mapname, mapname, 8);
    hs_last_exit_skill = skill;

    if( (! rec->has_record[skill]) || (hs_cumulative_time < rec->besttime[skill]) )
    {
        rec->has_record[skill] = true;
        rec->besttime[skill]   = hs_cumulative_time;
        HS_Save();

        if( demorecording )
        {
            char demopath[MAX_WADPATH];
            HS_BuildDemoPath(demopath, HS_GameId(), mapname, skill);
            G_SnapshotDemo(demopath);
        }
    }
}


void HS_Draw_IntermissionTable( int x, int y )
{
    hs_maprecord_t * rec;
    char   timebuf[16];
    int    i, sk;
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

    V_DrawString(x, row_y-14, 0, "Best Times");

    for( sk=0; sk<HS_NUMSKILLS; sk++ )
    {
        int option = (sk == hs_last_exit_skill) ? V_WHITEMAP : 0;

        V_DrawString(x, row_y, option, hs_skillnames[sk]);

        if( rec->has_record[sk] )
            HS_FormatTime(rec->besttime[sk], timebuf, sizeof(timebuf));
        else
            snprintf(timebuf, sizeof(timebuf), "--:--");

        V_DrawString(x+90-V_StringWidth(timebuf), row_y, option, timebuf);

        row_y += 10;
        if( row_y >= BASEVIDHEIGHT )
            break;
    }
}


void HS_Draw_AttractTable( void )
{
    char   timebuf[16];
    int    i, sk;
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
    y += 14;

    // Only the running game's records: the attract screen advertises the
    // game that is about to be played, and Doom 2 / Plutonia / TNT all have
    // a MAP01 which would otherwise be indistinguishable in this list.
    for( i=0, shown=0; i<hs_table_count && y < BASEVIDHEIGHT-10; i++ )
    {
        if( strncmp(hs_table[i].game, HS_GameId(), HS_GAMEID_LEN-1) != 0 )
            continue;

        for( sk=0; sk<HS_NUMSKILLS; sk++ )
        {
            if( ! hs_table[i].has_record[sk] )  continue;
            shown ++;

            // Columns: map at x, skill at x+50 (up to 5 chars, "ITYTD"),
            // time right-justified at x+150 so it clears the skill name.
            V_DrawString(x, y, 0, hs_table[i].mapname);
            V_DrawString(x+50, y, 0, hs_skillnames[sk]);

            HS_FormatTime(hs_table[i].besttime[sk], timebuf, sizeof(timebuf));
            V_DrawString(x+150-V_StringWidth(timebuf), y, 0, timebuf);

            y += 10;
            if( y >= BASEVIDHEIGHT-10 )  break;
        }
    }

    if( shown == 0 )
        V_DrawString(x, y, 0, "No times recorded yet");
}


const char * HS_NextRecordDemoPath( void )
{
    static char path[MAX_WADPATH];
    static int  cursor = 0;
    int  total;
    int  tries;
    int  mi, sk;

    if( hs_table_count == 0 )  return NULL;

    total = hs_table_count * HS_NUMSKILLS;

    for( tries=0; tries<total; tries++, cursor=(cursor+1)%total )
    {
        mi = cursor / HS_NUMSKILLS;
        sk = cursor % HS_NUMSKILLS;
        // Only demos from the running game: the same map name is a
        // different level in Doom 2, Plutonia and TNT, so replaying another
        // game's demo would desync immediately.
        if( strncmp(hs_table[mi].game, HS_GameId(), HS_GAMEID_LEN-1) != 0 )
            continue;
        if( hs_table[mi].has_record[sk] )
        {
            HS_BuildDemoPath(path, hs_table[mi].game, hs_table[mi].mapname,
                             (skill_e)sk);
            if( access(path, R_OK) == 0 )
            {
                cursor = (cursor+1) % total;
                return path;
            }
        }
    }

    return NULL;
}
