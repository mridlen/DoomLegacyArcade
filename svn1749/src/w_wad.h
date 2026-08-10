// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: w_wad.h 1746 2025-04-10 09:37:38Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2016 by DooM Legacy Team.
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
// $Log: w_wad.h,v $
// Revision 1.13  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.12  2001/02/28 17:50:55  bpereira
// Revision 1.11  2001/02/24 13:35:21  bpereira
//
// Revision 1.10  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.9  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.8  2000/09/28 20:57:19  bpereira
// Revision 1.7  2000/08/31 14:30:56  bpereira
// Revision 1.6  2000/04/16 18:38:07  bpereira
// Revision 1.5  2000/04/13 23:47:48  stroggonmeth
// 
// Revision 1.4  2000/04/11 19:07:25  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.3  2000/04/04 00:32:48  stroggonmeth
// Initial Boom compatability plus few misc changes all around.
//
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      WAD I/O functions, wad resource definitions (some).
//
//-----------------------------------------------------------------------------


#ifndef W_WAD_H
#define W_WAD_H

#include "doomdef.h"
  // ZIPWAD
#include "doomtype.h"

#include "r_defs.h"
// patch_t

#ifdef HWRENDER
#include "hardware/hw_data.h"
#endif

#ifdef __GNUG__
#pragma interface
#endif


// ==============================================================
//               WAD FILE STRUCTURE DEFINITIONS
// ==============================================================

// Wad header format.
typedef struct
{
    char       identification[4];   // should be "IWAD" or "PWAD"
    uint32_t   numlumps;            // how many resources
    uint32_t   infotableofs;        // the 'directory' of resources
} wadinfo_t;

// Wad lump format.
typedef struct
{
    uint32_t   filepos;             // file offset of the resource
    uint32_t   size;                // size of the resource
    char       name[8];             // name of the resource
} filelump_t;


// in memory : initted at game startup

// [WDJ] Fast lump name compare, transform to numerical.
typedef union {
    char  s[9];  // extra byte for 0 term, needed for strupr
    uint64_t  namecode;  // name as numerical  (8 bytes)
} lump_name_t;

void  numerical_name( const char * name, lump_name_t * numname );



// [WDJ] Track the lump namespace (Wad markers)
// Lump namespace
typedef enum {
   LNS_any,    // find lump in any namespace
 // Boom namespaces
   LNS_global, // any lump not in a namespace section
   LNS_sprite, // S_START to S_END, or SS_START to SS_END
   LNS_flat,   // F_START to F_END, or FF_START to FF_END
   LNS_colormap,  // C_START to C_END
 // Other namespaces
   LNS_patch,  // PP_START to PP_END
   LNS_dehacked,
   LNS_legacy
} lump_namespace_e;


// Lump lookup info.
typedef struct
{
    char        name[8];            // filelump_t name[]
    uint32_t    position;           // filelump_t filepos
    uint32_t    size;               // filelump_t size
    lump_namespace_e  lump_namespace;
} lumpinfo_t;


// =========================================================================
//                         DYNAMIC WAD LOADING
// =========================================================================

// typedef int32_t   lumpnum_t;  // see doomdefs.h
// Format:
//  0x80000000 : NO_LUMP, tests as < 0
//  0x7FFF0000 : wad number field
//  0x0000FFFF : lump number field
typedef enum {
  NO_LUMP = (lumpnum_t) -1,
} lump_spec_e;

// [WDJ] Compatible with older signed tests for -1.
#define VALID_LUMP(lump)       (((int)lump)>=0)
//#define VALID_LUMP(lump)       (lump!=NO_LUMP)

// wad file number in upper word
#define WADFILENUM(lump)       ((lump)>>16)
// lump number for this pwad
#define LUMPNUM(lump)          ((lump)&0xffff)
// Combined wad and lump parameter.
#define WADLUMP(wad,lump)      (((wad)<<16)+(lump))
// This is tricky math that is dependent upon the lumpnum representation.
#define ADD_TO_LUMPNUM(lump,offset)    ((lump)+(offset))

// MAX_WADPATH moved to doomdef.h, for other users.

// Maximum of wad files used at the same time (there is a max of
// simultaneous open files anyway, and this should be plenty)
#define MAX_WADFILES  32

#define lumpcache_t  void*

typedef enum {
    FC_other,  // generic file
    FC_wad,
    FC_deh,
    FC_bex,
    FC_zip,
} file_classify_e;

typedef struct wadfile_s
{
    char *           filename;
    lumpinfo_t *     lumpinfo;   // array
    lumpcache_t *    lumpcache;
#ifdef HWRENDER
    MipPatch_t *     hwrcache;   // patches are cached in renderer's native format
#endif
#ifdef ZIPWAD
    // Immediately after the archive wadfile_t are the wadfile_t for files within the archive.
    byte             classify;   // file_classify_e
    byte             archive_parent;    // archive wadfile index,  0xFF= unused
    byte             archive_num_wadfile; // number of wadfile in this archive, 0= unused
#endif
    int              numlumps;   // this wad's number of resources
    int              handle;     // from system
    uint32_t         filesize;   // for network
    unsigned char    md5sum[16];
} wadfile_t;

extern  int          numwadfiles;
extern  wadfile_t*   wadfiles[MAX_WADFILES];

// Return the wadfile info for the lumpnum
wadfile_t * lumpnum_to_wad( lumpnum_t lumpnum );

// [WDJ] Indicates cache miss, new lump read requires endian fixing.
extern boolean lump_read;

// return file_classify_e
byte  W_filename_classify( const char * filename );



// =========================================================================
// Patch handling

// Patch Header
typedef struct
{
    uint16_t            width;          // bounding box size
    uint16_t            height;
    int16_t             leftoffset;     // pixels to the left of origin
    int16_t             topoffset;      // pixels below the origin
} pat_hdr_t;

// =========================================================================

void W_Shutdown(void);

// load and add a wadfile to the active wad files, return wad file number
// (you can get wadfile_t pointer: the return value indexes wadfiles[])
int     W_Load_WadFile (const char *filename);

//added 4-1-98 initmultiplefiles now return 1 if all ok 0 else
//             so that it stops with a message if a file was not found
//             but not if all is ok.
int     W_Init_MultipleFiles( char** filenames );

//  WADFILE_RELOAD
void    W_Reload (void);

//  Return lump id, or NO_LUMP if name not found.
lumpnum_t  W_Check_Namespace (const char* name, lump_namespace_e within_namespace);
//  Return lump id, or NO_LUMP if name not found.
lumpnum_t  W_CheckNumForName (const char* name);
// this one checks only in one pwad
lumpnum_t  W_CheckNumForNamePwad (const char* name, int wadid, int startlump);
lumpnum_t  W_GetNumForName (const char* name);

// modified version that scan forwards
// used to get original lump instead of patched using -file
lumpnum_t  W_CheckNumForNameFirst (const char* name);
lumpnum_t  W_GetNumForNameFirst (const char* name);  

uint32_t   W_LumpLength (lumpnum_t lump);
//added:06-02-98: read all or a part of a lump size==0 meen read all
int     W_ReadLumpHeader (lumpnum_t lump, void* dest, int size);
//added:06-02-98: now calls W_ReadLumpHeader() with full lump size
void    W_ReadLump (lumpnum_t lump, void *dest);

//  ztag : the Zone memory allocation tag (see memtag_e)
//  lump : lump number with embedded wad number

void*   W_CacheLumpNum ( lumpnum_t lumpnum, int ztag );
void*   W_CacheLumpName (const char* name, int ztag);

void*   W_CachePatchName (const char* name, int ztag);

void*   W_CachePatchNum ( lumpnum_t lump, int ztag);  // return a patch_t
void*   W_CachePatchNum_Endian ( lumpnum_t lump, int ztag );
#ifdef HWRENDER
// [WDJ] Called from hardware render for special mapped sprites
void*   W_CacheMappedPatchNum ( lumpnum_t lumpnum, uint32_t drawflags );
#endif


// Release patches made with W_CachePatchNum, W_CachePatchName.
void W_release_patch( patch_t * patch );

// These are used for loading, and releasing patches.
typedef struct {
   patch_t ** patch_owner;  // ptr to patch owner
   char     * name;
} load_patch_t;


//  pl : a patch list, maybe offset into a patch list
void load_patch_list( load_patch_t * pl );

//  pl : a patch list, maybe offset into a patch list
void release_patch_list( load_patch_t * pl );

//  pp : an array of patch_t ptr
//  count : number of patches to release
void release_patch_array( patch_t ** pp, int count );


// return a pic_t
void*   W_CacheRawAsPic( lumpnum_t lumpnum, int width, int height, int ztag);

// Cache and endian convert a pic_t
void*   W_CachePicNum( lumpnum_t lumpnum, int ztag );
void*   W_CachePicName( const char* name, int ztag );


// [WDJ] Return a sum unique to a lump, to detect replacements.
// The lumpptr must be to a Z_Malloc lump.
uint64_t  W_lump_checksum( void* lumpptr );

//SoM: 4/13/2000: Store lists of lumps for F_START/F_END etc.
typedef struct {
  int         wadfile;
  int         firstlump;
  int         numlumps;
} lumplist_t;
                    


#ifdef ZIPWAD

// zip wad
typedef enum {
    FH_none,
    FH_file = 0x70, // generic file
    FH_zip_file = 0x80,
    FH_zip_archive = 0xF0,
    FH_mask = 0xF0,
} file_handle_e;

// Dynamically loaded libzip
#if HAVE_LIBZIP == 3
# define OPT_LIBZIP
extern byte  libzip_present;
void  WZ_available( void );
#endif

extern byte  archive_open;
extern byte  archive_filenum;

byte  WZ_filename_cmp( const char * filename1, const char * filename2 );
byte  WZ_find_file_in_archive( const char * filename, const char * archive_name );
// Return filename with extension, otherwise NULL.
char *  WZ_make_name_with_extension( const char * filename, const char * extension, /*OUT*/ char * buf );
void    WZ_save_archive_name( const char * filename );
char *  WZ_make_archive_name( const char * filename );
unsigned int  WZ_filesize( const char * filename );
void  WZ_open_archive( const char * archive_name );
void  WZ_close_archive( void );
byte  WZ_open_file_z( const char * filename );
byte  WZ_open( const char * filename );
void  WZ_close( byte handle );
void  WZ_close_all( void );
int  WZ_seek( byte handle, uint32_t offset );
int  WZ_read( byte handle, uint32_t read_count, /*OUT*/ byte * dest );
int  WZ_read_archive_file( uint32_t offset, uint32_t read_size, /*OUT*/ byte * dest );
int  WZ_read_wadfile_from_archive_file_offset( byte fn, wadfile_t * wf, uint32_t offset, uint32_t read_size, /*OUT*/ byte * dest );
int  WZ_Load_zip_archive( const char * filename, int as_archive_filenum );
filestatus_e  WZ_md5_stream( const char * filename, byte * digest_block );
#endif


#endif // __W_WAD__
