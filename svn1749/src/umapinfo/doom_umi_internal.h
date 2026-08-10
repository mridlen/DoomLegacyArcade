//-----------------------------------------------------------------------------
//
// $Id: doom_umi_internal.h 1610 2023-10-06 09:40:13Z wesleyjohnson $
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

#ifndef DOOM_UMI_INTERNAL_H
#define DOOM_UMI_INTERNAL_H  1


/* Header file for library internals */


#include <stddef.h>

#include "doomtype.h"
#include "doom_umi.h"


/* Tables to map keyword or thing index to string */
#define NUM_UMI_KEYWORDS 22U
#define NUM_UMI_THINGS   250U
// Max length of thing name, (things have max length of 21)
#define UMI_THINGS_MAXLEN  28
extern const char * const umi_keyword_name[NUM_UMI_KEYWORDS];
extern const char * const umi_thing_name[NUM_UMI_THINGS];

// indexed by  umi_keyword_e
// Not found must be neg number.
#define UMI_KEY_NOT_FOUND  (-21)
#define UMI_THING_NOT_FOUND  (-22)

// Return UMI_KEY_ value.
// Return UMI_KEY_NOT_FOUND when not found (neg value).
int  umi_keyword_lookup( const char * istr, unsigned int len,
                         const char * const * keyword_table, const unsigned int keyword_table_len );


/* Octets between UMAPINFO quoted string delimiters */
extern umi_qstring_t  umi_lexer_string;


/* Print debug messages to stderr */
extern byte umi_verbose_mode;  /* 0: off, 1: Normal, 2: More */

// Parse the umapinfo source text.
/* Return an opaque UMAPINFO object */
int umi_parse_umapinfo( /*IN*/ const byte * src_txt, /*IN*/ unsigned int src_txt_size );


#ifdef UMI_MEMORY_ALLOC
// This is no longer needed, due to the removal of the intermediate umapinfo storage.
// The last call appears in ENABLE_BUFFER_STACK code, which is disabled.
// It is retained in case of future needs.

void * umi_malloc(size_t len);
void * umi_realloc(void * p, size_t len);
void   umi_free(void * p);
#endif

#endif  /* DOOM_UMI_INTERNAL_H */
/* EOF */
