// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// [Arcade] Operator audit counters.  See au_stuff.h for what this is for.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "au_stuff.h"
#include "d_clisrv.h"
#include "d_main.h"
#include "g_game.h"
#include "m_menu.h"
#include "m_misc.h"
#include "v_video.h"
#include "command.h"

#include <time.h>


// How many distinct maps to keep play counts for.  A cabinet sees one or two
// IWADs and the odd level pack, so this is generous; when it fills, further
// new maps simply are not counted rather than evicting something older, which
// would make the numbers quietly wrong instead of visibly incomplete.
#define AU_MAXMAPS      192
#define AU_KEY_LEN      48
#define AU_MAP_LEN      12

typedef struct {
    char          game[AU_KEY_LEN];   // wad combination, as high scores key it
    char          map[AU_MAP_LEN];
    unsigned int  plays;
} au_maprec_t;

static au_maprec_t  au_map[AU_MAXMAPS];
static int          au_map_count = 0;

static unsigned int  au_boots;
static unsigned int  au_games;                 // games started
static unsigned int  au_games_players[MAXSPLITSCREENPLAYERS];   // by headcount
static unsigned int  au_games_single_level;
static unsigned int  au_levels_completed;
static unsigned int  au_deaths;
static unsigned int  au_placements;
static unsigned int  au_unranked[AU_NUMUNRANKED];
static tic_t         au_play_tics;   // tics with a real game on screen
static tic_t         au_up_tics;     // tics the program has been running
static char          au_since[24];   // date the counters were started

static char     au_file[MAX_WADPATH];


//===========================================================================
//  Keys
//===========================================================================

// The wad combination a map belongs to, so Doom II's MAP01 and a level pack's
// MAP01 are counted apart -- the same reasoning, and deliberately the same
// shape, as the high score key (HS_GameId_Mode in hs_stuff.c).  Kept local
// rather than shared because that one folds in the single-level mode flag,
// which is a scoring distinction and not a "which map is this" one: a map
// played in Single Level and in a campaign is the same map to an operator
// asking which maps get played.
static const char * AU_GameKey( void )
{
    static char  key[AU_KEY_LEN];
    const char * game = ( gamedesc.idstr && gamedesc.idstr[0] )
                        ? gamedesc.idstr : "game";
    const char * pack = M_LevelPack_LoadedName();
    char * p;

    if( pack )
        snprintf( key, sizeof(key), "%s+%s", game, pack );
    else
        snprintf( key, sizeof(key), "%s", game );

    // A single filename-safe word: this is a space separated field in
    // audit.dat and pack names come from arbitrary filenames.
    for( p = key; *p; p++ )
    {
        if( ! ( isalnum((unsigned char)*p)
                || *p=='-' || *p=='_' || *p=='.' || *p=='+' ) )
            *p = '_';
    }
    return key;
}


static au_maprec_t * AU_FindOrAddMap( const char * game, const char * map )
{
    int i;

    for( i = 0; i < au_map_count; i++ )
    {
        if( strcasecmp(au_map[i].game, game) == 0
            && strcasecmp(au_map[i].map, map) == 0 )
            return &au_map[i];
    }
    if( au_map_count >= AU_MAXMAPS )  return NULL;

    dl_strncpy( au_map[au_map_count].game, game, AU_KEY_LEN );
    dl_strncpy( au_map[au_map_count].map,  map,  AU_MAP_LEN );
    au_map[au_map_count].plays = 0;
    return &au_map[au_map_count++];
}


//===========================================================================
//  Persistence
//===========================================================================

static void AU_Load( void )
{
    FILE * fr;
    char   line[160];
    char   key[32], a[AU_KEY_LEN], b[AU_MAP_LEN];
    unsigned int  v;

    fr = fopen(au_file, "r");
    if( ! fr )  return;

    while( fgets(line, sizeof(line), fr) )
    {
        if( line[0] == '#' || line[0] == '\n' || line[0] == 0 )
            continue;

        // Per-map lines carry two names and a count; everything else is a
        // single "key value" pair.  Unknown keys are skipped rather than
        // rejected, so a file written by a later build still loads.
        if( sscanf(line, "map %47s %11s %u", a, b, &v) == 3 )
        {
            au_maprec_t * m = AU_FindOrAddMap(a, b);
            if( m )  m->plays = v;
            continue;
        }
        if( sscanf(line, "since %23s", a) == 1 )
        {
            dl_strncpy( au_since, a, sizeof(au_since) );
            continue;
        }
        if( sscanf(line, "%31s %u", key, &v) != 2 )  continue;

        if     ( strcmp(key, "boots")      == 0 )  au_boots = v;
        else if( strcmp(key, "games")      == 0 )  au_games = v;
        else if( strcmp(key, "players1")   == 0 )  au_games_players[0] = v;
        else if( strcmp(key, "players2")   == 0 )  au_games_players[1] = v;
        else if( strcmp(key, "players3")   == 0 )  au_games_players[2] = v;
        else if( strcmp(key, "players4")   == 0 )  au_games_players[3] = v;
        else if( strcmp(key, "singlelevel")== 0 )  au_games_single_level = v;
        else if( strcmp(key, "levels")     == 0 )  au_levels_completed = v;
        else if( strcmp(key, "deaths")     == 0 )  au_deaths = v;
        else if( strcmp(key, "placements") == 0 )  au_placements = v;
        else if( strcmp(key, "unranked_ruleset") == 0 ) au_unranked[AU_UR_ruleset] = v;
        else if( strcmp(key, "unranked_cheat")   == 0 ) au_unranked[AU_UR_cheat]   = v;
        else if( strcmp(key, "unranked_death")   == 0 ) au_unranked[AU_UR_death]   = v;
        else if( strcmp(key, "playtics")   == 0 )  au_play_tics = (tic_t) v;
        else if( strcmp(key, "uptics")     == 0 )  au_up_tics   = (tic_t) v;
    }

    fclose(fr);
}


void AU_Save( void )
{
    FILE * fw;
    int    i;

    if( ! au_file[0] )  return;

    fw = fopen(au_file, "w");
    if( ! fw )
    {
        GenPrintf(EMSG_warn, "AU_Save: could not write %s\n", au_file);
        return;
    }

    fprintf(fw, "# DoomLegacy arcade audit counters."
                "  \"clearaudit\" resets them.\n");
    fprintf(fw, "since %s\n", au_since[0] ? au_since : "-");
    fprintf(fw, "boots %u\n", au_boots);
    fprintf(fw, "games %u\n", au_games);
    for( i = 0; i < MAXSPLITSCREENPLAYERS; i++ )
        fprintf(fw, "players%d %u\n", i+1, au_games_players[i]);
    fprintf(fw, "singlelevel %u\n", au_games_single_level);
    fprintf(fw, "levels %u\n", au_levels_completed);
    fprintf(fw, "deaths %u\n", au_deaths);
    fprintf(fw, "placements %u\n", au_placements);
    fprintf(fw, "unranked_ruleset %u\n", au_unranked[AU_UR_ruleset]);
    fprintf(fw, "unranked_cheat %u\n",   au_unranked[AU_UR_cheat]);
    fprintf(fw, "unranked_death %u\n",   au_unranked[AU_UR_death]);
    fprintf(fw, "playtics %u\n", (unsigned int) au_play_tics);
    fprintf(fw, "uptics %u\n",   (unsigned int) au_up_tics);

    for( i = 0; i < au_map_count; i++ )
    {
        if( ! au_map[i].plays )  continue;
        fprintf(fw, "map %s %s %u\n",
                au_map[i].game, au_map[i].map, au_map[i].plays);
    }

    fclose(fw);
}


void AU_Clear( void )
{
    time_t     now = time(NULL);
    struct tm * lt = localtime(&now);
    int i;

    au_map_count = 0;
    memset( au_map, 0, sizeof(au_map) );
    au_boots = au_games = au_games_single_level = 0;
    au_levels_completed = au_deaths = au_placements = 0;
    au_play_tics = au_up_tics = 0;
    for( i = 0; i < MAXSPLITSCREENPLAYERS; i++ )  au_games_players[i] = 0;
    for( i = 0; i < AU_NUMUNRANKED; i++ )  au_unranked[i] = 0;

    au_since[0] = 0;
    if( lt )
        strftime( au_since, sizeof(au_since), "%Y-%m-%d", lt );

    AU_Save();
}


//===========================================================================
//  Counting
//===========================================================================

void AU_Ticker( void )
{
    au_up_tics++;

    // Play time is a real game on screen, which is not the same as "a level
    // is loaded": the attract cycle plays record demos in GS_LEVEL all day,
    // and counting those would make an idle cabinet look busy.
    // D_Attract_Running() is the engine's own "a real game is running" flag,
    // the same one the attract volume and the menu backdrop ask.
    if( gamestate == GS_LEVEL && !demoplayback && !D_Attract_Running() )
        au_play_tics++;

    // Deliberately does not ask for a save: that would be once a tic.  These
    // two clocks ride along with whatever save the next event triggers, and
    // with the one in D_Quit_Save.
}


void AU_Game_Started( void )
{
    byte n;

    if( demoplayback )  return;

    au_games++;
    if( single_level_mode )
        au_games_single_level++;

    n = D_NumLocalPlayers();
    if( n < 1 )  n = 1;
    if( n > MAXSPLITSCREENPLAYERS )  n = MAXSPLITSCREENPLAYERS;
    au_games_players[n-1]++;
}


void AU_Level_Started( void )
{
    au_maprec_t * m;

    if( demoplayback )  return;

    m = AU_FindOrAddMap( AU_GameKey(), G_BuildMapName(gameepisode, gamemap) );
    if( m )  m->plays++;
}


void AU_Level_Completed( void )
{
    if( demoplayback )  return;
    au_levels_completed++;
}


void AU_Player_Death( void )
{
    if( demoplayback )  return;
    au_deaths++;
}


void AU_Unranked( au_unranked_e reason )
{
    if( demoplayback )  return;
    if( reason < 0 || reason >= AU_NUMUNRANKED )  return;
    au_unranked[reason]++;
}


void AU_Board_Placement( void )
{
    if( demoplayback )  return;
    au_placements++;
}


//===========================================================================
//  Console
//===========================================================================

// hours:mm:ss from a tic count.  Hours are not wrapped: a cabinet that has
// been up for 300 hours should say so.
static void AU_Format_Time( char * dest, size_t len, tic_t tics )
{
    unsigned int secs = (unsigned int)(tics / TICRATE);
    snprintf( dest, len, "%u:%02u:%02u",
              secs / 3600, (secs / 60) % 60, secs % 60 );
}


static void Command_Audit_f( void )
{
    char  buf[32];
    int   i;

    CONS_Printf("Audit counters since %s\n", au_since[0] ? au_since : "?");
    CONS_Printf("  games started    %u\n", au_games);
    for( i = 0; i < MAXSPLITSCREENPLAYERS; i++ )
        CONS_Printf("    %d player       %u\n", i+1, au_games_players[i]);
    CONS_Printf("  single level     %u\n", au_games_single_level);
    CONS_Printf("  levels completed %u\n", au_levels_completed);
    CONS_Printf("  player deaths    %u\n", au_deaths);
    CONS_Printf("  board placements %u\n", au_placements);
    CONS_Printf("  unranked: settings %u  cheat %u  death %u\n",
                au_unranked[AU_UR_ruleset], au_unranked[AU_UR_cheat],
                au_unranked[AU_UR_death]);
    AU_Format_Time(buf, sizeof(buf), au_play_tics);
    CONS_Printf("  play time        %s\n", buf);
    AU_Format_Time(buf, sizeof(buf), au_up_tics);
    CONS_Printf("  uptime           %s\n", buf);
    CONS_Printf("  boots            %u\n", au_boots);

    for( i = 0; i < au_map_count; i++ )
    {
        if( ! au_map[i].plays )  continue;
        CONS_Printf("  map %s %s %u\n",
                    au_map[i].game, au_map[i].map, au_map[i].plays);
    }
}


static void Command_ClearAudit_f( void )
{
    AU_Clear();
    CONS_Printf("Audit counters cleared.\n");
}


//===========================================================================
//  The page
//===========================================================================

// Pick the most played maps, highest first, into a caller supplied index
// array.  A selection sort over at most AU_MAXMAPS entries, run once per
// frame of a page nobody looks at for long: clarity is worth more here than
// the microseconds a better sort would save.
static int AU_Top_Maps( int * out, int want )
{
    boolean used[AU_MAXMAPS];
    int  n = 0, i, k, best;

    memset( used, 0, sizeof(used) );
    while( n < want )
    {
        best = -1;
        for( i = 0; i < au_map_count; i++ )
        {
            if( used[i] || ! au_map[i].plays )  continue;
            if( best < 0 || au_map[i].plays > au_map[best].plays )  best = i;
        }
        if( best < 0 )  break;
        used[best] = true;
        out[n++] = best;
    }
    for( k = n; k < want; k++ )  out[k] = -1;
    return n;
}


// One "label            value" row.  The value is right justified at x+width
// so the numbers line up in a column, which is the whole point of a table.
// hu_font is proportional, so the position is measured from the string rather
// than assumed (see CLAUDE.md on never eyeballing text layout).
static void AU_Row( int x, int y, int width, const char * label, const char * value )
{
    V_DrawString( x, y, V_WHITEMAP, (char*) label );
    V_DrawString( x + width - V_StringWidth((char*)value), y,
                  V_WHITEMAP, (char*) value );
}


// Two columns, because one column of all of this does not fit in 200 base
// units -- it ran to y=205 before the split.  Glyphs are 7 tall, so 8 is the
// tightest pitch that still leaves a gap between rows.  Labels are kept to
// about 12 characters so that even the widest of them clears the right
// justified value in a 150 unit column.
void AU_Drawer( void )
{
    char  buf[40];
    char  lab[40];
    const int  pitch = 8;
    const int  lx = 6,   lw = 150;   // left column: label at lx, value ends lx+lw
    const int  rx = 166, rw = 148;
    int   i, n, top[3], y;

    snprintf(buf, sizeof(buf), "SINCE %s", au_since[0] ? au_since : "?");
    V_DrawString( lx, 26, V_WHITEMAP, buf );

    // ---- left column: what got played
    y = 42;
    snprintf(buf, sizeof(buf), "%u", au_games);
    AU_Row( lx, y, lw, "GAMES", buf );  y += pitch;

    for( i = 0; i < MAXSPLITSCREENPLAYERS; i++ )
    {
        snprintf(lab, sizeof(lab), "  %d PLAYER", i+1);
        snprintf(buf, sizeof(buf), "%u", au_games_players[i]);
        AU_Row( lx, y, lw, lab, buf );  y += pitch;
    }

    snprintf(buf, sizeof(buf), "%u", au_games_single_level);
    AU_Row( lx, y, lw, "  SINGLE LEVEL", buf );  y += pitch;

    snprintf(buf, sizeof(buf), "%u", au_levels_completed);
    AU_Row( lx, y, lw, "LEVELS DONE", buf );  y += pitch;

    snprintf(buf, sizeof(buf), "%u", au_deaths);
    AU_Row( lx, y, lw, "DEATHS", buf );  y += pitch;

    snprintf(buf, sizeof(buf), "%u", au_placements);
    AU_Row( lx, y, lw, "PLACINGS", buf );  y += pitch;

    // ---- right column: how hard it has worked, and how it is scoring
    y = 42;
    AU_Format_Time(buf, sizeof(buf), au_play_tics);
    AU_Row( rx, y, rw, "PLAY TIME", buf );  y += pitch;

    AU_Format_Time(buf, sizeof(buf), au_up_tics);
    AU_Row( rx, y, rw, "UPTIME", buf );  y += pitch;

    // What share of the cabinet's running time is actually being played --
    // the number an operator wants when deciding whether it earns its corner.
    snprintf(buf, sizeof(buf), "%u%%",
             au_up_tics ? (unsigned int)((au_play_tics * 100) / au_up_tics) : 0);
    AU_Row( rx, y, rw, "IN PLAY", buf );  y += pitch;

    snprintf(buf, sizeof(buf), "%u", au_boots);
    AU_Row( rx, y, rw, "BOOTS", buf );  y += pitch * 2;

    V_DrawString( rx, y, V_WHITEMAP, "UNRANKED" );  y += pitch;
    snprintf(buf, sizeof(buf), "%u", au_unranked[AU_UR_ruleset]);
    AU_Row( rx, y, rw, "  BY SETTINGS", buf );  y += pitch;
    snprintf(buf, sizeof(buf), "%u", au_unranked[AU_UR_cheat]);
    AU_Row( rx, y, rw, "  BY CHEAT", buf );  y += pitch;
    snprintf(buf, sizeof(buf), "%u", au_unranked[AU_UR_death]);
    AU_Row( rx, y, rw, "  BY DEATH", buf );  y += pitch;

    // ---- across the bottom
    y = 126;
    V_DrawString( lx, y, V_WHITEMAP, "MOST PLAYED" );  y += pitch;
    n = AU_Top_Maps( top, 3 );
    if( n == 0 )
    {
        V_DrawString( lx, y, V_WHITEMAP, "  NOTHING YET" );
    }
    else
    {
        for( i = 0; i < n; i++ )
        {
            snprintf(lab, sizeof(lab), "  %s", au_map[top[i]].map);
            snprintf(buf, sizeof(buf), "%u", au_map[top[i]].plays);
            AU_Row( lx, y, lw, lab, buf );  y += pitch;
        }
    }
}


//===========================================================================
//  Startup
//===========================================================================

void AU_Init( void )
{
    cat_filename( au_file, legacyhome, "audit.dat" );

    AU_Load();

    // First ever run: stamp the date so the page can say what the numbers are
    // "since".  Only here, never on load, or every launch would reset it.
    if( ! au_since[0] )
    {
        time_t     now = time(NULL);
        struct tm * lt = localtime(&now);
        if( lt )
            strftime( au_since, sizeof(au_since), "%Y-%m-%d", lt );
    }

    au_boots++;

    COM_AddCommand("audit",      Command_Audit_f,      CC_command);
    COM_AddCommand("clearaudit", Command_ClearAudit_f, CC_command);

    // Written straight away rather than only at shutdown.  A cabinet is far
    // likelier to be switched off at the wall than quit cleanly, and a file
    // that only ever lands on a clean exit would lose most of what it counted.
    AU_Save();
}
