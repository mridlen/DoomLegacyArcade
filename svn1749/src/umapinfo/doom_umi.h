//-----------------------------------------------------------------------------
//
// $Id: doom_umi.h 1610 2023-10-06 09:40:13Z wesleyjohnson $
//
// Copyright (C) 2023 by DooM Legacy Team.
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

#ifndef DOOM_UMI_H
#define DOOM_UMI_H  1

/*
 * File: libdoom-umapinfo-1/doom_umi.h
 * Library header of libdoom-umapinfo
 *
 * Public API (compatible between versions with same major number).
 * 
 * API documentation: DOOM_UMAPINFO
 *
 * Author    Michael Baeuerle
 * Date      2022-2023
 * Copyright GPL (see file LICENSE)
 */


#include <stddef.h>
  // size_t
#include "doomtype.h"
  // byte
  // uint16_t

// [WDJ] Have replaced common uses of size_t with unsigned int, to ensure that it is unsigned.
// Lately size_t is required to be unsigned, but it is NOT ALWAYS unsigned on 32 bit systems.

// [WDJ] DoomLegacy name conventions.
// Types in DoomLegacy are xxxx_t,
// even though that namespace is claimed by gnu libraries.
// Struct are xxxx_s.
// Enum are xxxx_e.


/* Documentation
 *
 * see "README"
 *
 * see "SYNTAX"
 */


/*  DOOM_UMAPINFO UMAPINFO
 *
 * Parser for Doom UMAPINFO data.
 *
 * This library uses the prefix "umi_" for its functions and interface.
 */

/*  UMI_ERROR : Error return values
 *
 * Error code values are always negative.
 *
 * The meaning of the error code values may differ between major API versions.
 * 
 * Always use the symbolic names to checks.
 */

// Returning error codes to caller will not accomplish anything better than
// this library has already done.
// There are no actions the caller could take to fix anything.
// The only thing they need to know is failure, and is it severe enough that they should abort.
// Failure levels:
//   successfully processed umapinfo
//   partial processed umapinfo, with errors
//   not umapinfo, buffer content ignored
//   total failure abort, system failure, memory failure
//   internal error detected, these should not happen

// **** These are returned to main program callers, and API callers.
#define UMI_ERROR_INVALID  -2  /* Invalid argument(s) */
  // This error is most entirely during initial debug.  Checking should be DEBUG and PARANOID code.
  // In production code the caller cannot fix the problem.
#define UMI_ERROR_NOTFOUND -4  /* Selected object not found */
  // This error is caused by having to search and having to handle the case where it is not found.
  // If the data is consistent, it should never be not found.
  // These can mostly be eliminated by handling that case when it that entity was first encountered and
  // not putting it into some general database where it has to be re-discovered.

// These are warnings.  They are due to badly written umapinfo lump data, and cannot be avoided.
#define UMI_ERROR_SYNTAX   -5
    // Flags a syntax error.
#define UMI_ERROR_GENERAL  -6
    // Semanatic and general errors.
#define UMI_ERROR_PARTIAL  -7
    // Partial processed umapinfo, with errors.  Indicates buggy wad.
#define UMI_ERROR_NOT_UMAPINFO -8
    // Not umapinfo, buffer content ignored.

// These are fatal errors.
#define UMI_ERROR_FATAL    -100
    // Indicates the boundary between non-fatal, and fatal errors.
    // It may be possible for the umapinfo lump to be so badly formed that it forces a FATAL exit.
#define UMI_ERROR_INTERNAL -101
    // Internal data structure failure.
    // This indicates that something within the umapinfo code is not consistent, or is inadequate.
#define UMI_ERROR_MEMORY   -104
    // Memory allocation failed.
#define UMI_ERROR_SYSTEM   -105
    // Total failure abort, system failure.

// Lookup the error string for errcode.
// Return empty string for unknown error codes.
// For printing only, not meant to be stored.
const char * umi_error_name( int8_t errcode );

// Fatal error indication, used by memory and separate functions.
extern int8_t umi_fatal_error;

// To route umapinfo messages to Doom print functions.
//  errcode:
//   <0  for errors, such as UMI_ERROR_xxx
//    0  for verbose message
//    1  for debug messages
typedef void (*umi_doom_print_ft)( int8_t errcode, const char *, const char *, const char * );  // define function type
extern umi_doom_print_ft  umi_doom_print;



/*  UMI_TYPE_ : Value types
 *
 * The meaning of the type code values may differ between major API versions.
 * 
 * Always use the symbolic names for checks.
 */
// The parser tags the parsed values to classify them, to indicate
// which parser value and interpretation is to be used.
typedef enum {
  UMI_TYPE_INVALID, /* Reserved for internal use */
  UMI_TYPE_CLEAR,   /* Clear flag ('clear') */
  UMI_TYPE_BOOL,    /* Boolean value ('false'/'true') */
  UMI_TYPE_NUMBER,  /* Unsigned number */
  UMI_TYPE_QSTRING, /* Quoted string */
  UMI_TYPE_THING,   /* Thing type */
} umi_type_e;



/*  UMI_KEY_ :  Key identifiers
 *
 * The meaning of the key code values may differ between major API versions.
 * 
 * Always use the symbolic names for checks.
 */
// Tables indexed by umi_keyword_e: umi_keyword_name[]
typedef enum {
  UMI_KEY_author,
  UMI_KEY_label,
  UMI_KEY_levelname,
  UMI_KEY_levelpic,
  UMI_KEY_next,
  UMI_KEY_nextsecret,
  UMI_KEY_skytexture,
  UMI_KEY_music,
  UMI_KEY_exitpic,
  UMI_KEY_enterpic, 
  UMI_KEY_partime,
  UMI_KEY_endgame,
  UMI_KEY_endpic,
  UMI_KEY_endbunny,
  UMI_KEY_endcast,
  UMI_KEY_nointermission,
  UMI_KEY_intertext,
  UMI_KEY_intertextsecret,
  UMI_KEY_interbackdrop,
  UMI_KEY_intermusic,
  UMI_KEY_episode,
  UMI_KEY_bossaction,
   
  UMI_KEY_none = 255,  // no action, no entry
}  umi_keyword_e;




/* Episode and map number from UMAPINFO mapname */
typedef struct umi_mapid_s  umi_mapid_t;
struct umi_mapid_s
{
    uint16_t episode;  /* Doom 1 episode (zero for Doom 2) */
    uint16_t map;      /* Map (of episode for Doom 1) */
};



// ----------------------------
// Direct passing of line from parser to Doom umapinfo storage.


/* Octets between UMAPINFO quoted string delimiters */
typedef struct umi_qstring  umi_qstring_t;
struct umi_qstring
{
    const unsigned char * octet;  /* Octets from quoted string, ref into buffer */
    unsigned int          len;    /* Number of octets */
};

typedef struct umi_line_value_s
{
    byte  type;  // umi_type_e
    union
    {
        unsigned int  number;  // Number or boolean
        unsigned int  identifier;  // thingid
        umi_qstring_t qstring; // Quoted string, by len
    } v;
} umi_line_value_t;

// The max number of values per line.
// This would be the max number of lines allowed for intertext.
// There is only room for about 25 lines on a page.
// Not having a max would require dynamic allocation and handling that has widespread effects.
// Limited to 254 (byte).
#define MAX_VALUES_PER_LINE  64

// This is the store parameter. It has one instance, and only during the parse.
typedef struct {
    umi_mapid_t  epimap;  // map identification
    byte keyid;      // keyword
    byte num_value;  // in value[]
    umi_line_value_t  value[MAX_VALUES_PER_LINE];
} umi_line_t;


// To route parser line value to Doom store functions.
// No error codes, as any errors are handled directly by Doom.
typedef void (*umi_doom_store_ft)( const umi_line_t * );  // define function type
extern umi_doom_store_ft  umi_doom_store;


/*  Parse UMAPINFO lump
 *
 * This function executes the parser which calls umi_doom_store for each
 * keyword assignment.
 */
// Parse the umapinfo data, and call umi_doom_store for each keyword store.
//  src_txt:  Pointer UMAPINFO lump data.
//      This data cannot be freed until the umapinfo has been totally parsed and stored.
//  src_txt_size:  Size of the src_txt.
//      Zero length UMAPINFO data will return UMI_ERROR_NOT_UMAPINFO.
//  parse_verbose: Enable verbose messages (when non-zero).
// Return:
//   0 on success
//   Negative value on error (check with UMI_ERROR_ )
//     UMI_ERROR_INVALID : invalid parameters
//     UMI_ERROR_NOT_UMAPINFO : lump did not contain umapinfo
//     UMI_ERROR_MEMORY : memory allocation failure
//     UMI_ERROR_PARTIAL: partial processed umapinfo, with errors
//     maybe other error codes
int umi_parse_umapinfo_lump( const byte * src_txt, unsigned int src_txt_size, byte parse_verbose );


#endif  /* DOOM_UMI_H */
/* EOF */
