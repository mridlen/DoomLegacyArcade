// -----------------------------------------------------------------------------
//
// Copyright(C) 2023 by DooM Legacy Team
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
// DESCRIPTION:
//      Support for additional map information in UMAPINFO format.
//
// -----------------------------------------------------------------------------

// [MB] 2023-01-21: Support for Rev 2.2 added
//      Description of UMAPINFO lump format:
//      https://doomwiki.org/wiki/UMAPINFO

#include "doomdef.h"
#ifdef ENABLE_UMAPINFO

//#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "umapinfo/doom_umi.h"
#include "umapinfo.h"

#include "dehacked.h"
#include "doomincl.h"
#include "doomstat.h"
#include "p_info.h"
#include "w_wad.h"
#include "z_zone.h"

// -----------------------------------------------------------------------------

// Connect umi_doom_print to GenPrintf
void  dl_umi_doom_print( int8_t errcode, const char * m1, const char * m2, const char * m3 )
{
    byte  emsg = 0;
    if( errcode < 0 )
    {
        // errcodes are neg, with most severe the most negative
        if( errcode <= UMI_ERROR_FATAL )  // first fatal error
            emsg = EMSG_error2;  // serious fatal error
        else
            emsg = EMSG_warn;  // non-fatal
    }
    else if (errcode == 0)
    {
       emsg = EMSG_ver;  // verbose
    }
    else
    {
       emsg = EMSG_debug;
    }

    if( m2 == NULL )  m2 = "";
    if( m3 == NULL )  m3 = "";

    GenPrintf( emsg, "%s: %s %s\n", m1, m2, m3 );
}

static
void  umi_dl_error( int8_t errcode, const char * msg )
{
    dl_umi_doom_print( errcode, "UMAPINFO", umi_error_name(errcode), msg );
}

// -----------------------------------------------------------------------------

// Internal representation of UMAPINFO data (merged from PWADs)
umapinfo_t umapinfo = { NULL, NULL, false };


// -----------------------------------------------------------------------------
// Memory management

// [WDJ] The only reason for using Z_Malloc would be if the data was mixed in with
// existing data that is allocated with Z_Malloc.
// Data that is managed, and not automatically released on map change, does not need
// to use Z_Malloc at all.  Such can use malloc, and free it themselves.

// [WDJ] These must be distinguishable from the other umi_malloc, and umi_free in doom_umi_memory.c

//  memsize: size to be allocated by Z_Malloc
static
void * UMI_ZMalloc( size_t memsize )
{
    return Z_Malloc( memsize, PU_STATIC, 0 );
    // [WDJ] DoomLeagcy Z_Malloc never returns NULL.
    // If it cannot allocate, it does not return.
    // None of the umapinfo callers have any contingency for out-of-memory, and none of them test for it.
    // If there was some need for coping with out-of-memory situations, then should not be using Z_Malloc.
}

//  ptr: memory to be freed.  This memory must have been allocated by Z_Malloc.
static
void UMI_ZFree( void * zptr )
{
    if (zptr)
        Z_Free(zptr);
}


// -----------------------------------------------------------------------------
// Constructors and destructors

//  entry: episode entry to be init
static
void UMI_init_episode_menu_entry( emenu_t * entry )
{
    entry->next  = NULL;

    entry->patch = NULL;
    entry->name  = NULL;
    entry->key   = NULL;
}


//  epi_list: episode list to be freed
static
void UMI_destroy_episode_menu( emenu_t * epi_list )
{
    while (epi_list)
    {
        emenu_t * entry = epi_list;
        epi_list = epi_list->next;  // step list before freed

        // Free the components
        UMI_ZFree( (void*) entry->patch );
        UMI_ZFree( (void*) entry->name );
        UMI_ZFree( (void*) entry->key );
        UMI_ZFree(entry);
    }
}

//  ba_entry: boss action entry to be init
static
void UMI_init_bossaction_entry( bossaction_t * ba_entry )
{
    ba_entry->next    = NULL;

    ba_entry->thing   = 0;
    ba_entry->special = 0;
    ba_entry->tag     = 0;
}


//  ba_list: boss action list to be freed
static
void UMI_destroy_bossactions( bossaction_t * ba_list )
{
    while (ba_list)
    {
        bossaction_t * ba_entry = ba_list;
        ba_list = ba_list->next; // step list before it is freed
        UMI_ZFree( ba_entry );  // free one entry
    }
}

//  map_entry: map entry to be init
//  episode_num: init to episode number
//  map_num: init to map number
static
void UMI_init_map_entry( mapentry_t * map_entry,
                         byte episode_num, byte map_num )
{
    map_entry->next              = NULL;

    map_entry->author            = NULL;
    map_entry->label             = NULL;
    map_entry->levelname         = NULL;
    map_entry->intertext         = NULL;
    map_entry->intertextsecret   = NULL;
    map_entry->interbackdrop     = NULL;
    map_entry->intermusic        = NULL;
    map_entry->nextmap           = NULL;
    map_entry->nextsecret        = NULL;
    map_entry->music             = NULL;
    map_entry->skytexture        = NULL;
    map_entry->levelpic          = NULL;
    map_entry->exitpic           = NULL;
    map_entry->enterpic          = NULL;
    map_entry->endpic            = NULL;
    map_entry->bossactions       = NULL;
    map_entry->episode           = episode_num;
    map_entry->map               = map_num;
    map_entry->flags             = 0;  // endgame unchanged
    map_entry->partime           = 0;
}


//  map_list: map list to be freed
static
void UMI_destroy_maps( mapentry_t * map_list )
{
    while (map_list)
    {
        mapentry_t *  map_entry = map_list;
        map_list = map_list->next;  // step before entry is freed

        // Release the components
        UMI_ZFree( (void*) map_entry->author );
        UMI_ZFree( (void*) map_entry->label );
        UMI_ZFree( (void*) map_entry->levelname );
        UMI_ZFree( (void*) map_entry->intertext );
        UMI_ZFree( (void*) map_entry->intertextsecret );
        UMI_ZFree( (void*) map_entry->interbackdrop );
        UMI_ZFree( (void*) map_entry->intermusic );
        UMI_ZFree( (void*) map_entry->nextmap );
        UMI_ZFree( (void*) map_entry->nextsecret );
        UMI_ZFree( (void*) map_entry->music);
        UMI_ZFree( (void*) map_entry->skytexture );
        UMI_ZFree( (void*) map_entry->levelpic );
        UMI_ZFree( (void*) map_entry->exitpic );
        UMI_ZFree( (void*) map_entry->enterpic );
        UMI_ZFree( (void*) map_entry->endpic );
        UMI_destroy_bossactions( map_entry->bossactions );
        UMI_ZFree(map_entry);
    }
}


// -----------------------------------------------------------------------------
// The parser calls dl_umi_doom_store for each keyword assignment.
// These functions store the assignments to the DoomLegacy umapinfo structures.

// -----------------------------------------------------------------------------

// Accept only printable ASCII characters. Others are replaced with '?'
//  str: string to be modified
//  length: length of the string
//  multiline: when true, LF control characters are accepted too
static
void UMI_ConvertToASCII( char* str, unsigned int length, boolean multiline )
{
    unsigned int i = 0;

    for (i = 0; length > i; ++i)
    {
       char ch = str[i];
       if (multiline && ( ch == 0x0A))
           continue;

#if 0       
// This assumes that all extended ASCII character codes are BAD.  This is not true.
// Someone from another country changing text to their language will need those characters.
// It is the print that should decide if the font has glyphs for those codes, or not.
       if ((ch < 0x20) || (ch > 0x7E))
            str[i] = '?';
#endif
    }
}

// Copy as a terminated string
// Return the string.  Does not return on allocation failure.
static
char * UMI_copy_qstring( const umi_qstring_t * qstr, byte convert_to_ascii )
{
    char * str = UMI_ZMalloc( qstr->len + 1 );  // add term
    // Does not return if not allocated.
    memcpy( str, qstr->octet, qstr->len );
    str[qstr->len] = 0;

    if( convert_to_ascii )
        UMI_ConvertToASCII( str, qstr->len, false );
     
    return str;
}

//  ldv : must be a ldv with type QSTRING
static
char * UMI_copy_ld_qstring( const umi_line_value_t * ldv, byte convert_to_ascii )
{
    return UMI_copy_qstring( & ldv->v.qstring, convert_to_ascii );
}

// Replace a string in the umapinfo map structure.
// Control characters (e.g. line breaks) are not allowed
//  ldv : value from parser.  Needs to have type QSTRING
//  out_str : string slot in map_entry, must be present
static
void UMI_replace_string( const umi_line_value_t * ldv, /*INOUT*/ const char** out_str )
{
    if( ldv->type != UMI_TYPE_QSTRING )
        return;

    // This is only used to store into umapinfo data structures.
    if ( ldv->v.qstring.octet )
    {
        char * str = UMI_copy_qstring( &ldv->v.qstring, 1 );
        UMI_ZFree( (void*) *out_str );  // old string
        *out_str = str;  // replace string
    }
}


// Like UMI_Replace_String(), but ignores strings with more than 8 characters
//  ldv : value from parser.  Needs to have type QSTRING
//  out_str : string slot in map_entry, must be present
static
void UMI_replace_string_max8( const umi_line_value_t * ldv, /*INOUT*/ const char** out_str )
{
    // This is only used to store into umapinfo data structures.
    if( ldv->type != UMI_TYPE_QSTRING )
        return;

#if 1
    umi_qstring_t qs8 = ldv->v.qstring;  // so can modify
    if( qs8.octet )
    {
        if( qs8.len > 8 )  qs8.len = 8;  // only 8 char
        UMI_ZFree( (void*) *out_str );  // old string
        *out_str = UMI_copy_qstring( &qs8, 1 );  // convert_to_ascii, replace string
    }
#else
    if( ldv->v.qstring.octet && ldv->v.qstring.len <= 8 )  // ignore if too long
    {
        UMI_ZFree( (void*) *out_str );  // old string
        *out_str = UMI_copy_qstring( &ldv->v.qstring, 1 );  // convert_to_ascii, replace string
    }
#endif
}


// Allocate an empty string.
// Cannot fail.
static
const char* UMI_create_empty_string()
{
    // This is only used to store into umapinfo data structures.
    char *  estr = UMI_ZMalloc(1);  // If this failed, it would not return.
    estr[0] = 0;
    return estr;
}


// Same as UMI_Replace_String(), but a 'clear' identifier will give an empty string.
//  ldv : value from parser.  Needs to have type QSTRING or CLEAR
//  out_str : string slot in map_entry, must be present
static
void UMI_replace_string_clear( const umi_line_value_t * ldv, /*INOUT*/ const char** out_str )
{
    // This is only used to store into umapinfo data structures.
    if (ldv->type == UMI_TYPE_CLEAR)
    {
        *out_str = UMI_create_empty_string();
    }
    else
    {
        UMI_replace_string( ldv, out_str );
    }
}


// It is allowed that multiple lines are contained in one value.
//  ld : line value list from parser.  All values need to have type QSTRING.
//  out_str : string slot in map_entry, must be present
static
void UMI_replace_multistring( const umi_line_t * ld, /*INOUT*/ const char** out_str )
{
    // This is only used to store into umapinfo data structures.
    char *  multi        = NULL;
    int  multi_alloc_length = 0;  // term
    unsigned int  multi_length = 0;
    unsigned int  valnum       = 0;
    unsigned int  valcount = ld->num_value;

    // Step through the value list, which should all be QSTRING.
for_all_value:  // goes through the list twice   
    for (valnum = 0; valnum < valcount; valnum++)
    {
        const umi_line_value_t * ldv = & ld->value[valnum];
        if( ldv->type != UMI_TYPE_QSTRING )
            break;

        unsigned int  qstr_length = ldv->v.qstring.len;
        const char *  qstr = (const char *) ldv->v.qstring.octet;
        if (qstr == NULL)
            break;

        if (multi == NULL)
        {
            // just total the length needed
            // Each line either gets an LF after it, or a 0 after it.
            multi_alloc_length += qstr_length + 1;
            continue;
        }
        else if (qstr_length)  // Append to multi
        {
            // multi was allocated at correct length already.
            // Each line either gets an LF after it, or a 0 after it.
            if( multi_length + qstr_length + 1 > multi_alloc_length )
                break;  // something wrong, we cannot count right

            // Put LF between lines of the multi.
            if( valnum > 0 )
                multi[multi_length++] = 0x0A;  // LF

            memcpy( &multi[multi_length], qstr, qstr_length );  // append
            multi_length += qstr_length;
            multi[multi_length] = 0; // term
        }
    }

    if( multi == NULL )  // first time here
    {
        if( multi_alloc_length < 1 )  // something wrong, like a len was negative
            multi_alloc_length = 1;  // empty string

        // Even if only length 1, replace the text.
        multi = UMI_ZMalloc( multi_alloc_length );
        multi[0] = 0;  // empty string
        goto for_all_value;  // go through list again, append all to multi
    }

    if ((valcount == 0) || (valcount != valnum))
        umi_dl_error( -1, "(RMS) Incomplete multi-line string");


    // multi must exist, due to ZMalloc not returning otherwise
    UMI_ConvertToASCII( multi, multi_length, true );
    UMI_ZFree( (void*) *out_str );
    *out_str = multi;
}


// Same as UMI_Replace_MultiString(), but 'clear' identifier gives empty string.
//  ld : line value list from parser.  All values need to have type QSTRING, or CLEAR.
//  out_str : string slot in map_entry, must be present
static
void UMI_replace_multistring_clear( const umi_line_t * ld, const char** out_str )
{
    // This is only used to store into umapinfo data structures.
    const umi_line_value_t * ldv = &ld->value[0]; // first value

    if( ldv->type == UMI_TYPE_CLEAR )
    {
        *out_str = UMI_create_empty_string();  // cannot fail
    }
    else
    {
        UMI_replace_multistring( ld, out_str );
    }
}


// -----------------------------------------------------------------------------
// Episode menu


//  ld : line value list from parser.
//  em : menu slot in map_entry, must be present
static
void UMI_append_episode_menu_entry( const umi_line_t * ld, /*INOUT*/ emenu_t** em )
{
    emenu_t *  entry = UMI_ZMalloc(sizeof(emenu_t));
    UMI_init_episode_menu_entry( entry );

    // Fill in the entry
    const umi_line_value_t * ldv = &ld->value[0]; // first value
    if( ld->num_value < 3 )
        goto fail;

    // Must be exact, so fail if not all correct type.
    if( ldv->type != UMI_TYPE_QSTRING )
        goto fail;
    entry->patch = UMI_copy_ld_qstring( ldv, 1 );

    ldv++;
    if( ldv->type != UMI_TYPE_QSTRING )
        goto fail;
    entry->name = UMI_copy_ld_qstring( ldv, 1 );
   
    ldv++;
    if( ldv->type != UMI_TYPE_QSTRING )
        goto fail;
    entry->key = UMI_copy_ld_qstring( ldv, 1 );
   
    // Append this episode menu entry to list
    if (*em == NULL)
    {
        // Empty list
        *em = entry;
    }
    else
    {
        // Find end of list
        emenu_t * etailp = *em;
        while (etailp->next)  etailp = etailp->next;
        etailp->next = entry; // append
    }
    return;

fail:
    umi_dl_error( UMI_ERROR_INTERNAL, "(AEME) append menu entry");
    UMI_destroy_episode_menu( entry );
    return;

}


// -----------------------------------------------------------------------------
// Boss actions


//  ld : line value list from parser.  All values need to have type QSTRING.
//  bal : bossaction entry list in map_entry, must be present
static
void UMI_append_bossaction( const umi_line_t * ld, bossaction_t** bal )
{
    // Append new Boss Action entry	
    bossaction_t *  ba_entry = UMI_ZMalloc( sizeof(bossaction_t) );
    UMI_init_bossaction_entry( ba_entry );

    // Exit whenever the wrong type occurs.
    const umi_line_value_t * ldv = &ld->value[0]; // first value
    if( ld->num_value < 3 )
        goto fail;

    if( ldv->type != UMI_TYPE_THING )
        goto fail;
    unsigned int thing_index = ldv->v.identifier; // thing number
    // thing_index limited to NUM_UMI_THINGS by parser
    ba_entry->thing = thing_index;

    ldv++;
    if( ldv->type != UMI_TYPE_NUMBER )
        goto fail;
    ba_entry->special = ldv->v.number;  // action

    ldv++;
    if( ldv->type != UMI_TYPE_NUMBER )
        goto fail;
    ba_entry->tag = ldv->v.number;  // line tag

    // Append to end of list   
    if (*bal == NULL)
    {
        *bal = ba_entry;  // Was empty list
    }
    else
    {
        // Append to list		
        bossaction_t * last = *bal;
        while (last->next)  last = last->next;
        last->next = ba_entry;
    }
    return;

fail:   
    umi_dl_error( UMI_ERROR_INTERNAL, "(ABA) append boss action" );
    UMI_destroy_bossactions( ba_entry );
    return;
}


// -----------------------------------------------------------------------------
// Check whether map entry object for episode/map already exists.
// If not, create an empty map entry object.
//  episode_num: episode num to be found
//  map_num: map_num to be found
static
mapentry_t *  UMI_get_map_entry( byte episode_num, byte map_num )
{
    mapentry_t *  map_entry  = umapinfo.map_list;
    mapentry_t **  link_point = &umapinfo.map_list;  // append to list here

    while (map_entry)  // the list exists
    {
        if ((map_entry->episode == episode_num) && (map_entry->map == map_num))
            goto done;  // found match, it already exists

        link_point = &map_entry->next;  // last known next, for append later
        map_entry = map_entry->next;
    }

    if (map_entry == NULL)   // end of list
    {
        // Not found, create new entry
        map_entry = UMI_ZMalloc( sizeof(mapentry_t) );
        UMI_init_map_entry( map_entry, episode_num, map_num );

        // Append to end of map list.
        *link_point = map_entry;  // append
    }

done:   
    return map_entry;
}


static inline
void  store_map_entry_flag( const umi_line_value_t * ldv, mapentry_t * map_entry, uint8_t uma_flag )
{
    if( ldv->type == UMI_TYPE_BOOL || ldv->type == UMI_TYPE_NUMBER )
    {
        if( ldv->v.number )        
            map_entry->flags |=  uma_flag;
        else
            map_entry->flags &= ~uma_flag;
    }
}


// -----------------------------------------------------------------------------
// Store parser data direct

// The current map entry being filled with assignments.
static mapentry_t *  map_entry = NULL;
// To force failure, when num_value == 0, so can keep ld as const.
const umi_line_value_t invalid_ldv = { UMI_TYPE_INVALID, { 0 } };

void dl_umi_doom_store( const umi_line_t * ld )
{
    // If have advanced to a different map, then change the map_entry.
    if( map_entry )
    {
        if( map_entry->episode != ld->epimap.episode
            || map_entry->map != ld->epimap.map )
            map_entry = NULL;
    }

    // Create structure for new map.
    if( ! map_entry )
    {
        map_entry = UMI_get_map_entry( ld->epimap.episode, ld->epimap.map );
        if( ! map_entry )
            return;
    }

    // Some keywords have only one value, and can be handled easily.
    const umi_line_value_t * ldv = &ld->value[0]; // first value
    if( ld->num_value == 0 )
    {
#ifdef PARANOID
        // UMI_TYPE_CLEAR should have num_value = 1, but it is not necessary.
        if( ld->keyid != UMI_KEY_none && ldv->type != UMI_TYPE_CLEAR )  // internal error check
        {
            umi_dl_error( UMI_ERROR_INTERNAL, "(UDS) umi_doom_store with no values");
            return;
        }
#endif
        ldv = & invalid_ldv;  // only UMI_KEY_none has no values, force errors for others
    }
   
    switch (ld->keyid)
    {
        case UMI_KEY_author:
            UMI_replace_string( ldv, &map_entry->author);
            break;
        case UMI_KEY_label:
            UMI_replace_string_clear( ldv, &map_entry->label);
            break;
        case UMI_KEY_levelname:
            UMI_replace_string( ldv, &map_entry->levelname);
            break;
        case UMI_KEY_intertext:
            UMI_replace_multistring_clear( ld, &map_entry->intertext );
            break;
        case UMI_KEY_intertextsecret:
            UMI_replace_multistring_clear( ld, &map_entry->intertextsecret );
            break;
        case UMI_KEY_interbackdrop:
            UMI_replace_string_max8( ldv, &map_entry->interbackdrop);
            break;
        case UMI_KEY_intermusic:
            UMI_replace_string_max8( ldv, &map_entry->intermusic);
            break;
        case UMI_KEY_next:
            UMI_replace_string_max8( ldv, &map_entry->nextmap);
            break;
        case UMI_KEY_nextsecret:
            UMI_replace_string_max8( ldv, &map_entry->nextsecret);
            break;
        case UMI_KEY_music:
            UMI_replace_string_max8( ldv, &map_entry->music);
            break;
        case UMI_KEY_skytexture:
            UMI_replace_string_max8( ldv, &map_entry->skytexture);
            break;
        case UMI_KEY_levelpic:
            UMI_replace_string_max8( ldv, &map_entry->levelpic);
            break;
        case UMI_KEY_exitpic:
            UMI_replace_string_max8( ldv, &map_entry->exitpic);
            break;
        case UMI_KEY_enterpic:
            UMI_replace_string_max8( ldv, &map_entry->enterpic);
            break;
        case UMI_KEY_endpic:
            UMI_replace_string_max8( ldv, &map_entry->endpic);
            break;
        case UMI_KEY_episode:
            // Associated with global episode list (not the map entry object)
            // Clear here, so do not have to leak info from some buried detection.
            if( ldv->type == UMI_TYPE_CLEAR )
            {
                // Destroy the episode menu
                UMI_destroy_episode_menu( umapinfo.emenu_list );
                umapinfo.emenu_list = NULL;
                // Store flag that default episode menu must be cleared
                umapinfo.emenu_clear = true;
            }
            else
            {
                UMI_append_episode_menu_entry( ld, &umapinfo.emenu_list );
            }
            break;
        case UMI_KEY_bossaction:
            // Clear here, so do not have to leak info from some buried detection.
            if( ldv->type == UMI_TYPE_CLEAR )
            {
                // Destroy the Boss Action list
                UMI_destroy_bossactions( map_entry->bossactions );
                map_entry->bossactions = NULL;
                map_entry->flags |= UMA_bossactions_clear;
            }
            else
            {
                UMI_append_bossaction( ld, &map_entry->bossactions );
                map_entry->flags &= ~UMA_bossactions_clear;
            }
            break;
        case UMI_KEY_endgame:
            map_entry->flags &= ~UMA_endgame_field;
            if( ldv->type == UMI_TYPE_NUMBER || ldv->type == UMI_TYPE_BOOL )
            {
                map_entry->flags |= (ldv->v.number)? UMA_endgame_enabled : UMA_endgame_disabled;
            }
            break;
        case UMI_KEY_partime:
            if( ldv->type == UMI_TYPE_NUMBER )
            {
                map_entry->partime = ldv->v.number;
            }
            break;
        case UMI_KEY_nointermission:
            store_map_entry_flag( ldv, map_entry, UMA_nointermission );
            break;
        case UMI_KEY_endbunny:
            store_map_entry_flag( ldv, map_entry, UMA_endbunny );
            break;
        case UMI_KEY_endcast:
            store_map_entry_flag( ldv, map_entry, UMA_endcast );
            break;
        case UMI_KEY_none:
            break; // no action, ignore
        default:
            umi_dl_error( UMI_ERROR_INTERNAL, "(SKD) Store, Unknown key ignored" );
            break;
    }
}



// -----------------------------------------------------------------------------
// API

// Import and merge UMAPINFO lump into current data
// If parts of the new data are already present, they overwrite the current data
//   lumpnum: lump that contains the umapinfo
void UMI_Load_umapinfo_lump( lumpnum_t lumpnum )
{
    int  lump_len = W_LumpLength(lumpnum);

    // Connect the Umapinfo lib to GenPrintf.
    umi_doom_print = &dl_umi_doom_print;
    umi_doom_store = &dl_umi_doom_store;

    if( lump_len <= 0 )
        return;  // Need to Exit via established paths.

    // ReadLump typically reads into a Z_Malloc allocated data buffer.
    byte * lump  = Z_Malloc(lump_len + 1, PU_IN_USE, 0);  // temp
        // If fails, it will not return.

    W_ReadLump(lumpnum, lump);

    // Parse the umapinfo lump.
    // This will call umi_doom_store for each keyword assignment.
    int retval = umi_parse_umapinfo_lump( lump, lump_len, verbose );
    if (retval <= UMI_ERROR_FATAL)
    {
        umi_dl_error( retval, "Parsing umapinfo lump failed");
    }
    else if (retval == UMI_ERROR_PARTIAL)
    {
        umi_dl_error( UMI_ERROR_PARTIAL, "umapinfo lump may be incomplete");
    }

    // The parse saves ptrs to strings within the lump.
    // The lump cannot be released until all strings have been copied.
    Z_Free(lump);  // have parse_data, done with the lump
}


void UMI_Destroy_umapinfo(void)
{
    UMI_destroy_episode_menu( umapinfo.emenu_list );
    umapinfo.emenu_list  = NULL;
    umapinfo.emenu_clear = false;

    UMI_destroy_maps( umapinfo.map_list );
    umapinfo.map_list = NULL;
}


// Extract episode and map numbers from map name.
// For Doom 2 map names, zero is returned for the episode.
// Returns true on success (numbers are valid)
//   name: name string
//   episode_num_p : output for episode num
//   map_num_p : output for map num
boolean UMI_Parse_mapname( const char* name, /*OUT*/ byte* out_episode_num, byte* out_map_num )
{
    boolean      result = false;
    int          retval = 0;
    unsigned int epi_num  = 0;
    unsigned int map_num  = 0;

    retval = sscanf(name, "%*[Mm]%*[Aa]%*[Pp]%u", /*OUT*/ &map_num);
    if ((retval == 1) && (map_num <= 255u))  // valid range for doom2
    {
        // This seems to allow map_num=0.  Is that intentional?
        result = true; 	// valid doom2 numbering
    }
    else
    {
        // episode, map numbering
        retval = sscanf(name, "%*[Ee]%u%*[Mm]%u", /*OUT*/ &epi_num, &map_num);
        if ((retval == 2) && (epi_num > 0) && (epi_num <= 255u) && (map_num > 0) && (map_num <= 255u))
            result = true;  // valid episode, map numbering
    }

    if (result)
    {
        if( out_episode_num )
            *out_episode_num = epi_num;
        if( out_map_num )
            *out_map_num = map_num;
    }

    return result;
}


// Search for UMAPINFO map entry that matches episode and map parameters
// NULL is returned if nothing was found
//   episode_num : episode num to be found
//   map_num : map num to be found
mapentry_t *  UMI_Lookup_umapinfo( byte episode_num, byte map_num )
{
    mapentry_t *  map_entry = umapinfo.map_list;

    // libdoom-umapinfo uses episode 0 for Doom 2
    if (gamemode == doom2_commercial)
        episode_num = 0;

    while (map_entry)
    {
        if ((map_entry->episode == episode_num) && (map_entry->map == map_num))
            goto done;
        map_entry = map_entry->next;
    }
    return NULL;  // not found

done:
    if (map_entry)
    {
        GenPrintf(EMSG_info, "UMAPINFO: Map entry found for Episode %u, Map %u\n", episode_num, map_num);
    }
    return map_entry;
}


// Load UMAPINFO data for current gameepisode/gamemap
// [WDJ] Stuck with still using this naming scheme, because of too many other "LevelXXX".
void UMI_Load_LevelInfo( void )
{
    game_umapinfo = UMI_Lookup_umapinfo( gameepisode, gamemap );

    // SMMU level info is replaced with UMAPINFO data, where possible
    if (game_umapinfo)
    {
        // Key author is handled by WI_Draw_LF() and WI_Draw_EL()
        // Also mapped to info_creator for COM_MapInfo_f()
        if (game_umapinfo->author)
            info_creator = game_umapinfo->author;

        // Keys label and levelname are handled by P_LevelName()

        // Keys intertext and intertextsecret are handled by F_StartFinale()

        // Key interbackdrop is handled by F_StartFinale()

        // Key intermusic is handled by F_StartFinale()

        // Keys nextmap and nextsecret are handled by G_DoUMapInfo()

        // Key music is mapped to info_music
        if (game_umapinfo->music)
            info_music = UMI_Get_MusicLumpName(game_umapinfo->music);

        // Key skytexture is mapped to info_skyname
        if (game_umapinfo->skytexture)
            info_skyname = game_umapinfo->skytexture;

        // Key levelpic is handled by WI_Draw_LF() and WI_Draw_EL()

        // Key exitpic is mapped to info_interpic
        if (game_umapinfo->exitpic)
            info_interpic = game_umapinfo->exitpic;

        // Key enterpic is handled by WI_Draw_ShowNextLoc()

        // Key endpic is handled by F_Drawer()

        // Key bossactions is handled by A_Bosstype_Death()

        // Key endgame is handled by G_NextLevel(), WI_Draw_ShowNextLoc(),
        // F_Ticker() and WI_Draw_ShowNextLoc()

        // Key partime is mapped to info_partime
        if (game_umapinfo->partime)
        {
            // A normal partime is in the range 30 to 240 seconds.	    
            // Must be able to multiply partime by 35 ticks per sec.
#if 1
            // [WDJ] DoomLegacy Time display is limited to 24 hours.
            if ( game_umapinfo->partime > (24*60*60) )
                info_partime = (24*60*60);
#else
            // DoomLegacy partime is an int, the value cannot exceed INT_MAX.
            // This is still 17044 hours which will break the DoomLegacy display.
            if ( game_umapinfo->partime > ((unsigned int) (INT_MAX/64)) )
                info_partime = (INT_MAX/64);
#endif
            else
                info_partime = game_umapinfo->partime;
            pars_valid_bex = true;  // Abuse this flag for activation with PWAD
        }

        // Key nointermission is handled by WI_Start()

        // Key endcast is handled by G_NextLevel, F_Ticker() and
        // WI_Draw_ShowNextLoc()

        // Key endbunny is handled by G_NextLevel, F_Ticker() and
        // WI_Draw_ShowNextLoc()
    }
}


// Returns a pointer into name, past the lead-in "d_" prefix,
// (because the function S_AddMusic() adds a "d_" prefix).
//  name: name of the music lump
const char*  UMI_Get_MusicLumpName( const char* name )
{
    if( ((name[0] == 'd') || (name[0] == 'D') || (name[0] == 'o')) && (name[1] == '_'))
        return &name[2];
    return name;
}


#endif

