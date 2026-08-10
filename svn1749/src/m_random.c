// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_random.c 1668 2024-03-03 04:38:16Z wesleyjohnson $
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
// $Log: m_random.c,v $
// Revision 1.5  2001/06/10 21:16:01  bpereira
// Revision 1.4  2001/03/30 17:12:50  bpereira
// Revision 1.3  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      Random number LUT.
//
//-----------------------------------------------------------------------------

#include "doomincl.h"
#include "m_random.h"
//
// M_Random
// Returns a 0-255 number
//
byte rndtable[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

static byte     rndindex = 0;
static byte     prndindex = 0;
static byte     arndindex = 0;  // ambience
static byte     brndindex = 0;  // bots and other automation
static byte     nrndindex = 0;  // new legacy stuffs

#ifndef DEBUGRANDOM

// P_Random is used throughout all the p_xxx game code.
byte P_Random ()
{
    return rndtable[++prndindex];
}

// Alot of code used P_Random()-P_Random().
// Because C does not define the evaluation order, it is compiler dependent
// so this allows network play between different compilers
int P_SignedRandom ()
{
    int r = P_Random();
    return r-P_Random();
}

#else

byte P_RandomFL (char *fn, int ln)
{
    debug_Printf("P_Random at : %sp %d\n", fn, ln);
    return rndtable[++prndindex];
}

int P_SignedRandomFL (char *fn, int ln)
{
    int r;
    debug_Printf("P_SignedRandom at : %sp %d\n", fn, ln);
    r = rndtable[++prndindex];
    return r-rndtable[++prndindex];
}

#endif

#ifdef PP_RANDOM_EXPOSED
// For debugging, like in PrBoom
byte PP_Random( byte pr )
{
    byte rv = rndtable[++prndindex];
    return rv;
}

int PP_SignedRandom ( byte pr )
{
    int r = PP_Random(pr);
    return r - PP_Random(pr);
}
#endif



byte M_Random (void)
{
    return rndtable[++rndindex];
}

void M_ClearRandom (void)
{
    rndindex = prndindex = 0;
    arndindex = brndindex = 0;
    nrndindex = 0;
}

// For new Legacy stuff, to not use any existing random.
byte N_Random (void)
{
    return rndtable[++nrndindex];
}

int  N_SignedRandom(void)
{
    int n1 = N_Random();
    return n1 - N_Random();
}

// for savegame and join in game
byte P_Rand_GetIndex(void)
{
    return prndindex;
}

// load game
void P_Rand_SetIndex(byte rindex)
{
    prndindex = rindex;
}

// [WDJ]
// separate so ambience does not affect demo playback
byte A_Random (void)
{
    arndindex += 3;
    return rndtable[arndindex];
}

// bots and other DoomLegacy automations
byte B_Random (void)
{
    brndindex += 11;
    return rndtable[brndindex];
}

byte B_Rand_GetIndex(void)
{
    return brndindex;
}

void B_Rand_SetIndex(byte rindex)
{
    brndindex = rindex;
}


#if 0
// [WDJ] This mess is one reason we do not want get caught up in
// maintaining demo sync.
// This has been simplified from what appears in PrBoom, to make it
// more comprehensible.
int P_Random(pr_class_t pr_class)
{
  // killough 2/16/98:  We always update both sets of random number
  // generators, to ensure repeatability if the demo_compatibility
  // flag is changed while the program is running. Changing the
  // demo_compatibility flag does not change the sequences generated,
  // only which one is selected from.
  //
  // All of this RNG stuff is tricky as far as demo sync goes --
  // it's like playing with explosives :) Lee

  int compat = 0;
  uint32_t boom;
  
  if( pr_class == pr_misc )
  {
    rng.prndindex = (rng.prndindex + 1) & 255;
    compat = rng.prndindex;
  }
  else
  {
    rng.rndindex = (rng.rndindex + 1) & 255;
    compat = rng.rndindex;
    // killough 3/31/98:
    // If demo sync insurance is not requested, use much more unstable
    // method by putting everything except pr_misc into pr_all_in_one
    if( !demo_insurance )
      pr_class = pr_all_in_one;
  }

  boom = rng.seed[pr_class];

  // killough 3/26/98: add pr_class*2 to addend
  rng.seed[pr_class] = boom * 1664525ul + 221297ul + pr_class*2;

  if (demo_compatibility)
    return rndtable[compat];

  boom >>= 20;

  // killough 3/30/98: use gametic-levelstarttic to shuffle RNG
  // killough 3/31/98: but only if demo insurance requested,
  // since it's unnecessary for random shuffling otherwise
  // killough 9/29/98: but use basetic now instead of levelstarttic
  // [WDJ] demo_comp_tic = gametic - basetic
  // cph - DEMOSYNC - this change makes MBF demos work,
  //       but does it break Boom ones?

  if (demo_insurance)
    boom += demo_comp_tic * 7;

  return boom & 255;
}
#endif


// [WDJ] Extended random, has long repeat period.
// This is necessary for graphical creation, as the short period random numbers show patterns.
// E_Random state.
static uint32_t rng1 = 33331;
static uint32_t rng_stir = 1;

//  returns unsigned 16 bit
int  E_Random(void)
{
    // A Linear Congruential Generator with long period.
    // Multiply 16 bit value by a const, with addition of previous multiplication carry out bits.
    // Adding an odd constant ensures it is self starting from 0, but often makes it get stuck (undesireable).
    // Want a safe-prime p, where (p-1)/2 is also prime.
    // Then the period will be (p-1)/2, where p = (mult_a * 2**16) - 1.
//    rng1 = ((rng1 & 0xFFFF) * 57827) + 1 + (rng1 >> 16);  // period   6_059115
//    rng1 = ((rng1 & 0xFFFF) * 57829) + 1 + (rng1 >> 16);  // period  92_017900
    // Multiplier a = 65534, p = 4294836223, (p - 1)/2 = 2147418111.  ?? not primes
//    rng1 = ((rng1 & 0xFFFF) * 65534) + (rng1 >> 16);      // period 261_880256
    // Multiplier a = 65184, p = 65184 * 2**16 - 1 = 427189623 (prime), and (p-1)/2 = 2135949311 (prime).
    rng1 = ((rng1 & 0xFFFF) * 65184) + 1 + (rng1 >> 16);  // period 2135_949311

    rng_stir += 21611; // add prime, period 2**32
    // Extended period = (rng1_period * rng_stir_period), as long as the periods do not have a common factor.
    return (rng1 ^ rng_stir) & 0xFFFF;
}

//  returns -range, 0, +range
int  E_SignedRandom( int range )
{
    return ((int)( E_Random() % (range + range + 1) )) - range;
}

uint32_t  E_Rand_Get( uint32_t * rs )
{
    if( rs )
        *rs = rng_stir;
    return rng1;
}

void E_Rand_Set( uint32_t rn, uint32_t rs )
{
    rng1 = rn;
    rng_stir = rs;
}

//#define TEST_ERANDOM
#ifdef TEST_ERANDOM
void test_erandom()
{
#define HASHSIZE  0x1000
#define BINSIZE   0x1000
#define SAMPLESIZE    4096
    // Do not need to save every value.  When it repeats it will repeat perfectly.
static    uint32_t val[HASHSIZE][BINSIZE];
static    uint32_t ind[HASHSIZE][BINSIZE];
static    uint32_t num[HASHSIZE];

    unsigned int bin, numval;
    unsigned int c2, n2;
    unsigned int c1, n1;
    unsigned int prev_bin, last_bin;
    uint32_t * last;
    uint32_t * p_end;
    uint32_t * p;

    memset( &num[0], 0, sizeof(num));
    prev_bin = last_bin = 0xFFFFFFFF;

    for( c2=0; c2< 0x3FFFFFFF; c2++ )
    {
        // sample state
        prev_bin = last_bin;
        bin = rng1 & (HASHSIZE-1);
        last_bin = bin;
        numval = num[bin];
        if( numval < BINSIZE )
        {
            val[bin][numval] = rng1;
            ind[bin][numval] = c2;
            num[bin] = numval + 1;
        }

        for( n2=0; n2<SAMPLESIZE; n2++ )
        {
            E_Random();  // next

            // Check hashed per SAMPLE
            bin = rng1 & (HASHSIZE-1);
            numval = num[bin];
            if( numval )
            {
                p_end = & val[bin][numval];
                p = & val[bin][0];
                for( ; p < p_end; p++ )
                {
                    if( *p == rng1 )
                    {
                        unsigned int bi2 = (p - & val[bin][0]);
                        c1 = ind[bin][bi2];
                        n1 = 0;
                        goto found_hit;
                    }
                }
            }
        }
    }
    printf(" E_Random: no repeats found in %i tests\n", c2 );
    fflush( stdout );
    return;

found_hit:
    if( c2 > c1 )  goto print_hit;

    // The detected repeat value was the last sample saved.
    // Might have had a very small tight loop, or have become stuck on a value.
    // Do not know when the loop started.
    printf("E_Random repeat val=%x, 1st at %i.%i, 2nd at %i.%i, preliminary.\n", rng1, c1, 0, c2, n2 );

    // Reset RNG to previous sample.
    if( prev_bin < BINSIZE )
    {
        // Use the previous sample
        bin = prev_bin;
        numval = num[bin];
        if((last_bin == prev_bin) && (numval>0) )   numval--;
    }
    else
    {
        // Previous sample invalid, so this must be the very first sample.
        bin = last_bin;
        numval = num[bin];
    }

    rng1 = val[bin][numval];
    c1 = c2 = ind[bin][numval];

    last = &val[0][0];  // reuse val array space
    *last = rng1;
    last++;

    for( n2=0; n2 < SAMPLESIZE*2; n2++ )
    {
        E_Random();  // next

        // Check last SAMPLE
        p = &val[0][0];  // reuse val array space
        for( ; p < last; p++ )
        {
            if( *p == rng1 )
            {
                n1 = (p - &val[0][0]);
                goto print_hit;
            }
        }
        *last = rng1;
    }
    printf("E_Random repeat failed\n");
    n1 = 0;

print_hit:
    printf("E_Random repeat val=%x, 1st at %i.%i, 2nd at %i.%i\n", rng1, c1, n1, c2, n2 );
    uint64_t period = ((uint64_t)(c2 - c1) * SAMPLESIZE) + n2 - n1 + 1;
    printf("E_Random period %Lu\n", period );
    fflush( stdout );
}
#endif



