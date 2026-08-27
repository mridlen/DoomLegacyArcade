// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_map.c 1721 2025-01-28 15:15:00Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2012 by DooM Legacy Team.
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
// $Log: p_map.c,v $
// Revision 1.29  2005/12/20 14:58:26  darkwolf95
// Monster behavior CVAR - Affects how monsters react when they shoot each other
//
// Revision 1.28  2004/01/09 01:22:20  darkwolf95
// bug fix: stop checking non-solids against other things; was responsible for corona movement bug in bug wad
//
// Revision 1.27  2003/11/21 17:52:05  darkwolf95
// added "Monsters Infight" for Dehacked patches
//
// Revision 1.26  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.25  2001/08/07 00:53:33  hurdler
//
// Revision 1.24  2001/08/06 23:57:09  stroggonmeth
// Removed portal code, improved 3D floors in hardware mode.
//
// Revision 1.23  2001/07/28 16:18:37  bpereira
// Revision 1.22  2001/06/16 08:07:55  bpereira
// Revision 1.21  2001/05/27 13:42:47  bpereira
//
// Revision 1.20  2001/04/30 17:19:24  stroggonmeth
// HW fix and misc. changes
//
// Revision 1.19  2001/04/01 17:35:06  bpereira
// Revision 1.18  2001/03/30 17:12:50  bpereira
// Revision 1.17  2001/03/19 18:52:01  hurdler
//
// Revision 1.16  2001/03/13 22:14:19  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.15  2001/03/09 21:53:56  metzgermeister
//
// Revision 1.14  2001/01/25 22:15:43  bpereira
// added heretic support
//
// Revision 1.13  2000/11/02 19:49:35  bpereira
//
// Revision 1.12  2000/11/02 17:50:08  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.11  2000/10/21 08:43:30  bpereira
// Revision 1.10  2000/10/01 10:18:17  bpereira
// Revision 1.9  2000/09/28 20:57:16  bpereira
// Revision 1.8  2000/08/31 14:30:55  bpereira
// Revision 1.7  2000/04/23 16:19:52  bpereira
// Revision 1.6  2000/04/16 18:38:07  bpereira
// Revision 1.5  2000/04/15 22:12:57  stroggonmeth
//
// Revision 1.4  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.3  2000/04/04 00:32:47  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Movement, collision handling.
//      Shooting and aiming.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "g_game.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_inter.h"
#include "r_state.h"
#include "r_main.h"
#include "r_sky.h"
#include "s_sound.h"
#include "infoext.h"

#include "r_splats.h"
  //faB: testing

#include "z_zone.h"
  //SoM: 3/15/2000


// Replacement for PRBOOM_OOF_2S
#define OOF_2S

// TryMove, thing map global vars
fixed_t         tm_bbox[4];	// box around the thing
mobj_t        * tm_thing;	// the thing itself
uint32_t        tm_flags;	// thing flags of tm_thing
fixed_t         tm_x, tm_y;	// thing map position
static byte     tm_mbf_unstuck; // mbf player unstick

// TryMove, thing map return global vars
byte            tmr_floatok;    // If true, move would be ok
                                // if within "tmfloorz - tmceilingz".
byte            tmr_felldown;   // MBF, went off deep dropoff
fixed_t         tmr_floorz;	// floor and ceiling of new position
fixed_t         tmr_ceilingz;
fixed_t         tmr_dropoffz;   // the lowest point contacted (monster check)

// [WDJ] tmr_floorthing is in demoversion 113..131, otherwise NULL
mobj_t        * tmr_floorthing; // standing on another thing
                                // the thing corresponding to tmr_floorz
                                // or NULL if tmr_floorz is from a sector

//added:28-02-98: used at P_ThingHeightClip() for moving sectors
fixed_t         tmr_sectorfloorz;
fixed_t         tmr_sectorceilingz;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t        * tmr_ceilingline;
line_t        * tmr_floorline;  // MBF: highest touched floor

// set by PIT_CheckLine() for any line that stopped the PIT_CheckLine()
// that is, for any line which is 'solid'
line_t        * tmr_blockingline;
line_t        * tmr_dropoffline;
sector_t      * tmr_sector;  // for bots

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
int           * spechit = NULL;                //SoM: 3/15/2000: Limit removal
                // realloc, never deallocated
int             numspechit = 0;

//[WDJ] Attempt to track player that triggers voodoo inflicted damage
player_t *      spechit_player = NULL;

//SoM: 3/15/2000
msecnode_t * sector_list = NULL;

// [WDJ] only used in PIT_CrossLine (line_t* ld)
//SoM: 3/15/2000, [WDJ] modified for general usage
// In Lost Soul checks, from_ is Pain Elemental position, targ_ is Lost Soul spawn
static int from_x, from_y;
static int targ_x, targ_y;



#ifdef OOF_2S
// [WDJ] only used in P_UseLines
static byte     oof_2s_arm = 0x80;  // 0 when enabled, 0x80 when disabled
static byte     oof_2s_hit;     // hit 2s line, replaces PrBoom 2s noway test
#endif

extern byte  EV_blood_vanilla;  // set in DemoAdapt_p_mobj due to dependency

void DemoAdapt_p_map( void )
{
#ifdef OOF_2S
    oof_2s_arm = (cv_oof_2s.EV && EN_boom)? 0 : 0x80;  // 0 is armed, 0x80 is disabled
#endif
    // EV_blood_vanilla is set in DemoAdapt_p_mobj due to dependency.
}


//
// TELEPORT MOVE
//

// Parameter to Stomp
static byte  tm_stomp_telefrag;  // MBF: enables telefrag

//
// PIT_StompThing
//
// Return false if something in the way, and cannot Stomp it.
static boolean PIT_StompThing (mobj_t* thing)
{
    fixed_t     blockdist;

    //SoM: 3/15/2000: Move self check to start of routine.

    if (thing == tm_thing)     // don't clip against self
        return true;

    if (!(thing->flags & MF_SHOOTABLE) )
        return true;

    blockdist = thing->radius + tm_thing->radius;

    if ( abs(thing->x - tm_x) >= blockdist || abs(thing->y - tm_y) >= blockdist )
        return true;        // didn't hit it

    // monsters don't stomp things except on boss level
    // Heretic: telestomp included in tm_stomp_telefrag
#ifdef MBF21
    // MBF21: MI_ALLOW_MONSTER_TELEFRAGS included in tm_stomp_telefrag
#endif
    if( !tm_stomp_telefrag ) // killough 8/9/98: make consistent across all levels
        return false;

    int  damage = 10000;  // fatal
    if( (tm_thing->player && (tm_thing->player->mo != tm_thing))
        && (thing->player && (thing->player->mo != thing)) )
    {
        // [WDJ] Insta-death by teleporting a voodoo doll onto another one.
        // Used by some wads.
        // Once a voodoo teleport instadeath trap is triggered by one player,
        // most wads cannot reset, and other players can then walk through the trap.
        // On cv_instadeath=damage or zap, the same player can trip it and then
        // walk through it on the second try.  Best we can do.
        if( voodoo_mode != VM_vanilla )
        {
            // Original Doom target is thing->player
            if( voodoo_mode >= VM_target )
            {
                // Because the wad decouples the trigger from the effect, there
                // is no good way to determine which player should be killed.
                // Target last player to trigger a switch or linedef.
                if( spechit_player && spechit_player->mo)
                    thing = spechit_player->mo;   // redirect the damage
            }
            // knock player back
            thing->momx *= -32;
            thing->momy *= -32;
            thing->momz += -8;
        }
        switch( cv_instadeath.EV )
        {
         // 0: default .. DIE
         case 1: // Damage instead
           damage = 20;
           thing->reactiontime = 36;  // stun them
           break;
         case 2: // Zap instead
           damage = 5;
           thing->reactiontime = 72;  // stun them better
           break;
         default:
           // Block damage in deathmatch, it would always kill player1
           if( deathmatch )
              damage = 0;
        }
    }
    P_DamageMobj (thing, tm_thing, tm_thing, damage);

    return true;
}


// [WDJ] 3/2011 Get sector oriented friction.
// This routine is based on one from prboom, by killough 8/28/98.
// For most sectors it returns ORIG_FRICTION.
// Return lowest value (most sticky friction).
// A high return value is very little friction (ice).
// It also considers being on sector borders and sector heights.

fixed_t  got_friction;
int    got_movefactor;  // return values

fixed_t  P_GetFriction( const mobj_t * mo )
{
    // [WDJ] Most ports test (sec->special & FRICTION_MASK) as a friction enable.
    // This misses the normal sectors that could affect the edge of ice sheets.
    // This implementation uses the sector friction fields for all sectors,
    // and updates whenever sec->special is changed.  That is several fewer
    // tests in this heavily used code.
    got_movefactor = ORIG_FRICTION_FACTOR;

    if( EN_variable_friction
        && (EN_mbf || (EN_boom && mo->player))
        && !(mo->flags & (MF_NOCLIP|MF_NOGRAVITY)) )
    {
        // Boom and MBF only
        fixed_t mo_top = mo->z + mo->height;
        const msecnode_t * msnp = mo->touching_sectorlist;
        const sector_t * secp;
        const ffloor_t * fff;

        got_friction = FIXED_MAX;  // init search
        // traverse the list of sectors touching this thing
        while( msnp )
        {
            secp = msnp->m_sector;
            // DoomLegacy does not use FRICTION_MASK, see note above.
            // Uses model and modelsec, instead of the PrBoom heightsec.
            // Sector friction is set at special sector setup.
            // Init search at FIXED_MAX instead of ORIG_FRICTION,
            // so do not need test for "got_friction == ORIG_FRICTION".
            // However, Boom deep water setup cannot fix test for floorheight.
            if( (mo->z <= secp->floorheight)
                || ( EN_mbf
                     && (secp->model > SM_fluid)  // check for water
                     && mo->z <= sectors[secp->modelsec].floorheight
                   ) )
            {
                // on sector floor
                if( secp->friction < got_friction )
                {
                    got_friction = secp->friction;
                    got_movefactor = secp->movefactor;
                }
            }

            // also check if any 3d floor has friction
            for(fff = secp->ffloors; fff; fff = fff->next)
            {
                if(mo->z <= *fff->topheight && mo_top > *fff->bottomheight )
                {
                    // on 3d floor (or embedded in it)
                    sector_t * fffsec = &sectors[fff->model_secnum];
                    if(fffsec->friction < got_friction)
                    {
                        got_friction = fffsec->friction;
                        got_movefactor = fffsec->movefactor;
                    }
                }
            }
            msnp = msnp->m_tnext;
        }

        // DoomLegacy also supports floating.
        if( got_friction == FIXED_MAX )
        {
            got_friction = 0xFFF0;  // must not be touching floor, air friction
        }
    }
    else
    {
        got_friction = FRICTION_NORM;
    }
    return got_friction;
}



//SoM: 3/15/2000
// P_GetMoveFactor() returns the value by which the x,y
// movements are multiplied to add to player movement.

// mud factor based on momentum
#define MORE_FRICTION_MOMENTUM 15000

int  P_GetMoveFactor(mobj_t* mo)
{
  // If the floor is icy or muddy, it's harder to get moving. This is where
  // the different friction factors are applied to 'trying to move'. In
  // p_mobj.c, the friction factors are applied as you coast and slow down.

#ifdef FRICTIONTHINKER
  if ( friction_model >= FR_mbf )   // MBF and newer
#else   
  if ( friction_model >= FR_boom )  // MBF, and Boom friction (NOT using friction thinker)
#endif
  {
      // EN_mbf
      // modern friction model, introduced in MBF
      P_GetFriction( mo );  // sets got_friction and got_movefactor
      if( got_friction < ORIG_FRICTION   // muddy, sludge
          && (got_movefactor < (ORIG_FRICTION_FACTOR/4)))  // not water
      {
          // phares 3/11/98: you start off slowly, then increase as
          // you get better footing
          
          int momentum = (P_AproxDistance(mo->momx,mo->momy));
          if (momentum > MORE_FRICTION_MOMENTUM<<2)
              got_movefactor <<= 3;

          else if (momentum > MORE_FRICTION_MOMENTUM<<1)
              got_movefactor <<= 2;
          
          else if (momentum > MORE_FRICTION_MOMENTUM)
              got_movefactor <<= 1;
      }
      
  }
#ifdef FRICTIONTHINKER
  // older demo friction
  else if( EN_boom_friction_thinker
      && !(mo->flags & (MF_NOGRAVITY | MF_NOCLIP)) )
  {
      // Boom friction, using friction thinker
      got_friction = mo->friction;
      got_movefactor = ORIG_FRICTION_FACTOR;
      if (got_friction == ORIG_FRICTION)            // normal floor
          ;
      else if (got_friction > ORIG_FRICTION)        // ice
      {
          got_movefactor = mo->movefactor;
          mo->movefactor = ORIG_FRICTION_FACTOR;  // reset
      }
      else                                      // sludge
      {
          got_movefactor = mo->movefactor;
          mo->movefactor = ORIG_FRICTION_FACTOR;  // reset

          if(got_movefactor < (ORIG_FRICTION_FACTOR/4)) // not water
          {
              // phares 3/11/98: you start off slowly, then increase as
              // you get better footing
          
              int momentum = (P_AproxDistance(mo->momx,mo->momy));
              if (momentum > MORE_FRICTION_MOMENTUM<<2)
                 got_movefactor <<= 3;
          
              else if (momentum > MORE_FRICTION_MOMENTUM<<1)
                 got_movefactor <<= 2;
          
              else if (momentum > MORE_FRICTION_MOMENTUM)
                 got_movefactor <<= 1;
          }
      }
  }
#endif
  else
  {
      // Vanilla friction
      got_friction = ORIG_FRICTION;
      got_movefactor = ORIG_FRICTION_FACTOR;
  }
  return got_movefactor;
}

//
// P_TeleportMove
//
boolean P_TeleportMove( mobj_t* thing, fixed_t x, fixed_t y, byte stomp )
{
    int xl, xh;
    int yl, yh;
    int bx, by;

    subsector_t*        tele_newsubsec;

    // kill anything occupying the position
    tm_thing = thing;
    tm_flags = thing->flags;

    tm_x = x;
    tm_y = y;

    tm_bbox[BOXTOP] = y + tm_thing->radius;
    tm_bbox[BOXBOTTOM] = y - tm_thing->radius;
    tm_bbox[BOXRIGHT] = x + tm_thing->radius;
    tm_bbox[BOXLEFT] = x - tm_thing->radius;

    tele_newsubsec = R_PointInSubsector (x,y);
    tmr_ceilingline = NULL;

    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any lines contacted during the step
    // may adjust them closer together.
    tmr_floorz = tmr_dropoffz = tele_newsubsec->sector->floorheight;
    tmr_ceilingz = tele_newsubsec->sector->ceilingheight;

    validcount++;
    numspechit = 0;

    // [WDJ] MBF, combined telefrag tests.
    if( EN_doom_etc )
    {
        // MBF: Enable stomp for bosses too.
        tm_stomp_telefrag = (tm_thing->player != NULL)
            || ( // monsters don't stomp things except on boss level
                 EN_mbf_telefrag ? stomp : (gamemap == 30) );
    }
    else if( EN_heretic )
    {
        // Some are allowed to stomp things.
        tm_stomp_telefrag = (tm_thing->flags2 & MF2_TELESTOMP);
    }

    // stomp on any things contacted
    xl = (tm_bbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (tm_bbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (tm_bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (tm_bbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
    {
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
                return false;
    }

    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    thing->floorz = tmr_floorz;
    thing->ceilingz = tmr_ceilingz;
    thing->dropoffz = tmr_dropoffz;  // MBF
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (thing);

    return true;
}


// =========================================================================
//                       MOVEMENT ITERATOR FUNCTIONS
// =========================================================================



static void add_spechit( line_t* ld )
{
    static int spechit_max = 0;

    //SoM: 3/15/2000: Boom limit removal.
    if (numspechit >= spechit_max)
    {
        spechit_max = spechit_max ? spechit_max*2 : 16;
        spechit = (int *)realloc(spechit,sizeof(int)*spechit_max);
    }
    
    spechit[numspechit] = ld - lines;
    numspechit++;
}


#ifdef MBF21
// Return 1 when target is immune to source projectiles
static
byte mbf21_projectile_immune( mobj_t* target, mobj_t* source )
{
    byte tar_group = target->info->projectile_group;

#if 1
    // This is the same result as the original.
    // By tar_group first, as the cases are mutually exclusive.
    return (tar_group == PG_DEFAULT) ? (source->type == target->type)  // immune from same type
      // (PG_GROUPLESS) PG_NO_IMMUNITY means no immunity, even from own type.
      : (tar_group == PG_NO_IMMUNITY) ? (target == source)  // immune from self only
        : (tar_group == source->info->projectile_group);  // immune from group
#endif
#if 0
    // Rewritten
    return (
        // (PG_GROUPLESS) PG_NO_IMMUNITY means no immunity, even from own type.
        ((tar_group != PG_NO_IMMUNITY) || (target == source))
        && (
            (tar_group == PG_DEFAULT) ?
                // target default projectile behavior, when same type
	          (source->type == target->type)
	        // target special projectile behavior, when same group
                : (tar_group == mobjinfo[source->type].projectile_group)
           )
        );
#endif    
#if 0
    // As originally written
    uint32_t tar_group = mobjinfo[ target->type ].projectile_group;
    return (
            // (PG_GROUPLESS) PG_NO_IMMUNITY means no immunity, even from own type.
            ((tar_group != PG_NO_IMMUNITY) || (target == source))
            && (
                // target default projectile behavior, when same type
                ((tar_group == PG_DEFAULT) && (source->type == target->type))
                // target special projectile behavior, when same group
                || ((tar_group != PG_DEFAULT) && (tar_group == mobjinfo[source->type].projectile_group))
                )
            );
#endif	    
}
#endif

#ifdef HEXEN
//  mt : attacking LIGHTENING
static void  hexen_set_indirect_lightening_target( mobj_t * mt )
{
    if( mt->type == MT_HEXEN_LIGHTINGING_FLOOR )
    {
        if( mt->special2.m
            &&  ! mt->special2.m->special1.m )
        {
            P_SetTarget( & mt->special2.m->special1.m, thing );
        }
    }
    else if( ! mt->special1.m )
    {
        P_SetTarget( & mt->special1.m, thing );
    }
}
#endif


//
// [Arcade] Monster height model.
//
// Vanilla Doom things are infinitely tall: a thing blocks another thing
// whatever their relative heights, so a monster in a doorway or on a ledge
// blocks it completely and you can never stand on one.  Legacy adopted
// Heretic's over-under passing for the Doom player unconditionally --
// MT_PLAYER carries MF2_PASSMOBJ in its info entry (info.c) and PIT_CheckThing
// below is gated on nothing else -- so the player could climb on top of
// monsters and get stuck in places vanilla cannot reach, notably the lift by
// the E1M2 exit.  There was no setting for it either way.
//
// "Infinite" is the default because it is what Doom maps were built and
// played against.  Heretic is unaffected: over-under is core to its own
// monsters (imps and wizards fly, and their info entries carry MF2_PASSMOBJ),
// so this only governs Doom.
//
CV_PossibleValue_t monsterheight_cons_t[] = {
   {0, "Over-Under"},
   {1, "Infinite"},
   {0, NULL}
};

consvar_t cv_tall_monsters =
  {"tallmonsters", "1", CV_NETVAR | CV_SAVE, monsterheight_cons_t};

//
// [Arcade] Does over-under passing apply to this thing?
//
// There are exactly two live MF2_PASSMOBJ gates, and they must agree:
// PIT_CheckThing below decides whether a thing may move *into* the space of
// another, and the MF2_PASSMOBJ branch of P_MobjThinker (p_mobj.c) decides
// whether it may come to rest *on top of* one.  Gating only the first is not
// enough -- it stops you walking into a monster but still lets a lift carry
// one up under you, or a fall drop you onto one, which is how the player was
// still ending up stuck on a monster on the E1M2 lift after the first attempt
// at this.  Both call here so they cannot drift apart again.
//
boolean  P_Mobj_Pass_Over_Under( mobj_t * mo )
{
    if( ! (mo->flags2 & MF2_PASSMOBJ) )  return false;
    // Over-under is core Heretic behavior, not an option there.
    if( EN_heretic_hexen )  return true;
    return ! cv_tall_monsters.EV;
}

//
// PIT_CheckThing
//
// Check moving tm_thing against iterator thing
// Requires tmr_floorz, and tmr_ceilingz from CheckPosition.
// Return false when tm_thing is stopped by hitting this thing.
static boolean PIT_CheckThing (mobj_t* thing)
{
    fixed_t             blockdist;
    boolean             solid;
    int                 damage;

    //added:22-02-98:
    fixed_t             thing_topz;
    fixed_t             tmtopz;

    //SoM: 3/15/2000: Moved to front.

    // don't clip against self
    if (thing == tm_thing)
        goto ret_pass;

    if( !(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE|MF_TOUCHY)) )
        goto ret_pass;

#ifdef CLIENTPREDICTION2
    // mobj and spirit of a same player cannot collide
    if( thing->player && (thing->player->spirit == tm_thing || thing->player->mo == tm_thing) )
        goto ret_pass;
#endif

    blockdist = thing->radius + tm_thing->radius;

    if ( abs(thing->x - tm_x) >= blockdist ||
         abs(thing->y - tm_y) >= blockdist )
        goto ret_pass;  // didn't hit it

    // thing and tm_thing overlap in x,y
    tmtopz = tm_thing->z + tm_thing->height;
    thing_topz = thing->z + thing->height;

    if( EV_legacy >= 145 )
    {
        // [WDJ] Fix problem with monsters on 3dfloors being stuck to monsters
        // on other levels.
        if( thing->z > tmr_ceilingz )
           goto ret_pass;  // other thing is above ceiling (on upper floor)
        if( thing_topz < tmr_floorz )
           goto ret_pass;  // other thing is below floor (on lower floor)
    }

#ifdef HEXEN
    if( EN_hexen )
    {
        blocking_mobj = thing;
    }
#endif

    // heretic stuffs
    // [Arcade] cv_tall_monsters restores vanilla infinitely tall things for
    // Doom.  See P_Mobj_Pass_Over_Under; P_MobjThinker has the other gate.
    if( P_Mobj_Pass_Over_Under( tm_thing ) )
    { // check if a mobj passed over/under another object
        if((tm_thing->type == MT_IMP || tm_thing->type == MT_WIZARD)
            && (thing->type == MT_IMP || thing->type == MT_WIZARD))
        { // don't let imps/wizards fly over other imps/wizards
            goto ret_blocked;
        }

#ifdef HEXEN
        if((tm_thing->type == MT_HEXEN_BISHOP && thing->type == MT_HEXEN_BISHOP))
        { // don't let bishops fly over other bishops
            goto ret_blocked;
        }
#endif

        if(!(thing->flags&MF_SPECIAL))
        {
#ifdef HEXEN_XX
            // [WDJ] Same as adopted by Legacy
            if( EN_hexen && (tm_thing->z >= thing_topz) ) goto ret_pass;
            else
#endif
            // if(tm_thing->z > thing_topz) goto ret_pass; // vanilla, DSDA-doom
            if(tm_thing->z >= thing_topz)  goto ret_pass;  // tm_thing over thing
            if(tmtopz < thing->z)  goto ret_pass;  // tm_thing under thing
        }
    }

    // [WDJ] From PrBoom, MBF
    // killough 11/98:
    //
    // TOUCHY flag, for mines or other objects which die on contact with solids.
    // If a solid object of a different type comes in contact with a touchy
    // thing, and the touchy thing is not the sole one moving relative to fixed
    // surroundings such as walls, then the touchy thing dies immediately.
    if( thing->flags & MF_TOUCHY  // touchy object
        && thing->health > 0      // touchy object is alive
        && ((thing->eflags & MF_ARMED)  // Thing is an armed mine
            || SENTIENT(thing))   // ... or a sentient thing
      )
    {
        if( tm_thing->flags & MF_SOLID   // solid object touches it
            && (thing->type != tm_thing->type  // only different species
                || thing->type == MT_PLAYER ) // ... or different players
            && thing_topz >= tm_thing->z    // touches vertically
            && tmtopz >= thing->z
            // PEs and lost souls are considered same
            // but Barons & Knights are intentionally not.
            // The following are 0 if the type matches the MT.
            && ( (thing->type ^ MT_PAIN) | (tm_thing->type ^ MT_SKULL) )
            && ( (thing->type ^ MT_SKULL) | (tm_thing->type ^ MT_PAIN) )
          )
        {
            P_DamageMobj(thing, NULL, NULL, thing->health);  // kill object
            goto ret_pass;
        }
    }

    // check for skulls slamming into things
    if (tm_flags & MF_SKULLFLY)
    {
#ifdef HEXEN
        if( EN_hexen )
        {
            // SKULLFLY being used by hexen things
            if( tm_thing->type == MT_HEXEN_MINOTAUR )
            {
                // Minotaurs do not move non-monsters.
                if( ! (thing->flags & MF_COUNTKILL) )
                    goto ret_blocked;
            }
            else if( tm_thing->type == MT_HEXEN_HOLY_FX )
            {
                if( (thing->flags & MF_SHOOTABLE) && (thing != tm_thing->target) )
                {
                    // Do not attack other coop players
                    if( netgame && !deathmatch && thing->player )
                      goto ret_pass;

                    if( (thing->flags3 & MF3_REFLECTIVE)
                        && (thing->player || (thing->flags2 & MF2_BOSS)) )
                    {
                        // Reflect the holy fx.
                        P_SetTarget( &tm_thing->special1.m, tm_thing->target );
                        P_SetTarget( &tm_thing->target, thing );
                        goto ret_pass;
                    }

                    if( (thing->flags & MF_COUNTKILL) || thing->player )
                    {
                        P_SetTarget( &tm_thing->special1.m, thing );
                    }

                    if( PP_Random(ph_hexen) < 96 )
                    {
                        damage = 12;
                        if( thing->player || (thing->flags2 & MF2_BOSS))
                        {
                            damage = 3;
                            // ghost burns out faster when attacking player or bosses
                            tm_thing->health -= 6;
                        }

                        P_DamageMobj( thing, tm_thing, tm_thing->target, damage );

                        if( PP_Random(ph_hexen) < 128 )
                        {
                            P_SpawnMobj( tm_thing->x, tm_thing->y, tm_thing->z, MT_HEXEN_HOLY_PUFF );
                            P_StartMobjSound( tm_thing, sfx_hexen_spirit_attack );
                            hexen_target_play_puppybeat( thing, 128 );
                        }
                    }

                    if( thing->heath <= 0 )
                    {
                        tm_thing->special1.i = 0;
                        P_SetTarget( &tm_thing->special1.m, NULL );
                    }
                } // thing is SHOOTABLE
                goto ret_pass;  // tm_thing == MT_HEXEN_HOLY_FX
            }
        }
#endif

        damage = ((PP_Random(pr_skullfly)%8)+1)*tm_thing->info->damage;

        P_DamageMobj (thing, tm_thing, tm_thing, damage);

        tm_thing->flags &= ~MF_SKULLFLY;
        tm_thing->momx = tm_thing->momy = tm_thing->momz = 0;

        // This call can recurse through this function again.
        P_SetMobjState (tm_thing,
#ifdef HEXEN
                        (EN_heretic_hexen)? tm_thing->info->seestate
#else
                        (EN_heretic)? tm_thing->info->seestate
#endif
                                    : tm_thing->info->spawnstate
                       );

        tm_flags = tm_thing->flags;  // flags may have been modified

        goto ret_blocked;  // stop moving
    } // MF_SKULLFLY
    
#ifdef HEXEN
    // Hexen: BLASTED thing running into another shootable monster thing.
    if( (tm_thing->flags3 & MF3_BLASTED)
        && (thing->flags & (MF_SHOOTABLE | MF_COUNTKILL) == (MF_SHOOTABLE | MF_COUNTKILL))
        && !(thing->flags2 & MF2_BOSS)
      )
    {
        // Blasted thing shares its momentum and damage.	
        thing->momx += tm_thing->momx;
        thing->momy += tm_thing->momy;
        if( (thing->momx + thing->momy) > (3*FRACUNIT))
        {
            damage = (tm_thing->info->mass / 100) + 1;  // per mass
            P_DamageMobj (thing, tm_thing, tm_thing, damage);
            damage = ((thing->info->mass / 100) + 1) >> 2;  // per mass / 4
            P_DamageMobj (tm_thing, thing, thing, damage);
        }
        goto ret_blocked;
    }
#endif

    // missiles can hit other things
    // killough 8/10/98: bouncing non-solid things can hit other things too
    if( (tm_thing->flags & MF_MISSILE)
         || ((tm_thing->flags & (MF_BOUNCES | MF_SOLID)) == MF_BOUNCES)
      )
    {
#ifdef HEXEN
        // Hexen Nonshootable mobj (should describe how this is different than MF_SHOOTABLE)
        if( thing->flags3 & MF3_NONSHOOTABLE )  // non-shootable missile, or non-solid bouncy thing
            goto ret_pass;
#endif

        // Check for passing through a ghost (heretic)
        if ((thing->flags & MF_SHADOW) && (tm_thing->flags2 & MF2_THRUGHOST))
            goto ret_pass;

        // see if it went over / under
        if (tm_thing->z > thing_topz)  goto ret_pass;  // overhead
        if (tmtopz < thing->z)  goto ret_pass;  // underneath

#ifdef HEXEN
        if( EN_hexen )
        {
            // Hexen things that can be gone over or under or through.
            if( tm_thing->flags2 & MF3_FLOORBOUNCE )
            {
                if( (tm_thing->target = thing)
                  || !(thing->flags & MF_SOLID ))
                  goto ret_pass; // The targeted thing is not solid
                else
                  goto ret_blocked;  // Not its target, or is solid
            }

            switch( tm_thing->type )
            {
             case MT_HEXEN_LIGHTINGING_FLOOR:
             case MT_HEXEN_LIGHTINGING_CEILING:
                if( (tm_thing->target != thing) && (thing->flags & MF_SHOOTABLE) )
                {
                    // thing is target,and is shootable.
                    if( thing->info->mass != INT_MAX ) // not overly massive
                    {
                        // push thing
                        thing->momx += tm_thing->momx >> 4;
                        thing->momy += tm_thing->momy >> 4;
                    }

                    if( !(thing->player || (thing->flags2 & MF2_BOSS) )
                        || !(leveltime & 1 ) )
                    {
                        int damage = 3;
                        if( thing->type == MT_HEXEN_CENTAUR
                             || thing->type == MT_HEXEN_CENTAURLEADER )
                        {
                            // Lightening does more damage to centaurs.
                            damage = 9;
                        }
                        P_DamageMobj( thing, tm_thing, tm_thing->target, damage );

                        if( ! (S_GetSoundPlayingInfo( tm_thing, sfx_hexen_mage_lightening_zap ) ) )
                          S_StartMobjSound( tm_thing, sfx_hexen_mage_lightening_zap );

                        hexen_target_play_puppybeat( thing, 64 );
                    }

                    tm_thing --;
                    if( tm_thing->health <= 0  || thing->health <= 0 )
                        goto ret_blocked;

                    hexen_set_indirect_lightening_target( tm_thing );

                } // thing is target and shootable
                goto ret_pass;  // ligtening zaps through everything
                // LIGHTENING

             case MT_HEXEN_LIGHTENINGZAP:
                if( (tm_thing->target != thing) && (thing->flags & MF_SHOOTABLE) )
                {
                    mobj_t * lzspec2 = tm_thing->special2.m;
                    if( lzspec2 )
                    {
                        hexen_set_indirect_lightening_target( lzspec2 );

                        if( ! (leveltime & 3) )
                           lzspec2->health--;
                    }
                }
                break;

             case MT_HEXEN_M_STAFF_FX2:
                if( tm_thing->target != thing
                    && !(thing->player)
                    && !(thing->flags2 & MF_BOSS) )
                {
                    switch( thing->type )
                    {
                        // These are not MF2_BOSS, so they must be tested.
                     case MT_HEXEN_FIGHTER_BOSS:
                     case MT_HEXEN_CLERIC_BOSS:
                     case MT_HEXEN_MAGE_BOSS:
                        break;
                     default:
                        P_DamageMobj( thing, tm_thing, tm_thing->target, 10 );
                        goto ret_pass;
                    }
                }
                break;
            } // switch MT_
        }  // EN_hexen
#endif

        if( tm_thing->target
#ifdef MBF21
            && mbf21_projectile_immune( thing, tm_thing->target )
#else
            &&( tm_thing->target->type == thing->type
                || (tm_thing->target->type == MT_KNIGHT  && thing->type == MT_BRUISER)
                || (tm_thing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) )
#endif
           )
        {
            // Don't hit same species as originator.
            // Cannot hit self with own missile.
            if (thing == tm_thing->target)
                goto ret_pass;

            if (thing->type != MT_PLAYER)
            {
                // Boom - Let players missile other players. ??
                // [WDJ] these are monsters, not players.
                // PrBoom and DoomLegacy allow infighting with missile damage.
                if( monster_infight != INFT_full_infight )  // when not missile infighting
                    goto ret_blocked;  // Explode, but do no damage.
            }
        }

        // DarkWolf95: Don't damage other monsters
        if( (monster_infight == INFT_coop)  // monster coop
           && tm_thing->target->type != MT_PLAYER
           && thing->type != MT_PLAYER)
            goto ret_blocked;

#if 0
        // [WDJ] was in DSDA-Doom
        // killough 8/10/98: if moving thing is not a missile, no damage
        // is inflicted, and momentum is reduced if object hit is solid.
        if( !(tm_things->flags & MF_MISSILE) )
        {
            if( !(thing->flags & MF_SOLID) )
              goto ret_pass;

            // reflect
            tm_thing->momx = -tm_thing->momx;
            tm_thing->momy = -tm_thing->momy;
            if( !(tm_thing->flags & MF_NOGRAVITY) )
            {
                // reduce momentum
                tm_thing->momx <<= 2;
                tm_thing->momy <<= 2;
            }
            goto ret_blocked;
        }
#endif

        if (! (thing->flags & MF_SHOOTABLE) )
        {
            // didn't do any damage
            return !(thing->flags & MF_SOLID);
        }

        // more heretic stuff
#ifdef MBF21
        // MBF21: also uses MF2_RIP
#endif
        if (tm_thing->flags2 & MF2_RIP)
        {
            // [WDJ] Heretic, MBF21 do 6 P_Random calls in
            // P_RipperBlood before here.
            // DoomLegacy does different blood splats, later, and conditional on P_DamageMobj.
            // This will affect demo sync.

#ifdef HEXEN
            byte ripper_blood = ( !(thing->flags & MF_NOBLOOD)
                                  && !(thing->flags3 & (MF3_REFLECTIVE|MF3_INVULNERABLE)) );
#else
            byte ripper_blood = !(thing->flags & MF_NOBLOOD);
#endif

#ifdef MBF21
            uint16_t rip_sfx = sfx_ripslop;  // default sound for RIP
            byte rip_damage_rand;  // the damage is mostly the same, except for this
            if( EN_mbf21 )
            {
                rip_damage_rand = PP_Random(pr_mbf21);
                rip_sfx = tm_thing->info->rip_sound;
            }
            else if( EN_heretic_hexen )
            {
                // Heretic
                if( ripper_blood )
                {
                    if( EV_blood_vanilla > 128 )
                    {
                        // Blood splats as in Heretic.
                        P_RipperBlood( tm_thing, thing );  // P_Random calls
                        ripper_blood = 0;  // already did this blood
                    }
                    else if( EV_blood_vanilla )
                    {
                        P_dummy_RipperBlood();  // to preserve P_Random calls
                    }
                }
                rip_damage_rand = PP_Random(ph_ripdam);
//		damage = ((PP_Random(ph_ripdam) & 3) + 2) * tm_thing->damage;  // Heretic, Hexen
//              rip_sfx = sfx_ripslop;
# ifdef HEXEN
                if( EN_hexen )
                  rip_sfx = sfx_None;  // no RIP sound in Hexen
# endif
            }
            else
            {
                // default for DoomLegacy
                rip_damage_rand = PP_Random(ph_ripdam);
//              rip_sfx = sfx_ripslop;
            }
            damage = ((rip_damage_rand & 3) + 2) * tm_thing->info->damage;

# ifdef HEXEN
            if( ! EN_hexen )  // HEXEN, no RIP sound
              S_StartObjSound(tm_thing, rip_sfx);
# else
            S_StartObjSound(tm_thing, rip_sfx);
# endif
#else
            // Heretic
            damage = ((PP_Random(ph_ripdam) & 3) + 2) * tm_thing->info->damage;
            // damage = ((PP_Random(ph_ripdam) & 3) + 2) * tm_thing->damage;
//	    rip_sfx = sfx_ripslop;
            S_StartObjSound(tm_thing, sfx_ripslop);
#endif

            if( P_DamageMobj (thing, tm_thing, tm_thing->target, damage) )
            {
                // DoomLegacy: the blood is dependent upon taking damage.
                if( ripper_blood )
                {   // Ok to spawn some blood
                    P_SpawnBlood(tm_thing->x, tm_thing->y, tm_thing->z, damage );
                    //P_RipperBlood (tm_thing);
                }
            }

            if (thing->flags2 & MF2_PUSHABLE
                && !(tm_thing->flags2 & MF2_CANNOTPUSH))
            {             // Push thing
                thing->momx += tm_thing->momx >> 2;
                thing->momy += tm_thing->momy >> 2;
            }
            numspechit = 0;
            goto ret_pass;
        }

        // damage / explode
        int base_damage = tm_thing->info->damage;
        byte blood_effect = !(thing->flags & MF_NOBLOOD);  // [WDJ] To get all P_Random calls in right order.
#ifdef HEXEN
        if( EN_heretic_hexen )
            base_damage = tm_thing->damage;  // ??? WHAT IS THIS
#endif
        damage = ((PP_Random(pr_damage)%8)+1) * base_damage;

#ifdef HERETIC_TODO_VERSION
        // [WDJ] Heretic, Hexen.  This test added 1.49
        if( EN_heretic_hexen && (EV_legacy >= HERETIC_TODO_VERSION) )
        {
            if( ! damage )  // Heretic and Hexen, no damage skips all this
                goto ret_blocked;   // don't traverse any more

            blood_effect = ( blood_effect
#ifdef HEXEN
                && !(thing->flags3 & (MF3_REFLECTIVE|MF3_INVULNERABLE))
                && !(thing->type >= MT_HEXEN_TELEOTHER_FX1 && (thing->type <= MT_HEXEN_TELEOTHER_FX5)) // they are sequential
#endif
                // Random call is dependent upon all prev tests, this must be last
                && (PP_Random(ph_heretic) < 192)  // Heretic: implemented Legacy 1.49
                );
        }
#endif	

        // Hexen does not call P_DamageMobj when no damage.
        if( P_DamageMobj (thing, tm_thing, tm_thing->target, damage)
            && (EV_legacy >= 129)
            && blood_effect
            )
        {
            P_SpawnBloodSplats (tm_thing->x, tm_thing->y, tm_thing->z, damage, thing->momx, thing->momy);
            // P_BloodSplatter( tm_thing->x, tm_thing->y, tm_thing->z, thing );  Heretic
        }
        goto ret_blocked;   // don't traverse any more
    }

    if (thing->flags2 & MF2_PUSHABLE
        && !(tm_thing->flags2 & MF2_CANNOTPUSH))
    {                         // Push thing
        thing->momx += tm_thing->momx >> 2;
        thing->momy += tm_thing->momy >> 2;
    }

    // check for special pickup
    if (thing->flags & MF_SPECIAL)
    {
        solid = thing->flags&MF_SOLID;
        if (tm_flags&MF_PICKUP) // will pickup things
        {
            // can remove thing
            P_TouchSpecialThing (thing, tm_thing);
        }
        return !solid;
    }
    // check again for special pickup
    if(tm_thing->flags & MF_SPECIAL
       && (EV_legacy >= 132) )
    {
        solid = tm_thing->flags&MF_SOLID;
        if (thing->flags&MF_PICKUP)  // will pickup things
        {
            // can remove thing
            P_TouchSpecialThing (tm_thing, thing);
        }
        return !solid;
    }


    //added:24-02-98:compatibility with old demos, it used to return with...
    //added:27-02-98:for version 112+, nonsolid things pass through other things
    // For all versions, check thing !SOLID eventually leads to return true.
    if ( !(thing->flags & MF_SOLID) )
       goto ret_pass;
   
    // The check thing is solid, (thing->flags & MF_SOLID == true)
    // This function does not get called when (tm_thing->flags & MF_NOCLIP)
    // so there is no need to check that.
#if 0 
    // Demo compatibility. Is unnecessary, unless Legacy becomes different.
    if ( demoversion >= 200 )
    {
        // From prboom.
        return (thing->flags & MF_NOCLIP) || !(tm_thing->flags & MF_SOLID);
    }
#endif   
    if ( demoversion < 111 || demoversion >= 145 )
    {
        if( ! EN_boom  )  // and SOLID
            goto ret_blocked;

        // Also includes ( demoversion >= 200 ) from prboom, same code.

        // A non-solid moving thing is not blocked by a solid.
        // NOCLIP things are not blocking.
        return (thing->flags & MF_NOCLIP) || !(tm_thing->flags & MF_SOLID);
    }

    // old versions
    if( EV_legacy < 112 || EV_legacy >= 132 )
      goto ret_blocked;

    // [WDJ] This strange check is for DoomLegacy versions 113..131.
    // It looks like a mistake.
    if ( !(tm_thing->flags & MF_SOLID) )
      goto ret_blocked;  // A non-solid moving thing is blocked by a solid.

    // [WDJ] This z-checking code is for DoomLegacy versions 113..131.
    // After version 132, the heretic z-checking code was added (PASSMOBJ),
    // which is more specific.
    // This code causes monsters to escape ledges on top of other monsters.
 
    //added:22-02-98: added z checking at last
    //SoM: 3/10/2000: Treat noclip things as non-solid!
    // Already known: (thing->flags & MF_SOLID)
    //  (tm_thing->flags & MF_SOLID), !(tm_thing->flags & MF_NOCLIP)
    if ( !(thing->flags & MF_NOCLIP) )
    {
        // Only apply when tm_thing and thing are both SOLID, and !NOCLIP.

        // pass under
        if ( tmtopz < thing->z)
        {
            if (thing->z < tmr_ceilingz)
                tmr_ceilingz = thing->z;
            goto ret_pass;
        }

        // trying to go over thing
        thing_topz += FRACUNIT;

        // block only when jumping not high enough,
        // (dont climb max. 24units while already in air)
        // if not in air, let P_TryMove() decide if its not too high
        if (tm_thing->player
            && tm_thing->z < thing_topz
            && tm_thing->z > tm_thing->floorz )  // block while in air
        {
            // collide
#if 0
            // no bounce, so same momentum as other thing
            tm_thing->momx = thing->momx;
            tm_thing->momy = thing->momy;
#endif
            goto ret_blocked;  // did not make it over thing
        }

        if (thing_topz > tmr_floorz)
        {
            // standing on top of another thing
            tmr_floorz = thing_topz;
            tmr_floorthing = thing;       //thing we may stand on
        }

    }

ret_pass:   
    // not solid, or not blocked
    return true;

ret_blocked:
    return false;  // hit something
}


// SoM: 3/15/2000
// PIT_CrossLine
// Checks to see if a PE->LS trajectory line crosses a blocking
// line. Returns false if it does.
//
// tm_bbox holds the bounding box of the trajectory. If that box
// does not touch the bounding box of the line in question,
// then the trajectory is not blocked. If the PE is on one side
// of the line and the LS is on the other side, then the
// trajectory is blocked.
//
// Currently this assumes an infinite line, which is not quite
// correct. A more correct solution would be to check for an
// intersection of the trajectory and the line, but that takes
// longer and probably really isn't worth the effort.
//
// Called by: P_CheckCrossLine
static boolean PIT_CrossLine (line_t* ld)
{
  if (!(ld->flags & ML_TWOSIDED) ||
      (ld->flags & (ML_BLOCKING|ML_BLOCKMONSTERS)))
  {
    if (!(tm_bbox[BOXLEFT]   > ld->bbox[BOXRIGHT]  ||
          tm_bbox[BOXRIGHT]  < ld->bbox[BOXLEFT]   ||
          tm_bbox[BOXTOP]    < ld->bbox[BOXBOTTOM] ||
          tm_bbox[BOXBOTTOM] > ld->bbox[BOXTOP]))
    {
      if (P_PointOnLineSide(from_x,from_y,ld) != P_PointOnLineSide(targ_x,targ_y,ld))
          goto ret_blocked;
    }
  }
  return true; // line doesn't block trajectory

ret_blocked:
  return false;  // line blocks trajectory
}


// [WDJ] MBF, from PrBoom, MBF
// killough 8/1/98: used to test intersection between thing and line
// assuming NO movement occurs -- used to avoid sticky situations.
// ! untouched
//
// Return true when tm_thing touches the line
static byte  tm_touches( line_t *ld )
{
  fixed_t ubbox[4];
  fixed_t radius = tm_thing->radius;

  ubbox[BOXRIGHT] = tm_thing->x + radius;
  ubbox[BOXLEFT] = tm_thing->x - radius;
  if(    (ubbox[BOXRIGHT] <= ld->bbox[BOXLEFT])
      || (ubbox[BOXLEFT] >= ld->bbox[BOXRIGHT]) )
      return 0;

  ubbox[BOXTOP] = tm_thing->y + radius;
  ubbox[BOXBOTTOM] = tm_thing->y - radius;
  if(    (ubbox[BOXTOP] <= ld->bbox[BOXBOTTOM])
      || (ubbox[BOXBOTTOM] >= ld->bbox[BOXTOP]) )
      return 0;

  return  P_BoxOnLineSide(ubbox, ld) == -1;
}


#ifdef HEXEN
static void  hexen_tmthing_line_blocked( byte special )
{
    if( tm_thing->flags3 & MF3_BLASTED )  // ??? MF2_NONBLAST  FIXME
    {
        P_DamageMobj( tm_thing, NULL, NULL, tm_thing->info->mass >> 5 );
    }
    if( special )
    {
        CheckForPushSpecial( ld, 0, tm_thing );
        CheckForDamageSpecial( ld, tm_thing );
    }
}
#endif

//
// PIT_CheckLine
// Adjusts tmr_floorz and tmr_ceilingz as lines are contacted.
//
static
boolean PIT_CheckLine (line_t* ld)
{
    if (tm_bbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
        || tm_bbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
        || tm_bbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
        || tm_bbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
        return true;  // coarse check miss

    if (P_BoxOnLineSide (tm_bbox, ld) != -1)
        return true;  // miss

    // A line has been hit

    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.

    // 10-12-99 BP: moved this line out of the IF so upper and 
    //              lower texture can be hit by a splat
    tmr_blockingline = ld;
    if (!ld->backsector)
    {
        // one sided line
        // Heretic, adopted in Legacy
        if( tm_thing->flags & MF_MISSILE
            && ld->special
            && (EV_legacy >= 132)
          )
          add_spechit(ld);  // missile can trigger impact special line

#ifdef HEXEN
        if( EN_hexen )
        {
            hexen_tmthing_line_blocked(1);
        }
#endif

        // [WDJ] MBF player unstick.
        if( tm_mbf_unstuck && tm_touches(ld) )
        {
            return  FixedMul(tm_x - tm_thing->x, ld->dy)
                  > FixedMul(tm_y - tm_thing->y, ld->dx);
        }

        goto ret_blocked;  // blocked by one sided line
    }

    // missile and Camera can cross uncrossable line
    // killough 8/10/98: allow bouncing objects to pass through as missiles
    // MBF: also MF_BOUNCES
    if (!(tm_thing->flags & (MF_MISSILE | MF_BOUNCES))
        && !(tm_thing->type == MT_CHASECAM) )
    {
#ifdef MBF21
        if( ld->flags & ML_BLOCKING
            || ( EN_mbf21
                 && ld->flags & ML_BLOCK_PLAYERS
                 && tm_thing->player )  // MBF21 blocking player
          )
#else
        if (ld->flags & ML_BLOCKING)
#endif
        {
#ifdef HEXEN
            if( EN_hexen )
            {
                hexen_tmthing_line_blocked(1);
            }
#endif
            if( tm_mbf_unstuck && tm_touches(ld) )
                return true;
            goto ret_blocked;  // explicitly blocking everything
        }

        // killough 8/9/98: monster-blockers don't affect friends
        if( (ld->flags & ML_BLOCKMONSTERS
#ifdef MBF21
             || ( EN_mbf21
                  && (ld->flags & ML_BLOCK_LAND_MONSTERS) && !(tm_thing->flags & MF_FLOAT))
#endif
             )
            && !(tm_thing->player || (tm_thing->flags & MF_FRIEND) )  // is monster
            && !(EN_heretic && (tm_thing->type == MT_POD))   // Added, check this FIXME
          )
        {
#ifdef HEXEN
            if( EN_hexen )
            {
                hexen_tmthing_line_blocked(0); // no specials
            }
#endif
            goto ret_blocked;  // block monsters only
        }
    }

    // set openrange, opentop, openbottom
    P_LineOpening (ld);

    // adjust floor / ceiling heights
    if (opentop < tmr_ceilingz)
    {
        tmr_sectorceilingz = tmr_ceilingz = opentop;
        tmr_ceilingline = ld;
    }

    if (openbottom > tmr_floorz)
    {
        tmr_sectorfloorz = tmr_floorz = openbottom;
        tmr_floorline = ld;
    }

    if (lowfloor < tmr_dropoffz)
    {
        tmr_dropoffz = lowfloor;
        tmr_dropoffline = ld;
    }

    // if contacted a special line, add it to the list
    if (ld->special)
        add_spechit(ld);

    // [WDJ] Was missing detection of missile hitting 2s wall.
    // Missiles exploded only because of ZMovement detecting that they
    // were in the floor.
    // Missiles now explode on wall surface instead of within wall.
    // Needed for fix of Mancubus fireball going through wall.
    // [WDJ] Added MF_BOUNCES, due to exclusion in test above.
    if ( tm_thing->flags & (MF_MISSILE | MF_BOUNCES)
         && (EV_legacy >= 144) )
    {
        // Check if missile hit low or high on side of this wall
        if ( tm_thing->z < openbottom
             || (tm_thing->z + tm_thing->height) > opentop )
            goto ret_blocked;  // hit upper or lower texture
    }

    return true;  // pass the lines

ret_blocked:
    // collided with blocking line
    // no bounce
//    tm_thing->momx = tm_thing->momy = 0;  // fouls wall slide 
    return false;  // blocked by a line
}

// [WDJ]
// Check cross lines as movement across 1 sided or monster blocking lines.
// This is used to prevent spawning across such lines, and thus keep skulls
// from being spawned in void spaces.
// It is simplified check as done for movement.
// In Lost Soul checks, thing is pain elemental, and x,y is skull spawn position.
// This is mostly from prboom (phares) and Edge (-ACB-).
// Return true when crosses a blocking line.

boolean P_CheckCrossLine ( mobj_t* thing, fixed_t x, fixed_t y )
{
    int xl, xh;
    int yl, yh;
    int bx, by;
   
    from_x = thing->x;
    from_y = thing->y;
    targ_x = x;
    targ_y = y;

    // Fewer tests, more assignments, should be simpler, compiler will optimize
    if( y < from_y )
    {
        tm_bbox[BOXTOP] = from_y;
        tm_bbox[BOXBOTTOM] = y;
    }
    else
    {
        tm_bbox[BOXTOP] = y;
        tm_bbox[BOXBOTTOM] = from_y;
    }
    if( x < from_x )
    {
        tm_bbox[BOXRIGHT] = from_x;
        tm_bbox[BOXLEFT] = x;
    }
    else
    {
        tm_bbox[BOXRIGHT] = x;
        tm_bbox[BOXLEFT] = from_x;
    }

    // check lines
    xl = (tm_bbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tm_bbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tm_bbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tm_bbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    validcount++;  // marking signature to prevent duplicate work
    for (bx=xl ; bx<=xh ; bx++)
    {
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator (bx,by,PIT_CrossLine))
                goto ret_blocked;
    }

    return false;

ret_blocked:
    return true;  // hit a line that stopped it
}


// =========================================================================
//                         MOVEMENT CLIPPING
// =========================================================================

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
// Except that calls PIT_CheckThing that:
// Except TOUCHY things that explode, damage.
// Except skullfly damage.
// Except HEXEN BLASTED mutual damage.
// Except missile damage.
// Except HEXEN missile play sounds.
// Except HEXEN STAFF damage.
// Except Heretic and MBF21 ripper blood, and damage.
// Except touch special things (pickup).
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  tmr_floorz
//  tmr_ceilingz
//  tmr_dropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  tmr_floorz
//     the nearest floor or thing's top under tm_thing
//  tmr_ceilingz
//     the nearest ceiling or thing's bottom over tm_thing
//
// Use tm_ global vars, and return tmr_ global vars.
boolean P_CheckPosition ( mobj_t* thing, fixed_t x, fixed_t y )
{
    int xl, xh;
    int yl, yh;
    int bx, by;
    subsector_t * cp_newsubsec;

    tm_thing = thing;  // moving thing to be checked
    tm_flags = thing->flags;

    tm_x = x;
    tm_y = y;

    tm_bbox[BOXTOP] = y + tm_thing->radius;
    tm_bbox[BOXBOTTOM] = y - tm_thing->radius;
    tm_bbox[BOXRIGHT] = x + tm_thing->radius;
    tm_bbox[BOXLEFT] = x - tm_thing->radius;

    tm_mbf_unstuck = EN_mbf
       && thing->player
       && thing->player->mo == thing;  // not voodoo doll

    cp_newsubsec = R_PointInSubsector (x,y);
    tmr_ceilingline = tmr_floorline = NULL;
    tmr_blockingline = tmr_dropoffline = NULL;

    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any lines contacted during the step
    // may adjust them closer together.
    tmr_sector = cp_newsubsec->sector;
    tmr_floorz = tmr_sectorfloorz = tmr_dropoffz = tmr_sector->floorheight;
    tmr_ceilingz = tmr_sectorceilingz = tmr_sector->ceilingheight;

    //SoM: 3/23/2000: Check list of fake floors and see if
    //tmr_floorz/tmr_ceilingz need to be altered.
    if(tmr_sector->ffloors)
    {
      ffloor_t*  rovflr; // rover floor
      fixed_t    midfloor;
      int        thingtop = thing->z + thing->height;

      for(rovflr = tmr_sector->ffloors; rovflr; rovflr = rovflr->next)
      {
        if(!(rovflr->flags & FF_SOLID) || !(rovflr->flags & FF_EXISTS)) continue;

        midfloor =
           *rovflr->bottomheight + ((*rovflr->topheight - *rovflr->bottomheight)/2);
        if( abs(thing->z - midfloor) < abs(thingtop - midfloor) )
        {
            // feet are closer
            if(*rovflr->topheight > tmr_floorz)
                tmr_floorz = tmr_dropoffz = *rovflr->topheight;
        }
        else
        {
            // head is closer
            if(*rovflr->bottomheight < tmr_ceilingz )
                tmr_ceilingz = *rovflr->bottomheight;
        }
      }
    }

    // [WDJ] tmr_floorthing is in demoversion 113..131, otherwise NULL
    // tmr_floorthing is set when tmr_floorz comes from a thing's top
    tmr_floorthing = NULL;

    validcount++;
    numspechit = 0;

    if ( tm_flags & MF_NOCLIP )
        return true;

    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.

    // DarkWolf95:don't check non-solids against other things,
    // keep them in the map though, so still check against lines.
    // BP: added MF2_NOCLIPTHING :used by camera to not be blocked by things
    if((thing->flags & (MF_SOLID | MF_MISSILE))
        && !(thing->flags2 & MF2_NOCLIPTHING) )
    {
        xl = (tm_bbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
        xh = (tm_bbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
        yl = (tm_bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
        yh = (tm_bbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
        
        for (bx=xl ; bx<=xh ; bx++)
        {
            for (by=yl ; by<=yh ; by++)
                if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
                    return false;
        }
    }
    // check lines
    xl = (tm_bbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tm_bbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tm_bbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tm_bbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
    {
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
                goto ret_blocked;
    }

    return true;

ret_blocked:
    return false;  // hit a line that stopped it
}


//==========================================================================
//
// CheckMissileImpact
//
//==========================================================================

static void CheckMissileImpact(mobj_t *mobj)
{
    int i;
    
    if( (EV_legacy < 132) || !numspechit
        || !(mobj->flags&MF_MISSILE) || !mobj->target)
        return;

    if(!mobj->target->player)
        return;

    for(i = numspechit-1; i >= 0; i--)
        P_ShootSpecialLine(mobj->target, &lines[spechit[i]]);
}

//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
//  allowdropoff : 0, 1 allow, 2 allow dog jump
// Use tm_ global vars, and return tmr_ global vars.
boolean P_TryMove ( mobj_t*       thing,
                    fixed_t       x,
                    fixed_t       y,
                    byte          allowdropoff)
{
    line_t*     ld;
    fixed_t     oldx, oldy;
#if 0
    fixed_t     onz;  // maybe standing on something
#endif
    int         side;
    int         oldside;

    tmr_felldown = tmr_floatok = false;

    if (!P_CheckPosition (thing, x, y))
        goto impact;  // solid wall or thing

#ifdef CLIENTPREDICTION2
    if ( !(thing->flags & MF_NOCLIP) && !(thing->eflags & MF_NOZCHECKING))
#else
    if ( !(thing->flags & MF_NOCLIP) )
#endif
    {
        fixed_t maxstep = MAXSTEPMOVE;

        if( tmr_ceilingz - tmr_floorz < thing->height )
            goto boom_conditional_impact;  // thing doesn't fit room

        tmr_floatok = true;

        if( thing->flags2 & MF2_FLY )  // heretic fly
        {
            if((thing->z + thing->height) > tmr_ceilingz) // hit ceiling
            {
                thing->momz = -8*FRACUNIT;
                goto block_move;
            }
            else if( thing->z < tmr_floorz  // hit floor
                     && (tmr_floorz - tmr_dropoffz > 24*FRACUNIT)
                   )
            {
                thing->momz = 8*FRACUNIT;
                goto block_move;
            }
        }
        else
        {   // not heretic fly
            if(((thing->z + thing->height) > tmr_ceilingz) // hit ceiling
               && !(thing->flags & MF_TELEPORT) )
                goto boom_conditional_impact;  // mobj must lower itself to fit
        }

        // jump out of water
        if((thing->eflags & (MF_UNDERWATER|MF_TOUCHWATER))==(MF_UNDERWATER|MF_TOUCHWATER))
            maxstep = 37*FRACUNIT;

        // Hit step, check step height.
        // The Minotaur floor fire (MT_MNTRFX2) can step up any amount.
        if ( !(thing->flags & MF_TELEPORT) 
             && thing->type != MT_MNTRFX2
             && (tmr_floorz - thing->z > maxstep ) )
            goto boom_conditional_impact;  // too big a step up

        if((thing->flags & MF_MISSILE)
           && tmr_floorz > thing->z)
            CheckMissileImpact(thing);

#if 0
// As MBF had it, disorganized.
        if( !(thing->flags & (MF_DROPOFF|MF_FLOAT)) )
        {
            if( ! cv_mbf_dropoff.EV )
            {
                if( ( !EN_boom || (dropoff == 0)
                      // fix demosync bug in mbf compatibility mode
                      || (EN_mbf && compatibility_level <= prboom_2_compatibility)
                    )
                    && (tmr_floorz - tmr_dropoffz > 24*FRACUNIT)
                  )
                    return false;  // don't stand over a dropoff
            }
            else
            if( (dropoff == 0)  // dropoff not allowed
                ||( (dropoff == 2)  // large jump down (e.g. dogs)
                    &&( (tmr_floorz - tmr_dropoffz > 128*FRACUNIT) // too far
                        || !thing->target
                        || thing->target->z > tmr_dropoffz) // target above dropoff
                  )
              )
            {
                if( (EN_mbf && cv_monkeys.EV) ?
                      thing->floorz - tmr_floorz > 24*FRACUNIT ||
                      thing->dropoffz - tmr_dropoffz > 24*FRACUNIT)
                    :  tmr_floorz - tmr_dropoffz > 24*FRACUNIT
                    return false;
            }
            else
            { /* dropoff allowed -- check for whether it fell more than 24 */
                tmr_felldown = !(thing->flags & MF_NOGRAVITY) &&
                               thing->z - tmfloorz > 24*FRACUNIT;
            }
        }
#endif

        if( tmr_dropoffline )
        {
            // [WDJ] Due to complexity of MBF and integration with DoomLegacy,
            // used some GOTO.  Do not try to fix this!
            // It is much more understandable this way.
            if( thing->flags&(MF_DROPOFF|MF_FLOAT) ) // player, missile, shot, puff, etc.
                goto ignore_dropoff;

#if 0
            // may be standing on something
            onz = ( tmr_floorthing && (thing->z > tmr_floorz) )?
                    thing->z     // standing on thing
                  : tmr_floorz;  // the usual floor
#endif

            fixed_t dropoff_height = tmr_floorz - tmr_dropoffz;
#ifdef MBF21
            // MBF21
            byte ledgeblock = EN_ledgeblock
              && !(EN_mbf21 && thing->eflags & MF_SCROLLING);

            // [WDJ] DSDA-doom (e6y) had a large demo sync fix here, but
            // we do not demo sync with prboom anyway.
            // It tested a number of prboom comp flags.
            // It eventually did the normal Doom, Boom dropoff test that follows the MBF.
#endif

            // MBF
#ifdef MBF21
            if( cv_mbf_dropoff.EV && ! ledgeblock )  // MBF dropoff, not comp
#else	    
            if( cv_mbf_dropoff.EV )  // MBF dropoff
#endif
            {
                // Allow MBF dropoff, test dropoff height, test dogs.
                if(  (allowdropoff == 0)  // drop off not allowed
#if 0
                  ||((allowdropoff == 1)  // drop off allowed
                    && (dropoff_height > MAXSTEPMOVE))
#endif
#ifdef DOGS
                  ||((allowdropoff == 2) // large jump down (e.g. dogs)
                    &&((dropoff_height > 128*FRACUNIT)
                        || !thing->target
                        || thing->target->z > tmr_dropoffz )) // target above dropoff
#endif
                  )
                {
                    // Dropoff not allowed, or too high for what is allowed.
                    if( EN_mbf && cv_mbf_monkeys.EV )
                    {
                        // [WDJ] Still do not know what monkeys does.
                        if( thing->floorz - tmr_floorz > 24*FRACUNIT
                            || thing->dropoffz - tmr_dropoffz > 24*FRACUNIT)
                            goto block_move;  // too high for monkeys

                        goto ignore_dropoff;  // monkeys
                    }
                    else if( dropoff_height > 24*FRACUNIT )  // Doom, Boom, normal dropoff test
                        goto block_move;  // too high

                    goto ignore_dropoff;
                }
                else
                {
                    // dropoff allowed
                    // check for whether it fell more than 24.
                    tmr_felldown = !(thing->flags & MF_NOGRAVITY)
                                   && (thing->z - tmr_floorz > 24*FRACUNIT);
                    goto got_dropoff;
                }
            }

            // DoomLegacy
            // [WDJ] tmr_floorthing is for DoomLegacy versions 113..131.
            if( tmr_floorthing )  // standing on something
                goto ignore_dropoff;

            // Doom, Boom
            if( tmr_floorz > tmr_dropoffz + MAXSTEPMOVE )  // excessive height
            {
                // [WDJ] Meant to prevent walking off a dropoff, it also prevents
                // getting away from one once the thing is over it.
                // Monsters will repeat call until find successful direction.
                if( !EN_boom || (allowdropoff == 0) )
                {
                    goto block_move;  // inform caller, returning tmr_dropoffline
                }
                // MBF compatibility mode
                // Fix demosync bug in mbf compatibility mode.
                if( EN_mbf && demoversion <= 210 )
                    goto block_move;
                // [WDJ] Trying to moderate momentum here causes too many side-effects
                // like barrels getting stuck at conveyor edge.
                // Barrels only have momentum.
                // Successful move, returning tmr_dropoffline.
                goto got_dropoff;
            }

ignore_dropoff:
            // Necessary due to removal of a number of dependent tests.
            tmr_dropoffline = NULL;  // cancel notification
        }

got_dropoff:
        // [WDJ] From PrBoom, MBF, EternityEngine
        if( (thing->flags & MF_BOUNCES)    // killough 8/13/98
            && !(thing->flags & (MF_MISSILE|MF_NOGRAVITY))
            && !SENTIENT(thing)
            && (tmr_floorz - thing->z > 16*FRACUNIT)
          )
            goto block_move; // too big a step up for bouncers under gravity

        // killough 11/98: prevent falling objects from going up too many steps
        if( thing->eflags & MF_FALLING
            && (tmr_floorz - thing->z >
                FixedMul(thing->momx, thing->momx) + FixedMul(thing->momy, thing->momy))
          )
            goto block_move;
    }

    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    //added:28-02-98: gameplay hack : walk over a small wall while jumping
    //                stop jumping it succeeded
    // BP: removed in 1.28 because we can move in air now
    if( (EV_legacy >= 112) && (EV_legacy < 128)
         && thing->player
         && (thing->player->cheats & CF_JUMPOVER) )
    {
        if (tmr_floorz > thing->floorz + MAXSTEPMOVE)
            thing->momz >>= 2;
    }

    oldx = thing->x;
    oldy = thing->y;
    thing->floorz = tmr_floorz;
    thing->ceilingz = tmr_ceilingz;
    thing->dropoffz = tmr_dropoffz;  // MBF
    thing->x = x;
    thing->y = y;

    // [WDJ] tmr_floorthing is in demoversion 113..131, otherwise NULL
    //added:28-02-98:
    if (tmr_floorthing)
        thing->eflags &= ~MF_ONGROUND;  //not on real floor
    else
        thing->eflags |= MF_ONGROUND;

    P_SetThingPosition (thing);

    if (EN_heretic
        && thing->flags2 & MF2_FOOTCLIP
        && P_GetThingFloorType (thing) != FLOOR_SOLID
       )
        thing->flags2 |= MF2_FEETARECLIPPED;
    else if (thing->flags2 & MF2_FEETARECLIPPED)  // don't need this test
        thing->flags2 &= ~MF2_FEETARECLIPPED;

    // if any special lines were hit, do the effect
    if ( !(thing->flags&(MF_TELEPORT|MF_NOCLIP))
         && (thing->type != MT_CHASECAM) && (thing->type != MT_SPIRIT))
    {
        while (numspechit--)
        {
            // see if the line was crossed
            ld = &lines[spechit[numspechit]];
#ifdef PARANOIA
            if( ! ld->special)
            {
                GenPrintf( EMSG_warn, "Special line list has line which is not special.\n" );
                continue;
            }
#endif
            side = P_PointOnLineSide (thing->x, thing->y, ld);
            oldside = P_PointOnLineSide (oldx, oldy, ld);
            if (side != oldside)
            {
                if (ld->special)
                {
                    // [WDJ] Attempt to track player that triggers voodoo doll
                    if((voodoo_mode >= VM_target) && (thing->player))
                    {
                        if( thing->player->mo == thing )
                        {
                            // Real player
                            // remember last player that tripped a special
                            spechit_player = thing->player;
                        }
                    }
                    P_CrossSpecialLine(ld, oldside, thing);
                }
            }
        }
    }

    return true;

boom_conditional_impact:
    // PrBoom unstuck code
    if( tm_mbf_unstuck )
    {
      return !(tmr_ceilingline && ! tm_touches(tmr_ceilingline))
             && !(  tmr_floorline && ! tm_touches(  tmr_floorline));
    }

impact:
    // hit something solid
    CheckMissileImpact(thing);

block_move:
    return false;
}


// [WDJ] MBF, from MBF, PrBoom, EternityEngine.
// PIT_ApplyTorque
//
// killough 9/12/98:
//
// Apply "torque" to objects hanging off of ledges, so that they fall off.
// It's not really torque, since Doom has no concept of rotation, but it's a
// convincing effect which avoids anomalies such as lifeless objects hanging
// more than halfway off of ledges, and allows objects to roll off of the
// edges of moving lifts, or to slide up and then back down stairs,
// or to fall into a ditch.
// If more than one linedef is contacted, the effects are cumulative,
// so balancing is possible.
//
static boolean PIT_ApplyTorque(line_t *ld)
{
   // If thing touches two-sided pivot linedef
   if(ld->backsector
      && tm_bbox[BOXRIGHT]  > ld->bbox[BOXLEFT]
      && tm_bbox[BOXLEFT]   < ld->bbox[BOXRIGHT]
      && tm_bbox[BOXTOP]    > ld->bbox[BOXBOTTOM]
      && tm_bbox[BOXBOTTOM] < ld->bbox[BOXTOP]
      && P_BoxOnLineSide(tm_bbox, ld) == -1 )
   {
      mobj_t *mo = tm_thing;
      fixed_t x, y;

      // lever arm
      fixed_t dist =
         + (ld->dx >> FRACBITS) * (mo->y >> FRACBITS)
         - (ld->dy >> FRACBITS) * (mo->x >> FRACBITS) 
         - (ld->dx >> FRACBITS) * (ld->v1->y >> FRACBITS)
         + (ld->dy >> FRACBITS) * (ld->v1->x >> FRACBITS);

      // dropoff direction
      if(dist < 0 ?
           ld->frontsector->floorheight < mo->z &&
           ld->backsector->floorheight >= mo->z
         : ld->backsector->floorheight < mo->z &&
           ld->frontsector->floorheight >= mo->z
        )
      {
         // At this point, we know that the object straddles a two-sided
         // linedef, and that the object's center of mass is above-ground.

         x = abs(ld->dx);
         y = abs(ld->dy);

         if(y > x)
         {
            register fixed_t t = x;
            x = y;
            y = t;
         }

         y = finesine[(tantoangle[FixedDiv(y,x)>>DBITS] +
                      ANG90) >> ANGLETOFINESHIFT];

         // Applied Momentum is proportional to distance between the
         // object's center of mass and the pivot linedef.
         //
         // It is scaled by 2^(TIPSHIFT - tipcount). When tipcount is
         // increased, the momentum gradually decreases to 0 for
         // the same amount of pseudotorque, so that oscillations
         // are prevented, yet it has a chance to reach equilibrium.

         int tipping = TIPSHIFT - mo->tipcount;
         if( tipping >= 0 )
             y = y << tipping;
         else
             y = y >> -tipping;

         dist = FixedDiv( FixedMul(dist, y), x);

         // Apply momentum away from the pivot linedef.
                 
         x = FixedMul(ld->dy, dist);
         y = FixedMul(ld->dx, dist);

         // Avoid moving too fast all of a sudden (step into "overdrive")

         dist = FixedMul(x,x) + FixedMul(y,y);

         while(dist > FRACUNIT*4 && (mo->tipcount < MAXTIPCOUNT))
         {
            ++mo->tipcount;
            x >>= 1;
            y >>= 1;
            dist >>= 1;
         }
         
         mo->momx -= x;
         mo->momy += y;
      }
   }
   return true;
}


// [WDJ] MBF, from MBF, PrBoom, EternityEngine.
// P_ApplyTorque
//
// killough 9/12/98
// Applies "torque" to objects, based on all contacted linedefs
//
void P_ApplyTorque(mobj_t *mo)
{
    int xl, xh, yl, yh;
    int bx,by;
    // Remember the current state, for tipcount-change
    uint32_t moflags = mo->eflags;

    validcount++; // prevents checking same line twice
    tm_thing = mo;

    tm_bbox[BOXLEFT] = mo->x - mo->radius;
    tm_bbox[BOXRIGHT] = mo->x + mo->radius;
    tm_bbox[BOXBOTTOM] = mo->y - mo->radius;
    tm_bbox[BOXTOP] = mo->y + mo->radius;
    xl = (tm_bbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    xh = (tm_bbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    yl = (tm_bbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    yh = (tm_bbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    // Find tipping lines that apply torque.
    for(bx = xl ; bx <= xh ; bx++)
    {
        for(by = yl ; by <= yh ; by++)
           P_BlockLinesIterator(bx, by, PIT_ApplyTorque);
    }
      
    // If any momentum, mark object as 'falling' using engine-internal flags
    if( mo->momx | mo->momy )
        mo->eflags |= MF_FALLING;
    else  // Clear the engine-internal flag indicating falling object.
        mo->eflags &= ~MF_FALLING;

    // If the object has been moving, step up the tipcount.
    // This helps reach equilibrium and avoid oscillations.
    //
    // Doom has no concept of potential energy, much less
    // of rotation, so we have to creatively simulate these 
    // systems somehow :)

    if(!((mo->eflags | moflags) & MF_FALLING))  // If not falling for a while,
        mo->tipcount = 0;  // Reset it to full strength
    else if(mo->tipcount < MAXTIPCOUNT)  // Else if not at max tipcount,
        mo->tipcount++;    // lessen tipping
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
boolean P_ThingHeightClip (mobj_t* thing)
{
    // [WDJ] 10/12/2010 Floating monsters were not crushable because of added checks.
    // Check for crush first, and then modify if needed.
    boolean onfloor = (thing->z <= thing->floorz);
    boolean noncrush = 1;

    P_CheckPosition (thing, thing->x, thing->y);

    // what about stranding a monster partially off an edge?

    // tmr_ vars returned by P_CheckPosition
    thing->floorz = tmr_floorz;
    thing->ceilingz = tmr_ceilingz;
    thing->dropoffz = tmr_dropoffz;  // MBF
    noncrush = (thing->ceilingz - thing->floorz >= thing->height);
    
    // walker in contact with floor (direct or indirect)
    if ( onfloor
        && !(thing->flags & MF_NOGRAVITY))
    {
        // walking monsters rise and fall with the floor
        // [WDJ] tmr_floorthing is in demoversion 113..131, otherwise NULL
        if (!tmr_floorthing)  // unless standing on something
            thing->z = thing->floorz;
        // crush ok
    }
    else if (thing->z+thing->height >= tmr_ceilingz)  // in contact with ceiling
    {
        // don't adjust a floating monster unless forced to
        //added:18-04-98:test onfloor
        if (!onfloor)                    //was tmr_sectorceilingz
            thing->z = thing->ceilingz - thing->height;
        // crush ok
    }
    else
    { 
        noncrush = 1;
        //thing->eflags &= ~MF_ONGROUND;
    }

    //debug : be sure it falls to the floor
    thing->eflags &= ~MF_ONGROUND;

    return noncrush;

#if 0
    // [WDJ] Old code that caused floating monsters to not be crushable.

    //added:28-02-98:
    // test sector bouding top & bottom, not things

    //if (tmr_sectorceilingz - tmr_sectorfloorz < thing->height)
    //    return false;

    if (thing->ceilingz - thing->floorz < thing->height
        // BP: I know that this code cause many trouble but this fixes
        // much of problem, mainly this is implementation of the stepping 
        // for mobj (walk on solid corpse without jumping or fake 3d bridge).
        // Problem is imp into imp at map01 and monster going at top of others
        && thing->z >= thing->floorz)
    {
        return false;
    }

    return true;
#endif
}



//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//

// Try Slide Move global vars, P_HitSlideLine, PTR_SlideTraverse, P_SlideMove
fixed_t         tsm_bestslidefrac;
fixed_t         tsm_secondslidefrac;

line_t*         tsm_bestslideline;
line_t*         tsm_secondslideline;

mobj_t*         tsm_mo;	// sliding thing, set by P_SlideMove

fixed_t         tsm_xmove, tsm_ymove; // return from P_HitSlideLine, P_SlideMove




//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
// [phares] MBF: If the floor is icy, then you can bounce off a wall.

void P_HitSlideLine (line_t* ld)
{
    int                 side;

    angle_t             lineangle;
    angle_t             moveangle;
    angle_t             deltaangle;

    fixed_t             movelen;
    fixed_t             newlen;

    byte   icyfloor = 0;  // Boom, MBF, invoke icy floor bounce
         
    // [WDJ] MBF and Boom code for icy floor, from PrBoom and MBF.
    // phares
    // Under icy conditions, if the angle of approach to the wall
    // is more than 45 degrees, then you'll bounce and lose half
    // your momentum. If less than 45 degrees, you'll slide along
    // the wall. 45 is arbitrary and is believable.

    // Check for the special cases of horz or vert walls.

    // killough 10/98: only bounce if hit hard (prevents wobbling)
    // cph - DEMOSYNC - should only affect players in Boom demos?
    // e6y
    if( EN_variable_friction )
    {
        if( EN_mbf )
        {
            // killough 8/28/98: calc friction on demand       
            icyfloor =
              P_AproxDistance(tsm_xmove, tsm_ymove) > 4*FRACUNIT
              && tsm_mo->z <= tsm_mo->floorz
              && P_GetFriction( tsm_mo ) > ORIG_FRICTION;
        }
        else if( EN_boom )  // ! compatibility
        {
            icyfloor = onground  // player onground (p_user.c)
              && tsm_mo->player
              && tsm_mo->friction > ORIG_FRICTION;
        }
    }

    if (ld->slopetype == ST_HORIZONTAL)
    {
        if( icyfloor && (abs(tsm_ymove) > abs(tsm_xmove)) )
        {
            // bounce, and absorb half the momentum	   
            tsm_xmove /= 2;
            tsm_ymove = - tsm_ymove / 2;
            S_StartObjSound( tsm_mo, sfx_oof );	   
            return;
        }

        tsm_ymove = 0;
        return;
    }

    if (ld->slopetype == ST_VERTICAL)
    {
        if( icyfloor && (abs(tsm_xmove) > abs(tsm_ymove)) )
        {
            // bounce, and absorb half the momentum	   
            tsm_xmove = - tsm_xmove/2; // absorb half the momentum
            tsm_ymove /= 2;
            S_StartObjSound( tsm_mo, sfx_oof );	   
            return;
        }

        tsm_xmove = 0;
        return;
    }

    // [phares] The wall is angled. Bounce if the angle
    // of approach is less than 45 degrees.

    side = P_PointOnLineSide (tsm_mo->x, tsm_mo->y, ld);

    lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);

    if (side == 1)
        lineangle += ANG180;

    movelen = P_AproxDistance (tsm_xmove, tsm_ymove);
    moveangle = R_PointToAngle2 (0,0, tsm_xmove, tsm_ymove);
       
    // [WDJ] From PrBoom and MBF.
    // killough 3/2/98:
    // The moveangle+=10 breaks v1.9 demo compatibility in
    // some demos, so it needs demo_compatibility switch.
    if( EN_boom )  // ! demo_compatibility
    {
        // [phares] prevents sudden path reversal due to rounding error.
        moveangle += 10;
    }

    deltaangle = moveangle - lineangle;
    if( icyfloor && (deltaangle > ANG45) && (deltaangle < (ANG90+ANG45)) )
    {
        // Icy bounce
        moveangle = lineangle - deltaangle;
        movelen /= 2; // absorb
        tsm_xmove = FixedMul (movelen, cosine_ANG(moveangle));
        tsm_ymove = FixedMul (movelen, sine_ANG(moveangle));
        S_StartObjSound( tsm_mo, sfx_oof );
        return;
    }

    // Original slide      
    if (deltaangle > ANG180)
    {
        deltaangle += ANG180;
        //  I_Error ("SlideLine: ang>ANG180");
    }
    newlen = FixedMul (movelen, cosine_ANG(deltaangle));

    tsm_xmove = FixedMul (newlen, cosine_ANG(lineangle));
    tsm_ymove = FixedMul (newlen, sine_ANG(lineangle));
}


//
// PTR_SlideTraverse
//
boolean PTR_SlideTraverse (intercept_t* in)
{
    line_t*     li;

#ifdef PARANOIA
    if (!in->isaline)
        I_Error ("PTR_SlideTraverse: not a line?");
#endif

    li = in->d.line;

    if ( ! (li->flags & ML_TWOSIDED) )
    {
        if (P_PointOnLineSide (tsm_mo->x, tsm_mo->y, li))
        {
            // don't hit the back side
            return true;
        }
        goto isblocking;
    }

    // set openrange, opentop, openbottom
    P_LineOpening (li);

    if (openrange < tsm_mo->height)
        goto isblocking;                // doesn't fit

    if (opentop - tsm_mo->z < tsm_mo->height)
        goto isblocking;                // mobj is too high

    if (openbottom - tsm_mo->z > 24*FRACUNIT )
        goto isblocking;                // too big a step up

    // this line doesn't block movement
    return true;

    // the line does block movement,
    // see if it is closer than best so far
  isblocking:

    if (in->frac < tsm_bestslidefrac)
    {
        tsm_secondslidefrac = tsm_bestslidefrac;
        tsm_secondslideline = tsm_bestslideline;
        tsm_bestslidefrac = in->frac;
        tsm_bestslideline = li;
    }

    return false;       // stop
}



//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove (mobj_t* mo)
{
    fixed_t  leadx, leady;
    fixed_t  trailx, traily;
    fixed_t  newx, newy;
    int      hitcount;

    tsm_mo = mo;
    hitcount = 0;

  retry:
    // hitcount = 0, 1, 2
    if (++hitcount == 3)  // intercept on 3rd retry
        goto stairstep;         // don't loop forever


    // trace along the three leading corners
    if (mo->momx > 0)
    {
        leadx = mo->x + mo->radius;
        trailx = mo->x - mo->radius;
    }
    else
    {
        leadx = mo->x - mo->radius;
        trailx = mo->x + mo->radius;
    }

    if (mo->momy > 0)
    {
        leady = mo->y + mo->radius;
        traily = mo->y - mo->radius;
    }
    else
    {
        leady = mo->y - mo->radius;
        traily = mo->y + mo->radius;
    }

    tsm_bestslidefrac = FRACUNIT+1; // init for PTR_SlideTraverse

    P_PathTraverse ( leadx, leady, leadx+mo->momx, leady+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( trailx, leady, trailx+mo->momx, leady+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( leadx, traily, leadx+mo->momx, traily+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse );

    // move up to the wall
    if (tsm_bestslidefrac == FRACUNIT+1)
        goto stairstep;  // the move must have hit the middle, so stairstep

    // fudge a bit to make sure it doesn't hit
    tsm_bestslidefrac -= 0x800;
    if (tsm_bestslidefrac > 0)
    {
        newx = FixedMul (mo->momx, tsm_bestslidefrac);
        newy = FixedMul (mo->momy, tsm_bestslidefrac);

        if (!P_TryMove (mo, mo->x+newx, mo->y+newy, true))
            goto stairstep;
    }

    // Now continue along the wall.
    // First calculate remainder.
    tsm_bestslidefrac = FRACUNIT - (tsm_bestslidefrac+0x800);

    if (tsm_bestslidefrac > FRACUNIT)
        tsm_bestslidefrac = FRACUNIT;

    if (tsm_bestslidefrac <= 0)
        return;

    tsm_xmove = FixedMul (mo->momx, tsm_bestslidefrac);
    tsm_ymove = FixedMul (mo->momy, tsm_bestslidefrac);

    P_HitSlideLine (tsm_bestslideline);     // clip the moves

    mo->momx = tsm_xmove;
    mo->momy = tsm_ymove;

    // [WDJ] bobfactor from killough, via prboom, adapted to legacy.
    // killough 10/98: affect the bobbing the same way (but not voodoo dolls)
    if (mo->player && mo->player->mo == mo)
    {
        // [WDJ] Slide is an impact with a static object where only world mom applies.
        // The impact effect upon bob_mom depends upon that will force the bob_mom
        // to
        player_t * pl = mo->player;
#if 0
        // [WDJ] Because this is impact with fixed wall, it should
        // affect bob_mom the same way.
        pl->bob_momx = FixedMul (pl->bob_momx, tsm_bestslidefrac);
        pl->bob_momy = FixedMul (pl->bob_momy, tsm_bestslidefrac);
#else       
        // killough 10/98:
        // [WDJ] Should not transfer world mom to bob_mom.
        if( abs(pl->bob_momx) > abs(tsm_xmove) )
            pl->bob_momx = tsm_xmove;
        if( abs(pl->bob_momy) > abs(tsm_ymove) )
            pl->bob_momy = tsm_ymove;
#endif
    }

    if (!P_TryMove (mo, mo->x+tsm_xmove, mo->y+tsm_ymove, true))
    {
        goto retry;
    }
    return;

stairstep:
    //SoM: 4/10/2000  Allow things to drop off.
    if( !P_TryMove (mo, mo->x, mo->y + mo->momy, true) )
    {
      if( !P_TryMove (mo, mo->x + mo->momx, mo->y, true) )
      {
        // killough 10/98: keep buggy code around for old Boom demos
        // cph 2000/09//23: buggy code was only in Boom v2.01
        if( demoversion == 201 )
        {
          // phares 5/4/98: kill momentum if you can't move at all
          // This eliminates player bobbing if pressed against a wall
          // while on ice.
          mo->momx = mo->momy = 0;
        }
      }
    }
    return;
}


//
// P_LineAttack
//

// Line Attack global vars	   la_*
// Line Attack return global vars  lar_*
mobj_t*         la_shootthing;	    // attacker set by P_AimLineAttack, P_LineAttack
mobj_t*         lar_linetarget;     // who got hit (or NULL)

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t         la_shootz;
fixed_t         lar_lastz; //SoM: The last z height of the bullet when it crossed a line

int             la_damage;	 // damage per shot of weapon
fixed_t         la_attackrange;  // max range of weapon

fixed_t         lar_aimslope;

// killough 8/2/98: for more intelligent autoaiming (with friends)
static uint32_t  la_reject_flags;  // [WDJ]  MF_FRIEND or 0


//
// PTR_AimTraverse
// Sets lar_linetarget and lar_aimslope when a target is aimed at.
// Called by P_AimLineAttack
//
//added:15-02-98: comment
// Returns true if the thing is not shootable, else continue through..
//
boolean PTR_AimTraverse (intercept_t* in)
{
    line_t*             li;
    mobj_t*             th;
    fixed_t             slope;
    fixed_t             thingtopslope;
    fixed_t             thingbottomslope;
    fixed_t             dist;
    int                 dir;

    if (in->isaline)
    {
        li = in->d.line;

        if ( !(li->flags & ML_TWOSIDED) )
            return false;               // stop

        // Crosses a two sided line.
        // A two sided line will restrict
        // the possible target ranges.
        tm_thing = NULL;
        P_LineOpening (li);

        if (openbottom >= opentop)
            return false;               // stop

        dist = FixedMul (la_attackrange, in->frac);

        if (li->frontsector->floorheight != li->backsector->floorheight)
        {
            slope = FixedDiv (openbottom - la_shootz , dist);
            // see_bottomslope return by P_AimLineAttack
            if (slope > see_bottomslope)
                see_bottomslope = slope;
        }

        if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
            slope = FixedDiv (opentop - la_shootz , dist);
            // see_topslope return by P_AimLineAttack
            if (slope < see_topslope)
                see_topslope = slope;
        }

        if (see_topslope <= see_bottomslope)
            return false;               // stop

        if(li->frontsector->ffloors || li->backsector->ffloors)
        {
          int  frontflag;

          dir = lar_aimslope > 0 ? 1 : lar_aimslope < 0 ? -1 : 0;

          frontflag = P_PointOnLineSide(la_shootthing->x, la_shootthing->y, li);

          //SoM: Check 3D FLOORS!
          if(li->frontsector->ffloors)
          {
            ffloor_t*  rovflr = li->frontsector->ffloors;
            fixed_t    highslope, lowslope;

            for(; rovflr; rovflr = rovflr->next)
            {
              if( !(rovflr->flags & FF_SOLID)
                 || !(rovflr->flags & FF_EXISTS))
                 continue;

              highslope = FixedDiv (*rovflr->topheight - la_shootz, dist);
              lowslope = FixedDiv (*rovflr->bottomheight - la_shootz, dist);
              if((lar_aimslope >= lowslope && lar_aimslope <= highslope))
                return false;

              if(lar_lastz > *rovflr->topheight
                 && dir == -1
                 && lar_aimslope < highslope)
                frontflag |= 0x2;

              if(lar_lastz < *rovflr->bottomheight
                 && dir == 1
                 && lar_aimslope > lowslope)
                frontflag |= 0x2;
            }
          }

          if(li->backsector->ffloors)
          {
            ffloor_t*  rovflr = li->backsector->ffloors;
            fixed_t    highslope, lowslope;

            for(; rovflr; rovflr = rovflr->next)
            {
              if(!(rovflr->flags & FF_SOLID) || !(rovflr->flags & FF_EXISTS)) continue;

              highslope = FixedDiv (*rovflr->topheight - la_shootz, dist);
              lowslope = FixedDiv (*rovflr->bottomheight - la_shootz, dist);
              if((lar_aimslope >= lowslope && lar_aimslope <= highslope))
                return false;

              if( lar_lastz > *rovflr->topheight
                 && dir == -1
                 && lar_aimslope < highslope)
                frontflag |= 0x4;

              if( lar_lastz < *rovflr->bottomheight
                  && dir == 1
                  && lar_aimslope > lowslope)
                frontflag |= 0x4;
            }
          }
          if((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            return false;
        }

        lar_lastz = FixedMul (lar_aimslope, dist) + la_shootz;

        return true;                    // shot continues
    }

    // shoot at a thing th
    th = in->d.thing;
    if (th == la_shootthing)
        return true;                    // can't shoot self

    // DarkWolf95: Don't damage other monsters
    if( (monster_infight == INFT_coop)  // monster coop
        && la_shootthing->type != MT_PLAYER
        && th->type != MT_PLAYER)
    {
        return true;
    }

    if( (!(th->flags & MF_SHOOTABLE))
        || (th->flags & MF_CORPSE) || (th->type == MT_POD) )
        return true;                    // corpse or something

    // killough 7/19/98, 8/2/98:
    // friends don't aim at friends (except players), at least not first
    if( (th->flags & la_reject_flags) && !th->player )
        return true;

    // check angles to see if the thing can be aimed at
    dist = FixedMul (la_attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - la_shootz , dist);

    //added:15-02-98: bottomslope is negative!
    // see_bottomslope return by P_AimLineAttack
    if (thingtopslope < see_bottomslope)
        return true;                    // shot over the thing

    thingbottomslope = FixedDiv (th->z - la_shootz, dist);

    // see_topslope return by P_AimLineAttack
    if (thingbottomslope > see_topslope)
        return true;                    // shot under the thing

    // this thing can be hit!
    if (thingtopslope > see_topslope)
        thingtopslope = see_topslope;

    if (thingbottomslope < see_bottomslope)
        thingbottomslope = see_bottomslope;

    //added:15-02-98: find the slope just in the middle(y) of the thing!
    lar_aimslope = (thingtopslope+thingbottomslope)/2;
    lar_linetarget = th;

    return false;                       // don't go any farther
}


//
// PTR_ShootTraverse
//
//added:18-02-98: added clipping the shots on the floor and ceiling.
//
boolean PTR_ShootTraverse (intercept_t* in)
{
    fixed_t        x, y, z;
    fixed_t        frac;

    line_t*        li;
    sector_t*      sector=NULL;
    mobj_t*        th;

    fixed_t        slope;
    fixed_t        dist;
    fixed_t        thingtopslope, thingbottomslope;

    fixed_t        floorz = 0;  //SoM: Bullets should hit fake floors!
    fixed_t        ceilingz = 0;

    //added:18-02-98:
    fixed_t        distz;    //dist between hit z on wall       and gun z
    fixed_t        clipz;    //dist between hit z on floor/ceil and gun z
    boolean        hitplane;    //true if we clipped z on floor/ceil plane
    boolean        diffheights; //check for sky hacks with different ceil heights

    int            sectorside;
    int            dir;

    if(lar_aimslope > 0)
      dir = 1;
    else if(lar_aimslope < 0)
      dir = -1;
    else
      dir = 0;

    if (in->isaline)
    {
        //shut up compiler, otherwise it's only used when TWOSIDED
        diffheights = false;

        li = in->d.line;

#ifdef HEXEN
        if (li->special)
        {
            if( EN_hexen )
              P_ShootHexenSpecialLine (la_shootthing, li);
            else
              P_ShootSpecialLine (la_shootthing, li);
        }
#else
        if (li->special)
            P_ShootSpecialLine (la_shootthing, li);
#endif

        if ( !(li->flags & ML_TWOSIDED) )
            goto hitline;

        // crosses a two sided line
        //added:16-02-98: Fab comments : sets opentop, openbottom, openrange
        //                lowfloor is the height of the lowest floor
        //                         (be it front or back)
        tm_thing = NULL;
        P_LineOpening (li);

        dist = FixedMul (la_attackrange, in->frac);

        // hit lower texture ?
        if (li->frontsector->floorheight != li->backsector->floorheight)
        {
            //added:18-02-98: comments :
            // find the slope aiming on the border between the two floors
            slope = FixedDiv (openbottom - la_shootz , dist);
            if (slope > lar_aimslope)
                goto hitline;
        }

        // hit upper texture ?
        if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
            //added:18-02-98: remember : diff ceil heights
            diffheights = true;

            slope = FixedDiv (opentop - la_shootz , dist);
            if (slope < lar_aimslope)
                goto hitline;
        }

        if(li->frontsector->ffloors || li->backsector->ffloors)
        {
          int  frontflag;

          frontflag = P_PointOnLineSide(la_shootthing->x, la_shootthing->y, li);

          //SoM: Check 3D FLOORS!
          if(li->frontsector->ffloors)
          {
            ffloor_t*  rovflr = li->frontsector->ffloors;
            fixed_t    highslope, lowslope;

            for(; rovflr; rovflr = rovflr->next)
            {
              if(!(rovflr->flags & FF_SOLID) || !(rovflr->flags & FF_EXISTS)) continue;

              highslope = FixedDiv (*rovflr->topheight - la_shootz, dist);
              lowslope = FixedDiv (*rovflr->bottomheight - la_shootz, dist);
              if((lar_aimslope >= lowslope && lar_aimslope <= highslope))
                goto hitline;

              if( lar_lastz > *rovflr->topheight
                 && dir == -1
                 && lar_aimslope < highslope)
                frontflag |= 0x2;

              if( lar_lastz < *rovflr->bottomheight
                 && dir == 1
                 && lar_aimslope > lowslope)
                frontflag |= 0x2;
            }
          }

          if(li->backsector->ffloors)
          {
            ffloor_t*  rovflr = li->backsector->ffloors;
            fixed_t    highslope, lowslope;

            for(; rovflr; rovflr = rovflr->next)
            {
              if(!(rovflr->flags & FF_SOLID) || !(rovflr->flags & FF_EXISTS)) continue;

              highslope = FixedDiv (*rovflr->topheight - la_shootz, dist);
              lowslope = FixedDiv (*rovflr->bottomheight - la_shootz, dist);
              if((lar_aimslope >= lowslope && lar_aimslope <= highslope))
                goto hitline;

              if( lar_lastz > *rovflr->topheight
                 && dir == -1
                 && lar_aimslope < highslope)
                frontflag |= 0x4;

              if( lar_lastz < *rovflr->bottomheight
                 && dir == 1
                 && lar_aimslope > lowslope)
                frontflag |= 0x4;
            }
          }
          if((!(frontflag & 0x1) && frontflag & 0x2) || (frontflag & 0x1 && frontflag & 0x4))
            goto hitline;
        }
        lar_lastz = FixedMul (lar_aimslope, dist) + la_shootz;

        // shot continues
        return true;


        // hit line
      hitline:

        // position a bit closer
        frac = in->frac - FixedDiv (4*FRACUNIT, la_attackrange);
        dist = FixedMul (frac, la_attackrange);    //dist to hit on line

        distz = FixedMul (lar_aimslope, dist);      //z add between gun z and hit z
        z = la_shootz + distz;                     // hit z on wall

        //added:17-02-98: clip shots on floor and ceiling
        //                use a simple triangle stuff a/b = c/d ...
        // BP:13-3-99: fix the side usage
        hitplane = false;
        sectorside=P_PointOnLineSide(la_shootthing->x,la_shootthing->y,li);
        if( li->sidenum[sectorside] != NULL_INDEX ) // can happen in nocliping mode
        {
            sector = sides[li->sidenum[sectorside]].sector;
            floorz = sector->floorheight;
            ceilingz = sector->ceilingheight;
            if(sector->ffloors)
            {
              ffloor_t* rovflr;
              for(rovflr = sector->ffloors; rovflr; rovflr = rovflr->next)
              {
                if(!(rovflr->flags & FF_SOLID)) continue;

                if( dir == 1
                   && *rovflr->bottomheight < ceilingz
                   && *rovflr->bottomheight > lar_lastz)
                  ceilingz = *rovflr->bottomheight;
                if( dir == -1
                   && *rovflr->topheight > floorz
                   && *rovflr->topheight < lar_lastz)
                  floorz = *rovflr->topheight;
              }
            }

            if ((z > ceilingz) && distz)
            {
                clipz = ceilingz - la_shootz;
                frac = FixedDiv( FixedMul(frac,clipz), distz );
                hitplane = true;
            }
            else if ((z < floorz) && distz)
            {
                clipz = la_shootz - floorz;
                frac = -FixedDiv( FixedMul(frac,clipz), distz );
                hitplane = true;
            }
            if(sector->ffloors)
            {
                if(dir == 1 && z > ceilingz)
                    z = ceilingz;
                if(dir == -1 && z < floorz)
                    z = floorz;
            }
        }
        //SPLAT TEST ----------------------------------------------------------
        #ifdef WALLSPLATS
        if( !hitplane && (EV_legacy >= 129) )
        {
            divline_t   divl;
            fixed_t     frac;

            P_MakeDivline (li, &divl);
            frac = P_InterceptVector (&divl, &trace);
            R_AddWallSplat (li, sectorside, "A_DMG1", z, frac, SPLATDRAWMODE_SHADE);
        }
        #endif
        // --------------------------------------------------------- SPLAT TEST


        x = trace.x + FixedMul (trace.dx, frac);
        y = trace.y + FixedMul (trace.dy, frac);

        if (li->frontsector->ceilingpic == sky_flatnum)
        {
            // don't shoot the sky!
            if (z > li->frontsector->ceilingheight)
                return false;

            // it's a sky hack wall
            if( li->backsector && (li->backsector->ceilingpic == sky_flatnum))
            {
                if(demoversion<112)
                    return false;

                if( EV_legacy )
                {
                    //added:18-02-98:not for shots on planes
                    //added:18-02-98:skip only REAL sky hacks
                    //   eg: they use different ceil heights.
                    if( !hitplane && diffheights )
                        return false;
                }
                else if( EN_boom )
                {
                    if( li->backsector->ceilingheight < z )
                        return false;
                }
            }
        }

        if(sector && sector->ffloors)
        {
          if(dir == 1 && z + (16 << FRACBITS) > ceilingz)
            z = ceilingz - (16 << FRACBITS);
          if(dir == -1 && z < floorz)
            z = floorz;
        }
        // Spawn bullet puffs.
        P_SpawnPuff (x,y,z);

        // don't go any farther
        return false;
    }

    // shoot a thing
    th = in->d.thing;
    if (th == la_shootthing)
        return true;            // can't shoot self

    if (!(th->flags&MF_SHOOTABLE))
        return true;            // corpse or something

    // DarkWolf95: Don't damage other monsters
    if( (monster_infight == INFT_coop)  // monster coop
        && la_shootthing->type != MT_PLAYER
        && th->type != MT_PLAYER)
    {
        return true;
    }

    // check for physical attacks on a ghost
    if (EN_heretic
        && (th->flags & MF_SHADOW)
        && la_shootthing->player->readyweapon == wp_staff)
        return true;

    // check angles to see if the thing can be aimed at
    dist = FixedMul (la_attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - la_shootz , dist);

    if (thingtopslope < lar_aimslope)
        return true;            // shot over the thing

    thingbottomslope = FixedDiv (th->z - la_shootz, dist);

    if (thingbottomslope > lar_aimslope)
        return true;            // shot under the thing

    // SoM: SO THIS IS THE PROBLEM!!!
    // heh.
    // A bullet would travel through a 3D floor until it hit a LINEDEF! Thus
    // it appears that the bullet hits the 3D floor but it actually just hits
    // the line behind it. Thus allowing a bullet to hit things under a 3D
    // floor and still be clipped a 3D floor.
    if(th->subsector->sector->ffloors)
    {
      sector_t* sector = th->subsector->sector;
      ffloor_t* rovflr;

      for(rovflr = sector->ffloors; rovflr; rovflr = rovflr->next)
      {
        if(!(rovflr->flags & FF_SOLID))
          continue;

        if( dir == -1
           && *rovflr->topheight < lar_lastz
           && *rovflr->topheight > th->z + th->height)
          return true;
        if( dir == 1
           && *rovflr->bottomheight > lar_lastz
           && *rovflr->bottomheight < th->z)
          return true;
      }
    }

    // hit thing
    // position a bit closer
    frac = in->frac - FixedDiv (10*FRACUNIT, la_attackrange);

    x = trace.x + FixedMul (trace.dx, frac);
    y = trace.y + FixedMul (trace.dy, frac);
    z = la_shootz + FixedMul (lar_aimslope, FixedMul(frac, la_attackrange));

    if( EV_legacy < 125 )
    {
        // Spawn bullet puffs or blood spots,
        // depending on target type.
        if (th->flags & MF_NOBLOOD)
            P_SpawnPuff (x,y,z);
        else
            P_SpawnBlood (x,y,z, la_damage);
    }

    if (la_damage)
    {
#ifdef HERETIC_TODO_VERSION
        // [WDJ] Heretic, Hexen.  This test added 1.49
        if( EN_heretic_hexen && (EV_legacy >= HERETIC_TODO_VERSION)
            && !(th->flags & MF_NOBLOOD)
#ifdef HEXEN
            && !(th->flags3 & MF3_INVULNERABLE)
#endif
            )
        {
#ifdef HEXEN
            if( PuffType == MT_HEXEN_AXEPUFF
              || PuffType == MT_HEXEN_AXEPUFF_GLOW )
            {
                P_BloodSplatter2( x, y, z, th );  // MT_HEXEN_AXEBLOOD
            }
#endif
#if 0
            // [WDJ] Heretic, but not implemented
            // see other blood splatter.
            if( PP_Random(ph_heretic) < 192 )
            {
                P_BloodSplatter( x, y, z, th );  // MT_BLOODSPLATTER
            }
#endif	    
        }
#endif

#ifdef HEXEN
        if( EN_hexen
          && PuffType == MT_HEXEN_FLAMEPUFF2 )
        {
            extern  mobj_t  LavaInflictor;
            hitplane = P_DamageMobj (th, &LavaInflictor, la_shootthing, la_damage);
        }
        else
        {
            hitplane = P_DamageMobj (th, la_shootthing, la_shootthing, la_damage);
        }
#else
        hitplane = P_DamageMobj (th, la_shootthing, la_shootthing, la_damage);
#endif
    }
    else
    {
        hitplane = false;
    }

    if( EV_legacy >= 125 )
    {
        // Spawn bullet puffs or blood spots,
        // depending on target type.
        if ( EN_doom_etc
             && th->flags & MF_NOBLOOD )
        {
            P_SpawnPuff (x,y,z);
        }
#ifdef HEXEN
        else if( EN_hexen )
        {
            P_SpawnPuff(x, y, z);
        }
#endif
        else
        {
            if( EN_heretic )
            {
                if(PuffType == MT_BLASTERPUFF1)
                  // Make blaster big puff
                    S_StartObjSound(P_SpawnMobj(x, y, z, MT_BLASTERPUFF2), sfx_blshit);
                else
                    P_SpawnPuff(x, y, z);
            }    
            if (hitplane)
            {
                P_SpawnBloodSplats (x,y,z, la_damage, trace.dx, trace.dy);
                return false;
            }
        }
    }

    // don't go any farther
    return false;

}


//
// P_AimLineAttack
//
//  mbf_friend_protection is automatically disabled when not EN_mbf.
fixed_t P_AimLineAttack ( mobj_t*       atkr, // attacker
                          angle_t       angle,
                          fixed_t       distance,
                          byte          mbf_friend_protection )
{
    fixed_t     x2, y2;  // attack far endpoint

#ifdef PARANOIA
    if(!atkr)
       I_Error("P_aimlineattack: mobj == NULL !!!");
#endif

    int angf = ANGLE_TO_FINE(angle);
    la_shootthing = atkr;
   
    la_reject_flags = ( EN_mbf && mbf_friend_protection )?
                      ( atkr->flags & MF_FRIEND ) : 0;

    if(atkr->player && (EV_legacy >= 128) )
    {
        fixed_t cosineaiming= cosine_ANG( atkr->player->aiming );
        fixed_t aimtan = tangent_ANG( atkr->player->aiming );

        x2 = atkr->x + FixedMul(FixedMul(distance,finecosine[angf]),cosineaiming);
        y2 = atkr->y + FixedMul(FixedMul(distance,finesine[angf]),cosineaiming); 

        // Aimed result using CheckSight global vars
        see_topslope    =  100*FRACUNIT/160 + aimtan;
        see_bottomslope = -100*FRACUNIT/160 + aimtan;
    }
    else
    {
        // Doom, Boom
        x2 = atkr->x + (distance>>FRACBITS) * finecosine[angf];
        y2 = atkr->y + (distance>>FRACBITS) * finesine[angf];

        // Aimed result using CheckSight global vars
        //added:15-02-98: Fab comments...
        // Doom's base engine says that at a distance of 160,
        // the 2d graphics on the plane x,y correspond 1/1 with plane units
        see_topslope = 100*FRACUNIT/160;
        see_bottomslope = -100*FRACUNIT/160;
    }

    // Setup global parameters for P_PathTraverse, P_AimTraverse.
    la_shootz = atkr->z + (atkr->height>>1) + 8*FRACUNIT;
    lar_lastz = la_shootz;  // default result

    // can't shoot outside view angles

    la_attackrange = distance;
    lar_linetarget = NULL;  // default result

    //added:15-02-98: comments
    // traverse all linedefs and mobjs from the blockmap containing atkr,
    // to the blockmap containing the dest. point.
    // Call the function for each mobj/line on the way,
    // starting with the mobj/linedef at the shortest distance...
    P_PathTraverse ( atkr->x, atkr->y,
                     x2, y2,
                     PT_ADDLINES|PT_ADDTHINGS,
                     PTR_AimTraverse );

    //added:15-02-98: linetarget is only for mobjs, not for linedefs
    if (lar_linetarget)
        return lar_aimslope;

    return 0;
}


//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave lar_linetarget set.
//
//added:16-02-98: Fab comments...
//                atkr       est l'attaquant (player ou monstre)
//                angle    est l'angle de tir sur le plan x,y (orientation)
//                distance est la port‚e maximale de la balle
//                slope    est la pente vers la destination (up/down)
//                damage   est les degats infliges par la balle
void P_LineAttack ( mobj_t*       atkr,   // attacker
                    angle_t       angle,  // in x,y direction
                    fixed_t       distance, // max distance of weapon
                    fixed_t       slope,  // up/down
                    int           damage )  // damage per shot of weapon
{
    fixed_t     x2;
    fixed_t     y2;

    int angf = ANGLE_TO_FINE(angle);
    la_shootthing = atkr;
    la_damage = damage;

    // player autoaimed attack, 
    if( (EV_legacy < 128) || !atkr->player)
    {   
        x2 = atkr->x + (distance>>FRACBITS)*finecosine[angf]; 
        y2 = atkr->y + (distance>>FRACBITS)*finesine[angf];   
    }
    else
    {
        // Legacy player, with aiming.
        fixed_t cosineaiming=cosine_ANG(atkr->player->aiming);

        x2 = atkr->x + FixedMul(FixedMul(distance,finecosine[angf]), cosineaiming);
        y2 = atkr->y + FixedMul(FixedMul(distance,finesine[angf]), cosineaiming); 
    }

    la_shootz = lar_lastz = atkr->z + (atkr->height>>1) + 8*FRACUNIT;
#ifdef HEXEN
    if( EN_hexen )
    {
        la_shootz -= atkr->floorclip;
    }
    else
#endif
    if (atkr->flags2 & MF2_FEETARECLIPPED)
        la_shootz -= FOOTCLIPSIZE;

    la_attackrange = distance;
    lar_aimslope = slope;

    tm_thing = la_shootthing;

#ifdef HEXEN
    if( P_PathTraverse ( atkr->x, atkr->y,
                     x2, y2,
                     PT_ADDLINES|PT_ADDTHINGS,
                     PTR_ShootTraverse )  )
    {
        if( EN_hexen )
        {
            int sfx_punch = sfx_None;
            switch( PuffType )
            {
             case MT_HEXEN_PUNCHPUFF:
                sfx_punch = sfx_hexen_fighter_punch_miss;
                break;
             case MT_HEXEN_HAMMERPUFF:
             case MT_HEXEN_AXEPUFF:
             case MT_HEXEN_AXEPUFF_BLOW:
                sfx_punch = sfx_hexen_fighter_punch_miss;
                break;
             case MT_HEXEN_FLAMEPUFF:
                P_SpawnPuff( x2, y2, la_shootz + FixedMul( slope, distance ));
                break;
             default:
                break;
            }
            if( sfx_punch )
              S_StartMobjSound( th, sfx_punch );
        }
    }
#else
    P_PathTraverse ( atkr->x, atkr->y,
                     x2, y2,
                     PT_ADDLINES|PT_ADDTHINGS,
                     PTR_ShootTraverse );
#endif    
}

//
// USE LINES
//
mobj_t*         usething;

#ifdef HEXEN
static
int hexen_player_failed_sound( player_t * pl )
{
    int sfx_f = sfx_None;
    if( pl )
    {
        // If these are sequential, then use table lookup
        switch( pl->pclass )
        {
         case PCLASS_FIGHTER:
            sfx_f = sfx_hexen_player_fighter_failed_use;
            break;
         case PCLASS_CLERIC:
            sfx_f = sfx_hexen_player_cleric_failed_use;
            break;
         case PCLASS_MAGE:
            sfx_f = sfx_hexen_player_mage_failed_use;
            break;
         case PCLASS_PIG:
            sfx_f = sfx_hexen_pig_active1;
            break;
         default:
            sfx_f = sfx_None;
            break;
        }
    }
    return sfx_f;    
}
    
#endif


boolean PTR_UseTraverse (intercept_t* in)
{
    line_t *  ld = in->d.line;
    int       side;

    tm_thing = NULL;
    if( ! ld->special )
    {
        P_LineOpening( ld );
        if (openrange <= 0)
        {
#ifdef HEXEN
            int  sfx_trav = sfx_None;
            if( EN_hexen )
            {
                sfx_trav = hexen_player_failed_sound( usething->player );
            }
            else
            if( EN_doom_etc )
                sfx_trav = sfx_noway;

            if( sfx_trav )
                S_StartObjSound(usething, sfx_trav);
#else
            if( EN_doom_etc )
                S_StartObjSound(usething, sfx_noway);
#endif

            // can't use through a wall
            return false;
        }

#ifdef OOF_2S
        // [WDJ] Replace PrBoom 2s line oof test.
        // if (openrange <= 0) then the above test has already made a sound.
        if( oof_2s_hit == 0 )  // still enabled
        {
            if( ( ld->flags & ML_BLOCKING )   // Always blocking
                || (openbottom > (usething->z + 24*FRACUNIT))     // Too high it blocks
                || (opentop < (usething->z + usething->height)) ) // Too low it blocks
                oof_2s_hit = 2;
            // Need to keep looking for special lines.
        }
#endif

#ifdef HEXEN
        if( EN_hexen && usething->player )
        {
            fixed_t pu_height = usething->z + (usething->height >> 1);
            if( opentop < pu_height || openbottom > pu_height )
            {
                int sfx_f = hexen_player_failed_sound( usething->player );
                if( sfx_f )
                  S_StartObjSound(usething, sfx_f);
            }
        }
#endif

        // not a special line, but keep checking
        return true ;
    }

    // Special line
    side = 0;
    if( P_PointOnLineSide( usething->x, usething->y, ld ) == 1 )
        side = 1;

    if( P_UseSpecialLine( usething, ld, side ) )
    {
        // [WDJ] Attempt to track player that triggers voodoo doll
        if((voodoo_mode >= VM_target) && (usething->player))
        {
            if( usething->player->mo == usething )
            {
                // Real player pushed a switch
                spechit_player = usething->player;
            }
        }
    }

    // Normally can't use for than one special line.
    if(EN_boom && (ld->flags&ML_PASSUSE))  // SoM: USE MORE THAN ONE!  Boom passthru.
        return true;

    // found a special line, stop
    return false;
}

// From PrBoom, by Lee Killough
// An "oof" sound should be made because of a blocking linedef.
// Makes 2s middles which are impassable, as well as 2s uppers
// and lowers which block the player, cause the sound effect when the
// player tries to activate them. Specials are excluded, although it is
// assumed that all special linedefs within reach have been considered
// and rejected already (see P_UseLines).
// [WDJ] In PrBoom this search was done using PTR_NoWayTraverse, but
// Legacy code sets oof_2s_hit during PTR_UseTraverse.

#ifdef PRBOOM_NOWAY
boolean PTR_NoWayTraverse( intercept_t* in )
{
    line_t *ld = in->d.line;

    // [WDJ] In PrBoom this was, of course, crammed all into one expression.
    // This linedef
    if( ld->special )    // Ignore specials
        return true;
   
    if( ld->flags & ML_BLOCKING )   // Always blocking
        return false;  // hit

    P_LineOpening(ld);  // Find openings
   
    return
    !(
        openrange <= 0                       // No opening
        || openbottom > (usething->z + 24*FRACUNIT)    // Too high it blocks
        || opentop < (usething->z + usething->height)  // Too low it blocks
     );
}
#endif
 
#ifdef PRBOOM_NOWAY
// In P_UseLines
    // From PrBoom
    // Make the "oof" sound work on 2s lines -- killough
    if( P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse ) )
    {
        // Hit nothing
        if( !comp[comp_sound]
            && !P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_NoWayTraverse ) )
            S_StartSound (usething, sfx_noway);
    }
#endif


//
// P_UseLines
// Looks for special lines in front of the player to activate.
//
void P_UseLines (player_t* player)
{
    int         angf;
    fixed_t     x1, y1, x2, y2;

    usething = player->mo;

    angf = ANGLE_TO_FINE(player->mo->angle);

    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angf];
    y2 = y1 + (USERANGE>>FRACBITS)*finesine[angf];

#ifdef OOF_2S
    // Make the "oof" sound work on 2s lines
    // Avoid calling PathTraverse twice (like PrBoom) by doing it all the first time.
    oof_2s_hit = oof_2s_arm;  // 0 when enabled, 0x80 disabled ( !comp[comp_sound] )
    if( P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse ) )
    {
        // Did not hit any specials.
        if( oof_2s_hit == 2 )  // hit a 2s line
            S_StartObjSound (usething, sfx_noway);
    }
#else
    P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
#endif
}


//
// RADIUS ATTACK
//
mobj_t*         bomb_source;
mobj_t*         bomb_spot;
int             bomb_damage;
#if defined( MBF21 ) || defined( HEXEN )
int             bomb_distance;
#endif
#ifdef HEXEN
boolean         bomb_can_damage_source;
#endif

#if defined( MBF21 ) || defined( HEXEN )
// [WDJ] MBF21, From DSDA-Doom, to handle MBF21 splash damage.
static inline
boolean MBF21_splash_immune( int target_type, int spot_type )
{
    // not default behaviour and same group
    return
      ( mobjinfo[target_type].splash_group != SG_DEFAULT )
      && ( mobjinfo[target_type].splash_group == mobjinfo[spot_type].splash_group );
}

static inline
int P_SplashDamage( fixed_t dist )
{
    // [WDJ] MBF21, from DSDA-Doom, from EternityEngine
    // [XA] independent damage/distance calculation.
    //      same formula as eternity; thanks Quas :P
    // Modified for DoomLegacy.
    int damage1 = bomb_damage - dist;
#ifdef HEXEN    
    if( EN_hexen
        || bomb_damage != bomb_distance )  // or MBF21
#else	
    if( bomb_damage != bomb_distance )  // MBF21
#endif      
      return (bomb_damage * damage1 / bomb_distance) + 1;

    // The old splash damage.
    return damage1;
}
#endif



//
// PIT_RadiusAttack
// "bomb_source" is the creature
// that caused the explosion at "bomb_spot".
//
boolean PIT_RadiusAttack (mobj_t* thing)
{
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     dz;
    fixed_t     dist;

    // killough 8/20/98: allow bouncers to take damage
    // (missile bouncers are already excluded with MF_NOBLOCKMAP)
    if( !(thing->flags & (MF_SHOOTABLE | MF_BOUNCES)) )
        return true;

#if defined( MBF21 ) || defined( HEXEN )
    if( MBF21_splash_immune( thing->type, bomb_spot->type ) )
        return true;
#endif

#ifdef HEXEN
    if( EN_hexen )
    {
        if( ! bomb_can_damage_source && (thing == bomb_source) )
        {
            // Do not damage the source of the explosion.
            return true;
        }

        if( abs( (thing->z - bomb_spot->z) >> FRACBITS ) > (2 * bomb_distance))
        {
            // Too high or low
            return true;
        }
    }
    else
#endif
    // killough 8/10/98: allow grenades to hurt anyone, unless
    // fired by Cyberdemons, in which case it won't hurt Cybers.
    // EternityEngine has a more complicated system.
    if( bomb_spot->flags & MF_BOUNCES )
    {
        if( thing->type == MT_CYBORG && bomb_source->type == MT_CYBORG )
            return true;
    }
    // Boss spider and cyborg take no damage from concussion.
#ifdef MBF21
    // MT_CYBORG, MT_SPIDER, MT_MINOTAUR, MT_SORCERER1, MT_SORCERER2
    else if( (thing->flags2 & MF2_BOSS)
             && (thing->flags3 & MF3_NORADIUSDMG)
             && !(bomb_spot->flags3 & MF3_FORCERADIUSDMG) )
            return true;
#else
    else if (thing->type == MT_CYBORG
        || thing->type == MT_SPIDER
        || thing->type == MT_MINOTAUR 
        || thing->type == MT_SORCERER1
        || thing->type == MT_SORCERER2)
        return true;
#endif

    dx = abs(thing->x - bomb_spot->x);
    dy = abs(thing->y - bomb_spot->y);

    dist = dx>dy ? dx : dy;
    dist -= thing->radius;

    //added:22-02-98: now checks also z dist for rockets exploding
    //                above yer head...
    if( EV_legacy >= 112 )
    {
        dz = abs(thing->z + (thing->height>>1) - bomb_spot->z);
        dist = dist > dz ? dist : dz;
    }
    dist >>= FRACBITS;

    if (dist < 0)
        dist = 0;

    if (dist >= bomb_damage)
        return true;    // out of range

    if (thing->floorz > bomb_spot->z && bomb_spot->ceilingz < thing->z)
        return true;

    if (thing->ceilingz < bomb_spot->z && bomb_spot->floorz > thing->z)
        return true;

    if ( P_CheckSight (thing, bomb_spot) )
    {
#if defined( MBF21 ) || defined( HEXEN )
        int damage = P_SplashDamage( dist );
# ifdef HEXEN
        if( EN_hexen && thing->player )
        {
            damage >>= 2;  // less damage to player
        }
# endif
#else
        int  damage=bomb_damage - dist;
#endif
        int  momx=0,momy=0;
        if( dist )
        {
            momx = (thing->x - bomb_spot->x)/dist;
            momy = (thing->y - bomb_spot->y)/dist;
        }
        // must be in direct path
        if( P_DamageMobj (thing, bomb_spot, bomb_source, damage)
            && (thing->flags & MF_NOBLOOD)==0
            && (EV_legacy >= 129) )
            P_SpawnBloodSplats (thing->x,thing->y,thing->z, damage, momx, momy);
    }

    return true;
}


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
#ifdef MBF21
void P_RadiusAttack_MBF21 ( mobj_t* spot, mobj_t* source, int damage, int distance, boolean can_damage_source )
{
    int  x, y;
    int  xl, xh;
    int  yl, yh;
    fixed_t  dist;

    dist = (damage+MAXRADIUS)<<FRACBITS;
    yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
    bomb_spot = spot;
    if (spot->type == MT_POD && spot->target) // Heretic
        bomb_source = spot->target;
    else
        bomb_source = source;

    bomb_damage = damage;
    bomb_distance = distance;
#ifdef HEXEN    
    bomb_can_damage_source = can_damage_source;
#endif

    for (y=yl ; y<=yh ; y++)
    {
        for (x=xl ; x<=xh ; x++)
            P_BlockThingsIterator (x, y, PIT_RadiusAttack );
    }
}

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack ( mobj_t* spot, mobj_t* source, int damage )
{
    // vanilla: distance = damage
    // damage, distance, can_damage_source
    P_RadiusAttack_MBF21( spot, source, damage, damage, true );
}

#else

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack ( mobj_t*       spot,
                      mobj_t*       source,
                      int           damage )
{
    int         x;
    int         y;

    int         xl;
    int         xh;
    int         yl;
    int         yh;

    fixed_t     dist;

    dist = (damage+MAXRADIUS)<<FRACBITS;
    yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
    bomb_spot = spot;
    if (spot->type == MT_POD && spot->target)
        bomb_source = spot->target;
    else
        bomb_source = source;
    bomb_damage = damage;

    for (y=yl ; y<=yh ; y++)
    {
        for (x=xl ; x<=xh ; x++)
            P_BlockThingsIterator (x, y, PIT_RadiusAttack );
    }
}
#endif



//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//
boolean     crush_enable;	// enable crushing damage
boolean     crush_nofit;
sector_t *  crush_sectorchecked;  // not used

//
// PIT_ChangeSector
//
boolean PIT_ChangeSector (mobj_t*  thing)
{
    mobj_t*     mo;

    if (P_ThingHeightClip (thing))
        goto done;  // keep checking

    // crunch bodies to giblets
    if (thing->flags & MF_CORPSE)
    {
        if( EN_doom_etc )
        {
            // Doom crush
            P_SetMobjState (thing, S_GIBS);
            thing->flags &= ~MF_SOLID;
            //added:22-02-98: lets have a neat 'crunch' sound!
            S_StartObjSound(thing, sfx_slop);
            thing->skin = 0;
        }
        thing->height = 0;
        thing->radius = 0;
        thing->skin = 0;

        goto done; // keep checking
    }

    // crunch dropped items
    if (thing->flags & MF_DROPPED)
    {
        P_RemoveMobj (thing);

        goto done; // keep checking
    }

    // [WDJ] From Prboom, MBF, EternityEngine
    // killough 11/98: kill touchy things immediately
    if( (thing->flags & MF_TOUCHY)
        && (thing->eflags & MF_ARMED || SENTIENT(thing)) )
    {
        P_DamageMobj(thing, NULL, NULL, thing->health);  // kill object
        goto done; // keep checking
    }

    if (! (thing->flags & MF_SHOOTABLE) )
        goto done; // keep checking, assume it is bloody gibs or something

    crush_nofit = true;

    if (crush_enable && !(leveltime % (4*NEWTICRATERATIO)) )
    {
        // Crushing damage
        P_DamageMobj(thing,NULL,NULL,10);

        if( (EV_legacy < 132)
            || (!(leveltime % (16*NEWTICRATERATIO))
                && !(thing->flags&MF_NOBLOOD)) )
        {
            // Doom, Boom, Heretic
            // spray blood in a random direction
            mo = P_SpawnMobj (thing->x,
                              thing->y,
                              thing->z + thing->height/2, MT_BLOOD);
            
            mo->momx  = PP_SignedRandom(pr_crush)<<12;
            mo->momy  = PP_SignedRandom(pr_crush)<<12;
        }
    }

done: 
    // keep checking (crush other things)
    return true;
    // return boolean because of use in BlockThingsIterator
}



//
// P_ChangeSector
//
// crunch enables crushing damage
boolean P_ChangeSector ( sector_t*     sector,
                         boolean       crunch )
{
    int         x;
    int         y;

    crush_nofit = false;
    crush_enable = crunch;	// enable/disable crushing damage
    crush_sectorchecked = sector;

    // re-check heights for all things near the moving sector
    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
    {
        for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
            P_BlockThingsIterator (x, y, PIT_ChangeSector);
    }


    return crush_nofit;
}


//SoM: 3/15/2000: New function. Much faster.
// Check sector after floor or ceiling movement, update affected structures.
// crunch enables crushing damage
boolean P_CheckSector(sector_t* sector, boolean crunch)
{
  msecnode_t   *ns;

  if (!EN_boom) // use the old routine for old demos though
    return P_ChangeSector(sector,crunch);

  crush_nofit = false;
  crush_enable = crunch;


  // killough 4/4/98: scan list front-to-back until empty or exhausted,
  // restarting from beginning after each thing is processed. Avoids
  // crashes, and is sure to examine all things in the sector, and only
  // the things which are in the sector, until a steady-state is reached.
  // Things can arbitrarily be inserted and removed and it won't mess up.
  //
  // killough 4/7/98: simplified to avoid using complicated counter

  // [WDJ] Previous code made n! passes, so changed to reduce passes over list.
  // PIT_ChangeSector calls P_ThingHeightClip, which calls PIT_CheckPosition
  // which checks for pick-up items, which can remove any thing in sector.
  // If node has been freed, then start over from touching_thinglist.
  // Usually makes 3 passes, no matter how many things.

  if(sector->numattached)
  {
    int            i;
    sector_t*      sec;
    for(i = 0; i < sector->numattached; i ++)
    {
      // for each sector attached to the moved sector
      sec = &sectors[sector->attached[i]];
      // Mark things as stale.
      for (ns=sec->touching_thinglist; ns; ns=ns->m_snext)
        ns->visited = false;

#ifdef SECTOR_FLAGS
      sec->flags |= SCF_floor_moved;  // the sector lightlists are stale
#else
      sec->moved = true;  // the sector lightlists are stale
#endif

      // Update things until all are visited.
      do {
        for (ns=sec->touching_thinglist; ns; ns=ns->m_snext)
        {
          if (!ns->visited)               // unprocessed thing found
          {
            ns->visited  = true;         // mark thing as processed
            if (!(ns->m_thing->flags & MF_NOBLOCKMAP))
            {
              PIT_ChangeSector(ns->m_thing); // ns may get removed to freelist
              if( ns->m_sector != sec )  break;  // was freed
            }
          }
        }
      } while (ns);
    }
  }
  // Mark all things invalid in moved sector
#ifdef SECTOR_FLAGS
  sector->flags |= SCF_floor_moved;  // the moved sector lightlist is stale
#else
  sector->moved = true;  // the moved sector lightlist is stale
#endif

  // Mark things as stale.
  for (ns=sector->touching_thinglist; ns; ns=ns->m_snext)
      ns->visited = false;
  
  // Update things until all are visited.
  do {
      for (ns=sector->touching_thinglist; ns; ns=ns->m_snext)  // go through list
      {
          if (!ns->visited)               // unprocessed thing found
          {
              ns->visited  = true;          // mark thing as processed
              if (!(ns->m_thing->flags & MF_NOBLOCKMAP)) //jff 4/7/98 don't do these
              {
                  PIT_ChangeSector(ns->m_thing);    // process it
                  // may have removed ns to freelist
                  if( ns->m_sector != sector )  break;  // was freed
              }
          }
      }
  } while (ns);  // repeat from scratch until all things left are marked valid
  
  return crush_nofit;
}




/*
  SoM: 3/15/2000
  Lots of new Boom functions that work faster and add functionality.
*/

static msecnode_t* headsecnode = NULL;

void P_Init_Secnode( void )
{
    headsecnode = NULL;
}

// P_GetSecnode() retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.

msecnode_t* P_GetSecnode()
{
  msecnode_t* node;

  if (headsecnode)
  {
    node = headsecnode;
    headsecnode = headsecnode->m_snext;
  }
  else
    node = Z_Malloc (sizeof(*node), PU_LEVEL, NULL);
  return(node);
}

// P_PutSecnode() returns a node to the freelist.

void P_PutSecnode(msecnode_t* node)
{
    node->m_sector = NULL;  // to make being freed testable
    node->m_snext = headsecnode;
    headsecnode = node;
}

// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.

msecnode_t* P_AddSecnode(sector_t* s, mobj_t* thing, msecnode_t* nextnode)
{
  msecnode_t* node;

  node = nextnode;
  while (node)
  {
    if (node->m_sector == s)   // Already have a node for this sector?
    {
      node->m_thing = thing; // Yes. Setting m_thing says 'keep it'.
      return(nextnode);
    }
    node = node->m_tnext;
  }

  // Couldn't find an existing node for this sector. Add one at the head
  // of the list.

  node = P_GetSecnode();

  //mark new nodes unvisited.
  node->visited = 0;

  node->m_sector = s;       // sector
  node->m_thing  = thing;     // mobj
  node->m_tprev  = NULL;    // prev node on Thing thread
  node->m_tnext  = nextnode;  // next node on Thing thread
  if (nextnode)
    nextnode->m_tprev = node; // set back link on Thing

  // Add new node at head of sector thread starting at s->touching_thinglist

  node->m_sprev  = NULL;    // prev node on sector thread
  node->m_snext  = s->touching_thinglist; // next node on sector thread
  if (s->touching_thinglist)
    node->m_snext->m_sprev = node;
  s->touching_thinglist = node;
  return(node);
}


// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.

msecnode_t* P_DelSecnode(msecnode_t* node)
{
  msecnode_t* tp;  // prev node on thing thread
  msecnode_t* tn;  // next node on thing thread
  msecnode_t* sp;  // prev node on sector thread
  msecnode_t* sn;  // next node on sector thread

  if (node)
  {

    // Unlink from the Thing thread. The Thing thread begins at
    // sector_list and not from mobj_t->touching_sectorlist.

    tp = node->m_tprev;
    tn = node->m_tnext;
    if (tp)
      tp->m_tnext = tn;
    if (tn)
      tn->m_tprev = tp;

    // Unlink from the sector thread. This thread begins at
    // sector_t->touching_thinglist.

    sp = node->m_sprev;
    sn = node->m_snext;
    if (sp)
      sp->m_snext = sn;
    else
      node->m_sector->touching_thinglist = sn;
    if (sn)
      sn->m_sprev = sp;

    // Return this node to the freelist

    P_PutSecnode(node);
    return(tn);
  }
  return(NULL);
}

// Delete an entire sector list

void P_DelSeclist(msecnode_t* node)
{
    while (node)
        node = P_DelSecnode(node);
}


// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.

boolean PIT_GetSectors(line_t* ld)
{
//[WDJ] DEBUG
  if( ld->slopetype > ST_NEGATIVE || ld->slopetype < ST_HORIZONTAL )
  {
      GenPrintf(EMSG_warn, "PIT_GetSectors, bad line_t\n" );
      return false;
  }


  if (tm_bbox[BOXRIGHT]  <= ld->bbox[BOXLEFT]   ||
      tm_bbox[BOXLEFT]   >= ld->bbox[BOXRIGHT]  ||
      tm_bbox[BOXTOP]    <= ld->bbox[BOXBOTTOM] ||
      tm_bbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
    return true;

  if (P_BoxOnLineSide(tm_bbox, ld) != -1)
    return true;

  // This line crosses through the object.

  // Collect the sector(s) from the line and add to the
  // sector_list you're examining. If the Thing ends up being
  // allowed to move to this position, then the sector_list
  // will be attached to the Thing's mobj_t at touching_sectorlist.

  sector_list = P_AddSecnode(ld->frontsector,tm_thing,sector_list);

  // Don't assume all lines are 2-sided, since some Things
  // like MT_TFOG are allowed regardless of whether their radius takes
  // them beyond an impassable linedef.

  // Use sidedefs instead of 2s flag to determine two-sidedness.

  if (ld->backsector)
    sector_list = P_AddSecnode(ld->backsector, tm_thing, sector_list);

  return true;
}


// P_Create_SecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.

void P_Create_SecNodeList( mobj_t* thing, fixed_t x, fixed_t y )
{
  int xl, xh;
  int yl, yh;
  int bx, by;
  msecnode_t* node;

  // [WDJ] PrBoom has a problem here with tm_thing, tm_x, tm_y, due to
  // some functions called by PIT_GetSectors also using them.
  // PrBoom saves and restores those, unless it is in compatibility mode,
  // where it emulates the buggy behavior.
  // I have not found where tm_thing gets reused in this function.

  // First, clear out the existing m_thing fields. As each node is
  // added or verified as needed, m_thing will be set properly. When
  // finished, delete all nodes where m_thing is still NULL. These
  // represent the sectors the Thing has vacated.

  node = sector_list;
  while (node)
  {
    node->m_thing = NULL;
    node = node->m_tnext;
  }

  tm_thing = thing;
  tm_flags = thing->flags;

  tm_x = x;
  tm_y = y;

  tm_bbox[BOXTOP]  = y + tm_thing->radius;
  tm_bbox[BOXBOTTOM] = y - tm_thing->radius;
  tm_bbox[BOXRIGHT]  = x + tm_thing->radius;
  tm_bbox[BOXLEFT]   = x - tm_thing->radius;

  validcount++; // used to make sure we only process a line once

  xl = (tm_bbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
  xh = (tm_bbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
  yl = (tm_bbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
  yh = (tm_bbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

  for (bx=xl ; bx<=xh ; bx++)
  {
    for (by=yl ; by<=yh ; by++)
      P_BlockLinesIterator(bx, by, PIT_GetSectors);
  }
   
  // Add the sector of the (x,y) point to sector_list.

  sector_list = P_AddSecnode(thing->subsector->sector,thing,sector_list);

  // Now delete any nodes that won't be used. These are the ones where
  // m_thing is still NULL.

  node = sector_list;
  while (node)
  {
    if (node->m_thing == NULL)
    {
      if (node == sector_list)
        sector_list = node->m_tnext;
      node = P_DelSecnode(node);
    }
    else
      node = node->m_tnext;
  }
}

// heretic code

//---------------------------------------------------------------------------
//
// PIT_CheckOnmobjZ
//
//---------------------------------------------------------------------------
mobj_t * onmobj; //generic global onmobj...used for landing on pods/players

static
boolean PIT_CheckOnmobjZ(mobj_t * thing)
{
    fixed_t blockdist;
    
    if(!(thing->flags&(MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
    { // Can't hit thing
        return true;
    }
    blockdist = thing->radius+tm_thing->radius;
    if(abs(thing->x-tm_x) >= blockdist || abs(thing->y-tm_y) >= blockdist)
    { // Didn't hit thing
        return true;
    }
    if(thing == tm_thing)
    { // Don't clip against self
        return true;
    }
    if(tm_thing->z > thing->z+thing->height)
    {
        return true;
    }
    else if(tm_thing->z+tm_thing->height < thing->z)
    { // under thing
        return true;
    }
    if(thing->flags&MF_SOLID)
    {
        onmobj = thing;
    }
    return(!(thing->flags&MF_SOLID));
}

//=============================================================================
//
// P_FakeZMovement
//
//              Fake the zmovement so that we can check if a move is legal
//=============================================================================

static
void P_FakeZMovement(mobj_t * mo)
{
    int dist;
    int delta;
//
// adjust height
//
    mo->z += mo->momz;
    if(mo->flags&MF_FLOAT && mo->target)
    {       // float down towards target if too close
        if(!(mo->flags&MF_SKULLFLY) && !(mo->flags&MF_INFLOAT))
        {
            dist = P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y);
            delta =( mo->target->z + (mo->height>>1)) - mo->z;
            if (delta < 0 && dist < -(delta*3))
               mo->z -= FLOATSPEED;
            else if (delta > 0 && dist < (delta*3))
               mo->z += FLOATSPEED;
        }
    }
    if(mo->player && mo->flags2&MF2_FLY && !(mo->z <= mo->floorz)
        && leveltime&2)
    {
        mo->z += finesine[(FINEANGLES/20*leveltime>>2)&FINEMASK];
    }

//
// clip movement
//
    if(mo->z <= mo->floorz)
    { // Hit the floor
        mo->z = mo->floorz;
        if(mo->momz < 0)
        {
            mo->momz = 0;
        }
        if(mo->flags&MF_SKULLFLY)
        { // The skull slammed into something
            mo->momz = -mo->momz;
        }
        if(mo->info->crashstate && (mo->flags&MF_CORPSE))
        {
            return;
        }
    }
    else if(mo->flags2&MF2_LOGRAV)
    {
        if(mo->momz == 0)
           mo->momz = -(cv_gravity.value>>3)*2;
        else
           mo->momz -= cv_gravity.value>>3;
    }
    else if (! (mo->flags & MF_NOGRAVITY) )
    {
        if (mo->momz == 0)
           mo->momz = -cv_gravity.value*2;
        else
           mo->momz -= cv_gravity.value;
    }

    if (mo->z + mo->height > mo->ceilingz)
    {       // hit the ceiling
        if (mo->momz > 0)
           mo->momz = 0;
        mo->z = mo->ceilingz - mo->height;
        if (mo->flags & MF_SKULLFLY)
        {       // the skull slammed into something
            mo->momz = -mo->momz;
        }
    }
}

//=============================================================================
//
// P_CheckOnmobj(mobj_t *thing)
//
//              Checks if the new Z position is legal
//=============================================================================

mobj_t * P_CheckOnmobj( mobj_t * thing )
{
    int  xl,xh,yl,yh,bx,by;
    subsector_t  * cm_newsubsec;
    fixed_t x, y;
    mobj_t oldmo;
    
    x = thing->x;
    y = thing->y;
    tm_thing = thing;
    tm_flags = thing->flags;
    oldmo = *thing; // save the old mobj before the fake zmovement
    P_FakeZMovement(tm_thing);
    
    tm_x = x;
    tm_y = y;
    
    tm_bbox[BOXTOP] = y + tm_thing->radius;
    tm_bbox[BOXBOTTOM] = y - tm_thing->radius;
    tm_bbox[BOXRIGHT] = x + tm_thing->radius;
    tm_bbox[BOXLEFT] = x - tm_thing->radius;
    
    cm_newsubsec = R_PointInSubsector (x,y);
    tmr_ceilingline = NULL;
    
    //
    // The base floor / ceiling is from the subsector that contains the point.
    // Any lines contacted during the step may adjust them closer together.
    //
    tmr_floorz = tmr_dropoffz = cm_newsubsec->sector->floorheight;
    tmr_ceilingz = cm_newsubsec->sector->ceilingheight;
    
    validcount++;
    numspechit = 0;
    
    if ( tm_flags & MF_NOCLIP )
        return NULL;
    
    //
    // check things first, possibly picking things up
    // the bounding box is extended by MAXRADIUS because mobj_ts are grouped
    // into mapblocks based on their origin point, and can overlap into adjacent
    // blocks by up to MAXRADIUS units
    //
    xl = (tm_bbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (tm_bbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (tm_bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (tm_bbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
    {
        for (by=yl ; by<=yh ; by++)
        {
            if (!P_BlockThingsIterator(bx,by,PIT_CheckOnmobjZ))
            {
                *tm_thing = oldmo;
                return onmobj;
            }
        }
    }

    *tm_thing = oldmo;
    return NULL;
}

//----------------------------------------------------------------------------
//
// FUNC P_TestMobjLocation
//
// Returns true if the mobj is not blocked by anything at its current
// location, otherwise returns false.
//
//----------------------------------------------------------------------------

boolean P_TestMobjLocation( mobj_t * mobj )
{
        int flags;

        flags = mobj->flags;
        mobj->flags &= ~MF_PICKUP;
        if(P_CheckPosition(mobj, mobj->x, mobj->y))
        { // XY is ok, now check Z
                mobj->flags = flags;
                if((mobj->z < mobj->floorz)
                        || (mobj->z+mobj->height > mobj->ceilingz))
                { // Bad Z
                        return false;
                }
                return true;
        }
        mobj->flags = flags;
        return false;
}
