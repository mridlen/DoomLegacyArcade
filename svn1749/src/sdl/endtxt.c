//-----------------------------------------------------------------------------
//
// $Id: endtxt.c 1572 2021-01-28 09:25:24Z wesleyjohnson $
//
// As Modifified: Copyright 2020 by Doom Legacy Team
// Incorporated into the Doom Legacy program which is under
// GNU General Public License.
// Original is copyright and available under its original terms.
//
// DESCRIPTION:
//      Quit game final screen text.
//
//-----------------------------------------------------------------------------

/*
 * Function to write the Doom end message text
 *
 * Copyright (C) 1998 by Udo Munk <udo@umserver.umnet.de>
 *
 * This code is provided AS IS and there are no guarantees, none.
 * Feel free to share and modify.
 */

#include <stdio.h>
#include <stdlib.h>

#include "doomincl.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_argv.h"
#include "m_swap.h"
#include "v_video.h"

// [WDJ] The EndText uses FG and BG color attributes, and some special drawing
// characters among the text.

// The special characters are difficult to translate for arbitrary devices.
// They are mostly line draw characters, angles, double lines, and blocks.
// On Linux Console, there are many encodings to choose from, and that is usually
// setup to support the games.
// TODO: Linux term are VT100, so should use that char translation.

// The standard EndText was replaced in legacy.wad with a DoomLegacy message,
// that uses only characters that will work on consoles.
// Some wads have an ENDOOM Lump, and that will be displayed.


// Output some test tables instead of the EndText.
// #define TEST_TABLES_OUT

// The old DOS code seems to have had 4 bit background colors.
// When the MSB is blink, only have 3 bits for background colors.
//#define MSB_BLINK

// When have UTF capable term, and it has the fonts, and UTF8 is choosen as the encoding.
// This could be disabled for DOS and some Windows.
#define CP437_TO_UTF

#ifdef CP437_TO_UTF
// [MB] 2020-04-28: Added UTF-8 sequences for IBM437 codepage
// [WDJ] Linux console, with an encoding selected for game compatibility,
// this displays unusual characters, or two characters.  Must switch to UTF encoding to make it work.
// [WDJ] Linux Terminal, does not have the fonts so displays UTF-boxes.
// TODO: Use UTF16 to reduce table size, 2 bytes per entry, instead of 8 bytes per entry.  Due to the
// limited pages used, conversion to UTF8 should be simple.
static const char * cp437_to_utf[256] =
{
  " ", "\xE2\x98\xBA", "\xE2\x98\xBB", "\xE2\x99\xA5",
    "\xE2\x99\xA6", "\xE2\x99\xA3", "\xE2\x99\xA0", "\xE2\x80\xA2",
    "\xE2\x97\x98", "\xE2\x97\x8B", "\xE2\x97\x99", "\xE2\x99\x82",
    "\xE2\x99\x80", "\xE2\x99\xAA", "\xE2\x99\xAB", "\xE2\x98\xBC",
  "\xE2\x96\xBA", "\xE2\x97\x84", "\xE2\x86\x95", "\xE2\x80\xBC",
    "\xC2\xB6", "\xC2\xA7", "\xE2\x96\xAC", "\xE2\x86\xA8",
    "\xE2\x86\x91", "\xE2\x86\x93", "\xE2\x86\x92", "\xE2\x86\x90",
    "\xE2\x88\x9F", "\xE2\x86\x94", "\xE2\x96\xB2", "\xE2\x96\xBC",
  " ", "!", "\x22", "#", "$", "%", "&", "\x27", "(", ")", "*", "+", ",", "-", ".", "/",
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
  "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\x5C", "]", "^", "_",
  "\x60", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "\xC2\xA6", "}", "~", "\xE2\x8C\x82",
  "\xC3\x87", "\xC3\xBC", "\xC3\xA9", "\xC3\xA2",
    "\xC3\xA4", "\xC3\xA0", "\xC3\xA5", "\xC3\xA7",
    "\xC3\xAA", "\xC3\xAB", "\xC3\xA8", "\xC3\xAF",
    "\xC3\xAE", "\xC3\xAC", "\xC3\x84", "\xC3\x85",
  "\xC3\x89", "\xC3\xA6", "\xC3\x86", "\xC3\xB4",
    "\xC3\xB6", "\xC3\xB2", "\xC3\xBB", "\xC3\xB9",
    "\xC3\xBF", "\xC3\x96", "\xC3\x9C", "\xC2\xA2",
    "\xC2\xA3", "\xC2\xA5", "\xE2\x82\xA7", "\xC6\x92",
  "\xC3\xA1", "\xC3\xAD", "\xC3\xB3", "\xC3\xBA",
    "\xC3\xB1", "\xC3\x91", "\xC2\xAA", "\xC2\xBA",
    "\xC2\xBF", "\xE2\x8C\x90", "\xC2\xAC", "\xC2\xBD",
    "\xC2\xBC", "\xC2\xA1", "\xC2\xAB", "\xC2\xBB",
  "\xE2\x96\x91", "\xE2\x96\x92", "\xE2\x96\x93", "\xE2\x94\x82",
    "\xE2\x94\xA4", "\xE2\x95\xA1", "\xE2\x95\xA2", "\xE2\x95\x96",
    "\xE2\x95\x95", "\xE2\x95\xA3", "\xE2\x95\x91", "\xE2\x95\x97",
    "\xE2\x95\x9D", "\xE2\x95\x9C", "\xE2\x95\x9B", "\xE2\x94\x90",
  "\xE2\x94\x94", "\xE2\x94\xB4", "\xE2\x94\xAC", "\xE2\x94\x9C",
    "\xE2\x94\x80", "\xE2\x94\xBC", "\xE2\x95\x9E", "\xE2\x95\x9F",
    "\xE2\x95\x9A", "\xE2\x95\x94", "\xE2\x95\xA9", "\xE2\x95\xA6",
    "\xE2\x95\xA0", "\xE2\x95\x90", "\xE2\x95\xAC", "\xE2\x95\xA7",
  "\xE2\x95\xA8", "\xE2\x95\xA4", "\xE2\x95\xA5", "\xE2\x95\x99",
    "\xE2\x95\x98", "\xE2\x95\x92", "\xE2\x95\x93", "\xE2\x95\xAB",
    "\xE2\x95\xAA", "\xE2\x94\x98", "\xE2\x94\x8C", "\xE2\x96\x88",
    "\xE2\x96\x84", "\xE2\x96\x8C", "\xE2\x96\x90", "\xE2\x96\x80",
  "\xCE\xB1", "\xC3\x9F", "\xCE\x93", "\xCF\x80",
    "\xCE\xA3", "\xCF\x83", "\xC2\xB5", "\xCF\x84",
    "\xCE\xA6", "\xCE\x98", "\xCE\xA9", "\xCE\xB4",
    "\xE2\x88\x9E", "\xCF\x86", "\xCE\xB5", "\xE2\x88\xA9",
  "\xE2\x89\xA1", "\xC2\xB1", "\xE2\x89\xA5", "\xE2\x89\xA4",
    "\xE2\x8C\xA0", "\xE2\x8C\xA1", "\xC3\xB7", "\xE2\x89\x88",
    "\xC2\xB0", "\xE2\x88\x99", "\xC2\xB7", "\xE2\x88\x9A",
    "\xE2\x81\xBF", "\xC2\xB2", "\xE2\x96\xA0", " "
};
#endif


// [WDJ] On Linux Console, and on Linux Term, there are 16 FG colors, 8 BG colors,
// with BOLD characters for att_str of form "1;3x" and "1;4x".

static const char * fg_att_str[16] = {
  "30", // 0: black
  "34", // 1: blue
  "32", // 2: green
  "36", // 3: cyan
  "31", // 4: red
  "35", // 5: magenta
  "33", // 6: brown
  "37", // 7: bright grey
  // [WDJ] On Linux console, and term, these are BOLD, and colored.
  "1;30", // 8: dark grey
  "1;34", // 9: bright blue
  "1;32", // 10: bright green
  "1;36", // 11: bright cyan
  "1;31", // 12: bright red
  "1;35", // 13: bright magenta
  "1;33", // 14: yellow
  "1;37", // 15: white
};

static const char * bg_att_str[] = {
  "40", // 0: black
  "44", // 1: blue
  "42", // 2: green
  "46", // 3: cyan
  "41", // 4: red
  "45", // 5: magenta
  "43", // 6: brown
  "47", // 7: bright grey
#ifndef MSB_BLINK
  // [MB] 2020-04-26: Not available for bg color (MSB is the blink flag)
  // [WDJ] On Linux console, and term, these are BOLD, and same as 0..7.
  "1;40", // 8: dark grey
  "1;44", // 9: bright blue
  "1;42", // 10: bright green
  "1;46", // 11: bright cyan
  "1;41", // 12: bright red
  "1;45", // 13: bright magenta
  "1;43", // 14: yellow
  "1;47", // 15: white
#endif
};



// [WDJ] Changed to public interface name.
// Original name was ShowEndText.
void I_Show_EndText( uint16_t * text )
{
    int i;
    int nlflag = 1;
#if 0
//	char *col;

	/* if the xterm has more then 80 columns we need to add nl's */
	/* doesn't work, COLUMNS is not in the environment at this time ???
	col = getenv("COLUMNS");
	if (col) {
		if (atoi(col) > 80)
			nlflag++;
	}
	*/
#endif

#ifdef TEST_TABLES_OUT
    // Foreground colors
    for (i=0; i<16; i++)
    {
        // forground color
        byte fg = i & 0x0F;
        printf("\033[%sm", fg_att_str[fg] );
        printf(" FG ");
    }
    printf("\033[0m");
    printf("\n");
    // Background colors
    for (i=0; i<16; i++)
    {
        // background color
        byte bg = i & 0x0F;
        printf("\033[%sm", bg_att_str[bg] );
        printf(" BG ");
    }
    printf("\033[0m");
    printf("\n");
    // Text table
    for (i=0; i<255; i++)
    {
        byte c = i;
#ifdef CP437_TO_UTF
        printf( "%2X %c %s | ", c, c, cp437_to_utf[c] );
#else
        printf( "%X %c,  ", c, c );
#endif
        if( ((i+1) & 0x07) == 0 )
            printf("\n");
    }

#else  // TEST_TABLES_OUT

    // print 80x25 text and deal with the attributes too
    for (i=1; i<=80*25; i++)
    {
        // [MB] 2020-04-26: Fixed endianess
        uint16_t ac = (uint16_t)LE_SWAP16(*text++);
	// attribute is first in 16 bit char.
        byte att = ac >> 8;
        byte c = ac & 0xff;

        // [MB] 2020-04-26: Reset color configuration
        printf("\033[0m");

#ifdef MSB_BLINK
# if 0
        // [MB] 2020-04-26: Blink support does not work correctly
        if(att & 0x80)
             printf("\033[5m");
# endif
#endif
        // forground color
        byte fg = att & 0x0f;
        // background color
	// [MB] 2020-04-26: Mask only bg color bits here
#ifdef MSB_BLINK
        byte bg = (att >> 4) & 0x07;  // MSB is blink
#else
        byte bg = (att >> 4) & 0x0F;
#endif
        printf("\033[%sm\033[%sm", fg_att_str[fg], bg_att_str[bg] );

	// now the text
#ifdef CP437_TO_UTF
        if( cv_textout.EV == 2 )  // UTF8
        {
            // [MB] 2020-04-26: Convert data to Unicode and print as UTF-8
            printf( "%s", cp437_to_utf[c] );
	}
        else
        {
            putchar( c );
	}
#else
        putchar( c );
#endif

	// do we need a nl?
        if( nlflag )
        {
	    if( !(i % 80) )  // end of screen
            {
                printf("\033[0m");
                printf("\n");
	    }
        }
    }
#endif  // TEST_TABLES_OUT

    // all attributes off
    printf("\033[0m");

    if( nlflag )
        printf("\n");
}
