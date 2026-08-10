// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_lights.c 1736 2025-03-14 08:15:57Z wesleyjohnson $
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
// $Log: p_lights.c,v $
// Revision 1.5  2000/11/02 17:50:07  stroggonmeth
// Big 3Dfloors & FraggleScript commit!!
//
// Revision 1.4  2000/04/11 19:07:24  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.3  2000/04/04 00:32:46  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Handle Sector base lighting effects.
//      Muzzle flash?
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "doomstat.h"
#include "p_local.h"
#include "p_tick.h"
  // think
#include "r_state.h"
#include "z_zone.h"
#include "m_random.h"


// =========================================================================
//                           FIRELIGHT FLICKER
// =========================================================================

//
// T_FireFlicker
//
void T_FireFlicker (fireflicker_t* flick)
{
    lightlev_t  amount;

    if (--flick->count)
        return;

    amount = (PP_Random(pr_lights)&3)*16;

    if (flick->sector->lightlevel - amount < flick->minlight)
        flick->sector->lightlevel = flick->minlight;
    else
        flick->sector->lightlevel = flick->maxlight - amount;

    flick->count = 4;
}



//
// P_SpawnFireFlicker
//
void P_SpawnFireFlicker (sector_t*  sector)
{
    fireflicker_t*      flick;

    // Note that we are resetting sector attributes.
    // Nothing special about it during gameplay.
    sector->special &= ~31; //SoM: Clear non-generalized sector type

    flick = Z_Malloc ( sizeof(*flick), PU_LEVSPEC, 0);
    flick->thinker.function = TFI_FireFlicker;

    flick->sector = sector;
    flick->maxlight = sector->lightlevel;
    flick->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel)+16;
    flick->count = 4;

    P_AddThinker (&flick->thinker);
}



//
// BAD FLOURESCENT LIGHT FLASHING
//


//
// T_LightFlash
// Do flashing lights.
//
void T_LightFlash (lightflash_t* flash)
{
    if (--flash->count)
        return;

    if (flash->sector->lightlevel == flash->maxlight)
    {
        flash->sector->lightlevel = flash->minlight;
        flash->count = (PP_Random(pr_lights)&flash->mintime)+1;
    }
    else
    {
        flash-> sector->lightlevel = flash->maxlight;
        flash->count = (PP_Random(pr_lights)&flash->maxtime)+1;
    }

}




//
// P_SpawnLightFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnLightFlash (sector_t* sector)
{
    lightflash_t*  flash;

    // nothing special about it during gameplay
    sector->special &= ~31; //SoM: 3/7/2000: Clear non-generalized type

    flash = Z_Malloc ( sizeof(*flash), PU_LEVSPEC, 0);
    flash->thinker.function = TFI_LightFlash;

    flash->sector = sector;
    flash->maxlight = sector->lightlevel;

    flash->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
    flash->maxtime = 64;
    flash->mintime = 7;
    flash->count = (PP_Random(pr_lights)&flash->maxtime)+1;

    P_AddThinker (&flash->thinker);
}



//
// STROBE LIGHT FLASHING
//


//
// T_StrobeFlash
//
void T_StrobeFlash (strobe_t*  flash)
{
    if (--flash->count)
        return;

    if (flash->sector->lightlevel == flash->minlight)
    {
        flash-> sector->lightlevel = flash->maxlight;
        flash->count = flash->brighttime;
    }
    else
    {
        flash-> sector->lightlevel = flash->minlight;
        flash->count =flash->darktime;
    }

}



//
// P_SpawnStrobeFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void
P_SpawnStrobeFlash( sector_t* sector,
                    int fastOrSlow, int inSync )
{
    strobe_t*   flash;

    flash = Z_Malloc ( sizeof(*flash), PU_LEVSPEC, 0);
    flash->thinker.function = TFI_StrobeFlash;

    flash->sector = sector;
    flash->darktime = fastOrSlow;
    flash->brighttime = STROBEBRIGHT;
    flash->maxlight = sector->lightlevel;
    flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

    if (flash->minlight == flash->maxlight)
        flash->minlight = 0;

    // nothing special about it during gameplay
    sector->special &= ~31; //SoM: 3/7/2000: Clear non-generalized sector type

    if (!inSync)
        flash->count = (PP_Random(pr_lights)&7)+1;
    else
        flash->count = 1;

    P_AddThinker (&flash->thinker);
}


//
// Start strobing lights (usually from a trigger)
//
int EV_StartLightStrobing(line_t* line)
{
    sector_t*   sec;

    int secnum = -1; // init search FindSector
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
        if (P_SectorActive( S_lighting_special, sec)) //SoM: 3/7/2000: New way to check thinker
          continue;

        P_SpawnStrobeFlash (sec,SLOWDARK, 0);
    }
    return 1;
}



//
// TURN LINE'S TAG LIGHTS OFF
//
int EV_TurnTagLightsOff(line_t* line)
{
    lightlev_t          min;
    sector_t*           sector;
    sector_t*           tsec;
    line_t*             templine;

    sector = sectors;

    uint32_t j;
    for (j = 0; j < numsectors; j++, sector++)
    {
        // for each sector
        if (sector->tag == line->tag)
        {
            // for each sector with matching tag
            min = sector->lightlevel;
            uint32_t i;
            for (i = 0; i < sector->linecount; i++)
            {
                // for all lines in sector linelist
                templine = sector->linelist[i];
                tsec = getNextSector(templine,sector);
                if (!tsec)
                    continue;
                // find any lower light level
                if (tsec->lightlevel < min)
                    min = tsec->lightlevel;
            }
            sector->lightlevel = min;
        }
    }
    return 1;
}


//
// TURN LINE'S TAG LIGHTS ON
// Turn tagged sectors to specified or max neighbor level.
//
//  bright: light level,  0= use max neighbor light level
//  return 1
int EV_LightTurnOn ( line_t* line, lightlev_t bright )
{
    int         fsecn, j;
    lightlev_t  sll;  // set or max light level
    sector_t*   sector;
    sector_t*   adjsec;
    line_t*     adjline;

    // [WDJ] Fix segfault
    if( line == NULL )
        return -1;  // just to be different

    fsecn = -1;
    while ((fsecn = P_FindSectorFromLineTag(line, fsecn)) >= 0)
    {
        // For each sector with matching Tag
        sll = bright; //SoM: 3/7/2000: Search for maximum per sector
        sector = &sectors[fsecn];

        // For each sector with matching tag
        if( bright == 0 )
        {
            // Find max adjacent light.
            for (j = 0; j < sector->linecount; j++)
            {
                // for each line in sector linelist
                adjline = sector->linelist[j];
                adjsec = getNextSector(adjline,sector);

                if( !adjsec )
                    continue;

                // find any brighter light level
                if( sll < adjsec->lightlevel ) //SoM: 3/7/2000
                    sll = adjsec->lightlevel;
            }
        }
        sector->lightlevel = sll;

        if( !EN_boom_physics )  // old behavior
            bright = sll;  // maximums are not independent
    }
    return 1;
}


//
// Spawn glowing light
//

void T_Glow( glow_t* gp)
{
    switch(gp->direction)
    {
      case -1:
        // DOWN
        gp->sector->lightlevel -= GLOWSPEED;
        if (gp->sector->lightlevel <= gp->minlight)
        {
            gp->sector->lightlevel += GLOWSPEED;
            gp->direction = 1;
        }
        break;

      case 1:
        // UP
        gp->sector->lightlevel += GLOWSPEED;
        if (gp->sector->lightlevel >= gp->maxlight)
        {
            gp->sector->lightlevel -= GLOWSPEED;
            gp->direction = -1;
        }
        break;
    }
}


void P_SpawnGlowingLight( sector_t*  sector)
{
    glow_t* gp;

    gp = Z_Malloc( sizeof(*gp), PU_LEVSPEC, 0);
    gp->thinker.function = TFI_Glow;

    gp->sector = sector;
    gp->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
    gp->maxlight = sector->lightlevel;
    gp->direction = -1;

    sector->special &= ~0x1F; //SoM: 3/7/2000: Reset only non-generic types.

    P_AddThinker(& gp->thinker);
}



// P_FadeLight()
//
// Fade all the lights in sectors with a particular tag to a new value
//
void P_FadeLight( uint16_t tag, lightlev_t destvalue, lightlev_t speed)
{
  lightfader_t * lf;

  // search all sectors for ones with tag
  int secnum = -1; // init search FindSector
  while ((secnum = P_FindSectorFromTag(tag,secnum)) >= 0)
  {
      sector_t *sector = &sectors[secnum];
      sector->lightingdata = sector;    // just set it to something

      lf = Z_Malloc(sizeof(*lf), PU_LEVSPEC, 0);
      lf->thinker.function = TFI_LightFade;

      P_AddThinker(&lf->thinker);       // add thinker

      lf->sector = sector;
      lf->destlight = destvalue;
      lf->speed = speed;
  }
}



// T_LightFade()
//
// Just fade the light level in a sector to a new level
//

void T_LightFade(lightfader_t * lf)
{
  lightlev_t seclight = lf->sector->lightlevel;
   
  if(seclight < lf->destlight)
  {
    // increase the lightlevel
    seclight += lf->speed; // move lightlevel
    if(seclight >= lf->destlight)
      goto achieved_target;
  }
  else
  {
    // decrease lightlevel
    seclight -= lf->speed; // move lightlevel
    if(seclight <= lf->destlight)
      goto achieved_target;
  }
  lf->sector->lightlevel = seclight;
  return;
   
achieved_target:
  // stop changing light level
  lf->sector->lightlevel = lf->destlight;    // set to dest lightlevel

  lf->sector->lightingdata = NULL;          // clear lightingdata
  P_RemoveThinker(&lf->thinker);            // remove thinker       
}


// [WDJ] From MBF, PrBoom
// killough 10/98:
//
// Used for doors with gradual lighting effects.
// Turn light in tagged sectors to specified or max neighbor level.
//
//   line :  has sector TAG
//   level : light level fraction, 0=min, 1=max, else interpolate min..max
// Returns true
int EV_LightTurnOnPartway(line_t *line, fixed_t level)
{
  sector_t * sector;
  int i, j;
  int minll, maxll;

  // [WDJ] Fix segfault
  if( line == NULL )
    return -1;  // just to be different

  if (level < 0)          // clip at extremes
    level = 0;
  if (level > FRACUNIT)
    level = FRACUNIT;

  // search all sectors for ones with same tag as activating line
  i = -1;
  while ( (i = P_FindSectorFromLineTag(line,i)) >= 0 )
  {
      sector = &sectors[i];
      maxll = 0;
      minll = sector->lightlevel;

      for (j = 0; j < sector->linecount; j++)
      {
          sector_t * adjsec = getNextSector(sector->linelist[j], sector);
          if(adjsec)
          {
              if( maxll < adjsec->lightlevel )
                  maxll = adjsec->lightlevel;
              if( minll > adjsec->lightlevel )
                  minll = adjsec->lightlevel;
          }
      }

      // Set level in-between extremes
      sector->lightlevel =
        (level * maxll + (FRACUNIT-level) * minll) >> FRACBITS;
  }
  return 1;
}


// **** Corona and Dynamic lights

//Hurdler: now we can change those values via FS :)
// RGBA( r, g, b, a )
// Indexed by sprite_light_ind_e
spr_light_t  sprite_light[NUMLIGHTS] = {
    // type       offset x, y  coronas color, c_size,light color,l_radius, sqr radius computed at init
   // LT_NOLIGHT  
//    { SPLGT_none,     0.0f,   0.0f,        0x0,  24.0f,        0x0,   0.0f },
    { SPLGT_none,     0, 0, {RGBA(0,0,0,0)},  24.0f, {RGBA(0,0,0,0)},   0.0f },
    // weapons
    // LT_PLASMA
//    { SPLGT_dynamic,  0.0f,   0.0f, 0x60ff7750,  24.0f, 0x20f77760,  80.0f },
    { SPLGT_dynamic,  0, 0, {RGBA(0x50,0x77,0xff,0x60)},  24.0f, {RGBA(0x60,0x77,0xf7,0x20)},  80.0f },
    // LT_PLASMAEXP
//    { SPLGT_dynamic,  0.0f,   0.0f, 0x60ff7750,  24.0f, 0x40f77760, 120.0f },
    { SPLGT_dynamic,  0, 0, {RGBA(0x50,0x77,0xff,0x60)},  24.0f, {RGBA(0x60,0x77,0xf7,0x40)}, 120.0f },
    // LT_ROCKET
//    { SPLGT_rocket,   0,   0, 0x606060f0,  20, 0x4020f7f7, 120 },
    { SPLGT_rocket,   0, 0, {RGBA(0xf0,0x60,0x60,0x60)},  20.0f, {RGBA(0xf7,0xf7,0x20,0x40)}, 120.0f },
    // LT_ROCKETEXP
//    { SPLGT_dynamic,  0,   0, 0x606060f0,  20, 0x6020f7f7, 200 },
    { SPLGT_dynamic,  0, 0, {RGBA(0xf0,0x60,0x60,0x60)},  20.0f, {RGBA(0xf7,0xf7,0x20,0x60)}, 200.0f },
    // LT_BFG
//    { SPLGT_dynamic,  0,   0, 0x6077f777, 120, 0x8060f060, 200 },
    { SPLGT_dynamic,  0, 0, {RGBA(0x77,0xf7,0x77,0x60)}, 120.0f, {RGBA(0x60,0xf0,0x60,0x80)}, 200.0f },
    // LT_BFGEXP
//    { SPLGT_dynamic,  0,   0, 0x6077f777, 120, 0x6060f060, 400 },
    { SPLGT_dynamic,  0, 0, {RGBA(0x77,0xf7,0x77,0x60)}, 120.0f, {RGBA(0x60,0xf0,0x60,0x60)}, 400.0f },

    // tall lights
    // LT_BLUETALL
//    { SPLGT_light,    0,  27, 0x80ff7070,  75, 0x40ff5050, 100 },
//    { SPLGT_light,    0,27, {RGBA(0x70,0x70,0xff,0x80)},  75.0f, {RGBA(0x50,0x50,0xff,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,27, {RGBA(0x70,0x70,0xff,0x80)},  75.0f, {RGBA(0x50,0x50,0xff,0x40)}, 100.0f },
    // LT_GREENTALL
//    { SPLGT_light,    0,  27, 0x5060ff60,  75, 0x4070ff70, 100 },
//    { SPLGT_light,    0,27, {RGBA(0x60,0xff,0x60,0x50)},  75.0f, {RGBA(0x70,0xff,0x70,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,27, {RGBA(0x60,0xff,0x60,0x50)},  75.0f, {RGBA(0x70,0xff,0x70,0x40)}, 100.0f },
    // LT_REDTALL
//    { SPLGT_light,    0,  27, 0x705070ff,  75, 0x405070ff, 100 },
//    { SPLGT_light,    0,27, {RGBA(0xff,0x70,0x50,0x70)},  75.0f, {RGBA(0xff,0x70,0x50,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,27, {RGBA(0xff,0x70,0x50,0x70)},  75.0f, {RGBA(0xff,0x70,0x50,0x40)}, 100.0f },

    // small lights
    // LT_BLUESMALL
//    { SPLGT_light,    0,  14, 0x80ff7070,  60, 0x40ff5050, 100 },
//    { SPLGT_light,    0,14, {RGBA(0x70,0x70,0xff,0x80)},  60.0f, {RGBA(0x50,0x50,0xff,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,14, {RGBA(0x70,0x70,0xff,0x80)},  60.0f, {RGBA(0x50,0x50,0xff,0x40)}, 100.0f },
    // LT_GREENSMALL
//    { SPLGT_light,    0,  14, 0x6070ff70,  60, 0x4070ff70, 100 },
//    { SPLGT_light,    0,14, {RGBA(0x70,0xff,0x70,0x60)},  60.0f, {RGBA(0x70,0xff,0x70,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,14, {RGBA(0x70,0xff,0x70,0x60)},  60.0f, {RGBA(0x70,0xff,0x70,0x40)}, 100.0f },
    // LT_REDSMALL
//    { SPLGT_light,    0,  14, 0x705070ff,  60, 0x405070ff, 100 },
//    { SPLGT_light,    0,14, {RGBA(0xff,0x70,0x50,0x70)},  60.0f, {RGBA(0xff,0x70,0x50,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,14, {RGBA(0xff,0x70,0x50,0x70)},  60.0f, {RGBA(0xff,0x70,0x50,0x40)}, 100.0f },

    // other lights
    // LT_TECHLAMP
//    { SPLGT_light,    0,  33, 0x80ffb0b0,  75, 0x40ffb0b0, 100 },
//    { SPLGT_light,    0,33, {RGBA(0xb0,0xb0,0xff,0x80)},  75.0f, {RGBA(0xb0,0xb0,0xff,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_lamp,    0,33, {RGBA(0xb0,0xb0,0xff,0x80)},  75.0f, {RGBA(0xb0,0xb0,0xff,0x40)}, 100.0f },
    // LT_TECHLAMP2
//    { SPLGT_light,    0,  33, 0x80ffb0b0,  75, 0x40ffb0b0, 100 },
//    { SPLGT_light,    0,26, {RGBA(0xb0,0xb0,0xff,0x80)},  60.0f, {RGBA(0xb0,0xb0,0xff,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_lamp,    0,26, {RGBA(0xb0,0xb0,0xff,0x80)},  60.0f, {RGBA(0xb0,0xb0,0xff,0x40)}, 100.0f },
    // LT_COLUMN
//    { SPLGT_light,    3,  19, 0x80b0f0f0,  60, 0x40b0f0f0, 100 },
//    { SPLGT_light,    3,19, {RGBA(0xf0,0xf0,0xb0,0x80)},  60.0f, {RGBA(0xf0,0xf0,0xb0,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_lamp,    3,19, {RGBA(0xf0,0xf0,0xb0,0x80)},  60.0f, {RGBA(0xf0,0xf0,0xb0,0x40)}, 100.0f },
    // LT_CANDLE
//    { SPLGT_light,    0,   6, 0x60b0f0f0,  20, 0x30b0f0f0,  30 },
//    { SPLGT_light,    0, 6, {RGBA(0xf0,0xf0,0xb0,0x60)},  20.0f, {RGBA(0xf0,0xf0,0xb0,0x30)},  30.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0, 6, {RGBA(0xf0,0xf0,0xb0,0x60)},  20.0f, {RGBA(0xf0,0xf0,0xb0,0x30)},  30.0f },
    // LT_CANDLEABRE
//    { SPLGT_light,    0,  30, 0x60b0f0f0,  60, 0x30b0f0f0, 100 },
//    { SPLGT_light,    0,30, {RGBA(0xf0,0xf0,0xb0,0x60)},  60.0f, {RGBA(0xf0,0xf0,0xb0,0x30)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,    0,30, {RGBA(0xf0,0xf0,0xb0,0x60)},  60.0f, {RGBA(0xf0,0xf0,0xb0,0x30)}, 100.0f },
    
    // monsters
    // LT_REDBALL
//    { SPLGT_dynamic,   0,   0, 0x606060f0,   0, 0x302070ff, 100 },
    { SPLGT_dynamic,   0, 0, {RGBA(0xf0,0x60,0x60,0x60)},   0.0f, {RGBA(0xff,0x70,0x20,0x30)}, 100.0f },
    // LT_GREENBALL
//    { SPLGT_dynamic,   0,   0, 0x6077f777, 120, 0x3060f060, 100 },
    { SPLGT_dynamic,   0, 0, {RGBA(0x77,0xf7,0x77,0x60)}, 120, {RGBA(0x60,0xf0,0x60,0x30)}, 100.0f },
    // LT_ROCKET2
//    { SPLGT_dynamic,   0,   0, 0x606060f0,  20, 0x4020f7f7, 120 },
    { SPLGT_dynamic,   0, 0, {RGBA(0xf0,0x60,0x60,0x60)},  20.0f, {RGBA(0xf7,0xf7,0x20,0x40)}, 120.0f },

    // weapons
    // LT_FX03
//    { SPLGT_dynamic,   0,   0, 0x6077ff50,  24, 0x2077f760,  80 },
    { SPLGT_dynamic,   0, 0, {RGBA(0x50,0xff,0x77,0x60)},  24.0f, {RGBA(0x60,0xf7,0x77,0x20)},  80.0f },
    // LT_FX17
//    { SPLGT_dynamic,   0,   0, 0x60ff7750,  24, 0x40f77760,  80 },
    { SPLGT_dynamic,   0, 0, {RGBA(0x50,0x77,0xff,0x60)},  24.0f, {RGBA(0x60,0x77,0xf7,0x40)},  80.0f },
    // LT_FX00
//    { SPLGT_dynamic,   0,   0, 0x602020ff,  24, 0x302020f7,  80 },
    { SPLGT_dynamic,   0, 0, {RGBA(0xff,0x20,0x20,0x60)},  24.0f, {RGBA(0xf7,0x20,0x20,0x30)},  80.0f },
    // LT_FX08
//    { SPLGT_rocket,    0,   0, 0x606060f0,  20, 0x4020c0f7, 120 },
    { SPLGT_rocket,    0, 0, {RGBA(0xf0,0x60,0x60,0x60)},  20.0f, {RGBA(0xf7,0xc0,0x20,0x40)}, 120.0f },
    // LT_FX04
//    { SPLGT_rocket,    0,   0, 0x606060f0,  20, 0x2020c0f7, 120 },
    { SPLGT_rocket,    0, 0, {RGBA(0xf0,0x60,0x60,0x60)},  20.0f, {RGBA(0xf7,0xc0,0x20,0x20)}, 120.0f },
    // LT_FX02
//    { SPLGT_rocket,    0,   0, 0x606060f0,  20, 0x1720f7f7, 120 },
    { SPLGT_rocket,    0, 0, {RGBA(0xf0,0x60,0x60,0x60)},  20.0f, {RGBA(0xf7,0xf7,0x20,0x17)}, 120.0f },

    //lights
    // LT_WTRH
//    { SPLGT_dynamic,   0,  68, 0x606060f0,  60, 0x4020a0f7, 100 },
    { SPLGT_dynamic,   0,68, {RGBA(0xf0,0x60,0x60,0x60)},  60.0f, {RGBA(0xf7,0xa0,0x20,0x40)}, 100.0f },
    // LT_SRTC
//    { SPLGT_dynamic,   0,  27, 0x606060f0,  60, 0x4020a0f7, 100 },
    { SPLGT_dynamic,   0,27, {RGBA(0xf0,0x60,0x60,0x60)},  60.0f, {RGBA(0xf7,0xa0,0x20,0x40)}, 100.0f },
    // LT_CHDL
//    { SPLGT_dynamic,   0,  -8, 0x606060f0,  60, 0x502070f7, 100 },
    { SPLGT_dynamic,   0,-8, {RGBA(0xf0,0x60,0x60,0x60)},  60.0f, {RGBA(0xf7,0x70,0x20,0x50)}, 100.0f },
    // LT_KFR1
//    { SPLGT_dynamic,   0,  27, 0x606060f0,  60, 0x4020a0f7, 100 },
//    { SPLGT_dynamic,   0,27, {RGBA(0xf0,0x60,0x60,0x60)},  60.0f, {RGBA(0xf7,0xa0,0x20,0x40)}, 100.0f },
    { SPLGT_dynamic|SPLGT_corona|SPLT_fire,   0,27, {RGBA(0xf0,0x60,0x60,0x60)},  60.0f, {RGBA(0xf7,0xa0,0x20,0x40)}, 100.0f },
};



// sprite light indirection
// Indexed by spritenum_e
byte  sprite_light_ind[NUMSPRITES_DEF] = {
    LT_NOLIGHT,     // SPR_TROO
    LT_NOLIGHT,     // SPR_SHTG
    LT_NOLIGHT,     // SPR_PUNG
    LT_NOLIGHT,     // SPR_PISG
    LT_NOLIGHT,     // SPR_PISF
    LT_NOLIGHT,     // SPR_SHTF
    LT_NOLIGHT,     // SPR_SHT2
    LT_NOLIGHT,     // SPR_CHGG
    LT_NOLIGHT,     // SPR_CHGF
    LT_NOLIGHT,     // SPR_MISG
    LT_NOLIGHT,     // SPR_MISF
    LT_NOLIGHT,     // SPR_SAWG
    LT_NOLIGHT,     // SPR_PLSG
    LT_NOLIGHT,     // SPR_PLSF
    LT_NOLIGHT,     // SPR_BFGG
    LT_NOLIGHT,     // SPR_BFGF
    LT_NOLIGHT,     // SPR_BLUD
    LT_NOLIGHT,     // SPR_PUFF
    LT_REDBALL,   // SPR_BAL1 * // imp
    LT_REDBALL,   // SPR_BAL2 * // cacodemon
    LT_PLASMA,    // SPR_PLSS * // plasma
    LT_PLASMAEXP, // SPR_PLSE * // plasma explosion
    LT_ROCKET,    // SPR_MISL * // rocket
    LT_BFG,       // SPR_BFS1 * // bfg
    LT_BFGEXP,    // SPR_BFE1 * // bfg explosion
    LT_NOLIGHT,     // SPR_BFE2
    LT_GREENBALL, // SPR_TFOG * teleport fog
    LT_PLASMA,    // SPR_IFOG * respaw fog
    LT_NOLIGHT,     // SPR_PLAY
    LT_NOLIGHT,     // SPR_POSS
    LT_NOLIGHT,     // SPR_SPOS
    LT_NOLIGHT,     // SPR_VILE
    LT_NOLIGHT,     // SPR_FIRE
    LT_REDBALL,   // SPR_FATB * // revenent tracer
    LT_NOLIGHT,     // SPR_FBXP
    LT_NOLIGHT,     // SPR_SKEL
    LT_ROCKET2,   // SPR_MANF * // mancubus
    LT_NOLIGHT,     // SPR_FATT
    LT_NOLIGHT,     // SPR_CPOS
    LT_NOLIGHT,     // SPR_SARG
    LT_NOLIGHT,     // SPR_HEAD
    LT_GREENBALL, // SPR_BAL7 * // hell knight / baron of hell
    LT_NOLIGHT,     // SPR_BOSS
    LT_NOLIGHT,     // SPR_BOS2
    LT_REDBALL,   // SPR_SKUL // lost soul
    LT_NOLIGHT,     // SPR_SPID
    LT_NOLIGHT,     // SPR_BSPI
    LT_GREENBALL, // SPR_APLS * // arachnotron
    LT_GREENBALL, // SPR_APBX * // arachnotron explosion
    LT_NOLIGHT,     // SPR_CYBR
    LT_NOLIGHT,     // SPR_PAIN
    LT_NOLIGHT,     // SPR_SSWV
    LT_NOLIGHT,     // SPR_KEEN
    LT_NOLIGHT,     // SPR_BBRN
    LT_NOLIGHT,     // SPR_BOSF
    LT_NOLIGHT,     // SPR_ARM1
    LT_NOLIGHT,     // SPR_ARM2
    LT_NOLIGHT,     // SPR_BAR1
    LT_ROCKETEXP, // SPR_BEXP // barrel explosion
    LT_NOLIGHT,     // SPR_FCAN
    LT_NOLIGHT,     // SPR_BON1
    LT_NOLIGHT,     // SPR_BON2
    LT_NOLIGHT,     // SPR_BKEY
    LT_NOLIGHT,     // SPR_RKEY
    LT_NOLIGHT,     // SPR_YKEY
    LT_NOLIGHT,     // SPR_BSKU
    LT_NOLIGHT,     // SPR_RSKU
    LT_NOLIGHT,     // SPR_YSKU
    LT_NOLIGHT,     // SPR_STIM
    LT_NOLIGHT,     // SPR_MEDI
    LT_NOLIGHT,     // SPR_SOUL
    LT_NOLIGHT,     // SPR_PINV
    LT_NOLIGHT,     // SPR_PSTR
    LT_NOLIGHT,     // SPR_PINS
    LT_NOLIGHT,     // SPR_MEGA
    LT_NOLIGHT,     // SPR_SUIT
    LT_NOLIGHT,     // SPR_PMAP
    LT_NOLIGHT,     // SPR_PVIS
    LT_NOLIGHT,     // SPR_CLIP
    LT_NOLIGHT,     // SPR_AMMO
    LT_NOLIGHT,     // SPR_ROCK
    LT_NOLIGHT,     // SPR_BROK
    LT_NOLIGHT,     // SPR_CELL
    LT_NOLIGHT,     // SPR_CELP
    LT_NOLIGHT,     // SPR_SHEL
    LT_NOLIGHT,     // SPR_SBOX
    LT_NOLIGHT,     // SPR_BPAK
    LT_NOLIGHT,     // SPR_BFUG
    LT_NOLIGHT,     // SPR_MGUN
    LT_NOLIGHT,     // SPR_CSAW
    LT_NOLIGHT,     // SPR_LAUN
    LT_NOLIGHT,     // SPR_PLAS
    LT_NOLIGHT,     // SPR_SHOT
    LT_NOLIGHT,     // SPR_SGN2
    LT_COLUMN,    // SPR_COLU * // yellow little light column
    LT_NOLIGHT,     // SPR_SMT2
    LT_NOLIGHT,     // SPR_GOR1
    LT_NOLIGHT,     // SPR_POL2
    LT_NOLIGHT,     // SPR_POL5
    LT_NOLIGHT,     // SPR_POL4
    LT_NOLIGHT,     // SPR_POL3
    LT_NOLIGHT,     // SPR_POL1
    LT_NOLIGHT,     // SPR_POL6
    LT_NOLIGHT,     // SPR_GOR2
    LT_NOLIGHT,     // SPR_GOR3
    LT_NOLIGHT,     // SPR_GOR4
    LT_NOLIGHT,     // SPR_GOR5
    LT_NOLIGHT,     // SPR_SMIT
    LT_NOLIGHT,     // SPR_COL1
    LT_NOLIGHT,     // SPR_COL2
    LT_NOLIGHT,     // SPR_COL3
    LT_NOLIGHT,     // SPR_COL4
    LT_CANDLE,    // SPR_CAND * // candle
    LT_CANDLEABRE,// SPR_CBRA * // candleabre
    LT_NOLIGHT,     // SPR_COL6
    LT_NOLIGHT,     // SPR_TRE1
    LT_NOLIGHT,     // SPR_TRE2
    LT_NOLIGHT,     // SPR_ELEC
    LT_NOLIGHT,     // SPR_CEYE
    LT_NOLIGHT,     // SPR_FSKU
    LT_NOLIGHT,     // SPR_COL5
    LT_BLUETALL,  // SPR_TBLU *
    LT_GREENTALL, // SPR_TGRN *
    LT_REDTALL,   // SPR_TLT_RED *
    LT_BLUESMALL, // SPR_SMBT *
    LT_GREENSMALL,// SPR_SMGT *
    LT_REDSMALL,  // SPR_SMRT *
    LT_NOLIGHT,     // SPR_HDB1
    LT_NOLIGHT,     // SPR_HDB2
    LT_NOLIGHT,     // SPR_HDB3
    LT_NOLIGHT,     // SPR_HDB4
    LT_NOLIGHT,     // SPR_HDB5
    LT_NOLIGHT,     // SPR_HDB6
    LT_NOLIGHT,     // SPR_POB1
    LT_NOLIGHT,     // SPR_POB2
    LT_NOLIGHT,     // SPR_BRS1
    LT_TECHLAMP,  // SPR_TLMP *
    LT_TECHLAMP2, // SPR_TLP2 *
    LT_NOLIGHT,     // SPR_SMOK
    LT_NOLIGHT,     // SPR_SPLA
    LT_NOLIGHT,     // SPR_TNT1

// heretic sprites

    LT_NOLIGHT,     // SPR_IMPX,
    LT_NOLIGHT,     // SPR_ACLO,
    LT_NOLIGHT,     // SPR_PTN1,
    LT_NOLIGHT,     // SPR_SHLD,
    LT_NOLIGHT,     // SPR_SHD2,
    LT_NOLIGHT,     // SPR_BAGH,
    LT_NOLIGHT,     // SPR_SPMP,
    LT_NOLIGHT,     // SPR_INVS,
    LT_NOLIGHT,     // SPR_PTN2,
    LT_NOLIGHT,     // SPR_SOAR,
    LT_NOLIGHT,     // SPR_INVU,
    LT_NOLIGHT,     // SPR_PWBK,
    LT_NOLIGHT,     // SPR_EGGC,
    LT_NOLIGHT,     // SPR_EGGM,
    LT_NOLIGHT,     // SPR_FX01,
    LT_NOLIGHT,     // SPR_SPHL,
    LT_NOLIGHT,     // SPR_TRCH,
    LT_NOLIGHT,     // SPR_FBMB,
    LT_NOLIGHT,     // SPR_XPL1,
    LT_NOLIGHT,     // SPR_ATLP,
    LT_NOLIGHT,     // SPR_PPOD,
    LT_NOLIGHT,     // SPR_AMG1,
    LT_NOLIGHT,     // SPR_SPSH,
    LT_NOLIGHT,     // SPR_LVAS,
    LT_NOLIGHT,     // SPR_SLDG,
    LT_NOLIGHT,     // SPR_SKH1,
    LT_NOLIGHT,     // SPR_SKH2,
    LT_NOLIGHT,     // SPR_SKH3,
    LT_NOLIGHT,     // SPR_SKH4,
    LT_CHDL,      // SPR_CHDL,
    LT_SRTC,      // SPR_SRTC,
    LT_NOLIGHT,     // SPR_SMPL,
    LT_NOLIGHT,     // SPR_STGS,
    LT_NOLIGHT,     // SPR_STGL,
    LT_NOLIGHT,     // SPR_STCS,
    LT_NOLIGHT,     // SPR_STCL,
    LT_KFR1,      // SPR_KFR1,
    LT_NOLIGHT,     // SPR_BARL,
    LT_NOLIGHT,     // SPR_BRPL,
    LT_NOLIGHT,     // SPR_MOS1,
    LT_NOLIGHT,     // SPR_MOS2,
    LT_WTRH,      // SPR_WTRH,
    LT_NOLIGHT,     // SPR_HCOR,
    LT_NOLIGHT,     // SPR_KGZ1,
    LT_NOLIGHT,     // SPR_KGZB,
    LT_NOLIGHT,     // SPR_KGZG,
    LT_NOLIGHT,     // SPR_KGZY,
    LT_NOLIGHT,     // SPR_VLCO,
    LT_NOLIGHT,     // SPR_VFBL,
    LT_NOLIGHT,     // SPR_VTFB,
    LT_NOLIGHT,     // SPR_SFFI,
    LT_NOLIGHT,     // SPR_TGLT,
    LT_NOLIGHT,     // SPR_TELE,
    LT_NOLIGHT,     // SPR_STFF,
    LT_NOLIGHT,     // SPR_PUF3,
    LT_NOLIGHT,     // SPR_PUF4,
    LT_NOLIGHT,     // SPR_BEAK,
    LT_NOLIGHT,     // SPR_WGNT,
    LT_NOLIGHT,     // SPR_GAUN,
    LT_NOLIGHT,     // SPR_PUF1,
    LT_NOLIGHT,     // SPR_WBLS,
    LT_NOLIGHT,     // SPR_BLSR,
    LT_NOLIGHT,     // SPR_FX18,
    LT_FX17,      // SPR_FX17,
    LT_NOLIGHT,     // SPR_WMCE,
    LT_NOLIGHT,     // SPR_MACE,
    LT_FX02,      // SPR_FX02,
    LT_NOLIGHT,     // SPR_WSKL,
    LT_NOLIGHT,     // SPR_HROD,
    LT_FX00,      // SPR_FX00,
    LT_NOLIGHT,     // SPR_FX20,
    LT_NOLIGHT,     // SPR_FX21,
    LT_NOLIGHT,     // SPR_FX22,
    LT_NOLIGHT,     // SPR_FX23,
    LT_NOLIGHT,     // SPR_GWND,
    LT_NOLIGHT,     // SPR_PUF2,
    LT_NOLIGHT,     // SPR_WPHX,
    LT_NOLIGHT,     // SPR_PHNX,
    LT_FX04,      // SPR_FX04,
    LT_FX08,      // SPR_FX08,
    LT_NOLIGHT,     // SPR_FX09,
    LT_NOLIGHT,     // SPR_WBOW,
    LT_NOLIGHT,     // SPR_CRBW,
    LT_FX03,      // SPR_FX03,
//    LT_NOLIGHT,     // SPR_BLOD,
//    LT_NOLIGHT,     // SPR_PLAY,
    LT_NOLIGHT,     // SPR_FDTH,
    LT_NOLIGHT,     // SPR_BSKL,
    LT_NOLIGHT,     // SPR_CHKN,
    LT_NOLIGHT,     // SPR_MUMM,
    LT_NOLIGHT,     // SPR_FX15,
    LT_NOLIGHT,     // SPR_BEAS,
    LT_NOLIGHT,     // SPR_FRB1,
    LT_NOLIGHT,     // SPR_SNKE,
    LT_NOLIGHT,     // SPR_SNFX,
    LT_NOLIGHT,     // SPR_HHEAD,
    LT_NOLIGHT,     // SPR_FX05,
    LT_NOLIGHT,     // SPR_FX06,
    LT_NOLIGHT,     // SPR_FX07,
    LT_NOLIGHT,     // SPR_CLNK,
    LT_NOLIGHT,     // SPR_WZRD,
    LT_NOLIGHT,     // SPR_FX11,
    LT_NOLIGHT,     // SPR_FX10,
    LT_NOLIGHT,     // SPR_KNIG,
    LT_NOLIGHT,     // SPR_SPAX,
    LT_NOLIGHT,     // SPR_RAXE,
    LT_NOLIGHT,     // SPR_SRCR,
    LT_NOLIGHT,     // SPR_FX14,
    LT_NOLIGHT,     // SPR_SOR2,
    LT_NOLIGHT,     // SPR_SDTH,
    LT_NOLIGHT,     // SPR_FX16,
    LT_NOLIGHT,     // SPR_MNTR,
    LT_NOLIGHT,     // SPR_FX12,
    LT_NOLIGHT,     // SPR_FX13,
    LT_NOLIGHT,     // SPR_AKYY,
    LT_NOLIGHT,     // SPR_BKYY,
    LT_NOLIGHT,     // SPR_CKYY,
    LT_NOLIGHT,     // SPR_AMG2,
    LT_NOLIGHT,     // SPR_AMM1,
    LT_NOLIGHT,     // SPR_AMM2,
    LT_NOLIGHT,     // SPR_AMC1,
    LT_NOLIGHT,     // SPR_AMC2,
    LT_NOLIGHT,     // SPR_AMS1,
    LT_NOLIGHT,     // SPR_AMS2,
    LT_NOLIGHT,     // SPR_AMP1,
    LT_NOLIGHT,     // SPR_AMP2,
    LT_NOLIGHT,     // SPR_AMB1,
    LT_NOLIGHT,     // SPR_AMB2,
 };


void  Setup_sprite_light( byte  mons_ball_light )
{
    if( mons_ball_light )
    {
        sprite_light_ind[SPR_BAL1] = LT_REDBALL;
        sprite_light_ind[SPR_BAL2] = LT_REDBALL;
        sprite_light_ind[SPR_MANF] = LT_ROCKET2;
        sprite_light_ind[SPR_BAL7] = LT_GREENBALL;
        sprite_light_ind[SPR_APLS] = LT_GREENBALL;
        sprite_light_ind[SPR_APBX] = LT_GREENBALL;
        sprite_light_ind[SPR_SKUL] = LT_REDBALL;
        sprite_light_ind[SPR_FATB] = LT_REDBALL;
    }
    else
    {
        sprite_light_ind[SPR_BAL1] = LT_NOLIGHT;
        sprite_light_ind[SPR_BAL2] = LT_NOLIGHT;
        sprite_light_ind[SPR_MANF] = LT_NOLIGHT;
        sprite_light_ind[SPR_BAL7] = LT_NOLIGHT;
        sprite_light_ind[SPR_APLS] = LT_NOLIGHT;
        sprite_light_ind[SPR_APBX] = LT_NOLIGHT;
        sprite_light_ind[SPR_SKUL] = LT_NOLIGHT;
        sprite_light_ind[SPR_FATB] = LT_NOLIGHT;
    }
}


void CV_MonBall_OnChange( void );
void CV_corona_OnChange( void );


//consvar_t cv_dynamiclight = {"dynamiclighting",  "On", CV_SAVE, CV_OnOff };
//consvar_t cv_staticlight  = {"staticlighting",   "On", CV_SAVE, CV_OnOff };
//#ifdef CORONA_CHOICE
//CV_PossibleValue_t corona_draw_cons_t[] = { {0, "Off"}, {1, "Sprite"}, {2, "Dyn"}, {3, "Auto"}, {0, NULL} };
//consvar_t cv_corona_draw     = {"corona_draw",    "Auto", CV_SAVE, corona_cons_t };
//#else
//consvar_t cv_corona_draw         = {"corona_draw",    "On", CV_SAVE, CV_OnOff };
//#endif
CV_PossibleValue_t corona_cons_t[] = { {0, "Off"}, {1, "Special"}, {2, "Most"}, {14, "Dim"}, {15, "All"}, {16, "Bright"}, {20, "Old"}, {0, NULL} };
consvar_t cv_corona         = {"corona",    "All", CV_SAVE|CV_CALL, corona_cons_t, CV_corona_OnChange};
consvar_t cv_coronasize      = {"coronasize",        "1", CV_SAVE| CV_FLOAT, NULL };
CV_PossibleValue_t corona_draw_mode_cons_t[] = { {0, "Blend"}, {1, "Blend_BG"}, {2, "Additive"}, {3, "Additive_BG"}, {4, "Add_Limit"}, {0, NULL} };
consvar_t cv_corona_draw_mode = {"corona_draw_mode", "2", CV_SAVE, corona_draw_mode_cons_t, NULL};
// Monster ball weapon light
consvar_t cv_monball_light  = {"monball_light", "On", CV_SAVE|CV_CALL, CV_OnOff, CV_MonBall_OnChange };

void CV_MonBall_OnChange( void )
{
    Setup_sprite_light( cv_monball_light.EV );
}

void CV_corona_OnChange( void )
{
    int i;
    // Force light setup, without another test.
    for( i=0; i<NUMLIGHTS; i++ )
    {
        sprite_light[i].impl_flags |= SLI_changed;
    }
}


