// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_telept.c 1736 2025-03-14 08:15:57Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2011 by DooM Legacy Team.
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
// $Log: p_telept.c,v $
// Revision 1.8  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.7  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.6  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.5  2000/11/04 16:23:43  bpereira
//
// Revision 1.4  2000/11/02 17:50:09  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Teleportation.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "g_game.h"
#include "r_state.h"
#include "r_main.h"
  //SoM: 3/16/2000
#include "s_sound.h"



// Doom Teleport of player and monsters, by linedef or artifact.
// Not used by new Boom Teleport lines.
byte P_Teleport_with_effects(mobj_t * thing, fixed_t x, fixed_t y, angle_t angle)
{
    mobj_t*     fog;
    fixed_t     oldx = thing->x;
    fixed_t     oldy = thing->y;
    fixed_t     oldz = thing->z;
    fixed_t     aboveFloor;
    fixed_t     fogDelta = 0;

    // no voodoo player effects
    player_t * player = (thing->player && thing->player->mo == thing) ?
          thing->player : NULL;

    if( EN_heretic && !(thing->flags&MF_MISSILE))
        fogDelta = TELEFOGHEIGHT;
    aboveFloor = thing->z - thing->floorz;

    if (!P_TeleportMove (thing, x, y, false))
        return 0;

    // Teleport success, has new position.
    // [WDJ] opt thing z
    thing->z = thing->floorz;  // default, may be modified
    if (player)
    {
        // heretic code
        if(player->powers[pw_flight] && aboveFloor)
        {
            thing->z += aboveFloor;
            if(thing->z > (thing->ceilingz - thing->height))  // limit
                thing->z = thing->ceilingz - thing->height;
        }
        player->viewz = thing->z+player->viewheight;
    }
    else if(thing->flags&MF_MISSILE) // heretic stuff
    {
        thing->z += aboveFloor;
        if(thing->z > (thing->ceilingz - thing->height))  // limit
            thing->z = thing->ceilingz - thing->height;
    }

    // spawn teleport fog at source and destination
    fog = P_SpawnMobj (oldx, oldy, oldz+fogDelta, MT_TFOG);
    S_StartObjSound(fog, sfx_telept);

    angle_t angf = ANGLE_TO_FINE(angle);  // fine angle, used later
    fog = P_SpawnMobj (x+20*finecosine[angf], y+20*finesine[angf],
          thing->z+fogDelta, MT_TFOG);

    // emit sound, where?
    S_StartObjSound(fog, sfx_telept);

    // don't move for a bit
    if (player)
    {
        if( !player->powers[pw_weaponlevel2] )  // not heretic
            thing->reactiontime = 18;
        // added : absolute angle position
        if(thing== consoleplayer_ptr->mo)
            localangle[0] = angle;
        if(displayplayer2_ptr && thing== displayplayer2_ptr->mo) // NULL when unused
            localangle[1] = angle;

#ifdef CLIENTPREDICTION2
        if(thing== consoleplayer_ptr->mo)
        {
            consoleplayer_ptr->spirit->reactiontime = thing->reactiontime;
            CL_ResetSpiritPosition(thing);
        }
#endif
        // [WDJ] kill bob momentum or player will keep bobbing for a while
        player->bob_momx = player->bob_momy = 0;

        // move chasecam at new player location
        if ( camera.chase == player )
            P_ResetCamera (player);
    }

    thing->angle = angle;
    if( EN_heretic
        && thing->flags2&MF2_FOOTCLIP
        && P_GetThingFloorType(thing) != FLOOR_SOLID )
    {
        thing->flags2 |= MF2_FEETARECLIPPED;
    }
    else if(thing->flags2&MF2_FEETARECLIPPED)
    {
        thing->flags2 &= ~MF2_FEETARECLIPPED;
    }
    if(thing->flags&MF_MISSILE)
    {
#ifdef MONSTER_VARY
        thing->momx = FixedMul(thing->speed, finecosine[angf]);
        thing->momy = FixedMul(thing->speed, finesine[angf]);
#else
        thing->momx = FixedMul(thing->info->speed, finecosine[angf]);
        thing->momy = FixedMul(thing->info->speed, finesine[angf]);
#endif
    }
    else
        thing->momx = thing->momy = thing->momz = 0;

    return 1;
}

// =========================================================================
//                            TELEPORTATION
// =========================================================================
#ifdef ENABLE_TELE_CONTROL
uint16_t teleport_delay = 0;

#include "m_random.h"


// Will teleport to new location.
static mobj_t *  another_monster( mobj_t * mon )
{
    mobj_t * mt = P_SpawnMobj( mon->x, mon->y, mon->z, mon->type );
    if( mt )
    {
        mt->state = mon->state;
    }

    return mt;
}


// auto method, randomly selected
// indexed by [ cv_tele_control - 1, random ]
static
byte  monster_tele_auto_table[ 5][ 8] =
{
    {  7, 7, 8, 8, 10, 11, 11, 13 },  // easy2
    {  6, 7, 8, 9, 10, 11, 12, 13 },  // easy1
    {  6, 7, 9, 10, 11, 12, 14, 16 },  // any
    {  9, 0, 12, 13, 16, 15, 16, 17 },  // hard1
    { 13, 14, 15, 15, 16, 17, 18, 18 },  // hard2
};

// seconds of delay
// indexed by cv_telemonster, 6 to 18
static
byte  tele_delay[ 18 - 6 + 1 ] = {
    // 6
  2 * TICRATE/4, // stun2
  4 * TICRATE/4, // stun4
  6 * TICRATE/4, // stun6
    // 9
  2 * TICRATE/4, // rate2
  4 * TICRATE/4, // rate4
  6 * TICRATE/4, // rate6
    // 12
  3 * TICRATE/4, // scatter1
  6 * TICRATE/4, // scatter2
  5 * TICRATE/4, // scatter3
  4 * TICRATE/4, // scatter4
   // 16
  5 * TICRATE/4, // dup25
  3 * TICRATE/4, // dup40
  2 * TICRATE/4, // dup60
};

// Lookup in the table above.  So to not overflow byte when larger TICRATE.
static inline unsigned int  tele_delay_lookup( byte method )
{
    // x4 table value
    return ((unsigned int)tele_delay[ method - 6 ]) << 2;
}

// X+Y tele adj distance multiplier
// indexed by cv_telemonster, 12 to 18
static
byte  tele_dist[ 18 - 12 + 1 ] = {
   90, // scatter1
  120, // scatter2
  190, // scatter3
  252, // scatter4
  120, // dup25
  140, // dup40
  160, // dup60
};

// Lookup in the table above.
static inline fixed_t  tele_dist_lookup( byte method )
{
    // x2 of table value
    return ((unsigned int)tele_dist[ method - 12 ]) << (FRACBITS+1);
}

// indexed by cv_telemonster, 16 to 18
// More than 50% can create many monsters.
static
byte  tele_dup_per[ 18 - 16 + 1 ] = {
  (255*25/100),  // dup25
  (255*40/100),  // dup40
  (255*60/100),  // dup60
};

// extern
byte monster_scatter_xy( mobj_t * mon, fixed_t rel_dist, byte walkable_req, /*OUT*/ xyr_t * new_dest );

// return
//   0: normal teleport
//   1: when adjusted sector
//   9: when adjustment refuses to teleport the monster.
byte adjust_monster_teleport( mobj_t * mon, sector_t * secp, /*OUT*/ xyr_t * tele_dest )
{
    byte rv = 0;
    byte method;

    if( (!mon) || (mon->player))
        return 0; // not for players, nor zombies

    method = cv_tele_control.EV;  // caller has tested for this being > 0
    if( method >= 1 && method <= 5 ) // auto
    {
        method = monster_tele_auto_table[ method - 1][ PP_Random( pL_tele ) & 0x07 ]; // 0 to 7
    }

    if( method >= 12 )  goto scatter_search;
    if( method >= 9 )   goto teleport_with_delay;
    if( method >= 6 )   goto reaction_time;

    return 0; // normal teleport


scatter_search:  // and dup
    // scatter and dup are not appropriate for line-to-line teleport
    if( ! tele_dest )
        goto teleport_with_delay;

    fixed_t dist = tele_dist_lookup( method );

    rv = monster_scatter_xy( mon, dist, 1, /*OUT*/ tele_dest );  // random

    if( rv == 2 )
        goto teleport_with_delay;

    if( method >= 16 )  // dup
    {
#if 0
        if( ! mon->spawnpoint )  // monster not from previous dup
        {
 printf( "Teleport dup teleported\n" );
        }
#endif	

        // If mon is teleported, then dup another in its place.
        if( mon->spawnpoint
            && (PP_Random( pL_tele ) < tele_dup_per[ method - 16 ]) )
        {
            another_monster( mon );  // duplicate monster
            unsigned int delay_param = tele_delay_lookup( method );
            if( teleport_delay < delay_param )  teleport_delay = delay_param;

// printf( "Teleport dup with delay %i sec\n", delay_param / TICRATE );
// mon->reactiontime = 10*delay_param;  // DEBUG
        }
    }

    return rv;
    
reaction_time:
    mon->reactiontime = tele_delay_lookup( method );
    if( tele_dest )
        tele_dest->angle = (((unsigned int)PP_Random( pL_tele )) << 24);
// printf( "Teleport reaction time set to %i sec, angle %i\n", mon->reactiontime / TICRATE, ((unsigned int)tele_dest->angle)>>29 );
    return 1;

teleport_with_delay:
    if( teleport_delay )
    {
// printf( "Teleport delayed by %i sec\n", teleport_delay / TICRATE );
        return 9;
    }
    teleport_delay = tele_delay_lookup( method );
// printf( "Teleport set delay to %i sec\n", teleport_delay / TICRATE );
    return 1;
}
#endif


// Doom Teleport line and switch.
// Called by P_CrossSpecialLine, for players and monsters.
// Called by P_UseSpecialLine, for players and monsters.
// Called by SF_Teleport, for whatever mobj may be.
int EV_Teleport ( line_t*       line,
                  int           side,
                  mobj_t*       thing )
{
    int         tag;
    mobj_t*     m;
    thinker_t*  thinker;
    sector_t*   sector;

    // don't teleport missiles
    if ( (EN_doom_etc && (thing->flags & MF_MISSILE)) 
        || (thing->flags2 & MF2_NOTELEPORT) )  // heretic flag
        goto no_teleport;

    // Don't teleport if hit back of line,
    //  so you can get out of teleporter.
    if (side == 1)
        goto no_teleport;


    tag = line->tag;
    uint32_t i;
    for (i = 0; i < numsectors; i++)
    {
        sector = &sectors[ i ];
        if (sector->tag == tag )
        {
            // Find first MT_TELEPORTMAN in this sector.
            thinker = thinkercap.next;
            for (thinker = thinkercap.next;
                 thinker != &thinkercap;
                 thinker = thinker->next)
            {
                // not a mobj
                if (thinker->function != TFI_MobjThinker)
                    continue;

                m = (mobj_t *)thinker;

                // not a teleportman
                if (m->type != MT_TELEPORTMAN )
                    continue;

                // wrong sector
                if ( m->subsector->sector != sector )
                    continue;

                // Found a sector, end search.
#ifdef ENABLE_TELE_CONTROL
                xyr_t  tele_dest;
                tele_dest.x = m->x;
                tele_dest.y = m->y;
                tele_dest.angle = m->angle;
                if( cv_tele_control.EV && ! thing->player )
                {
                    // alter, or choose new dest
                    byte amt = adjust_monster_teleport( thing, sector, /*OUT*/ &tele_dest );
                    if( amt >= 9 )
                       goto no_teleport;  // do not teleport
                }
// printf( "Tele m (%i, %i, %x),  tele_dest (%i, %i, %x)\n", m->x, m->y, m->angle, tele_dest.x, tele_dest.y, tele_dest.angle );
                return P_Teleport_with_effects(thing, tele_dest.x, tele_dest.y, tele_dest.angle);
#else		
                return P_Teleport_with_effects(thing, m->x, m->y, m->angle);
#endif
            }
        }
    }

no_teleport:
    return 0;
}




/*
  SoM: 3/15/2000
  Added new boom teleporting functions.
*/
// Boom linedef to tagged sector teleport
int EV_SilentTeleport(line_t * line, int side, mobj_t * thing)
{
  int       i;
  thinker_t * th;

  // don't teleport missiles
  // Don't teleport if hit back of line,
  // so you can get out of teleporter.

  if (side || thing->flags & MF_MISSILE)
      goto no_teleport;

  for (i = -1; (i = P_FindSectorFromLineTag(line, i)) >= 0;)
  {
    // Find first MT_TELEPORTMAN in this sector.
    sector_t* sector = &sectors[ i ];
    for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      if (th->function == TFI_MobjThinker )
      {
          mobj_t * m = (mobj_t *) th;
          if( m->type != MT_TELEPORTMAN )  continue;

          if( m->subsector->sector != sector )  continue;

          // Height of thing above ground, in case of mid-air teleports:
          fixed_t z = thing->z - thing->floorz;

          // Get the angle between the exit thing and source linedef.
          // Rotate 90 degrees, so that walking perpendicularly across
          // teleporter linedef causes thing to exit in the direction
          // indicated by the exit thing.
          angle_t angle =
            R_PointToAngle2(0, 0, line->dx, line->dy) - m->angle + ANG90;

          // Sine, cosine of angle adjustment
          fixed_t sin_taa = sine_ANG(angle);
          fixed_t cos_taa = cosine_ANG(angle);

          // Momentum of thing crossing teleporter linedef
          fixed_t momx = thing->momx;
          fixed_t momy = thing->momy;

          // Whether this is a player, and if so, a pointer to its player_t
          player_t * player = thing->player;

          // Found a sector, end search.
#ifdef ENABLE_TELE_CONTROL
          xyr_t  tele_dest;
          tele_dest.x = m->x;
          tele_dest.y = m->y;
          if( cv_tele_control.EV && ! player )
          {
              // alter, or choose new dest
              byte amt = adjust_monster_teleport( thing, m->subsector->sector, /*OUT*/ &tele_dest );
              if( amt >= 9 )
                  goto no_teleport;  // do not teleport now
          }

          // Attempt to teleport, aborting if blocked
          if (!P_TeleportMove(thing, tele_dest.x, tele_dest.y, false))
              goto no_teleport;
#else
          // Attempt to teleport, aborting if blocked
          if (!P_TeleportMove(thing, m->x, m->y, false))
              goto no_teleport;
#endif

          // Rotate thing according to difference in angles
          thing->angle += angle;

          // Adjust z position to be same height above ground as before
          thing->z = z + thing->floorz;

          // Rotate thing's momentum to come out of exit just like it entered
          thing->momx = FixedMul(momx, cos_taa) - FixedMul(momy, sin_taa);
          thing->momy = FixedMul(momy, cos_taa) + FixedMul(momx, sin_taa);

          // Adjust player's view, in case there has been a height change
          // Voodoo dolls are excluded by making sure player->mo == thing.
          if (player && player->mo == thing)
          {
              // Save the current deltaviewheight, used in stepping
              fixed_t deltaviewheight = player->deltaviewheight;

              // Clear deltaviewheight, since we don't want any changes
              player->deltaviewheight = 0;

              // Set player's view according to the newly set parameters
              P_CalcHeight(player);

              // Reset the delta to have the same dynamics as before
              player->deltaviewheight = deltaviewheight;

              // SoM: 3/15/2000: move chasecam at new player location
              if ( camera.chase == player )
                 P_ResetCamera (player);

          }
          return 1;
      }
    }
  }

no_teleport:
  return 0;
}

//
// Silent linedef-based TELEPORTATION, by Lee Killough
// Primarily for rooms-over-rooms etc.
// This is the complete player-preserving kind of teleporter.
// It has advantages over the teleporter with thing exits.
//

// maximum fixed_t units to move object to avoid hiccups
#define FUDGEFACTOR 10

// Boom linedef to linedef teleport
int EV_SilentLineTeleport(line_t * line, int side, mobj_t * thing,
                          boolean reverse)
{
  int i;
  line_t *l;

  if (side || thing->flags & MF_MISSILE)
    return 0;

  for (i = -1; (i = P_FindLineFromLineTag(line, i)) >= 0;)
  {
    if ((l=lines+i) != line && l->backsector)
    {
        // Get the thing's position along the source linedef
        fixed_t pos = abs(line->dx) > abs(line->dy) ?
          FixedDiv(thing->x - line->v1->x, line->dx) :
          FixedDiv(thing->y - line->v1->y, line->dy) ;

        // Get the angle between the two linedefs, for rotating
        // orientation and momentum. Rotate 180 degrees, and flip
        // the position across the exit linedef, if reversed.
        angle_t angle = (reverse ? pos = FRACUNIT-pos, 0 : ANG180) +
          R_PointToAngle2(0, 0, l->dx, l->dy) -
          R_PointToAngle2(0, 0, line->dx, line->dy);

        // Interpolate position across the exit linedef
        fixed_t x = l->v2->x - FixedMul(pos, l->dx);
        fixed_t y = l->v2->y - FixedMul(pos, l->dy);

        // Sine, cosine of angle adjustment
        fixed_t sin_taa = sine_ANG(angle);
        fixed_t cos_taa = cosine_ANG(angle);

        // Maximum distance thing can be moved away from interpolated
        // exit, to ensure that it is on the correct side of exit linedef
        int fudge = FUDGEFACTOR;

        // Whether this is a player, and if so, a pointer to its player_t.
        // Voodoo dolls are excluded by making sure thing->player->mo==thing.
        player_t * player = (thing->player && thing->player->mo == thing) ?
          thing->player : NULL;

        // Whether walking towards first side of exit linedef steps down
        int stepdown =
          l->frontsector->floorheight < l->backsector->floorheight;

        // Height of thing above ground
        fixed_t z = thing->z - thing->floorz;

        // Side to exit the linedef on positionally.
        //
        // Notes:
        //
        // This flag concerns exit position, not momentum. Due to
        // roundoff error, the thing can land on either the left or
        // the right side of the exit linedef, and steps must be
        // taken to make sure it does not end up on the wrong side.
        //
        // Exit momentum is always towards side 1 in a reversed
        // teleporter, and always towards side 0 otherwise.
        //
        // Exiting positionally on side 1 is always safe, as far
        // as avoiding oscillations and stuck-in-wall problems,
        // but may not be optimum for non-reversed teleporters.
        //
        // Exiting on side 0 can cause oscillations if momentum
        // is towards side 1, as it is with reversed teleporters.
        //
        // Exiting on side 1 slightly improves player viewing
        // when going down a step on a non-reversed teleporter.

        int side = reverse || (player && stepdown);

        // Make sure we are on correct side of exit linedef.
        while (P_PointOnLineSide(x, y, l) != side && --fudge>=0)
        {
          if (abs(l->dx) > abs(l->dy))
            y -= ((l->dx < 0) != side) ? -1 : 1;
          else
            x += ((l->dy < 0) != side) ? -1 : 1;
        }

#ifdef ENABLE_TELE_CONTROL
        if( cv_tele_control.EV && ! player )
        {
            byte rv = adjust_monster_teleport( thing, NULL, NULL ); // modify dest
            if( rv >= 9 )
                return 0;  // do not teleport
        }
#endif

        // Attempt to teleport, aborting if blocked
        if (!P_TeleportMove(thing, x, y, false))
          return 0;

        // Adjust z position to be same height above ground as before.
        // Ground level at the exit is measured as the higher of the
        // two floor heights at the exit linedef.
        thing->z = z + sides[l->sidenum[stepdown]].sector->floorheight;

        // Rotate thing's orientation according to difference in linedef angles
        thing->angle += angle;

        // Momentum of thing crossing teleporter linedef
        x = thing->momx;
        y = thing->momy;

        // Rotate thing's momentum to come out of exit just like it entered
        thing->momx = FixedMul(x, cos_taa) - FixedMul(y, sin_taa);
        thing->momy = FixedMul(y, cos_taa) + FixedMul(x, sin_taa);

        // Adjust a player's view, in case there has been a height change
        if (player)
        {
            // Save the current deltaviewheight, used in stepping
            fixed_t deltaviewheight = player->deltaviewheight;

            // Clear deltaviewheight, since we don't want any changes now
            player->deltaviewheight = 0;

            // Set player's view according to the newly set parameters
            P_CalcHeight(player);

            // Reset the delta to have the same dynamics as before
            player->deltaviewheight = deltaviewheight;

            // SoM: 3/15/2000: move chasecam at new player location
            if ( camera.chase == player )
               P_ResetCamera (player);
        }

        return 1;
    }
  }
  return 0;
}

