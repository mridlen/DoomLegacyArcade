// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: b_game.c 1699 2024-11-27 07:20:27Z wesleyjohnson $
//
// Copyright (C) 2002 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
//
// $Log: b_game.c,v $
// Revision 1.5  2004/07/27 08:19:34  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.4  2003/06/11 04:04:50  ssntails
// Rellik's Bot Code!
//
// Revision 1.3  2002/09/28 06:53:11  tonyd
// fixed CR problem, fixed game options crash
//
// Revision 1.2  2002/09/27 16:40:07  tonyd
// First commit of acbot
//
//-----------------------------------------------------------------------------

// Bot include
#include "b_bot.h"
#include "b_game.h"
#include "b_look.h"
#include "b_node.h"

// Doom include
#include "doomincl.h"
#include "doomstat.h"
//#include "r_defs.h"
#include "m_random.h"
#include "p_local.h"
#include "z_zone.h"

#include "command.h"
#include "r_state.h"
#include "v_video.h"
#include "m_argv.h"
#include "p_setup.h"
#include "r_main.h"
#include "r_things.h"
#include "g_game.h"
#include "d_net.h"
#include "byteptr.h"

//#define DEBUG_COME_HERE
//#define DEBUG_BOT_DROP

// [WDJ] New Bot code is not compatible with old versions, is not old demo compatible.
// #define BOT_VERSION_DETECT

// Persistant random number, that changes after each use.  Only used to initialize.
// Only CV_NETVAR in case someone uses the B_Gen_Random outside of initializing names.
consvar_t  cv_bot_random = { "botrandom", "1333", CV_NETVAR | CV_SAVE, CV_Unsigned };

// User set random seed.  Only used to initialize.
// Only CV_NETVAR in case someone uses the B_Gen_Random outside of initializing names.
static void CV_botrandom_OnChange( void );
consvar_t  cv_bot_randseed = { "botrandseed", "0", CV_NETVAR | CV_SAVE | CV_CALL, CV_Unsigned, CV_botrandom_OnChange };

CV_PossibleValue_t botgen_cons_t[]={ {0,"Plain"}, {1,"Seed"}, {2,"Seed Random"}, {3,"Cfg Random"}, {4,"Sys Random"}, {0,NULL}};
consvar_t  cv_bot_gen = { "botgen", "0", CV_NETVAR | CV_SAVE | CV_CALL, botgen_cons_t, CV_botrandom_OnChange };

CV_PossibleValue_t botskin_cons_t[]={ {0,"Color"}, {1,"Skin"}, {0,NULL}};
consvar_t  cv_bot_skin = { "botskin", "0", CV_NETVAR | CV_SAVE, botskin_cons_t };

CV_PossibleValue_t botrespawn_cons_t[]={
  {5,"MIN"},
  {255,"MAX"},
  {0,NULL}};
consvar_t  cv_bot_respawn_time = { "botrespawntime", "8", CV_NETVAR | CV_SAVE, botrespawn_cons_t };

CV_PossibleValue_t botskill_cons_t[]={
  {0,"crippled"},
  {1,"baby"},
  {2,"easy"},
  {3,"medium"},
  {4,"hard"},
  {5,"nightmare"},
  {6,"randmed"},
  {7,"randgame"},
  {8,"gamemed"},
  {9,"gameskill"},
  {0,NULL}};
consvar_t  cv_bot_skill = { "botskill", "gamemed", CV_NETVAR | CV_SAVE, botskill_cons_t };

static void CV_botspeed_OnChange( void );

CV_PossibleValue_t botspeed_cons_t[]={
  {0,"walk"},
  {1,"trainer"},
  {2,"slow"},
  {3,"medium"},
  {4,"fast"},
  {5,"run"},
  {8,"gamemed"},
  {9,"gameskill"},
  {10,"botskill"},
  {0,NULL}};
consvar_t  cv_bot_speed = { "botspeed", "botskill", CV_NETVAR | CV_SAVE | CV_CALL, botspeed_cons_t, CV_botspeed_OnChange };

CV_PossibleValue_t botgrab_cons_t[]={
  {0,"low"},     // cannot call it min
  {1,"moderate"},
  {2,"friend"},  // friend rules
  {3,"grabby"},  // previous non-deathmatch, no friends
  {4,"selfish"}, // previous behavior for comp
  {0,NULL}};
consvar_t  cv_bot_grab =  { "botgrab", "moderate", CV_NETVAR | CV_SAVE, botgrab_cons_t };



// [WDJ] Tables just happened to be this way for now, they may change later.
static byte bot_botskill_to_speed[ 6 ] = { 0, 1, 2, 3, 4, 5 };
static byte bot_gameskill_to_speed[ 5 ] = { 1, 2, 3, 4, 5 };  // lowest value must be >= 1 (see gamemed)
static byte bot_speed_frac_table[ 6 ] = { 110, 90, 102, 112, 122, 128 };  // 128=full
static uint32_t bot_run_tics_table[ 6 ] = { TICRATE/4, 4*TICRATE, 6*TICRATE, 12*TICRATE, 24*TICRATE, 128*TICRATE };  // tics
static byte bot_speed_frac;
static byte bot_run_tics;

// A function of gameskill and cv_bot_speed.
// Must be called when either changes.
static void CV_botspeed_OnChange( void )
{
    byte bot_speed_index;
    switch( cv_bot_speed.EV )
    {
      case 8:  // gamemed
        // One step slower than gameskill.
        bot_speed_index = bot_gameskill_to_speed[ gameskill ] - 1;
        break;
      case 10: // botskill (temp value, deferred determination)
      case 9:  // gameskill
        bot_speed_index = bot_gameskill_to_speed[ gameskill ];
        break;
      default:
        bot_speed_index = cv_bot_speed.EV;  // 0..5
        break;
    }
#ifdef BOT_VERSION_DETECT
    if( demoversion < 148 )  bot_speed_index = 5; // always run
#endif

    bot_speed_frac = bot_speed_frac_table[ bot_speed_index ];
    bot_run_tics = bot_run_tics_table[ bot_speed_index ];
}


#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

boolean B_FindNextNode(player_t* p);

bot_info_t  botinfo[MAXPLAYERS];
fixed_t botforwardmove[2] = {25/NEWTICRATERATIO, 50/NEWTICRATERATIO};
fixed_t botsidemove[2]    = {24/NEWTICRATERATIO, 40/NEWTICRATERATIO};
angle_t botangleturn[4]   = {500, 1000, 2000, 4000};

extern consvar_t cv_skill;
extern thinker_t thinkercap;
extern mobj_t*	tm_thing;

// Player name that is seen for each bot.
#define NUM_BOT_NAMES 40
char* botnames[NUM_BOT_NAMES] = {
  "Frag-God",
  "TF-Master",
  "FragMaster",
  "Thresh",
  "Reptile",
  "Archer",
  "Freak",
  "Reeker",
  "Dranger",
  "Enrage",
  "Franciot",
  "Grimknot",
  "Fodder",
  "Rumble",
  "Crusher",
  "Crash",
  "Krunch",
  "Wreaker",
  "Punisher",
  "Quaker",
  "Reaper",
  "Slasher",
  "Tormenot",
  "Drat",
  "Labrat",
  "Nestor",
  "Akira",
  "Meikot",
  "Aliciot",
  "Leonardot",
  "Ralphat",
  "Xoleras",
  "Zetat",
  "Carmack",  // Hon
  "Romero",  // Hon
  "Hurdlerbot",  // Team member
  "Meisterbot",  // Team member
  "Borisbot",  // Team member
  "Tailsbot",  // Team member
  "TonyD-bot", // Team member
};

int botcolors[NUMSKINCOLORS] = 
{
   0, // = Green
   1, // = Indigo
   2, // = Blue
   3, // = Deep Red
   4, // = White
   5, // = Bright Brown
   6, // = Red
   7, // = Blue
   8, // = Dark Blue
   9, // = Yellow
   10 // = Bleached Bone
};

// By Server, for bots.
//  skinrand : the botinfo skinrand value
static
char * bot_skinname( int skinrand )
{
    if( cv_bot_skin.EV && (numskins > 1) )
    {
        skinrand = (skinrand % (numskins-1)) + 1;
    }
    else
    {
        skinrand = 0; // marine
    }
    return skins[skinrand]->name;
}


// Random number source for bot generation.
uint32_t  B_Gen_Random( void )
{
    uint32_t r;
    switch( cv_bot_gen.EV )
    {
     case 0:  // Plain
        r = B_Random();
        break;
     default:
     case 1:  // Seed
        r = B_Random() + cv_bot_randseed.value;
        break;
     case 2:  // Seed Random
        r = E_Random() + cv_bot_randseed.value;
        break;
     case 3:  // Cfg Random
        r = E_Random() + cv_bot_random.value;
        break;
     case 4:  // Sys Random
        r = rand();
        break;
    }
    return r;
}

void B_Init_Names()
{
    int br, i, j;
    uint16_t color_used = 0;
    byte   botindex = B_Rand_GetIndex();

    CV_ValueIncDec( &cv_bot_random, 7237 ); // add a prime

    // Initialize botinfo.
    for (i=0; i< MAXPLAYERS; i++)
    {
        // Give each prospective bot a unique name.
        do
        {
            br = B_Gen_Random() % NUM_BOT_NAMES;
            for( j = 0; j < i; j++ )
            {
                if( botinfo[j].name_index == br )  break;
            }
        } while ( j < i );  // when j==i then did not find any duplicates
        botinfo[i].name_index = br;

        // Assign a skin color.  Make them unique until have used all colors.
        j = NUMSKINCOLORS;
        br = B_Gen_Random();
        for(;;)
        {
            br = br % NUMSKINCOLORS;
            if( ((1<<br) & color_used) == 0 )  break;
            br++;
            if( --j < 0 )  color_used = 0;
        }
        botinfo[i].colour = br;
        color_used |= (1<<br);

        botinfo[i].skinrand = B_Gen_Random();
    }
   
    // Restore to keep reproducible bot gen in spite of multiple calls during setup,
    // and it minimizes effects of network races.
    if( cv_bot_gen.EV < 2 )
        B_Rand_SetIndex( botindex );

    // Existing bots keep their existing names and colors.
    // This only changes the tables used to create new bots, by the server.
    // Off-server clients will keep the original bot names that the server sent them.
}


static byte bot_init_done = 0;

static void CV_botrandom_OnChange( void )
{
#ifdef BOT_VERSION_DETECT
    if( demoversion < 148 )  return;
#endif

    // With so many NETVAR update paths, only let the server do this.
    // The bot names will be passed with the NetXCmd, so clients do not need this.
    // The random number generatators will be updated.
    if( ! server )
        return;

    // [WDJ] Updating the random number generators in the middle of a game, ugh.
    if( netgame )
        SV_network_wait_timer( 35 );  // pause everybody
    
    if( cv_bot_randseed.state & CS_MODIFIED )
        B_Rand_SetIndex( cv_bot_randseed.value );
   
    // Only change names after initial loading of config.
    if( bot_init_done )
        B_Init_Names();
   
    // Let network un-pause naturally.  Better timing.
}


void DemoAdapt_bots( void )
{
    CV_botspeed_OnChange();
}


// may be called multiple times
void B_Init_Bots()
{  
    DemoAdapt_bots();

    B_Init_Names();

    botNodeArray = NULL;

    bot_init_done = 1;
}

//
// bot commands
//

// By Server
void B_Send_bot_NameColor( byte pn )
{
    // From botinfo sources
    bot_info_t * bip = & botinfo[pn];
    Send_NameColor_pn( pn, botnames[bip->name_index], bip->colour, bot_skinname(bip->skinrand), 2 );  // server channel
}

// By Server.
// Send name, color, skin of all bots.
void B_Send_all_bots_NameColor( void )
{
    byte pn;

    for( pn=0; pn<MAXPLAYERS; pn++ )
    {
        if( playeringame[pn] && players[pn].bot )
        {
            B_Send_bot_NameColor( pn );
        }
    }
}


void Command_AddBot(void)
{
    byte buf[10];
    byte pn = 0;

    if( !server )
    {
        CONS_Printf("Only the server can add a bot\n");
        return;
    }

    pn = SV_get_player_num();

    if( pn >= MAXPLAYERS )
    {
        CONS_Printf ("You can only have %d players.\n", MAXPLAYERS);
        return;
    }

    bot_info_t * bip = & botinfo[pn];
    byte * b = &buf[0];
    // AddBot format: pn, color, name:string0
    WRITEBYTE( b, pn );
    WRITEBYTE( b, bip->colour );
    b = write_stringn(b, botnames[bip->name_index], MAXPLAYERNAME);
    SV_Send_NetXCmd(XD_ADDBOT, buf, (b - buf));  // as server

    // Cannot send NameColor XCmd before the bot exists.
}

// Only call after console player and splitscreen players have grabbed their player slots.
void B_Regulate_Bots( int req_numbots )
{
    byte pn;
    for( pn = 0; pn < MAXPLAYERS; pn++ )
    {
        if( playeringame[pn] && players[pn].bot )  req_numbots--;
    }

    if( req_numbots > (MAXPLAYERS - num_game_players) )
        req_numbots = (MAXPLAYERS - num_game_players);

    while( req_numbots > 0 )
    {
        Command_AddBot();
        req_numbots --;
    }
}

void B_Register_Commands( void )
{
    COM_AddCommand ("addbot", Command_AddBot, CC_command);
}

static
void B_AvoidMissile(player_t* p, mobj_t* missile)
{
    angle_t  missile_angle = R_PointToAngle2 (p->mo->x, p->mo->y, missile->x, missile->y);
    signed_angle_t  delta = p->mo->angle - missile_angle;

    if( delta >= 0)
        p->cmd.sidemove = -botsidemove[1];
    else if( delta < 0)
        p->cmd.sidemove = botsidemove[1];
}

static
void B_ChangeWeapon (player_t* p)
{
    bot_t * pbot = p->bot;
    boolean  usable_weapon[NUMWEAPONS]; // weapons with ammo
    byte  num_weapons = 0;
    byte  weaponChance;
    byte  i;

    for (i=0; i<NUMWEAPONS; i++)
    {
        byte hw = false;
        switch (i)
        {
         case wp_fist:
            hw = false;//true;
            break;
         case wp_pistol:
            hw = p->ammo[am_clip];
            break;
         case wp_shotgun:
            hw = (p->weaponowned[i] && p->ammo[am_shell]);
            break;
         case wp_chaingun:
            hw = (p->weaponowned[i] && p->ammo[am_clip]);
            break;
         case wp_missile:
            hw = (p->weaponowned[i] && p->ammo[am_misl]);
            break;
         case wp_plasma:
            hw = (p->weaponowned[i] && p->ammo[am_cell]);
            break;
         case wp_bfg:
            hw = (p->weaponowned[i] && (p->ammo[am_cell] >= 40));
            break;
         case wp_chainsaw:
            hw = p->weaponowned[i];
            break;
         case wp_supershotgun:
            hw = (p->weaponowned[i] && (p->ammo[am_shell] >= 2));
        }
        usable_weapon[i] = hw;
        if( hw ) // || ((i == wp_fist) && p->powers[pw_strength]))
            num_weapons++;
    }

    //or I have just picked up a new weapon
    if( !pbot->weaponchangetimer || !usable_weapon[p->readyweapon]
        || (num_weapons != pbot->lastNumWeapons))
    {
        if( (usable_weapon[wp_shotgun] && (p->readyweapon != wp_shotgun))
            || (usable_weapon[wp_chaingun] && (p->readyweapon != wp_chaingun))
            || (usable_weapon[wp_missile] && (p->readyweapon != wp_missile))
            || (usable_weapon[wp_plasma] && (p->readyweapon != wp_plasma))
            || (usable_weapon[wp_bfg] && (p->readyweapon != wp_bfg))
            || (usable_weapon[wp_supershotgun] && (p->readyweapon != wp_supershotgun)))
        {
            p->cmd.buttons &= ~BT_ATTACK;	//stop rocket from jamming;
            do
            {
                weaponChance = B_Random();
                if( (weaponChance < 30) && usable_weapon[wp_shotgun]
                     && (p->readyweapon != wp_shotgun))//has shotgun and shells
                    p->cmd.buttons |= (BT_CHANGE | (wp_shotgun<<BT_WEAPONSHIFT));
                else if( (weaponChance < 80) && usable_weapon[wp_chaingun]
                     && (p->readyweapon != wp_chaingun))//has chaingun and bullets
                    p->cmd.buttons |= (BT_CHANGE | (wp_chaingun<<BT_WEAPONSHIFT));
                else if( (weaponChance < 130) && usable_weapon[wp_missile]
                     && (p->readyweapon != wp_missile))//has rlauncher and rocket
                    p->cmd.buttons |= (BT_CHANGE | (wp_missile<<BT_WEAPONSHIFT));
                else if( (weaponChance < 180) && usable_weapon[wp_plasma]
                     && (p->readyweapon != wp_plasma))//has plasma and cells
                    p->cmd.buttons |= (BT_CHANGE | (wp_plasma<<BT_WEAPONSHIFT));
                else if( (weaponChance < 200) && usable_weapon[wp_bfg]
                     && (p->readyweapon != wp_bfg))//has bfg and cells
                    p->cmd.buttons |= (BT_CHANGE | (wp_bfg<<BT_WEAPONSHIFT));
                else if( usable_weapon[wp_supershotgun]
                     && (p->readyweapon != wp_supershotgun))
                    p->cmd.buttons |= (BT_CHANGE | BT_EXTRAWEAPON | (wp_shotgun<<BT_WEAPONSHIFT));
            } while (!(p->cmd.buttons & BT_CHANGE));
        }
        else if( usable_weapon[wp_pistol]
                 && (p->readyweapon != wp_pistol))//has pistol and bullets
            p->cmd.buttons |= (BT_CHANGE | wp_pistol<<BT_WEAPONSHIFT);
        else if( p->weaponowned[wp_chainsaw] && !p->powers[pw_strength]
                 && (p->readyweapon != wp_chainsaw))//has chainsaw, and not powered
            p->cmd.buttons |= (BT_CHANGE | BT_EXTRAWEAPON | (wp_fist<<BT_WEAPONSHIFT));
        else	//resort to fists, if have powered fists, better with fists then chainsaw
            p->cmd.buttons |= (BT_CHANGE | wp_fist<<BT_WEAPONSHIFT);

        pbot->weaponchangetimer = (B_Random()<<7)+10000;	//how long until I next change my weapon
    }
    else if( pbot->weaponchangetimer)
        pbot->weaponchangetimer--;

    if( num_weapons != pbot->lastNumWeapons)
        p->cmd.buttons &= ~BT_ATTACK;	//stop rocket from jamming;
    pbot->lastNumWeapons = num_weapons;

    //debug_Printf("pbot->weaponchangetimer is %d\n", pbot->weaponchangetimer);
}

#define ANG5 (ANG90/18)

// returns the difference between the angle mobj is facing,
// and the angle from mo to x,y

#if 0
// Not Used
static
signed_angle_t  B_AngleDiff(mobj_t* mo, fixed_t x, fixed_t y)
{
    return ((R_PointToAngle2 (mo->x, mo->y, x, y)) - mo->angle);
}
#endif

static
void B_TurnTowardsPoint(player_t* p, fixed_t x, fixed_t y)
{
    angle_t  angle = R_PointToAngle2(p->mo->x, p->mo->y, x, y);
    signed_angle_t  delta =  angle - p->mo->angle;
    angle_t  abs_delta = abs(delta);

    if( abs_delta < ANG5 )
    {
        p->cmd.angleturn = angle>>FRACBITS;	//perfect aim
    }
    else
    {
        angle_t  turnspeed = botangleturn[ (abs_delta < (ANG45>>2))? 0 : 1 ];

        if( delta > 0)
            p->cmd.angleturn += turnspeed;
        else
            p->cmd.angleturn -= turnspeed;
    }
}

// Turn away from a danger, or friend.
static
void B_Turn_Away_Point(player_t* p, fixed_t x, fixed_t y)
{
    signed_angle_t  delta = R_PointToAngle2(p->mo->x, p->mo->y, x, y) - p->mo->angle;
    angle_t  abs_delta = abs(delta);

    if( abs_delta < (ANG45*3) )
    {
        angle_t  turnspeed = botangleturn[ (abs_delta < ANG45)? 1 : 0 ];

        // No perfect aim
        if( delta > 0 )
            p->cmd.angleturn -= turnspeed;
        else
            p->cmd.angleturn += turnspeed;
    }
}

static
void B_AimWeapon(player_t* p)
{
    bot_t  * pbot = p->bot;
    mobj_t * source = p->mo;
    mobj_t * dest = pbot->closestEnemy;

    int  botspeed = 0;
    int  mtime, t;
    angle_t angle, perfect_angle;
    signed_angle_t  delta;
    angle_t  abs_delta;
    byte botskill = pbot->skill;

    fixed_t  px, py, pz;
    fixed_t  weapon_range;
    fixed_t  dist, missile_speed;
    subsector_t	*sec;
    boolean  canHit;

    missile_speed = 0;  // default
    switch (p->readyweapon)	// changed so bot projectiles don't lead targets at lower skills
    {
     case wp_fist: case wp_chainsaw:			//must be close to hit with these
        missile_speed = 0;
        weapon_range = FIXINT(20);
        break;
     case wp_pistol: case wp_shotgun: case wp_chaingun:	//instant hit weapons, aim directly at enemy
        missile_speed = 0;
        weapon_range = FIXINT(512);
        break;
     case wp_missile:
        missile_speed = mobjinfo[MT_ROCKET].speed;
        weapon_range = FIXINT(1024);
        break;
     case wp_plasma:
        missile_speed = mobjinfo[MT_PLASMA].speed;
        weapon_range = FIXINT(1024);
        break;
     case wp_bfg:
        missile_speed = mobjinfo[MT_BFG].speed;
        weapon_range = FIXINT(1024);
        break;
     default:
        missile_speed = 0;
        weapon_range = FIXINT(4096);
        break;
    }

    // botskill 0 to 5
    if( botskill < 2 )
    {
        missile_speed = 0;  // no aim prediction
    }
    else if( botskill == 3 )
    {
        missile_speed *= 3;  // throw off aiming
    }
    else if( botskill == 4 )
    {
        missile_speed *= 2;  // throw off aiming
    }

    dist = P_AproxDistance (dest->x - source->x, dest->y - source->y);

    if( (dest->type == MT_BARREL) || (dest->type == MT_POD) || (dest->flags & MF_TOUCHY) )
    {
        // [WDJ] Do not attack exploding things with fists.
        if( weapon_range < FIXINT(100))  goto reject_enemy;
        if( dist < FIXINT(200))  goto reject_enemy;  // too close, must get distance first.
    }

    if( dist > weapon_range )
    {
        if( B_Random() < 240 )  return;  // wait till in range, most of the time
    }
   
    if( (p->readyweapon != wp_missile) || (dist > FIXINT(100)))
    {
        if( missile_speed)
        {
            mtime = dist/missile_speed;
            mtime = P_AproxDistance ( dest->x + dest->momx*mtime - source->x,
                                      dest->y + dest->momy*mtime - source->y)
                            / missile_speed;

            t = mtime + 4;
            do
            {
                t-=4;
                if( t < 0)
                    t = 0;
                px = dest->x + dest->momx*t;
                py = dest->y + dest->momy*t;
                pz = dest->z + dest->momz*t;
                canHit = P_CheckSight2(source, dest, px, py, pz);
            } while (!canHit && (t > 0));

            sec = R_PointInSubsector(px, py);
            if( !sec)
                sec = dest->subsector;

            if( pz < sec->sector->floorheight)
                pz = sec->sector->floorheight;
            else if( pz > sec->sector->ceilingheight)
                pz = sec->sector->ceilingheight - dest->height;
        }
        else
        {
            px = dest->x;
            py = dest->y;
            pz = dest->z;
        }

        perfect_angle = angle = R_PointToAngle2 (source->x, source->y, px, py);
        p->cmd.aiming = FIXED_TO_INT((int)((atan ((pz - source->z + (dest->height - source->height)/2) / (double)dist)) * ANG180/M_PI));

        // Random aiming imperfections.
        if( P_AproxDistance(dest->momx, dest->momy) > FIXINT(8) )	//enemy is moving reasonably fast, so not perfectly acurate
        {
            if( dest->flags & MF_SHADOW)
                angle += P_SignedRandom()<<23;
            else if( missile_speed == 0 )
                angle += P_SignedRandom()<<22;
        }
        else
        {
            if( dest->flags & MF_SHADOW)
                angle += P_SignedRandom()<<22;
            else if( missile_speed == 0 )
                angle += P_SignedRandom()<<21;
        }

        delta = angle - source->angle;
        abs_delta = abs(delta);

        if( abs_delta < ANG45 )
        {
            // Fire weapon when aim is best.
            if( abs_delta <= ANG5 )
            {
                // cmd.angleturn is 16 bit angle (angle is not fixed_t)
                // lower skill levels have imperfect aim
                p->cmd.angleturn = (( botskill < 4 )?  // < hard
                      angle  // not so perfect aim
                    : perfect_angle // perfect aim
                    ) >>16;  // 32 bit to 16 bit angle
                p->cmd.buttons |= BT_ATTACK;
                return;
            }

            // Fire some weapons when aim is just close.
            if( (p->readyweapon == wp_chaingun) || (p->readyweapon == wp_plasma)
                || (p->readyweapon == wp_pistol))
                 p->cmd.buttons |= BT_ATTACK;
        }

        // Still turning to face target.
        botspeed = ( abs_delta < (ANG45>>1) )? 0
           : ( abs_delta < ANG45 )? 1
           : 3;

        if( delta > 0)
            p->cmd.angleturn += botangleturn[botspeed];	//turn right
        else if( delta < 0)
            p->cmd.angleturn -= botangleturn[botspeed]; //turn left
    }
    return;

reject_enemy:
    pbot->closestEnemy = NULL;
    return;
} 


// Bot recognize damaging sectors.
typedef struct {
    byte sector_type;
    byte sector_damage;
} sector_damage_entry_t;

// Seaches are limited to part of the table, with count.
sector_damage_entry_t  sector_damage_table[] =
{
    // sector type,  damage
// Doom
    { 5, 10 }, // HELLSLIME DAMAGE
    { 7, 5 },  // NUKAGE DAMAGE
    { 11, 20 },  // EXIT SUPER DAMAGE! (for E1M8 finale)
    { 16, 20 },  // SUPER HELLSLIME DAMAGE
// Heretic
    { 4, 5 },  // Scroll_EastLavaDamage
    { 5, 5 },  // Damage_LavaWimpy
    { 7, 4 },  // Damage_Sludge
    { 16, 8 }, // Damage_LavaHefty
// Boom
    { 1, 5 }, // 2/5 damage per 31 ticks
    { 2, 10 }, // 5/10 damage per 31 ticks
    { 3, 20 }, // 10/20 damage per 31 ticks
};

// Check if want to get into this sector.
//  bp: bot player
byte bot_sector_damage( sector_t * sec, player_t* bp )
{
    if( ! sec->special )
        return 0;

    byte ironfeet = (bp->powers[pw_ironfeet] > 0);
    byte invul = (bp->powers[pw_invulnerability] > 0);

    uint16_t spec16 = sec->special;
    byte spec = 0;
    byte indx = 0;
    byte indx_cnt = 4;
#ifdef MBF21
    if( spec16 & MBF21_PLAYER_DEATH_SECTOR )  // Detect Alternate Damage Mode
    {
        uint16_t mbf21_death_code = (spec16 & DAMAGE_MASK) >> DAMAGE_SHIFT;
        if( mbf21_death_code == 0 )
        {
            // Kills the player, unless they have rad suit, or invulnerability.
            return ( invul | ironfeet )? 40 : 200;
        }
        if( mbf21_death_code == 1 )
            return 200;  // instant death 
        return 0; // exit
    }
#endif
    if( ! (ironfeet | invul) )
    {
        if( spec16 < 32 )
        {
            spec = spec16;
            indx = ( EN_heretic )? 4 : 0;
            indx_cnt = 4;
            goto lookup_dam;
        }
        // Boom generalized sector
        spec = ((spec16&DAMAGE_MASK)>>DAMAGE_SHIFT);
        if( spec < 4 )
        {
            indx = 7;
            indx_cnt = 3;
            goto lookup_dam;
        }
    }
    return 0;

lookup_dam:
    // Lookup the damage in the table.
    while( indx_cnt-- )
    {
        if( sector_damage_table[indx].sector_type == spec )
            return sector_damage_table[indx].sector_damage;
        indx++;
    }
    return 0;
}


//
// MAIN BOT AI
//

static fixed_t bot_strafe_dist[6] = {
   FIXINT(20),  // crippled
   FIXINT(32),  // baby
   FIXINT(150), // easy
   FIXINT(150), // medium
   FIXINT(350), // hard
   FIXINT(650)  // nightmare
};

//  p : a bot player
void B_BuildTiccmd(player_t* p, ticcmd_t* netcmd)
{
    mobj_t * pmo = p->mo;
    bot_t  * pbot = p->bot;
    ticcmd_t*  cmd = &p->cmd;

    int  x, y;
    fixed_t  px, py;  //coord of where I will be next tick
    fixed_t  forwardmove = 0, sidemove = 0;
    int      forward_angf, side_angf;
    fixed_t  target_dist;  //how far away is my enemy, wanted thing
    int slow_down_tics = 0;
    byte new_action = 0;
    byte what = WH_none;
    mobj_t * avoid_mobj = (pbot->flags & BF_avoid)? pbot->lastMobj : NULL;  // avoid it

#ifdef BOT_VERSION_DETECT
    if(demoversion < 148)
    {
        avoid_mobj = NULL;
    }
#endif

    memset (cmd,0,sizeof(*cmd));

    // Exit now if locked
    if( p->GB_flags & GB_locked )
        return;

    if( (p->playerstate == PST_LIVE) && pmo )
    {
        byte  botspeed = 1;
        byte  has_friend = (pmo->flags & MF_FRIEND) && (cv_bot_grab.EV <= 2);
        byte  blocked = 0;
        boolean  button_not_used = true;

        // needed so bot doesn't hold down use before reaching switch object
        if( cmd->buttons & BT_USE )
            button_not_used = false;    //wouldn't be able to use switch

        cmd->angleturn = pmo->angle>>16;  // 32 bit angle to 16 bit angle
        cmd->aiming = 0;//p->aiming>>16;

        if( pbot->turn_timer )
        {
            pbot->turn_timer --;
        }
        else
        {
            B_LookForThings(p);
            new_action = 1;
        }
        B_ChangeWeapon(p);

        if( pbot->avoidtimer )
        {
            pbot->avoidtimer--;
            what = WH_avoid;
            if( pmo->eflags & MF_UNDERWATER)
            {
                forwardmove = botforwardmove[1];
                cmd->buttons |= BT_JUMP;
            }
            else
            {
                // Default movement
                // netcmd does not hold the previous move
                if( pbot->flags & BF_backup )
                    forwardmove = -botforwardmove[1];
                else
                    forwardmove = botforwardmove[1];
                sidemove = botsidemove[1];

#ifdef BOT_VERSION_DETECT
                if( demoversion >= 148 )
#endif
                {
                    // Avoidance
                    if( avoid_mobj )
                    {
                        if( pbot->turn_timer > 10 )
                        {
#ifdef DEBUG_COME_HERE
                            if( pbot->turn_timer == 16 )
                                GenPrintf(EMSG_debug, " AVOIDTIMER, turnaway, what=%x\n", pbot->what );
#endif
                            B_Turn_Away_Point( p, avoid_mobj->x, avoid_mobj->x );
                            forwardmove >>= 1; // move slow
                            sidemove = 0;
                        }
                        if( pbot->turn_timer < 15 )  // stop backing
                        {
                            pbot->flags &= ~BF_backup;
                        }

                        if( pbot->flags & BF_bump )
                        {
#ifdef DEBUG_COME_HERE
                            if( pbot->turn_timer == 16 )
                                GenPrintf(EMSG_debug, " AVOIDTIMER, BF_bump, what=%x\n", pbot->what );
#endif
                            forwardmove >>= 1; // move slow
                            if( pbot->turn_timer == 1 )
                            {
                                pbot->flags &= ~(BF_bump | BF_avoid);
                                pbot->what = 0;
                                if( pbot->blockedcount > 22 )
                                    pbot->blockedcount = 0;  // relieve stuck at blocked 
                            }
                        }
                        else if( pbot->flags & BF_avoid )
                        {
#ifdef DEBUG_COME_HERE
                            if( pbot->turn_timer == 14 )
                                  GenPrintf(EMSG_debug, " AVOIDTIMER, BF_avoid, what=%x\n", pbot->what );
#endif
//                            sidemove = 0;
                        }
                    }
                }
            }
        }
        else
        {
            if( pbot->bestSeenItem
                && !( avoid_mobj && (pbot->bestSeenItem == avoid_mobj))  // avoid it
              )
            {
#ifdef DEBUG_COME_HERE
                if( come_here )
                    GenPrintf(EMSG_debug, " COME_HERE bestSeenItem\n" );
#endif
                // Move towards the item.
                target_dist = P_AproxDistance (pmo->x - pbot->bestSeenItem->x, pmo->y - pbot->bestSeenItem->y);
                botspeed = (target_dist > FIXINT(64))? 1 : 0;
                B_TurnTowardsPoint(p, pbot->bestSeenItem->x, pbot->bestSeenItem->y);
                if( new_action )
                    pbot->turn_timer = 20;  // no decisions until have turned
                forwardmove = botforwardmove[botspeed];
                pbot->flags &= ~BF_bump;

                if( ( (pbot->bestSeenItem->floorz - pmo->z) > FIXINT(24) )
                    && (target_dist <= FIXINT(100)) )
                {
                    if( cv_allowjump.EV )
                        cmd->buttons |= BT_JUMP;
                }

                pbot->bestItem = NULL;
            }
            else if( pbot->closestEnemy && (pbot->closestEnemy->health > 0)
                     && !( avoid_mobj && (pbot->closestEnemy == avoid_mobj)) // avoid it
                   )
            {
                what = WH_enemy;
#ifdef DEBUG_COME_HERE
                if( come_here )
                    GenPrintf(EMSG_debug, " COME_HERE closestEnemy\n" );
#endif
                // Target exists and is still alive.
                // Prepare to attack the enemy.
                player_t * enemyp = pbot->closestEnemy->player;
                weapontype_t  enemy_readyweapon =
                 ( enemyp )? enemyp->readyweapon
                 : wp_nochange; // does not match anything
                boolean  enemy_linescan_weapon =
                   (enemy_readyweapon == wp_pistol)
                   || (enemy_readyweapon == wp_shotgun)
                   || (enemy_readyweapon == wp_chaingun);

                //debug_Printf("heading for an enemy\n");
                target_dist = P_AproxDistance (pmo->x - pbot->closestEnemy->x, pmo->y - pbot->closestEnemy->y);
                if( (target_dist > FIXINT(300))
                    || (p->readyweapon == wp_fist)
                    || (p->readyweapon == wp_chainsaw))
                    forwardmove = botforwardmove[botspeed];
                if( (p->readyweapon == wp_missile) && (target_dist < FIXINT(400)))
                    forwardmove = -botforwardmove[botspeed];

                // bot skill setting determines likelyhood bot will start strafing
                if(( target_dist <= bot_strafe_dist[ pbot->skill ])
                    || ( enemy_linescan_weapon && (pbot->skill >= 3) ))  // >= medium skill
                {
                    sidemove = botsidemove[botspeed];
                }

#ifdef BOT_NOT_REACHABLE
                if( pbot->flags & BF_not_reachable )
                {
                    forwardmove >>= 2;
                }
#endif

                pbot->flags &= ~BF_bump;

                B_AimWeapon(p);

                if( pbot->closestEnemy )
                {
                    pbot->lastMobj = pbot->closestEnemy;
                    pbot->lastMobjX = pbot->closestEnemy->x;
                    pbot->lastMobjY = pbot->closestEnemy->y;
                    pbot->flags &= ~BF_avoid;
                    pbot->what = WH_enemy;
                }
            }
            else
            {
                cmd->aiming = 0;
                //look for an unactivated switch/door
                if( (B_Random() > 190)  // not every time, so it does not obsess
                    && B_LookForSpecialLine(p, &x, &y)
                    && B_ReachablePoint(p, pmo->subsector->sector, x, y))
                {
                    //debug_Printf("found a special line\n");
                    what = WH_line;
                    B_TurnTowardsPoint(p, x, y);
                    if( new_action )
                        pbot->turn_timer = 20;  // no decisions until have turned
                    if( P_AproxDistance (pmo->x - x, pmo->y - y) <= USERANGE)
                    {
                        if( button_not_used )
                            cmd->buttons |= BT_USE;
                    }
                    else
                        forwardmove = botforwardmove[1];
                }
                else if( pbot->teammate )
                {
                    mobj_t * tmate = pbot->teammate;
                    // assume BOTH_FRIEND( p, tmate )
                    target_dist =
                        P_AproxDistance (pmo->x - tmate->x, pmo->y - tmate->y);

                    what = WH_friend;
                    // [WDJ]: Like MBF, Move away from friends when too close, except
                    // in certain situations.
#ifdef DEBUG_COME_HERE
                    if( come_here )
                        GenPrintf(EMSG_debug, " teammate distance = %i\n", target_dist >> 16 );
#endif

#ifdef ENABLE_COME_HERE
                    if( (target_dist < FIXINT(800)) && has_friend )
                    {
                        has_friend = 2;
                    }
#endif		    

                    if( target_dist < EV_mbf_distfriend )
                    {
                        botspeed = 0;  // low speed
                        slow_down_tics = 10*TICRATE; // forced slow time
                        forwardmove = 12/NEWTICRATERATIO; // move slow
#ifdef ENABLE_COME_HERE
                        if( come_here )
                        {
                            forwardmove = 9/NEWTICRATERATIO; // don't move
                            slow_down_tics = 40*TICRATE; // forced slow time
                        }
#endif
                        if( target_dist < (EV_mbf_distfriend<<1) )  // almost near friend
                        {
                            forwardmove = (forwardmove>>1) + 3; // move slower
#ifdef DEBUG_COME_HERE
                            GenPrintf( EMSG_debug, "target_dist=%i, thresh=%i\n", target_dist, (pmo->info->radius + tmate->info->radius + (FRACUNIT*23/8) ) );
#endif
                            // Allowed to bump bot away, even in crusher.
                            if( target_dist <= (pmo->info->radius + tmate->info->radius + FIXINT(12) ) )  // bump
                            {
#ifdef DEBUG_COME_HERE
                                GenPrintf( EMSG_debug, "BUMP: target_dist=%i\n", target_dist );
#endif
                                pbot->flags |= BF_bump;
                                pbot->flags &= ~BF_avoid;
                                pbot->avoidtimer = 0;
                                pbot->turn_timer = 0;
                                if( pbot->blockedcount > 22 )
                                    pbot->blockedcount = 0;  // relieve stuck at blocked
                                forwardmove = 8/NEWTICRATERATIO; // move slow
                                if( P_IsOnLift( tmate ) )
                                {
                                    sidemove = 4/NEWTICRATERATIO; // move slow
#ifdef ENABLE_COME_HERE
                                    if( ! come_here )
#else
                                    if( gametic & 0x80 )  // alternate
#endif
                                    {
                                        B_Turn_Away_Point(p, tmate->x, tmate->y);
                                    }
                                }
                                else if( !P_IsUnderDamage( pmo ) )
                                {
                                    B_Turn_Away_Point(p, tmate->x, tmate->y);
                                    forwardmove = botforwardmove[0];
                                }
#ifdef ENABLE_COME_HERE
                                else if( come_here )
                                {
                                    forwardmove = 4/NEWTICRATERATIO; // move slow
                                }
#endif
                            }
                        }
                    }
                    else if( target_dist > (EV_mbf_distfriend + FIXINT(8)) )
                    {
                        B_TurnTowardsPoint(p, tmate->x, tmate->y);
                        if( new_action )
                            pbot->turn_timer = 20;  // no decisions until have turned
                        forwardmove = botforwardmove[botspeed];
                        pbot->flags &= ~BF_bump;
                    }

                    pbot->lastMobj = tmate;
                    pbot->lastMobjX = tmate->x;
                    pbot->lastMobjY = tmate->y;
                    pbot->flags &= ~(BF_backup | BF_avoid);
                    pbot->what = WH_friend;
                }
                //since nothing else to do, go where last enemy/teammate was seen
                else if( pbot->lastMobj && (pbot->lastMobj->health > 0)
                         && !(pbot->flags & BF_avoid) )
                  // && B_ReachablePoint(p, R_PointInSubsector(pbot->lastMobjX, pbot->lastMobjY)->sector, pbot->lastMobjX, pbot->lastMobjY))
                {
                    what = pbot->what;
                    if( (pmo->momx == 0 && pmo->momy == 0)
                        || !B_NodeReachable(NULL, pmo->x, pmo->y, pbot->lastMobjX, pbot->lastMobjY))
                        pbot->lastMobj = NULL;	//just went through teleporter
                    else
                    {
                        //debug_Printf("heading towards last mobj\n");
                        B_TurnTowardsPoint(p, pbot->lastMobjX, pbot->lastMobjY);
                        if( new_action )
                            pbot->turn_timer = 20;  // no decisions until have turned
                        forwardmove = botforwardmove[botspeed];
                    }
                }
                else
                {
                    fixed_t sx, sy;		    
                    byte br = B_Random();
                    pbot->lastMobj = NULL;
                    pbot->flags &= ~BF_avoid;

                    if( pbot->bestItem && (br & 0x01) )  // do not obsess if cannot get to it
                    {
                        sx = pbot->bestItem->x;
                        sy = pbot->bestItem->y;
                        //debug_Printf("found a best item at x:%d, y:%d\n", FIXED_TO_INT(sx), FIXED_TO_INT(sy) );
                        what = WH_item;
                        goto start_node_walk;
                    }
                    else if( pbot->closestUnseenTeammate && (br & 0x02) )  // do not obsess if cannot get to it
                    {
                        sx = pbot->closestUnseenTeammate->x;
                        sy = pbot->closestUnseenTeammate->y;
                        what = WH_friend | WH_unseen;
                        goto start_node_walk;
                    }
                    else if( pbot->closestUnseenEnemy && (br & 0x04) )  // do not obsess if cannot get to it
                    {
                        sx = pbot->closestUnseenEnemy->x;
                        sy = pbot->closestUnseenEnemy->y;
                        what = WH_enemy | WH_unseen;
start_node_walk:
                      {
                        SearchNode_t* sn1 = B_GetNodeAt( sx, sy );
                        if( pbot->destNode != sn1 )
                            B_LLClear(pbot->path);
                        pbot->destNode = sn1;
                        pbot->flags &= ~BF_bump;
                      }
                    }
                    else
                    {
                        pbot->destNode = NULL;
                    }

                    if( pbot->destNode )
                    {
                        if( !B_LLIsEmpty(pbot->path)
                            && P_AproxDistance(pmo->x - posX2x(pbot->path->first->x), pmo->y - posY2y(pbot->path->first->y)) < (BOTNODEGRIDSIZE<<1))//BOTNODEGRIDSIZE>>1))
                        {
#ifdef SHOWBOTPATH
                            SearchNode_t* sn2 = B_LLRemoveFirstNode(pbot->path);  // not empty, so should be one
                            P_RemoveMobj(sn2->mo);
                            Z_Free(sn2);
#else
                            Z_Free( B_LLRemoveFirstNode(pbot->path) );
#endif
                        }


                        //debug_Printf("at x%d, y%d\n", FIXED_TO_INT(pbot->wantedItemNode->x), FIXED_TO_INT(pbot->wantedItemNode->y));
                        if( B_LLIsEmpty(pbot->path)
                            || !B_NodeReachable(NULL, pmo->x, pmo->y,
                                                posX2x(pbot->path->first->x), posY2y(pbot->path->first->y) )
                               // > (BOTNODEGRIDSIZE<<2))
                            )
                        {
                            if( !B_FindNextNode(p))	//search for next node
                            {
                                //debug_Printf("Bot stuck at x:%d y:%d could not find a path to x:%d y:%d\n",FIXED_TO_INT(pmo->x), FIXED_TO_INT(pmo->y), FIXED_TO_INT(posX2x(pbot->destNode->x)), FIXED_TO_INT(posY2y(pbot->destNode->y)));

                                pbot->destNode = NULL;	//can't get to it
                            }
                        }

                        if( !B_LLIsEmpty(pbot->path))
                        {
                            //debug_Printf("turning towards node at x%d, y%d\n", FIXED_TO_INT(pbot->nextItemNode->x), FIXED_TO_INT(pbot->nextItemNode->y));
                            //debug_Printf("it has a distance %d\n", FIXED_TO_INT(P_AproxDistance(pmo->x - pbot->nextItemNode->x, pmo->y - pbot->nextItemNode->y)));
                            B_TurnTowardsPoint(p, posX2x(pbot->path->first->x), posY2y(pbot->path->first->y));
                            if( new_action )
                                pbot->turn_timer = 20;  // no decisions until have turned
                            forwardmove = botforwardmove[1];//botspeed];
                        }
                    }
                }
            }

            // proportional forward and side movement
            forward_angf = ANGLE_TO_FINE(pmo->angle);
            side_angf = ANGLE_TO_FINE(pmo->angle - ANG90);
            // the extra momentum
// Number of steps ahead to check.  This has a strong effect.
#define STEP_SIZE  (2 * 2048)
            fixed_t forwardmove2 = forwardmove * STEP_SIZE;
            fixed_t sidemove2 = sidemove * STEP_SIZE;
            fixed_t cmomx = FixedMul(forwardmove2, finecosine[forward_angf]) + FixedMul(sidemove2, finecosine[side_angf]);
            fixed_t cmomy = FixedMul(forwardmove2, finesine[forward_angf]) + FixedMul(sidemove2, finesine[side_angf]);
            px = pmo->x + pmo->momx + cmomx;
            py = pmo->y + pmo->momy + cmomy;

            // tmr_floorz, tmr_ceilingz, tmr_sector returned by P_CheckPosition
            blocked = !P_CheckPosition (pmo, px, py)
                 || ((tmr_ceilingz - tmr_floorz) < pmo->height);

            int sector_height_diff = FIXED_TO_INT(tmr_floorz - pmo->z);  // + is higher wall, - is cliff edge

            //if its time to change strafe directions,
            if( sidemove && ((pmo->flags & MF_JUSTHIT) || blocked ))
            {
                pbot->flags ^= BF_straferight;  // reverse strafe direction
                pmo->flags &= ~MF_JUSTHIT;
            }

            byte bot_dam = bot_sector_damage( tmr_sector, p );
            byte bot_dam_entry = ((bot_dam > 4) && (tmr_sector != pmo->subsector->sector));
            if( blocked || bot_dam_entry || ( abs(sector_height_diff) > 24 ))  // wall and cliffs
            {
                byte full_stop = bot_dam_entry;

                // tm_thing is global var of P_CheckPosition
                if( pbot->blockedcount < 200 )
                    ++pbot->blockedcount;

                fixed_t mom_dist = P_AproxDistance(pmo->momx, pmo->momy);  // momentum
                if( (pbot->blockedcount > 20)
                    && (pbot->turn_timer == 0)
                    && ( bot_dam_entry
                        || (mom_dist < FIXINT(4))
                        || (tm_thing && (tm_thing->flags & MF_SOLID))
                       )
                  )
                {
                    pbot->avoidtimer = 80;
                    pbot->turn_timer = 40;
                    pbot->lastMobj = tm_thing;  // to avoid it
                    pbot->flags |= BF_backup | BF_avoid;
//                    pbot->flags &= ~BF_avoid;
                    pbot->what = what;
                    pbot->blockedcount = 10;
                    full_stop = 1;
                }

#ifdef BOT_VERSION_DETECT
                else if( demoversion >= 148 )
#else
                else
#endif
                {
                    if( sector_height_diff > 24 )  // too high
                    {
                        if( (sector_height_diff <= 37)  // jumpable
                            || ((sector_height_diff <= 45)
                                && (pmo->subsector->sector->floortype != FLOOR_WATER)) )
                        {
                            if( cv_allowjump.EV )
                                cmd->buttons |= BT_JUMP;
                        }
                    }
                    else if( sector_height_diff < -24 )  // negative, cliff
                    {
                        fixed_t drop_thresh = -24;

                        // but sometimes have to jump over dropoff
                        if( cv_allowjump.EV )
                        {
                            // Height that can get back up from.
                            drop_thresh = (tmr_sector->floortype == FLOOR_WATER)? -37 : -45; // can get back up
                        }
                        if( has_friend && ((what & 0x0F) == WH_friend) && (bot_dam < 4))
                        {
                            drop_thresh = -64;
                        }
                        if( has_friend && (pbot->flags & BF_bump))
                        {
                            drop_thresh = -128;
                        }
                        if( (pbot->blockedcount > 45) && (bot_dam < 4) )
                        {
                            drop_thresh = -140;
                        }
#ifdef ENABLE_COME_HERE
                        if( (has_friend == 2) && come_here )  // close friend
                        {
                            drop_thresh = -250;
                        }
#endif
#ifdef DEBUG_BOT_DROP
                        GenPrintf( EMSG_debug, "bot drop: sector_height_diff=%i, thresh=%i, damage=%i what=%x\n", sector_height_diff, drop_thresh, bot_dam, what );
#endif
                        if( sector_height_diff < drop_thresh ) // too much negative, cliff
                        {
#ifdef DEBUG_BOT_DROP
                            GenPrintf( EMSG_debug, "bot drop: stop, enemy=%p, item=%p, what=%x\n", pbot->closestEnemy, pbot->bestSeenItem, what );
                            GenPrintf( EMSG_debug, "bot drop: damage=%i, momx=%i, momy=%i\n", bot_dam, FIXED_TO_INT(pmo->momx), FIXED_TO_INT(pmo->y) );
#endif
                            full_stop = 3;
                        }
                    }
                }

                if( full_stop )
                {
                    // Stop
                    // FIXME DOES NOT STOP THEM FROM GOING OVER CLIFF
                    B_Turn_Away_Point( p, px, py );
                    if( new_action )
                      pbot->turn_timer = 30;
                    //	forwardmove = -forwardmove;
                    //	forwardmove = sector_height_diff;  // backwards proportionally
                    if( pbot->blockedcount < 10 )
                    {
                        if( mom_dist > FIXINT(10) )
                          forwardmove = -mom_dist;
                        else
                          forwardmove = -10/NEWTICRATERATIO;
                        pbot->flags |= BF_backup;
#ifdef DEBUG_BOT_DROP
                        GenPrintf( EMSG_debug, "bot drop: reverse -10\n" );
#endif
                    }
                    else
                    {
#ifdef DEBUG_BOT_DROP
                        GenPrintf( EMSG_debug, "bot drop: STOP \n" );
#endif
                        forwardmove = 0;
                        sidemove = 0; // cannot tell which way the edge might be
                        pbot->flags &= ~BF_backup;
                    }

                    if( ! (pbot->flags & BF_bump) && (full_stop > 1) )
                    {
#ifdef DEBUG_BOT_DROP
                        GenPrintf( EMSG_debug, "bot drop: AVOID \n" );
#endif
                        // pbot->avoidtimer = 20;
                        pbot->avoidtimer = 120;
                        pbot->turn_timer = 24;
                        pbot->lastMobj = tm_thing;  // to avoid it
                        pbot->flags |= BF_avoid;
                        pbot->what = what;
                    }

                    // pbot->closestEnemy = NULL;
                    // pbot->bestSeenItem = NULL;
                    pmo->momx = 0;  // necessary to slow them down, because detection is so late
                    pmo->momy = 0;
                    // pmo->momx = -pmo->momx;  // necessary to slow them down, because detection is so late
                    // pmo->momy = -pmo->momy;
#ifdef DEBUG_BOT_DROP
                    blocked = 2;
#endif
                }

                for (x=0; x<numspechit; x++)
                {
                    if( lines[spechit[x]].backsector)
                    {
                        if( !lines[spechit[x]].backsector->ceilingdata && !lines[spechit[x]].backsector->floordata && (lines[spechit[x]].special != 11))	//not the exit switch
                            cmd->buttons |= BT_USE;
                    }
                }
            }
            else
                pbot->blockedcount = 0;
        }

        if( sidemove )
        {
            if( pbot->strafetimer )
                pbot->strafetimer--;
            else
            {
                pbot->strafetimer = B_Random()/3;
                pbot->flags ^= BF_straferight;  // reverse strafe direction
            }
        }
        if( pbot->weaponchangetimer)
            pbot->weaponchangetimer--;

        if( cv_bot_speed.EV == 10 )  // botskill dependent speed
        {
            byte spd = bot_botskill_to_speed[ pbot->skill ];
            bot_speed_frac = bot_speed_frac_table[ spd ];
            bot_run_tics = bot_run_tics_table[ spd ];
        }

        if( slow_down_tics )
        {
            int run_tics = bot_run_tics + slow_down_tics;
            if( pbot->runtimer < run_tics )
              pbot->runtimer = run_tics;
            goto cmd_move;  // forwardmove already set
        }

        // [WDJ] Limit the running.
        if( abs(forwardmove) > botforwardmove[0] )
        {
            // Running
            if( pbot->runtimer < bot_run_tics )
            {
                pbot->runtimer++;
                goto cmd_move;
            }

            // Tired, must walk.
            forwardmove = forwardmove>>1;  // walk
            if( pbot->runtimer == bot_run_tics )
            {
                pbot->runtimer += 10 * TICRATE;  // rest time
                goto cmd_move;
            }
        }
       
        if( pbot->runtimer > 0 )
        {
            pbot->runtimer--;
            // Run time needs to be proportional to bot_run_tics,
            // so reset timer to constant value.
            if( pbot->runtimer == bot_run_tics )
            {
                pbot->runtimer = (4 * TICRATE) - 2;  // reset hysterisis
            }
        }

    cmd_move:
        // [WDJ] Regulate the run speed.
#ifdef DEBUG_BOT_DROP
        // DEBUG
        if( blocked == 2 )
        {
            // if( forwardmove > 0 )
            GenPrintf( EMSG_debug, "bot drop: CMD forwardmove=%i  sidemove=%i\n", forwardmove, sidemove );
            blocked = 0;
        }
#endif
        p->cmd.forwardmove = (forwardmove * bot_speed_frac) >> 7;
        p->cmd.sidemove = (pbot->flags & BF_straferight) ? sidemove : -sidemove;

        if( pbot->closestMissile )
            B_AvoidMissile(p, pbot->closestMissile);
    }
    else
    {
        // Dead
#ifdef BOT_VERSION_DETECT
        if( demoversion < 146 )
        {
            // Before version 1.46, immedidate respawn
            cmd->buttons |= BT_USE;	//I want to respawn
        }
        else
#endif
        {
            // Version 1.46
            // [WDJ] Slow down bot respawn, so they are not so overwhelming.
            cmd->buttons = 0;
            if( p->damagecount )
            {
                p->damagecount = 0;
                // respawn delay timer
                pbot->avoidtimer = cv_bot_respawn_time.EV * TICRATE; // wait
                pbot->lastMobj = NULL;  // bot is dead, don't get confused
            }
            if( --pbot->avoidtimer <= 0 )
                cmd->buttons |= BT_USE;	//I want to respawn
        }
    }

    memcpy (netcmd, cmd, sizeof(*cmd));
} // end of BOT_Thinker


// Forget what cannot be saved.  To sync client and server bots.
void  B_forget_stuff( bot_t * bot )
{
    bot->destNode = NULL;
    B_LLClear( bot->path );
}

void  B_Destroy_Bot( player_t * player )
{
    bot_t * bot = player->bot;

    if( bot )
    {
        B_LLClear( bot->path );

        Z_Free( bot );

        player->bot = NULL;
    }
}

void  B_Create_Bot( player_t * player )
{
    bot_t * bot = player->bot;
    if( bot )
    {
        B_LLClear( bot->path );
        B_LLDelete( bot->path );
    }
    else
    {
        bot = Z_Malloc (sizeof(*bot), PU_STATIC, 0);
        player->bot = bot;
    }
    memset( bot, 0, sizeof(bot_t));
    bot->path = B_LLCreate();
}


void B_SpawnBot(bot_t* bot)
{
    byte sk;

    bot->avoidtimer = 0;
    bot->blockedcount = 0;
    bot->weaponchangetimer = 0;
    bot->runtimer = 0;
    bot->flags = 0;
    bot->turn_timer = 0;

    bot->bestItem = NULL;
    bot->bestSeenItem = NULL;
    bot->lastMobj = NULL;
    bot->destNode = NULL;
    bot->teammate = NULL;
    bot->closestEnemy = NULL;
    bot->closestUnseenEnemy = NULL;
    bot->closestUnseenTeammate = NULL;

    // [WDJ] Bot skill = 0..5, Game skill = 0..4.
    switch( cv_bot_skill.EV )
    {
      case 6: // randmed
         sk = E_Random() % gameskill;  // random bots, 0 to gameskill-1
         break;
      case 7: // randgame
         sk = (E_Random() % gameskill) + 1;  // random bots, 1 to gameskill
         break;
      case 8: // gamemed
         sk = gameskill;  // bot lower skill levels
         break;
      case 9: // gameskill
         sk = gameskill + 1;  // bot upper skill levels
         break;
      default:  // fixed bot skill selection
         sk = cv_bot_skill.EV;  // 0..5
         break;
    }
    bot->skill = sk; // 0=crippled, 1=baby .. 5=nightmare

    B_LLClear(bot->path);
}
