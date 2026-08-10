//-----------------------------------------------------------------------------
//
// $Id: doom_umi_api.c 1610 2023-10-06 09:40:13Z wesleyjohnson $
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

/*
 * API functions for libdoom-umapinfo
 *
 * Copyright (c) 2022-2023 by the developers. See the LICENSE file for details.
 *
 * Generic API functions to parse and access UMAPINFO data.
 */

#include "doomdef.h"
#ifdef ENABLE_UMAPINFO

#include <stdio.h>
#include <string.h>

#include "doom_umi.h"
#include "doom_umi_internal.h"
#include "doom_umi_lexer.h"
#include "doom_umi_parser.h"

// [WDJ] This is the API for the umapinfo library.
// The lexer scans the source into tokens.
// The parser interprets the tokens according to the umapinfo format.
// There should not be any more layers of encapsulation.

// [WDJ] There is no point to returning special error codes for situations for which there is no graceful recovery.
// If they are debug only, then they should be within DEBUG, or PARANOID.

/* ======================================================================== */
/* Lexer and parser print verbose output to stderr if nonzero */
byte umi_verbose_mode = 0;
int8_t umi_fatal_error = 0;  // error codes < 0

// Default dumb print for all umapinfo.
void umi_dumb_print ( int8_t errcode, const char * m1, const char * m2, const char * m3 )
{
    if( errcode < 0 )
    {
        fprintf( stderr, "%s: %s %s\n", m1, m2, m3 );
        fflush(stderr);
    }
    else
    {
        printf( "%s: %s %s\n", m1, m2, m3 );
    }
}

// Program print function indirection.
// User sets this to their function, to connect umapinfo-lib print to their program print.
umi_doom_print_ft  umi_doom_print = umi_dumb_print;

typedef struct 
{
   int8_t  error_code;
   const char *  error_string;
} error_name_entry_t;

error_name_entry_t  error_name_table[] =
{
   { UMI_ERROR_INVALID, "Invalid parameters" },
   { UMI_ERROR_NOTFOUND, "Not Found" },
   { UMI_ERROR_NOT_UMAPINFO, "Not UMAPINFO lump" },
   { UMI_ERROR_SYNTAX, "Syntax Error" },
   { UMI_ERROR_GENERAL, "General Error" },
   { UMI_ERROR_PARTIAL, "Partial non-fatal errors" },
   { UMI_ERROR_FATAL, "Fatal Error" },
   { UMI_ERROR_INTERNAL, "Internal Error" },
   { UMI_ERROR_MEMORY, "Memory Error" },
   { UMI_ERROR_SYSTEM, "System Error" },
};

// Lookup the error string for errcode.
// Return empty string for unknown error codes.
// For printing only, not meant to be stored.
const char * umi_error_name( int8_t errcode )
{
   error_name_entry_t *  enep = error_name_table;
   int cnt = sizeof( error_name_table ) / sizeof(error_name_entry_t);
   while( cnt-- )
   {
      if( enep->error_code == errcode )
          return enep->error_string;
      enep++;
   }
   return "";
}


/* ======================================================================== */
/* Uses global state (lexer and parser are not thread-safe anyway) */
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
int umi_parse_umapinfo_lump( const byte * src_txt, unsigned int src_txt_size, byte parse_verbose )
{
    int res = 0;

    umi_fatal_error = 0;
    umi_verbose_mode = parse_verbose;

    // TBD maybe: Lock lexer/parse here !
    // But nothing else in DoomLegacy allows multi-cpu either !

    if (umi_verbose_mode)
    {
        umi_doom_print( 0, "UMAPINFO", "Parse source text ...", NULL );  // verbose
    }

    if( (src_txt_size == 0) || (src_txt == NULL) )
    {
        if (umi_verbose_mode)
        {
            umi_doom_print( 0, "UMAPINFO", "No umapinfo source text", NULL );  // verbose
        }

        return UMI_ERROR_NOT_UMAPINFO;
    }
   
    // Do not need to copy the umapinfo lump.
    // The FLEX lexer required two 0 at end of buffer, which is no longer required.
    // The new lexer can accept any buffer, and tests for EOB using the buffer size.
    umi_lexer_init( (char *) src_txt, src_txt_size );
    // Parse the umapinfo lump
    umi_parser_init();
    res = umi_parser_parse();  // Parse the umapinfo lump, call back umi_doom_store.
    // res = possible syntax error codes
    // Fatal errors are reported to umi_fatal_error, so parser can report syntax errors.
    umi_lexer_destroy();

    // TBD maybe: Unlock lexer/parse here !

    // Returning special error codes to caller will not accomplish anything better than
    // this library has already done.
    // There are no actions the caller could take to fix anything.
    // The only thing they need to know is failure, and is it severe enough that they should abort.
    // Failure levels:
    // 0: successfully processed umapinfo
    // UMI_ERROR_NOT_UMAPINFO: not umapinfo, buffer content ignored
    // UMI_ERROR_PARTIAL: partial processed umapinfo, with errors
    // <= UMI_ERROR_FATAL: total failure abort, system failure, memory failure, (neg error codes)
    
    if( umi_fatal_error < 0 )
        res = umi_fatal_error;

    return res;
}


#ifdef UMI_MEMORY_ALLOC
// [WDJ] This is no longer needed, due to the removal of the intermediate umapinfo storage.
// The last call appears in the lexer, in ENABLE_BUFFER_STACK code, which is disabled.
// It is retained in case of future needs.

#include <stdlib.h>


/* ======================================================================== */
void * umi_malloc( size_t len )
{
    // [WDJ] malloc can handle 0 size, returning NULL or a ptr to a 0 sized allocation.
    if( len > 0 )
    {
        void * res = malloc(len);
        if( res == NULL )  goto memory_error;
        return res;
    }
#ifdef PARANOID
    umi_doom_print( 1, "UMI_MEMORY", "alloc of size <= 0", "" );  // debug
#endif
    return NULL;

memory_error:    
    umi_fatal_error = UMI_ERROR_MEMORY;
    return NULL;
}


/* ======================================================================== */
void * umi_realloc( void * p, size_t len )
{
    /* Behaviour is ambiguous for (len == 0) */
    // [WDJ] Umapinfo grows allocations, it does not reduce them to 0.
    // size_t is supposedly unsigned, but not always, esp. older compilers.
    if( len >= 0 )  // cannot be false when size_t is unsigned.
    {
        // realloc of 0 is same as free.
        void * res = realloc( p, len );
        if( res == NULL && len )  goto memory_error;  // contradictory with above test
        return res;
    }
#ifdef PARANOID
    umi_doom_print( 1, "UMI_MEMORY", "realloc of size < 0", "" );  // debug
#endif
    return NULL;

memory_error:    
    umi_fatal_error = UMI_ERROR_MEMORY;
    return NULL;
}


/* ======================================================================== */
void umi_free( void * p )
{
    free(p);
}
#endif


/* ======================================================================== */

// UMAPINFO end
#endif
/* EOF */
