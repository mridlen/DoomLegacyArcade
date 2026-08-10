//-----------------------------------------------------------------------------
//
// $Id: doom_umi_lexer.h 1610 2023-10-06 09:40:13Z wesleyjohnson $
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

#ifndef DOOM_UMI_LEXER_H
#define DOOM_UMI_LEXER_H  1

/* Header file for lexer */

extern unsigned int umi_lexer_lineno;
extern char * umi_lexer_text;
extern int umi_lexer_text_len;
extern int umi_lexer_value;


void umi_lexer_init(char *, unsigned int);
void umi_lexer_destroy(void);

// T_name is used as type in so many projects, that it is distracting.
// TOK_ is standard for token types, and is easy for search.
// Special token returned by umi_lexer_get_token.
typedef enum {
// TOK symbols 0..31
  TOK_EOB = 0,
  TOK_MAP = 4,
  TOK_MAPNAME_DOOM1 = 5,
  TOK_MAPNAME_DOOM2 = 6,
  TOK_NUMBER = 8,
  TOK_KEYWORD = 9,
  TOK_QSTRING = 10,
  TOK_ID = 11,
  TOK_UNKNOWN_SYMBOL = 12,
// Token char as ASCII value. Simpler for lexer.
  TOK_OBRACE = 0x7B,
  TOK_CBRACE = 0x7D,
  TOK_EQUAL = 0x3D,
  TOK_COMMA = 0x2C,
// status, not returned by any lexer function   
  TOK_ERROR = 128,
  TOK_EMPTY = 129,
  TOK_INVALID = 130,  // not valid
     // TOK_INVALID is used as max token value
} umi_token_e;

int umi_lexer_get_token (void);


// Tokens passed lexer to parser.

// Discriminant: umi_type_e
typedef union YYSTYPE
{
    /* For tokens */
    int           err;    /* Error code (suitable for API) */
    umi_mapid_t   epimap; /* Epsiode and map numbers from <map_name> */
    umi_qstring_t qstr;   /* Quoted string */
    unsigned int  num;    /* Number */

    /* For non-terminals */
//    umi_value_t   val;    /* Value */
//    unsigned int  index;  /* Index for array or table */
//    umi_mapid_t   emnum;  /* Episode and map numbers */
} umi_tokenvalue_t;

extern umi_tokenvalue_t  umi_token;


// Return token when match, otherwise 0.
uint16_t umi_lexer_scan_map_identifier( /*OUT*/ umi_mapid_t * out_mapid );

// In case of an error.
void umi_lexer_report_line( int8_t errcode );


#endif  /* DOOM_UMI_LEXER_H */
