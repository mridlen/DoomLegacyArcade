//-----------------------------------------------------------------------------
//
// $Id: doom_umi_parser.c 1610 2023-10-06 09:40:13Z wesleyjohnson $
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
 * Parser for DoomLegacy libdoom-umapinfo
 *
 * Copyright (c) 2022-2023 by the developers. See the LICENSE file for details.
 *
 *
 * Input data must be an ASCII text lump.
 * Arbitrary octets (extended encodings) are accepted in quoted strings (except delimiter itself).
 */

#include "doomdef.h"
#ifdef ENABLE_UMAPINFO

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
  // vsnprintf

//#include <stdlib.h>
  // malloc
#include <string.h>
  // memset, strcasecmp

#include "doom_umi.h"
#include "doom_umi_internal.h"
#include "doom_umi_lexer.h"
  // umi_lexer_get_token
  // umi_tokenvalue_t
#include "doom_umi_parser.h"


#define UMI_PARSER_PRINT_STR  "UMI PARSER"
#define MSG_PRINT_BUF_SIZE  1024


#define DEBUG_UMAPINFO_PARSE



// TOK_name are defined in umi_lexer.h

#ifdef DEBUG_UMAPINFO_PARSE
// indexed by TOK_
static const char * const umi_tok_name[] = {
"TOK_EOB",0,0,0,  // 0x00
"TOK_MAP","TOK_MAPNAME_DOOM1","TOK_MAPNAME_DOOM2",0, // 0x4, 4
"TOK_NUMBER","TOK_KEYWORD","TOK_QSTRING","TOK_ID", // 0x08, 8
"TOK_UNKNOWN_SYMBOL",0,0,0, // 0x0C, 12
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x10
0,0,0,0, 0,0,0,0, 0,0,0,0, "TOK_COMMA",0,0,0, // 0x20
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,"TOK_EQUAL",0,0, // 0x30
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x40
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x50
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x60
0,0,0,0, 0,0,0,0, 0,0,0,"TOK_OBRACE", 0,"TOK_CBRACE",0,0, // 0x70
    
"TOK_ERROR","TOK_EMPTY","TOK_INVALID",0, // 0x80, 128
#  if 0
0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x84
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0x90
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0xA0
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0xB0
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0xC0
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0xD0
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0xE0
0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 0xF0
#  endif
};

// tok : umi_parser_tok
const char * token_lookup( uint16_t tok )
{
    if( tok > TOK_INVALID )  tok = TOK_INVALID;
    const char * yys = umi_tok_name[tok];
    if( yys == NULL )  yys = umi_tok_name[TOK_INVALID];
    return yys;
}

#define DEBUG_PRINT_BUF_SIZE  1024
static  const char * UMI_PARSER_DEBUG_STR = "UMI PARSER";
byte  EN_umi_debug_out = 0;  // PUBLIC, used in main code

static
void umi_parse_debug_printf( const char * format, ... )
{
    // [WDJ] print from va_list
    va_list args;
    char  buf[ DEBUG_PRINT_BUF_SIZE ];

    va_start(args, format);
   
    // print the error
    vsnprintf(buf, DEBUG_PRINT_BUF_SIZE-1, format, args);
    buf[DEBUG_PRINT_BUF_SIZE-1] = '\0'; // term, when length limited
    va_end(args);

    umi_doom_print( 1, UMI_PARSER_DEBUG_STR, buf, "" ); // debug
}
#endif


// PUBLIC
int  umi_parser_result;  // API error code
int  umi_parser_num_errors;

umi_doom_store_ft  umi_doom_store = NULL;

// umi_tokenvalue_t  umi_token;  // from lexer

// END PUBLIC


/* ======================================================================== */
// Indicate by errcode if verbose mode, or debug, is enabled, or there is an important error.
//   errcode: 0=verbose, 1=debug, otherwise errcode.
static
void umi_parser_msg( int8_t errcode, const char*  format, ... )
{
    va_list args;
    char  buf[ MSG_PRINT_BUF_SIZE ];

    va_start(args, format);
    vsnprintf( buf, MSG_PRINT_BUF_SIZE-1, format, args );
    buf[MSG_PRINT_BUF_SIZE-1] = 0; // term, when length limited
    va_end(args);

    umi_doom_print( errcode, UMI_PARSER_PRINT_STR, buf, "" ); // verbose. debug, or error
}



/* ======================================================================== */
// Copy to buffer as tolower, for comparisons.
static
void umi_parser_qstring_tolower( umi_qstring_t * src, unsigned int dest_len, /*OUT*/ char * dest )
{
    unsigned int i;
    dest_len--;  // for term
    if( src->len < dest_len )   dest_len = src->len;
    for (i = 0; i < dest_len; i++)
    {
       // tolower may be a macro, or table lookup, requires unsigned
       unsigned char ch = src->octet[i];  // uppercase and lowercase
       if( ch < 128 )
           ch = tolower(ch);
       *dest++ = ch;
    }
    *dest = 0;
}


/* ======================================================================== */
/* Case insensitive search for US-ASCII thingtype, due to mixed case tables */
// Return UMI_KEY_ value.
// Return UMI_THING_NOT_FOUND when not found (neg value).
//   thstr : thing string, lowercase, 0 term
static
int umi_parser_thing_lookup( const char * thstr )
{
    unsigned int ti = NUM_UMI_THINGS;
    while (ti)  // search bottom to top
    {
        const char * stn = umi_thing_name[--ti];
        if( ! strcasecmp( stn, thstr ) )
        {
            // thing id num, value equals table index	   
            return ti; // found	    
        }
    }
    return UMI_THING_NOT_FOUND;
}


/* ======================================================================== */
//  ldv : result, required
static
int umi_value_check( umi_qstring_t * qstr, /*OUT*/ umi_line_value_t * ldv )
{
    int v_type  = UMI_TYPE_INVALID;
  
    // Convert input qstr tolower once.
    // then compare that to known identifiers
    char qubuf[ UMI_THINGS_MAXLEN+1 ];
   
    if( qstr->len >= UMI_THINGS_MAXLEN )
        goto done; // too long, cannot match anything

    umi_parser_qstring_tolower( qstr, UMI_THINGS_MAXLEN, /*OUT*/ qubuf );

    /* For the 'clear' identifier the index has no meaning */
    if( strcmp("clear", qubuf) == 0 )
    {
        v_type  = UMI_TYPE_CLEAR;
        ldv->v.number = 0;
    }

    /* For boolean values a nonzero index means true without table resolution */
    else if ( strcmp("false", qubuf) == 0 )
    {
        v_type  = UMI_TYPE_BOOL;
        ldv->v.number = 0;
    }
    else if ( strcmp("true", qubuf) == 0 )
    {
        v_type  = UMI_TYPE_BOOL;
        ldv->v.number = 1;
    }

    /* For things the index can be used to get the associated name string */
    else
    {
        int t_index = umi_parser_thing_lookup( qubuf );
        if (t_index == UMI_THING_NOT_FOUND)  // neg error code
        {
            return UMI_ERROR_NOTFOUND;
        }
#ifdef PARANOID
        if( t_index >= NUM_UMI_THINGS )
        {
            return UMI_ERROR_INTERNAL;  // should not happen
        }
#endif
        ldv->v.identifier = (unsigned int) t_index;
        v_type = UMI_TYPE_THING;
    }

done:   
    ldv->type = v_type;

    return 0;
}


/* ======================================================================== */
/* Print error message */
void umi_parser_error( const char * msg )
{
    // umi_lexer_lineno from DoomLegacy umapinfo lexer.
    umi_parser_msg( UMI_ERROR_SYNTAX, "Line %d: %s\n", umi_lexer_lineno, msg );
    if (umi_verbose_mode)
        umi_lexer_report_line( UMI_ERROR_SYNTAX );

    return;
}


/* ======================================================================== */
/* Init parser */
void umi_parser_init( void )
{
    umi_parser_result = 0;

#ifdef DEBUG_UMAPINFO_PARSE
    /* Enable yacc debug output if verbose mode is larger than one */
    EN_umi_debug_out = (umi_verbose_mode > 1);
#endif    

#ifdef DEBUG_UMAPINFO_PARSE
    unsigned int ti = NUM_UMI_THINGS;
    while (ti)  // search bottom to top
    {
        int slen = strlen( umi_thing_name[--ti] );
        if( slen > UMI_THINGS_MAXLEN )
        {
            umi_parser_msg( UMI_ERROR_INTERNAL, "UMI_THINGS_MAXLEN too short: index=%d len=%d\n", ti, slen );
        }
    }
#endif
   
    return;
}


// Straight umapinfo lump parser.
// Returns 0, or UMI_ERROR_xxx code
// Fatal errors are reported to umi_fatal_error, so parser can report syntax errors.
int umi_parser_parse(void)
{
    umi_line_t  ld;  // line data
      // This is 776 bytes, small enough to be fixed size.
    umi_mapid_t  parser_mapid = {0,0};
    const char * errmsg;  // error message parameter
    byte  brace_level = 0;
    unsigned int map_count = 0;
    unsigned int num_key;  // in a map
    uint16_t  parser_tok;  // TOK_ value from lexer

#ifdef DEBUG_UMAPINFO_PARSE
    if( EN_umi_debug_out )
        umi_parse_debug_printf( "umi_parser_parse" );
#endif

    ld.epimap.episode = 0;
    ld.epimap.map = 0;
    ld.num_value = 0;

    umi_parser_num_errors = 0;

loop_new_token:
    parser_tok = umi_lexer_get_token();

#ifdef DEBUG_UMAPINFO_PARSE
    if( EN_umi_debug_out )
    {
        const char * yys = token_lookup( parser_tok );
        umi_parse_debug_printf( "==> token %d (%s)", parser_tok, yys );
    }
#endif
   
    // Allow token substitution in the parser.
loop_have_token:

    // Each map is considered separately, one or more.  There is no <map_list>.  No check for empty <map_list>.
    if( parser_tok == TOK_MAP )
    {
        map_count++;
        if( brace_level )
        {
            umi_parser_error( "MAP cannot be nested." );
            umi_parser_num_errors++;
        };

        uint16_t maptok = umi_lexer_scan_map_identifier( /*OUT*/ & parser_mapid );
        if( maptok == 0 )  goto tok_map_syntax;

        if (umi_verbose_mode)
        {
            // Same message for Doom1 and Doom2 maps.
            umi_parser_msg( 0, "Map (Episode: %u, Map: %u)", parser_mapid.episode, parser_mapid.map);
        }

        parser_tok = umi_lexer_get_token();
        if( parser_tok != TOK_OBRACE )  goto tok_map_syntax;
        if( brace_level < 250 )
            brace_level ++;  // to detect nesting, even though umapinfo only allows 1 level of braces

#ifdef DEBUG_UMAPINFO_PARSE    
        if (EN_umi_debug_out)
        {
            umi_parse_debug_printf( "Open <data_block>" );
        }
#endif

        ld.epimap = parser_mapid;
        ld.keyid = UMI_KEY_none;  // no action
        ld.num_value = 0;
        umi_doom_store( & ld );  // switch to new map

        num_key = 0;
        goto loop_new_token;
    }

    if( parser_tok == TOK_EOB ) // End of data
        goto end_of_data;
   
    // It is possible to detect brace level in one place,
    // and the content within braces would only be parsed after this point.
    // While that satisfies parsing correct input, it limits the
    // error detection and messages when such appears outside of the braces.
    // To get better than one "unknown" error message, the token handler would need
    // to detect that the braces context is not correct.
    // An alternative is to duplicate much of this parsing in the error message handler,
    // in order to say that such and such can only appear within the braces context.
    if( brace_level == 0 )  goto unknown_main_syntax;
   
    // Parses that are only valid within braces.
    // Does not detect <expr_list> as they had no effect.  The YACC parser
    // had empty productions for them.
    // The loop allows one or more keyword statements within a map braces context.
    if( parser_tok == TOK_ID )
    {
        int value_count = 0;
        umi_line_value_t * ldv = & ld.value[0];

        int keyi = umi_keyword_lookup( umi_lexer_text, umi_lexer_text_len, umi_keyword_name, NUM_UMI_KEYWORDS );
        // keyi = UMI_KEY_ value
        if (keyi < 0)  // invalid UMI_KEY_ value
        {
            // keyi = error code
            umi_parser_error( "Unknown keyword" );
            // continue parsing, production is guarded by keyi.
        }

        // must be followed by "="
        parser_tok = umi_lexer_get_token();
        if( parser_tok != TOK_EQUAL )  goto assign_syntax;

#ifdef DEBUG_UMAPINFO_PARSE    
        if (EN_umi_debug_out)
        {
            const char * keystr = ( keyi < 0 )? "INVALID" : umi_keyword_name[keyi] ;
            umi_parse_debug_printf( "<assign_expr> %s =", keystr );
        }
#endif       

        // value_list
        // This does not detect <empty_list> as anything special.
        for(;;)
        {
            if( value_count < MAX_VALUES_PER_LINE )  // otherwise, repeat last ldv
                ldv = & ld.value[value_count];

            parser_tok = umi_lexer_get_token();

            if( parser_tok == TOK_NUMBER )
            {
#ifdef DEBUG_UMAPINFO_PARSE
                if (EN_umi_debug_out)
                {
                    umi_parse_debug_printf( "<value> (Number) %d", umi_lexer_value );
                }
#endif
                ldv->type = UMI_TYPE_NUMBER;
                ldv->v.number = umi_lexer_value;
                goto got_value;
            }

            if( parser_tok == TOK_QSTRING )
            {
#ifdef DEBUG_UMAPINFO_PARSE
                if (EN_umi_debug_out)
                {
                    umi_parse_debug_printf( "<value> (Quoted string)" );
                }
#endif
                ldv->type = UMI_TYPE_QSTRING;
                ldv->v.qstring = umi_lexer_string;
                goto got_value;
            }

            if( parser_tok == TOK_ID )
            {
#ifdef DEBUG_UMAPINFO_PARSE
                if (EN_umi_debug_out)
                {
                    umi_parse_debug_printf( "<value> (Identifier)" );
                }
#endif
                // Identifier passed in umi_lexer_string
                umi_lexer_string.octet = (unsigned char*) umi_lexer_text;
                umi_lexer_string.len = umi_lexer_text_len;

                // Parse special values.	        
                int rv = umi_value_check( &umi_lexer_string, /*OUT*/ ldv );
                if (rv != 0)
                {
                    umi_parser_result = rv;  // not found
                    // identifier not recognized is not a syntax error, it is a semantic error.
                    umi_parser_error("Invalid identifier");
                    goto value_other;  // cannot predict if what follows is another value, so abort the assign
                }

                goto got_value;
            }

value_other:
            // Something other than a valid value.
            // Cannot terminate the value list without reading the next token
            if( parser_tok == TOK_KEYWORD )
                break;
            if( parser_tok == TOK_CBRACE )
                break;
            if( parser_tok == TOK_EOB )
                break;
            if( parser_tok == TOK_COMMA )
            {
                if (umi_verbose_mode)
                    umi_parser_msg( 0, "extraneous comma in value_list");
                continue; // next value
            }

            // Token is not valid value_list terminator.
#ifdef DEBUG_UMAPINFO_PARSE
            if( EN_umi_debug_out )
                umi_parse_debug_printf( "<value_list>, irregular token %s", token_lookup( parser_tok ) );
#endif
            if (umi_verbose_mode)
                umi_parser_msg( UMI_ERROR_SYNTAX, "unrecognized token in value list: token= %i", parser_tok );
            break;

got_value:
            // Got a value
            if( keyi >= 0 )  // only if it is a valid keyword
            {
                // There is no nesting within the value parsing, so stack is not needed here.
                ld.keyid = keyi;
                value_count++;
                ld.num_value = value_count;
            }

            // if value_list continues, there will be a comma
            parser_tok = umi_lexer_get_token();
            if( parser_tok != TOK_COMMA )
                break;  // end of list, valid or not
        }

        if( keyi >= 0 )  // only if it is a valid keyword
        {
            num_key++;
            umi_doom_store( & ld );  // store the line data
        }

        // Cannot terminate the value list without reading the next token
        goto  loop_have_token;
    }

    if( parser_tok == TOK_CBRACE )
    {
#ifdef DEBUG_UMAPINFO_PARSE
        if (EN_umi_debug_out)
        {
            umi_parse_debug_printf( "Close <data_block>, MAP(%u,%u) had %u keys",
                                    parser_mapid.episode, parser_mapid.map, num_key );
        }
#endif
        brace_level --;

        // close current map
        if( parser_mapid.map == 0 ) // don't know how this could happen, but protect it
        {
#ifdef DEBUG_UMAPINFO_PARSE
            umi_parse_debug_printf( "MAP invalid at close, episode=%i, map=%i",
                                    parser_mapid.episode, parser_mapid.map );
#endif
            umi_parser_msg( UMI_ERROR_INTERNAL, "MAP invalid at close" );
            goto loop_new_token;
        }

        // Maps do not nest, so there is only one possible mapid.
        parser_mapid.map = 0;
        num_key = 0;

        goto loop_new_token;
    }
    // Failed to be recognized.    
    goto unknown_token_syntax;
    
#if 0
    // This is currently unused, but available.
    // It uses:  parser_tok = TOK_EMPTY;
maybe_get_token:
    if (parser_tok == TOK_EMPTY)  // empty
        goto loop_new_token;
    goto loop_have_token;  // parse the current value of parser_tok
#endif

end_of_data:
    // This must clear error detection indicators as it reports errors,
    // so it can eventually exit.
    // EOB is persistant, so it will still be there even if we get another token.
    if( brace_level )
    {
        errmsg= "Unmatched open brace";
        brace_level = 0;  // clear brace nesting
        goto syntax_err;
    }
    goto exit_parser;

    // [WDJ] This is not a umapinfo compiler, so error messaging does not
    // need to be complete.
    // It only needs to indicate why the umapinfo lump did not load.
    // Errcodes are neg, with most severe the most negative.

unknown_main_syntax:
    if( umi_parser_result > UMI_ERROR_SYNTAX )
        umi_parser_result = UMI_ERROR_SYNTAX;

    if (umi_verbose_mode)
    {
        umi_parser_msg( UMI_ERROR_SYNTAX, "Toplevel map entry expected");
    }
   
    if( parser_tok == TOK_ID )
        errmsg= "Keyword statements must be within map brace context";
    else if( parser_tok == TOK_CBRACE )
        errmsg= "Unmatched closing brace";
    else
        goto unknown_token_syntax;
    goto syntax_err;

unknown_token_syntax:
    errmsg= "Syntax, cannot parse";
#ifdef DEBUG_UMAPINFO_PARSE
    // have debug tables
    umi_parse_debug_printf( "%s: %s", errmsg, token_lookup( parser_tok ) );
#else
    // Do not have token_lookup without DEBUG.  Normal user will not know about TOK_ anyway.
    umi_parser_msg( "%s: token= %i", errmsg, parser_tok );
#endif
    goto syntax_err;

tok_map_syntax:
    errmsg= "MAP syntax: MAP ExMx { stmts }, or with MAPx";
    goto syntax_err;

assign_syntax:
    errmsg= "Assign syntax";
    goto syntax_err;

syntax_err:
    umi_parser_num_errors++;
    umi_parser_error(errmsg);
    goto partial_recover_continue;

#if 0
    // Does not seem to have any fatal errors anymore.
fatal_err:
    // If go here without setting FATAL, then must be INTERNAL ERROR.
    // Fatal errors are reported to umi_fatal_error, so parser can report syntax errors.
    if( umi_fatal_error > UMI_ERROR_FATAL )  // less than a fatal error
        umi_fatal_error = UMI_ERROR_INTERNAL;

    if( parser_mapid.map )
        goto abort_map;  // have a map open, so kill it.
    goto abort_parser;
#endif    
   
#if 0
    // Does not seem to need abort_map anymore.
abort_map:
    if( parser_mapid.map )
    {
        umi_parser_error("Map aborted");
        parser_mapid.map = 0;
    }
   
    if( umi_parser_result <= UMI_ERROR_FATAL )  // any fatal error
        goto abort_parser;

    // Scan to end of map brace context.
    while( parser_tok != TOK_EOB )
    {
        if( parser_tok == TOK_CBRACE )
            goto partial_recover_continue;  // end of map, parse next map

        parser_tok = umi_lexer_get_token();
    }
    // Do not abort_parser, because that implies FATAL.
    // Let the parser exit normally, indirectly.
    goto partial_recover_continue;
#endif   

#if 0
    // Does not seem to need to abort parser anymore
abort_parser:
    // aborting, cannot be PARTIAL
    if( umi_parser_result >= UMI_ERROR_PARTIAL )  // less than PARTIAL
    {
        if( umi_parser_num_errors )  // syntax errors
            umi_parser_result = UMI_ERROR_SYNTAX;
        else
            umi_parser_result = UMI_ERROR_FATAL; // unknown fatal parse
    }
    goto exit_parser;
#endif

partial_recover_continue:
#if 0
    if( ! umi_parser_result )
        umi_parser_result = UMI_ERROR_PARTIAL;
#endif
    goto loop_new_token;  // continue parsing

exit_parser:
    if (umi_verbose_mode)
    {
        umi_parser_msg( 0, "%u Maps, %u Errors", map_count, umi_parser_num_errors );
    }

    if( umi_parser_result > UMI_ERROR_PARTIAL ) // less than PARTIAL
    {
        // A bit of a guess
        if( map_count == 0 )
            umi_parser_result = UMI_ERROR_NOT_UMAPINFO;
        else if( umi_parser_num_errors )  // some maps
            umi_parser_result = UMI_ERROR_PARTIAL;
    }
    return umi_parser_result;
}


// UMAPINFO end
#endif
