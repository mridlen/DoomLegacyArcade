// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_user.c 1699 2024-11-27 07:20:27Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log: p_user.c,v $
// Revision 1.17  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.16  2003/07/14 12:37:54  darkwolf95
// Fixed bug where frags don't display for Player 2 on death while in splitscreen.
//
// Revision 1.15  2001/05/27 13:42:48  bpereira
//
// Revision 1.14  2001/04/04 20:24:21  judgecutor
// Added support for the 3D Sound
//
// Revision 1.13  2001/03/03 06:17:33  bpereira
// Revision 1.12  2001/01/27 11:02:36  bpereira
//
// Revision 1.11  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.10  2000/11/04 16:23:43  bpereira
//
// Revision 1.9  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.8  2000/10/21 08:43:31  bpereira
// Revision 1.7  2000/08/31 14:30:56  bpereira
// Revision 1.6  2000/08/03 17:57:42  bpereira
// Revision 1.5  2000/04/23 16:19:52  bpereira
// Revision 1.4  2000/04/16 18:38:07  bpereira
// Revision 1.3  2000/03/29 19:39:48  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Player related stuff.
//      Bobbing POV/weapon, movement.
//      Pending weapon.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "d_event.h"
#include "g_game.h"
#include "p_local.h"
#include "r_main.h"
#include "r_things.h"
  // skins
#include "s_sound.h"
#include "p_setup.h"
#include "p_inter.h"
#include "m_random.h"

#include "hardware/hw3sound.h"


// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP         32


//
// Movement.
//

// 16 pixels of bob
#define MAXBOB  0x100000

//added:22-02-98: initial momz when player jumps (moves up)
fixed_t jumpgravity = (6*FRACUNIT/NEWTICRATERATIO); // variable by fragglescript

boolean  onground;
int	 extramovefactor = 0;


//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_Thrust(player_t *player, angle_t angle, fixed_t move)
{
    mobj_t * pmo = player->mo;
#if 0
    // friction and movefactor are now sector attributes
    if(pmo->subsector->sector->special == 15  // heretic ice
       && !(player->powers[pw_flight] && (pmo->z > pmo->floorz)))
    {
        move>>=2;  // Friction_Low
    }
#endif   
    pmo->momx += FixedMul(move, cosine_ANG(angle));
    pmo->momy += FixedMul(move, sine_ANG(angle));
}


// P_Thrust_Bob
// [WDJ] Thrust and independent bob.
// Contribute to bob effort, independent of thrust momentum.
// Do not bob when riding along on conveyors, or sliding, even though moving.
// Bob is adapted for effort in mud and ice by the caller. 

static void P_Thrust_Bob( player_t * player, angle_t angle, fixed_t moveth, fixed_t movebob )
{
    int angf = ANGLE_TO_FINE( angle );
    // thrust
    player->mo->momx += FixedMul(moveth, finecosine[angf]);
    player->mo->momy += FixedMul(moveth, finesine[angf]);
    // bob
    player->bob_momx += FixedMul(movebob, finecosine[angf]);
    player->bob_momy += FixedMul(movebob, finesine[angf]);
}



#ifdef CLIENTPREDICTION2
//
// P_ThrustSpirit
// Moves the given origin along a given angle.
//
void P_ThrustSpirit(player_t *player, angle_t angle, fixed_t move)
{
#if 0   
    // friction and movefactor are now sector attributes
    if(player->spirit->subsector->sector->special == 15
    && !(player->powers[pw_flight] && !(player->spirit->z <= player->spirit->floorz))) // Friction_Low
    {
        move>>=2;  // Friction_Low
    }
#endif   
    player->spirit->momx += FixedMul(move, cosine_ANG(angle));
    player->spirit->momy += FixedMul(move, sine_ANG(angle));
}
#endif


static fixed_t  prev_viewheight = -1;  // detect changes in cv_viewheight

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
// Called from P_PlayerThink after P_PlayerMove
// Called from P_MoveSpirit from Local_Maketic
// Called from P_DeathThink from P_PlayerThink, without P_PlayerMove
void P_CalcHeight (player_t* player)
{
    int         angle;
    fixed_t     bob;
    fixed_t     calc_viewheight, on_floor_viewheight;
    mobj_t      * pmo = player->mo;
    mobj_t      * smo = pmo;  // spirit (same as pmo unless using CLIENTPREDICTION2)

    // Regular movement bobbing
    // (needs to be calculated for gun swing even if not on ground)
    // OPTIMIZE: tablify angle
    // Note: a LUT allows for effects like a ramp with low health.

#ifdef CLIENTPREDICTION2
    if( player->spirit )
        smo = player->spirit;
#endif

    player->bob = ((FixedMul (player->bob_momx,player->bob_momx)
                   +FixedMul (player->bob_momy,player->bob_momy))*NEWTICRATERATIO)>>2;

    // [WDJ] Boom 2.02, when on ice, limited bob to MAXBOB>>2.
    // Moving onto ice would cause sudden bob position change.
    // Went obsolete with MBF player bob.

    if (player->bob>MAXBOB)
        player->bob = MAXBOB;

    // from heretic
    if( pmo->flags2&MF2_FLY && !onground )
        player->bob = FRACUNIT/2;

    if (player->cheats & CF_NOMOMENTUM)  // as in heretic because of fly bob
    {
        //added:15-02-98: it seems to be useless code!
        //player->viewz = pmo->z + (((unsigned int)cv_viewheight.EV)<<FRACBITS);

        //if (player->viewz > pmo->ceilingz-4*FRACUNIT)
        //    player->viewz = pmo->ceilingz-4*FRACUNIT;
        player->viewz = smo->z + player->viewheight;
        return;
    }

    if( EV_legacy )
#ifdef CLIENTPREDICTION2
      angle = (FINEANGLES/20*localgametic/NEWTICRATERATIO)&FINEMASK;
#else     
      angle = (FINEANGLES/20*leveltime/NEWTICRATERATIO)&FINEMASK;
#endif
    else
      angle = (FINEANGLES/20*leveltime)&FINEMASK;
    bob = FixedMul ( player->bob/2, finesine[angle]);

    // move viewheight
    calc_viewheight = ((unsigned int)cv_viewheight.EV) << FRACBITS; // default eye view height

    // The original was designed for a constant viewheight.
    // Some users want to vary it during play using fragglescript.
    if (player->playerstate == PST_LIVE)
    {
        on_floor_viewheight = calc_viewheight;

        if (calc_viewheight != prev_viewheight)
        {
            // cv_viewheight has changed
            if ( prev_viewheight < 0 )
            {
                player->viewheight = calc_viewheight;  // init quickly
            }
            else
            {
                // provide a gradual viewheight change
                fixed_t dv = (calc_viewheight - prev_viewheight) >> 3;
                // when dv == 0, let through unaltered viewheight,
                // otherwise altered viewheight keeps retriggering this code
                if (dv)
                   calc_viewheight = prev_viewheight + dv;  // slow rise and fall
                on_floor_viewheight = prev_viewheight;  // lessen falling effect
                player->deltaviewheight = (calc_viewheight - player->viewheight) >> 3;
            }
            prev_viewheight = calc_viewheight;
        }

        player->viewheight += player->deltaviewheight;

        if (player->viewheight > on_floor_viewheight)
        {
            // player feet not on floor, so fall to viewheight
            player->viewheight = calc_viewheight;
            player->deltaviewheight = 0;
        }

        if (player->viewheight < calc_viewheight/2)
        {
            // rise from floor
            player->viewheight = calc_viewheight/2;
            if (player->deltaviewheight <= 0)
                player->deltaviewheight = 1;
        }

        // changers of viewheight will have also set deltaviewheight
        if (player->deltaviewheight)
        {
            player->deltaviewheight += FRACUNIT/4;
            if (!player->deltaviewheight)
                player->deltaviewheight = 1;
        }
    }   

    if(player->chickenTics)  // heretic chicken morph
        player->viewz = smo->z + player->viewheight-(20*FRACUNIT);
    else
        player->viewz = smo->z + player->viewheight + bob;

    if(pmo->flags2&MF2_FEETARECLIPPED
        && player->playerstate != PST_DEAD
        && pmo->z <= pmo->floorz)
    {
        player->viewz -= FOOTCLIPSIZE;
    }

    if (player->viewz > smo->ceilingz-4*FRACUNIT)
        player->viewz = smo->ceilingz-4*FRACUNIT;
    if (player->viewz < smo->floorz+4*FRACUNIT)
        player->viewz = smo->floorz+4*FRACUNIT;
}



static byte  EN_move_doom = 0;
static byte  EN_cmd_abs_angle = 1;  // legacy absolute angle commands

// local version control
void DemoAdapt_p_user( void )
{
    // Move for Doom, Boom, MBF demo
    EN_move_doom =
       EN_doom_etc
       && (EV_legacy < 128);  // Legacy demo, orig Doom, Boom, MBF

    // abs angle in legacy demos only
    EN_cmd_abs_angle = (EV_legacy >= 125);
}


//
// P_MovePlayer
//
// Called from P_PlayerThink
void P_MovePlayer (player_t* player)
{
    mobj_t *   pmo = player->mo;
    ticcmd_t*  cmd = &player->cmd;
    int  movefactor = ORIG_FRICTION_FACTOR; // default
    int  bobfactor = ORIG_FRICTION_FACTOR;

    if(EN_cmd_abs_angle)
        pmo->angle = (cmd->angleturn<<16);
    else
        pmo->angle += (cmd->angleturn<<16);

    stat_tic_moved++;
#ifdef TICCMD_148
    if( (cmd->ticflags & TC_received) == 0)
        stat_tic_miss++;
#else
    if( (cmd->angleturn & TICCMD_RECEIVED) == 0)
        stat_tic_miss++;
#endif

    // Do not let the player control movement
    //  if not onground.
    onground = (pmo->z <= pmo->floorz) 
               || (pmo->flags & MF_BOUNCES) // MBF
               || (pmo->flags2&(MF2_ONMOBJ|MF2_FLY))  // heretic
               || (player->cheats & CF_FLYAROUND);   // cheat

    if(EN_variable_friction && onground)
    {
        movefactor = P_GetMoveFactor(pmo); // gets got_movefactor, got_friction
//        CONS_Printf("friction: %X, movefactor: %i\n", got_friction, movefactor);

        // [WDJ] bobfactor from killough, via prboom, adapted to legacy.
        // killough 11/98:
        // On sludge, make bobbing depend on efficiency.
        // On ice, make it depend on effort.

        // [WDJ] Test the friction and movefactor from GetMovefactor.
        bobfactor = (got_friction < ORIG_FRICTION) ?
             got_movefactor  // mud
           : ORIG_FRICTION_FACTOR;  // ice  (killough)
//           : (ORIG_FRICTION_FACTOR + got_movefactor)/2;  // ice [WDJ]

    }
   
    if (!onground)
        bobfactor >>= 2;  // air and underwater
    else if (pmo->eflags & MF_UNDERWATER)
        bobfactor >>= 1;

    if( EN_move_doom )
    {
        // Doom, Boom, MBF movement
        boolean  jumpover = player->cheats & CF_JUMPOVER;  // legacy cheat
        if (cmd->forwardmove && (onground || jumpover))
        {
            // dirty hack to let the player avatar walk over a small wall
            // while in the air
            if (jumpover)
            {
                if(pmo->momz > 0)
                    P_Thrust (player, pmo->angle, 5*movefactor);
            }
            else
            {
                P_Thrust_Bob( player, pmo->angle, cmd->forwardmove*movefactor, cmd->forwardmove*bobfactor );
            }
        }
    
        if (cmd->sidemove && onground)
        {
            P_Thrust_Bob( player, pmo->angle-ANG90, cmd->sidemove*movefactor, cmd->sidemove*bobfactor );
        }

        player->aiming = (signed char)cmd->aiming;
    }
    else
    {
        // most current
        fixed_t   movepushforward=0, movepushside=0;
        player->aiming = cmd->aiming<<16;
        if( player->chickenTics )
        {
            // [WDJ] Moved to after other movefactor, so it can have some effect.
            // movefactor = 2500;  // heretic chicken
            // Modify movefactor to chicken size, (chicken on ice)
            movefactor = movefactor * 625 / 512;
              // * 2500 / 2048
        }

#ifdef ENABLE_TIRED_RUN
#define  TIRED_THRESHOLD   (TICRATE*4)
        if( cv_tired_run.EV )
        {
            if( player->tired > TIRED_THRESHOLD )
            {
# if NEWTICRATERATIO > 1
                int tire_ex = (player->tired - TIRED_THRESHOLD) / (2 * NEWTICRATERATIO);
# else
                int tire_ex = (player->tired - TIRED_THRESHOLD) >> 1;
# endif
                int tf = (1024 - (tire_ex * cv_tired_run.EV));
                if( tf < 2 )  tf = 2;
                movefactor = (movefactor * tf) >> 10;
                bobfactor = (bobfactor * tf) >> 10;
            }

            if( player->tired < (TIRED_THRESHOLD + (TICRATE*45)))  // add to tired
            {
                player->tired += ( abs(cmd->forwardmove) + abs(cmd->sidemove) ) / (24/NEWTICRATERATIO);
            }
        }
#endif

        if (cmd->forwardmove)
        {
            movepushforward = cmd->forwardmove * (movefactor + extramovefactor);

            if (pmo->eflags & MF_UNDERWATER)
            {
                // half forward speed when waist under water
                // a little better grip if feets touch the ground
                if (!onground)
                    movepushforward >>= 1;
                else
                    movepushforward = movepushforward *3/4;
            }
            else
            {
                // allow very small movement while in air for gameplay
                if (!onground)
                    movepushforward >>= 3;
            }

            P_Thrust_Bob( player, pmo->angle, movepushforward, cmd->forwardmove*bobfactor);
        }

        if (cmd->sidemove)
        {
            movepushside = cmd->sidemove * (movefactor + extramovefactor);
            if (pmo->eflags & MF_UNDERWATER)
            {
                if (!onground)
                    movepushside >>= 1;
                else
                    movepushside = movepushside *3/4;
            }
            else
            {
                if (!onground)
                    movepushside >>= 3;
            }

            P_Thrust_Bob( player, pmo->angle-ANG90, movepushside, cmd->sidemove*bobfactor);
        }

        // mouselook swim when waist underwater
        pmo->eflags &= ~MF_SWIMMING;
        if (pmo->eflags & MF_UNDERWATER)
        {
            fixed_t a;
            // swim up/down full move when forward full speed
            a = FixedMul( movepushforward*50, sine_ANG(player->aiming) >>5 );
            
            if ( a != 0 ) {
                pmo->eflags |= MF_SWIMMING;
                pmo->momz += a;
            }
        }
    }

    //added:22-02-98: jumping
    if (cmd->buttons & BT_JUMP)
    {
        if( pmo->flags2&MF2_FLY )
            player->flyheight = 10;
        else 
        if(pmo->eflags & MF_UNDERWATER)
            //TODO: goub gloub when push up in water
            pmo->momz = jumpgravity/2;
        else 
        // can't jump while in air, can't jump while jumping
        if( onground && !(player->GB_flags & GB_jumpdown))
        {
            pmo->momz = jumpgravity;
            if( !(player->cheats & CF_FLYAROUND) )
            {
                S_StartScreamSound (pmo, sfx_jump);
                // keep jumping ok if FLY mode.
                player->GB_flags |= GB_jumpdown;
            }
        }
    }
    else
        player->GB_flags &= ~(GB_jumpdown);


    if (cmd->forwardmove || cmd->sidemove)
    {
        if( player->chickenTics )
        {
            if( pmo->state == &states[S_CHICPLAY])
                P_SetMobjState(pmo, S_CHICPLAY_RUN1);
        }
        else
            if(pmo->state == &states[S_PLAY])
                P_SetMobjState(pmo, S_PLAY_RUN1);
    }
#ifdef TICCMD_148
    if( EN_heretic && (cmd->ticflags & TC_flydown) )
#else
    if( EN_heretic && (cmd->angleturn & BT_FLYDOWN) )
#endif
    {
        player->flyheight = -10;
    }
/* HERETODO
    fly = cmd->lookfly>>4;
    if(fly > 7)
        fly -= 16;
    if(fly && player->powers[pw_flight])
    {
        if(fly != TOCENTER)
        {
            player->flyheight = fly*2;
            if(!(pmo->flags2&MF2_FLY))
            {
                pmo->flags2 |= MF2_FLY;
                pmo->flags |= MF_NOGRAVITY;
            }
        }
        else
        {
            pmo->flags2 &= ~MF2_FLY;
            pmo->flags &= ~MF_NOGRAVITY;
        }
    }
    else if(fly > 0)
    {
        P_PlayerUseArtifact(player, arti_fly);
    }*/
    if(pmo->flags2&MF2_FLY)
    {
        pmo->momz = player->flyheight*FRACUNIT;
        if(player->flyheight)
            player->flyheight /= 2;
    }

#ifdef ENABLE_TIRED_RUN
    if( player->tired )
    {
        player->tired --;
    }
#endif
#ifdef ENABLE_COME_HERE
# ifdef TICCMD_148
    if( cmd->ticflags & TC_comehere )
    {
        come_here = 3*TICRATE;
        come_here_player = pmo;
        GenPrintf( EMSG_info, "Come here\n" );
#  ifdef DEBUG_COME_HERE	
        GenPrintf( EMSG_debug, "Come here ## \n" );
#  endif
    }
# endif
#endif
}



//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
#define ANG5    (ANG90/18)

void P_DeathThink (player_t* player)
{
    mobj_t * pmo = player->mo;
    angle_t  angle;

    P_MovePsprites (player);

    // fall to the ground
    if (player->viewheight > 6*FRACUNIT)
        player->viewheight -= FRACUNIT;

    if (player->viewheight < 6*FRACUNIT)
        player->viewheight = 6*FRACUNIT;

    player->deltaviewheight = 0;
    onground = pmo->z <= pmo->floorz;

    P_CalcHeight (player);

    mobj_t *attacker = player->attacker;

    // watch my killer (if there is one)
    if (attacker && attacker != pmo)
    {
        angle = R_PointToAngle2 (pmo->x,
                                 pmo->y,
                                 player->attacker->x,
                                 player->attacker->y);

        angle_t delta = angle - pmo->angle;

        if (delta < ANG5 || delta > (unsigned)-ANG5)
        {
            // Looking at killer,
            //  so fade damage flash down.
            pmo->angle = angle;

            if (player->damagecount)
                player->damagecount--;
        }
        else if (delta < ANG180)
            pmo->angle += ANG5;
        else
            pmo->angle -= ANG5;

        //added:22-02-98:
        // change aiming to look up or down at the attacker (DOESNT WORK)
        // FIXME : the aiming returned seems to be too up or down... later
        /*
        fixed_t dist = P_AproxDistance(attacker->x - pmo->x, attacker->y - pmo->y);
        fixed_t dz = attacker->z +(attacker->height>>1) -pmo->z;
        angle_t pitch = 0;
        if (dist)
          pitch = ArcTan(FixedDiv(dz, dist));
        */
        int32_t pitch = (attacker->z - pmo->z)>>17;
        player->aiming = G_ClipAimingPitch(pitch);

    }
    else if (player->damagecount)
        player->damagecount--;

    if (player->cmd.buttons & BT_USE)
    {
        player->playerstate = PST_REBORN;
        pmo->special2 = 666;
    }
}

//----------------------------------------------------------------------------
//
// PROC P_ChickenPlayerThink
//
//----------------------------------------------------------------------------

void P_ChickenPlayerThink(player_t *player)
{
    mobj_t * pmo = player->mo;

    if(player->health > 0)
    { // Handle beak movement
        P_UpdateBeak(player, &player->psprites[PS_weapon]);
    }
    if(player->chickenTics&15)
    {
        return;
    }
    // Heretic uses of P_Random
    if(!(pmo->momx+pmo->momy) && PP_Random(ph_chickenthink) < 160)
    { // Twitch view angle
        pmo->angle += PP_SignedRandom(ph_chickenthink)<<19;
    }
    if((pmo->z <= pmo->floorz) && (PP_Random(ph_chickenthink) < 32))
    { // Jump and noise
        pmo->momz += FRACUNIT;
        P_SetMobjState(pmo, S_CHICPLAY_PAIN);
        return;
    }
    if(PP_Random(ph_chickenthink) < 48)
    { // Just noise
        S_StartScreamSound(pmo, sfx_chicact);
    }
}

//----------------------------------------------------------------------------
//
// FUNC P_UndoPlayerChicken
//
//----------------------------------------------------------------------------

// [WDJ] Fixed to keep the same player mobj.
// Used to change the player mobj, and hide the prev as a corpse above
// the ceiling using S_FREETARGMOBJ.
boolean P_UndoPlayerChicken(player_t *player)
{
    mobj_t *fog;
    mobj_t * pmo = player->mo;
    weapontype_t weapon;
    int oldflags2;

    weapon = pmo->special1;  // saved by player morph
    oldflags2 = pmo->flags2;
    // Morph back into player
    if( ! P_MorphMobj(pmo, MT_PLAYER, MM_testsize,
#ifdef PLAYER_CHICKEN_KEEPS_SHADOW
                      MF_SHADOW
#else
                      0
#endif
                      ) )
    { // Didn't fit
        player->chickenTics = 2*35;  // retry later
        return false;
    }
    if(oldflags2&MF2_FLY)  // preserve fly flags
    {
        pmo->flags2 |= MF2_FLY;
        pmo->flags |= MF_NOGRAVITY;
    }
    // Restore player skin and skincolor.
    pmo->skin = skins[player->skin];  // restore player skin
    pmo->tflags |= (player->skincolor) << MFT_TRANSSHIFT;
    pmo->reactiontime = 18;
    player->chickenTics = 0;
#ifndef PLAYER_CHICKEN_KEEPS_SHADOW
    // Not in vanilla Heretic, but implied by clearing MF_SHADOW.
    // Legacy2 clears it.
    player->powers[pw_invisibility] = 0;
#endif
    player->powers[pw_weaponlevel2] = 0;
    player->weaponinfo = wpnlev1info;
    player->health = pmo->health = MAXHEALTH;

    // This telefog is placed differently than others.
    int angf = ANGLE_TO_FINE( pmo->angle );
    fog = P_SpawnMobj(pmo->x+20*finecosine[angf], pmo->y+20*finesine[angf],
                      pmo->z+TELEFOGHEIGHT, MT_TFOG);
    S_StartObjSound(fog, sfx_telept);
    P_PostChickenWeapon(player, weapon);
    return true;
}

//
// P_MoveCamera : make sure the camera is not outside the world
//                and looks at the player avatar
//

// [WDJ] There is only one camera, so when there are two players, it can
// only be used as chase camera for one, and that is player1
// It now records who it is chasing.
camera_t camera;

//#define VIEWCAM_DIST    (128<<FRACBITS)
//#define VIEWCAM_HEIGHT  (20<<FRACBITS)

consvar_t cv_cam_dist   = {"cam_dist"  ,"128"  ,CV_FLOAT,NULL};
consvar_t cv_cam_height = {"cam_height", "20"   ,CV_FLOAT,NULL};
consvar_t cv_cam_speed  = {"cam_speed" ,  "0.25",CV_FLOAT,NULL};

void P_ResetCamera (player_t *player)
{
    fixed_t x,y,z;

    camera.chase = player;
    x = player->mo->x;
    y = player->mo->y;
    z = player->mo->z + (((unsigned int)cv_viewheight.EV)<<FRACBITS);

    // hey we should make sure that the sounds are heard from the camera
    // instead of the marine's head : TO DO

    // set bits for the camera object
    if (!camera.mo)
        camera.mo = P_SpawnMobj (x,y,z, MT_CHASECAM);
    else
    {
        camera.mo->x = x;
        camera.mo->y = y;
        camera.mo->z = z;
    }

    camera.mo->angle = player->mo->angle;
    camera.aiming = 0;
}

// Unused
#if 0
fixed_t cameraz;

boolean PTR_FindCameraPoint (intercept_t* in)
{
//  Disabled all except return false
#if 0
    int         side;
    fixed_t             slope;
    fixed_t             dist;
    line_t*             li;

    li = in->d.line;

    if ( !(li->flags & ML_TWOSIDED) )
        return false;

    // crosses a two sided line
    //added:16-02-98: Fab comments : sets opentop, openbottom, openrange
    //                lowfloor is the height of the lowest floor
    //                         (be it front or back)
    P_LineOpening (li);

    dist = FixedMul (attackrange, in->frac);

    if (li->frontsector->floorheight != li->backsector->floorheight)
    {
        //added:18-02-98: comments :
        // find the slope aiming on the border between the two floors
        slope = FixedDiv (openbottom - cameraz , dist);
        if (slope > aimslope)
            return false;
    }

    if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
    {
        slope = FixedDiv (opentop - shootz , dist);
        if (slope < aimslope)
            goto hitline;
    }

    return true;

    // hit line
  hitline:
#endif
    // stop the search
    return false;
}
#endif



void P_MoveChaseCamera (player_t *player)
{
    angle_t       angle;
    int           angf;   
    fixed_t       x, y, z, viewpointx, viewpointy;
    fixed_t       dist;
    float         f1, f2;
    subsector_t*  newsubsec;
    mobj_t*       pmo = player->mo;

    if (!camera.mo)
        P_ResetCamera (player);

    // [Arcade] Unstick the camera.
    //
    // MT_CHASECAM is MF_NOBLOCKMAP|MF_NOSECTOR, so nothing can collide with
    // *it*, but it still collides with the world: it is moved by momentum
    // through the ordinary mobj thinker, so P_TryMove stops it on walls.  In
    // doorways and around corners it wedges, and because the follow below only
    // ever nudges it a fraction of the way toward the ideal spot, it stays
    // wedged and the player walks off and leaves it behind -- showing an
    // empty room while the run continues somewhere else.
    //
    // Measured on the doom2_ep1_sk3_speed record demo: ordinary play holds
    // 100..200 units, but the camera spent 40% of that run beyond 256, in five
    // excursions, the worst of them stuck for 823 tics -- 23 seconds of
    // attract screen pointed at nothing.
    //
    // Recovery is P_ResetCamera, which puts the camera on the player's own
    // position.  That spot is always valid (the player is standing in it), so
    // unlike snapping to the ideal spot behind the player it can never drop
    // the camera inside a wall; the follow below immediately eases it back
    // out.  It is also exactly what a teleport already does to the camera,
    // so the movement is one the cabinet has always shown.
    //
    // The threshold has to clear the legitimate follow lag, which is not
    // small: the camera closes only cv_cam_speed (0.25) of the gap per tic, so
    // a player moving v units per tic settles about v/0.25 = 4v units behind
    // the ideal spot.  The demo peaked at 20.3 units per tic, i.e. ~81 units
    // of lag on top of the 128 hover distance, ~209 in all.  P_AproxDistance
    // overestimates a true distance by up to 12%, so that reads as ~234 here.
    // 128 + 160 = 288 clears it, and every stuck excursion measured was far
    // beyond it.
#define CAM_SNAP_SLACK  (160*FRACUNIT)
    if( P_AproxDistance( camera.mo->x - pmo->x, camera.mo->y - pmo->y )
        > (cv_cam_dist.value + CAM_SNAP_SLACK) )
    {
        P_ResetCamera (player);
        camera.mo->momx = camera.mo->momy = camera.mo->momz = 0;
    }

    angf = ANGLE_TO_FINE( pmo->angle );

    // sets ideal cam pos
    dist  = cv_cam_dist.value;
    x = pmo->x - FixedMul( finecosine[angf], dist);
    y = pmo->y - FixedMul( finesine[angf], dist);
    z = pmo->z + (((unsigned int)cv_viewheight.EV)<<FRACBITS) + cv_cam_height.value;

/*    P_PathTraverse ( pmo->x, pmo->y, x, y, PT_ADDLINES, PTR_UseTraverse );*/

    // move camera down to move under lower ceilings
    newsubsec = R_IsPointInSubsector ((pmo->x + camera.mo->x)>>1,(pmo->y + camera.mo->y)>>1);
    if (!newsubsec)
    {
        // use player sector 
        if (pmo->subsector->sector->ceilingheight - camera.mo->height < z)
            z = pmo->subsector->sector->ceilingheight - camera.mo->height-11*FRACUNIT; // don't be blocked by a opened door
    }
    else
    // camera fit ?
    if (newsubsec->sector->ceilingheight - camera.mo->height < z)
    {
        // no fit
        z = newsubsec->sector->ceilingheight - camera.mo->height - 11*FRACUNIT;
    }

    // does the camera fit in its own sector
    newsubsec = R_PointInSubsector (camera.mo->x, camera.mo->y);
    if (newsubsec->sector->ceilingheight - camera.mo->height < z)
        z = newsubsec->sector->ceilingheight - camera.mo->height - 11*FRACUNIT;


    // point viewed by the camera
    // this point is just 64 unit forward the player
    dist = 64 << FRACBITS;
    viewpointx = pmo->x + FixedMul( finecosine[angf], dist);  // player angle
    viewpointy = pmo->y + FixedMul( finesine[angf], dist);

    camera.mo->angle = R_PointToAngle2(camera.mo->x, camera.mo->y,
                                       viewpointx, viewpointy);

    // follow the player
    camera.mo->momx = FixedMul(x - camera.mo->x, cv_cam_speed.value);
    camera.mo->momy = FixedMul(y - camera.mo->y, cv_cam_speed.value);
    camera.mo->momz = FixedMul(z - camera.mo->z, cv_cam_speed.value);

    // compute aiming to look toward the viewed point
    f1=FIXED_TO_FLOAT(viewpointx - camera.mo->x);
    f2=FIXED_TO_FLOAT(viewpointy - camera.mo->y);
    dist=sqrt(f1*f1+f2*f2)*FRACUNIT;
    angle=R_PointToAngle2(0, camera.mo->z, dist,
                         pmo->z + (pmo->height>>1) + sine_ANG(player->aiming)*64);

    angle = G_ClipAimingPitch(angle);
    dist = camera.aiming - angle;
    camera.aiming -= (dist>>3);
}


byte weapontobutton[NUMWEAPONS]=
  {wp_fist    <<BT_WEAPONSHIFT,
   wp_pistol  <<BT_WEAPONSHIFT,
   wp_shotgun <<BT_WEAPONSHIFT,
   wp_chaingun<<BT_WEAPONSHIFT,
   wp_missile <<BT_WEAPONSHIFT,
   wp_plasma  <<BT_WEAPONSHIFT,
   wp_bfg     <<BT_WEAPONSHIFT,
  (wp_fist    <<BT_WEAPONSHIFT) | BT_EXTRAWEAPON,// wp_chainsaw
  (wp_shotgun <<BT_WEAPONSHIFT) | BT_EXTRAWEAPON};//wp_supershotgun

#ifdef CLIENTPREDICTION2

void CL_ResetSpiritPosition(mobj_t *mobj)
{
    P_UnsetThingPosition(mobj->player->spirit);
    mobj->player->spirit->x=mobj->x;
    mobj->player->spirit->y=mobj->y;
    mobj->player->spirit->z=mobj->z;
    mobj->player->spirit->momx=0;
    mobj->player->spirit->momy=0;
    mobj->player->spirit->momz=0;
    mobj->player->spirit->angle=mobj->angle;
    P_SetThingPosition(mobj->player->spirit);
}

void P_ProcessCmdSpirit (player_t* player,ticcmd_t *cmd)
{
    fixed_t   movepushforward=0, movepushside=0;
#ifdef PARANOIA
    if(!player)
        I_Error("P_MoveSpirit : player null");
    if(!player->spirit)
        I_Error("P_MoveSpirit : player->spirit null");
    if(!cmd)
        I_Error("P_MoveSpirit : cmd null");
#endif

    // don't move if dead
    if( player->playerstate != PST_LIVE )
    {
#ifdef TICCMD_148
        cmd->ticflags &= ~TC_XY;  // turn off clientprediction
#else
        cmd->angleturn &= ~TICCMD_XY;
#endif
        return;
    }
    onground = (player->spirit->z <= player->spirit->floorz) ||
               (player->cheats & CF_FLYAROUND);

    if (player->spirit->reactiontime)
    {
        player->spirit->reactiontime--;
        return;
    }

    player->spirit->angle = cmd->angleturn<<16;
#ifdef TICCMD_148
    cmd->ticflags |= TC_XY;
#else
    cmd->angleturn |= TICCMD_XY;
#endif
/*
    // now weapon is allways send change is detected at receiver side
    if(cmd->buttons & BT_CHANGE) 
    {
        player->spirit->movedir = cmd->buttons & (BT_WEAPONMASK | BT_EXTRAWEAPON);
        cmd->buttons &=~BT_CHANGE;
    }
    else
    {
        if( player->pendingweapon!=wp_nochange )
            player->spirit->movedir=weapontobutton[player->pendingweapon];
        cmd->buttons&=~(BT_WEAPONMASK | BT_EXTRAWEAPON);
        cmd->buttons|=player->spirit->movedir;
    }
*/
    if (cmd->forwardmove)
    {
        movepushforward = cmd->forwardmove * movefactor;
        
        if (player->spirit->eflags & MF_UNDERWATER)
        {
            // half forward speed when waist under water
            // a little better grip if feets touch the ground
            if (!onground)
                movepushforward >>= 1;
            else
                movepushforward = movepushforward *3/4;
        }
        else
        {
            // allow very small movement while in air for gameplay
            if (!onground)
                movepushforward >>= 3;
        }
        
        P_ThrustSpirit (player->spirit, player->spirit->angle, movepushforward);
    }
    
    if (cmd->sidemove)
    {
        movepushside = cmd->sidemove * movefactor;
        if (player->spirit->eflags & MF_UNDERWATER)
        {
            if (!onground)
                movepushside >>= 1;
            else
                movepushside = movepushside *3/4;
        }
        else 
            if (!onground)
                movepushside >>= 3;
            
        P_ThrustSpirit (player->spirit, player->spirit->angle-ANG90, movepushside);
    }
    
    // mouselook swim when waist underwater
    player->spirit->eflags &= ~MF_SWIMMING;
    if (player->spirit->eflags & MF_UNDERWATER)
    {
        fixed_t a;
        // swim up/down full move when forward full speed
        a = FixedMul( movepushforward*50, finesine[ (cmd->aiming>>(ANGLETOFINESHIFT-16)) ] >>5 );
        
        if ( a != 0 ) {
            player->spirit->eflags |= MF_SWIMMING;
            player->spirit->momz += a;
        }
    }

    //added:22-02-98: jumping
    if (cmd->buttons & BT_JUMP)
    {
        // can't jump while in air, can't jump while jumping
        if( !(player->GB_flags & GB_jumpdown2)
            && (onground || (player->spirit->eflags & MF_UNDERWATER)) )
        {
            if (onground)
                player->spirit->momz = jumpgravity;
            else //water content
                player->spirit->momz = jumpgravity/2;

            //TODO: goub gloub when push up in water
            
            if ( !(player->cheats & CF_FLYAROUND) && onground && !(player->spirit->eflags & MF_UNDERWATER))
            {
                S_StartScreamSound(player->spirit, sfx_jump);

                // keep jumping ok if FLY mode.
                player->GB_flags |= GB_jumpdown2;
            }
        }
    }
    else
        player->GB_flags &= ~(GB_jumpdown2);

}

void P_MoveSpirit (player_t* p,ticcmd_t *cmd, int realtics)
{
    if( gamestate != GS_LEVEL )
        return;

    if(p->spirit)
    {
        int    i;

        p->spirit->flags|=MF_SOLID;
        for(i=0;i<realtics;i++)
        {
            P_ProcessCmdSpirit(p,cmd);
            P_MobjThinker(p->spirit);
        }                 
        p->spirit->flags&=~MF_SOLID;
        P_CalcHeight (p);                 // z-bobing of player
        A_TicWeapon(p, &p->psprites[0]);  // bobing of weapon
        cmd->x=p->spirit->x;
        cmd->y=p->spirit->y;
        spirit_update=true;
    }
    else
    if(p->mo)
    {
        cmd->x=p->mo->x;
        cmd->y=p->mo->y;
    }
}

#endif


//
// P_PlayerThink
//

boolean playerdeadview; //Fab:25-04-98:show dm rankings while in death view

// Fixed to not change the player mobj.
void P_PlayerThink (player_t* player)
{
    ticcmd_t*           cmd;
    weapontype_t        newweapon;
    mobj_t *  pmo = player->mo;

#ifdef PARANOIA
    if(!pmo) I_Error("p_playerthink : players[%d].mo == NULL",player-players);
#endif

    // fixme: do this in the cheat code
    if (player->cheats & CF_NOCLIP)
        pmo->flags |= MF_NOCLIP;
    else
        pmo->flags &= ~MF_NOCLIP;

    // chain saw run forward
    cmd = &player->cmd;
    if (pmo->flags & MF_JUSTATTACKED)
    {
        // added : now angle turn is a absolute value not relative
        if( ! EN_cmd_abs_angle )
            cmd->angleturn = 0;
        cmd->forwardmove = 0xc800/512;
        cmd->sidemove = 0;
#ifdef TICCMD_148
        cmd->ticflags = 0;  // stun
#endif
        pmo->flags &= ~MF_JUSTATTACKED;
    }

    if (player->playerstate == PST_REBORN)
    {
#ifdef PARANOIA
        I_SoftError("player %d is in PST_REBORN\n");
#endif
        // it is not "normal" but far to be critical
        return;
    }

    if (player->playerstate == PST_DEAD)
    {
        //Fab:25-04-98: show the dm rankings while dead, only in deathmatch
        //DarkWolf95:July 03, 2003:fixed bug where rankings only show on player1's death
        if (player== displayplayer_ptr
            || player== displayplayer2_ptr ) // NULL when unused
            playerdeadview = true;

        P_DeathThink (player);

        //added:26-02-98:camera may still move when guy is dead
        if (camera.chase == player)
            P_MoveChaseCamera ( player );
        return;
    }
    else
    {
        if ( player== displayplayer_ptr )
            playerdeadview = false;
    }
    if( player->chickenTics )
        P_ChickenPlayerThink(player);

    // check water content, set stuff in mobj
    P_MobjCheckWater (pmo);

#ifdef ENABLE_TIRED_RUN
    if( cv_drown.EV )
    {
        // This is only needed temporarily, and eflags is full.
        extern byte  checkwater_is_drowning;

        if( checkwater_is_drowning )  // from P_MobjCheckWater
        {
            // [WDJ] Effect of drown is to be tired, and darkening view.
            // We do not have a specialized death for this, and it is not worth it to make one.
            // Also, many maps will have misused the water, so minimize interact with that.
            if( player->drown > (TICRATE*55) )  // 55 sec
            {
                if( player->tired < 550 )  player->tired += 2;  // override tired--
            }

            if( player->drown < (TICRATE*300) )  player->drown ++;  // 300 sec
        }
        else if( player->drown )
        {
            // got air, recover
            if( player->drown > (TICRATE*60) )  // 60 sec
                player->drown -= 7;
            else
                player->drown --;
        }
    }
# if 0
    if( player->tired | player->drown )  // DEBUG
        printf( "TIRED=%i  DROWN=%i\n", player->tired, player->drown );
# endif
#endif

    // Move around.
    // Reactiontime is used to prevent movement
    //  for a bit after a teleport.
    if (pmo->reactiontime)
        pmo->reactiontime--;
    else
        P_MovePlayer (player);

    //added:26-02-98: calculate the camera movement
    //added:22-02-98: bob view only if looking by the marine's eyes
    if (camera.chase == player)
        P_MoveChaseCamera ( player );  // camera view adjust
    else
#ifdef CLIENTPREDICTION2
        ;
#else
        P_CalcHeight (player);  // viewheight adjust, bob view
#endif


    // check special sectors : damage & secrets
    P_PlayerInSpecialSector (player);

    //
    // TODO water splashes
    //
#if 0
    if( (EV_legacy >= 125) && player->specialsector == )
    {
        if ((pmo->momx >  (2*FRACUNIT) ||
             pmo->momx < (-2*FRACUNIT) ||
             pmo->momy >  (2*FRACUNIT) ||
             pmo->momy < (-2*FRACUNIT) ||
             pmo->momz >  (2*FRACUNIT)) &&  // jump out of water
             !(gametic % (32 * NEWTICRATERATIO)) )
        {
            //
            // make sure we disturb the surface of water (we touch it)
            //
            int waterz = pmo->subsector->sector->floorheight + (FRACUNIT/4);

            // half in the water
            if(pmo->eflags & MF_TOUCHWATER)
            {
                if (pmo->z <= pmo->floorz) // onground
                {
                    fixed_t whater_height = waterz - pmo->subsector->sector->floorheight;

                    if( whater_height < (pmo->height>>2))
                        S_StartObjSound(pmo, sfx_splash);
                    else
                        S_StartObjSound(pmo, sfx_floush);
                }
                else
                    S_StartObjSound(pmo, sfx_floush);
            }                   
        }
    }
#endif

    // Check for weapon change.
//#ifndef CLIENTPREDICTION2
    if (cmd->buttons & BT_CHANGE)
//#endif
    {

        // The actual changing of the weapon is done
        //  when the weapon psprite can do it
        //  (read: not in the middle of an attack).
        newweapon = (cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT;
        if(demoversion<128)
        {
            // Old Doom, !EN_boom
            if (newweapon == wp_fist
                && player->weaponowned[wp_chainsaw]
                && !(player->readyweapon == wp_chainsaw
                     && player->powers[pw_strength]))
            {
                newweapon = wp_chainsaw;
            }
        
            if ( (gamemode == doom2_commercial)
                && newweapon == wp_shotgun
                && player->weaponowned[wp_supershotgun]
                && player->readyweapon != wp_supershotgun)
            {
                newweapon = wp_supershotgun;
            }
        }
        else
        {
            if(cmd->buttons&BT_EXTRAWEAPON)
               switch(newweapon) {
                  case wp_shotgun : 
                       if( (gamemode == doom2_commercial)
                            && player->weaponowned[wp_supershotgun])
                           newweapon = wp_supershotgun;
                       break;
                  case wp_fist :
                       if( player->weaponowned[wp_chainsaw])
                           newweapon = wp_chainsaw;
                       break;
                  default:
                       break;
               }
        }

        if (player->weaponowned[newweapon]
            && newweapon != player->readyweapon)
        {
            // Do not go to plasma or BFG in shareware,
            //  even if cheated.
            if ((newweapon != wp_plasma
                 && newweapon != wp_bfg)
                || (gamemode != doom_shareware) )
            {
                player->pendingweapon = newweapon;
            }
        }
    }

    // check for use
    if (cmd->buttons & BT_USE)
    {
        if( !(player->GB_flags & GB_usedown) )
        {
            P_UseLines (player);
            player->GB_flags |= GB_usedown;
        }
    }
    else
        player->GB_flags &= ~(GB_usedown);

    // Chicken counter
    if(player->chickenTics)
    {
        // Chicken attack counter
        if(player->chickenPeck)
            player->chickenPeck -= 3;
        // Attempt to undo the chicken
        if(!--player->chickenTics)
        {
            // Fixed to not change the player mobj.
            P_UndoPlayerChicken(player);
        }
    }

    // cycle psprites
    P_MovePsprites (player);
    // Counters, time dependend power ups.

    // Strength counts up to diminish fade.
    if (player->powers[pw_strength])
        player->powers[pw_strength]++;

    if (player->powers[pw_invulnerability])
        player->powers[pw_invulnerability]--;

    // the MF_SHADOW activates the tr_transhi translucency while it is set
    // (it doesnt use a preset value through FF_TRANSMASK)
    if (player->powers[pw_invisibility])
        if (! --player->powers[pw_invisibility] )
            pmo->flags &= ~MF_SHADOW;

    if (player->powers[pw_infrared])
        player->powers[pw_infrared]--;

    if (player->powers[pw_ironfeet])
        player->powers[pw_ironfeet]--;

    if (player->powers[pw_flight])
    {
        if(!--player->powers[pw_flight])
        {
/* HERETODO
            if(pmo->z != pmo->floorz)
                player->centering = true;
*/
            // timed out heretic fly power
            pmo->flags2 &= ~MF2_FLY;
            pmo->flags &= ~MF_NOGRAVITY;
           // BorderTopRefresh = true; //make sure the sprite's cleared out
        }
    }
    if(player->powers[pw_weaponlevel2])
    {
        if( --player->powers[pw_weaponlevel2] == 0 )
        {
            player->weaponinfo = wpnlev1info;
            // end of weaponlevel2 power
            if((player->readyweapon == wp_phoenixrod)
                && (player->psprites[PS_weapon].state
                != &states[S_PHOENIXREADY])
                && (player->psprites[PS_weapon].state
                != &states[S_PHOENIXUP]))
            {
                P_SetPsprite(player, PS_weapon, S_PHOENIXREADY);
                player->ammo[am_phoenixrod] -= USE_PHRD_AMMO_2;
                player->refire = 0;
            }
            else if((player->readyweapon == wp_gauntlets)
                || (player->readyweapon == wp_staff))
            {
                player->pendingweapon = player->readyweapon;
            }
            //BorderTopRefresh = true;
        }
    }

    if (player->damagecount)
        player->damagecount--;

    if (player->bonuscount)
        player->bonuscount--;

    if (player->key_pickup)
        player->key_pickup--;
    if (player->health_pickup)
        player->health_pickup--;
    if (player->armor_pickup)
        player->armor_pickup--;
    if (player->ammo_pickup)
        player->ammo_pickup--;
    if (player->weapon_pickup)
        player->weapon_pickup--;

    // Handling colormaps.
    if (player->powers[pw_invulnerability])
    {
        if (player->powers[pw_invulnerability] > BLINKTHRESHOLD
            || (player->powers[pw_invulnerability]&8) )
            player->fixedcolormap = INVERSECOLORMAP;
        else
            player->fixedcolormap = 0;
    }
    else if (player->powers[pw_infrared])
    {
        if (player->powers[pw_infrared] > BLINKTHRESHOLD
            || (player->powers[pw_infrared]&8) )
        {
            // almost full bright
            player->fixedcolormap = 1;
        }
        else
            player->fixedcolormap = 0;
    }
    else
        player->fixedcolormap = 0;
}

//----------------------------------------------------------------------------
//
// PROC P_PlayerNextArtifact
//
//----------------------------------------------------------------------------

void P_PlayerNextArtifact(player_t *player)
{
    player->inv_ptr--;
    if(player->inv_ptr < 6)
    {
        player->st_curpos--;
        if(player->st_curpos < 0)
            player->st_curpos = 0;
    }
    if(player->inv_ptr < 0)
    {
        player->inv_ptr = player->inventorySlotNum-1;
        if(player->inv_ptr < 6)
            player->st_curpos = player->inv_ptr;
        else
            player->st_curpos = 6;
    }
}

//----------------------------------------------------------------------------
//
// PROC P_PlayerRemoveArtifact
//
//----------------------------------------------------------------------------

static void P_PlayerRemoveArtifact(player_t *player, int slot)
{
    int i;
    
    if(!(--player->inventory[slot].count))
    { // Used last of a type - compact the artifact list
        player->inventory[slot].type = arti_none;
        for(i = slot+1; i < player->inventorySlotNum; i++)
            player->inventory[i-1] = player->inventory[i];
        player->inventorySlotNum--;

        // Set position markers and get next readyArtifact
        player->inv_ptr--;
        if(player->inv_ptr < 6)
        {
            player->st_curpos--;
            if( player->st_curpos < 0 )
                player->st_curpos = 0;
        }
        if( player->inv_ptr >= player->inventorySlotNum)
            player->inv_ptr = player->inventorySlotNum-1;
        if( player->inv_ptr < 0)
            player->inv_ptr = 0;
    }
}

//----------------------------------------------------------------------------
//
// PROC P_PlayerUseArtifact
//
//----------------------------------------------------------------------------

void P_PlayerUseArtifact(player_t *player, artitype_t arti)
{
    int i;
    
    for(i = 0; i < player->inventorySlotNum; i++)
    {
        if(player->inventory[i].type == arti)
        { // Found match - try to use
            if(P_UseArtifact(player, arti))
            { // Artifact was used - remove it from inventory
                P_PlayerRemoveArtifact(player, i);
                if(player == consoleplayer_ptr 
                   || player == displayplayer2_ptr ) // NULL when unused
                {
                    S_StartSound(sfx_artiuse);
                    H_ArtifactFlash = 4;
                }
            }
            else
            { // Unable to use artifact, advance pointer
                P_PlayerNextArtifact(player);
            }
            break;
        }
    }
}

//----------------------------------------------------------------------------
//
// PROC P_ArtiTele
//
//----------------------------------------------------------------------------

void P_ArtiTele(player_t *player)
{
    int i;
    fixed_t destX;
    fixed_t destY;
    angle_t destAngle;
    mapthing_t * mtp;
    
    if( deathmatch )
    {
        // Heretic use of P_Random
        i = PP_Random(ph_telearti) % numdmstarts;
        mtp = deathmatchstarts[i];
    }
    else
    {
        mtp = playerstarts[0];
    }

    destX = mtp->x<<FRACBITS;
    destY = mtp->y<<FRACBITS;
    destAngle = wad_to_angle(mtp->angle);
    P_Teleport_with_effects(player->mo, destX, destY, destAngle);
    S_StartSound(sfx_wpnup); // Full volume laugh
}


//----------------------------------------------------------------------------
//
// FUNC P_UseArtifact
//
// Returns true if artifact was used.
//
//----------------------------------------------------------------------------

boolean P_UseArtifact(player_t *player, artitype_t arti)
{
    mobj_t *mo;
    mobj_t * pmo = player->mo;
    angle_t angle;
    
    switch(arti)
    {
    case arti_invulnerability:
        if(!P_GivePower(player, pw_invulnerability))
            goto ret_fail;
        break;
    case arti_invisibility:
        if(!P_GivePower(player, pw_invisibility))
            goto ret_fail;
        break;
    case arti_health:
        if(!P_GiveHealth(player, 25))
            goto ret_fail;
        break;
    case arti_superhealth:
        if(!P_GiveHealth(player, 100))
            goto ret_fail;
        break;
    case arti_tomeofpower:
        if(player->chickenTics)
        { // Attempt to undo chicken
            // Fixed to not change the player mobj.
            if(P_UndoPlayerChicken(player) == false)
            { // Failed
                P_DamageMobj(pmo, NULL, NULL, 10000);
            }
            else
            { // Succeeded
                player->chickenTics = 0;
#ifdef XPEREMNTAL_HW3S
                S_StartScreamSound(pmo, sfx_wpnup);
#else
                S_StartObjSound(pmo, sfx_wpnup);
#endif
            }
        }
        else
        {
            if(!P_GivePower(player, pw_weaponlevel2))
                goto ret_fail;
            if(player->readyweapon == wp_staff)
            {
                P_SetPsprite(player, PS_weapon, S_STAFFREADY2_1);
            }
            else if(player->readyweapon == wp_gauntlets)
            {
                P_SetPsprite(player, PS_weapon, S_GAUNTLETREADY2_1);
            }
        }
        break;
    case arti_torch:
        if(!P_GivePower(player, pw_infrared))
            goto ret_fail;
        break;
    case arti_firebomb:
        angle = pmo->angle>>ANGLETOFINESHIFT;
        mo = P_SpawnMobj(pmo->x+24*finecosine[angle],
            pmo->y+24*finesine[angle], pmo->z - 15*FRACUNIT*
            ((pmo->flags2&MF2_FEETARECLIPPED) != 0), MT_FIREBOMB);
        mo->target = pmo;
        break;
    case arti_egg:
        P_SpawnPlayerMissile(pmo, MT_EGGFX);
        P_SPMAngle(pmo, MT_EGGFX, pmo->angle-(ANG45/6));
        P_SPMAngle(pmo, MT_EGGFX, pmo->angle+(ANG45/6));
        P_SPMAngle(pmo, MT_EGGFX, pmo->angle-(ANG45/3));
        P_SPMAngle(pmo, MT_EGGFX, pmo->angle+(ANG45/3));
        break;
    case arti_fly:
        if(!P_GivePower(player, pw_flight))
            goto ret_fail;
        break;
    case arti_teleport:
        P_ArtiTele(player);
        break;
    default:
        goto ret_fail;
    }
    return(true);

ret_fail:   
    return(false);
}


void P_SetPlayer_color( player_t * player, byte color )
{
    color = color % NUMSKINCOLORS;
    player->skincolor = color;

    // a copy of color, in proper position for use with color tables
    mobj_t *  pmo = player->mo;
    if( pmo )
        pmo->tflags = (pmo->tflags & ~MFT_TRANSLATION6) | (color << MFT_TRANSSHIFT);
}
