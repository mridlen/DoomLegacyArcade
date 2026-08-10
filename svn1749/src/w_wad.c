// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: w_wad.c 1746 2025-04-10 09:37:38Z wesleyjohnson $
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
// $Log: w_wad.c,v $
// Revision 1.34  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.33  2002/07/29 21:52:25  hurdler
// Revision 1.32  2002/07/26 15:21:36  hurdler
// Revision 1.31  2002/01/14 18:54:45  hurdler
// Revision 1.30  2001/07/28 16:18:37  bpereira
// Revision 1.29  2001/05/27 13:42:48  bpereira
//
// Revision 1.28  2001/05/21 14:57:05  crashrl
// Readded directory crawling file search function
//
// Revision 1.27  2001/05/16 22:33:34  bock
// Initial FreeBSD support.
//
// Revision 1.26  2001/05/16 17:12:52  crashrl
// Added md5-sum support, removed recursiv wad search
//
// Revision 1.25  2001/04/17 22:26:07  calumr
// Initial Mac add
//
// Revision 1.24  2001/03/03 06:17:34  bpereira
// Revision 1.23  2001/02/28 17:50:55  bpereira
// Revision 1.22  2001/02/24 13:35:21  bpereira
//
// Revision 1.21  2001/01/25 22:15:44  bpereira
// added heretic support
//
// Revision 1.20  2000/10/04 16:19:24  hurdler
// Change all those "3dfx names" to more appropriate names
//
// Revision 1.19  2000/09/28 20:57:19  bpereira
// Revision 1.18  2000/08/31 14:30:56  bpereira
//
// Revision 1.17  2000/08/11 21:37:17  hurdler
// fix win32 compilation problem
//
// Revision 1.16  2000/08/11 19:10:13  metzgermeister
// Revision 1.15  2000/07/01 09:23:49  bpereira
//
// Revision 1.14  2000/05/09 20:57:58  hurdler
// use my own code for colormap (next time, join with Boris own code)
// (necessary due to a small bug in Boris' code (not found) which shows strange effects under linux)
//
// Revision 1.13  2000/04/30 10:30:10  bpereira
//
// Revision 1.12  2000/04/27 17:43:19  hurdler
// colormap code in hardware mode is now the default
//
// Revision 1.11  2000/04/16 18:38:07  bpereira
//
// Revision 1.10  2000/04/13 23:47:47  stroggonmeth
// See logs
//
// Revision 1.9  2000/04/11 19:07:25  stroggonmeth
// Finished my logs, fixed a crashing bug.
//
// Revision 1.8  2000/04/09 02:30:57  stroggonmeth
// Fixed missing sprite def
//
// Revision 1.7  2000/04/08 11:27:29  hurdler
// fix some boom stuffs
//
// Revision 1.6  2000/04/07 01:39:53  stroggonmeth
// Fixed crashing bug in Linux.
// Made W_ColormapNumForName search in the other direction to find newer colormaps.
//
// Revision 1.5  2000/04/06 20:40:22  hurdler
// Mostly remove warnings under windows
//
// Revision 1.4  2000/04/05 15:47:47  stroggonmeth
// Added hack for Dehacked lumps. Transparent sprites are now affected by colormaps.
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
//      Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

#include <fcntl.h>
  // open
#include <unistd.h>
  // close, read, lseek

#include "doomincl.h"
#include "w_wad.h"
#include "z_zone.h"

#include "v_video.h"
  // HWR_patchstore
#include "d_netfil.h"
#include "dehacked.h"
  // DEH_LoadDehackedLump
#include "r_defs.h"
#include "i_system.h"

#include "md5.h"
#include "m_swap.h"
#include "m_misc.h"
#include "infoext.h"
  // prefix_detect_t

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef ENABLE_UMAPINFO
// [MB] 2023-03-19: Support for UMAPINFO added
#include "doomstat.h"
#endif



//===========================================================================
//                                                                    GLOBALS
//===========================================================================
int          numwadfiles;             // number of active wadfiles
wadfile_t *  wadfiles[MAX_WADFILES];  // 0 to numwadfiles-1 are valid
  // wadfile_t are Z_MALLOC


// Return the wadfile info for the lumpnum
wadfile_t * lumpnum_to_wad( lumpnum_t lumpnum )
{
    // level_lumpnum contains index to the wad
    int wadnum = WADFILENUM( lumpnum );
    if( wadnum < numwadfiles )
      return  wadfiles[ wadnum ];
    return NULL;
}


// W_Shutdown
// Closes all of the WAD files before quitting
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected
void W_Shutdown(void)
{
#ifdef ZIP_WAD   
    WZ_close_all();
#endif
    while (numwadfiles--)
    {
        wadfile_t * wf = wadfiles[numwadfiles];
        if( wf->handle >= 0 )
            close(wf->handle);
    }
}

//===========================================================================
//                                                        WAD ROUTINES
//===========================================================================

// Lump Markers

typedef struct {
   char      markername[9];
   uint8_t   marker_namespace;  // start marker namespace
} marker_ident_t;

// At the end of each section, return to LNS_global namespace.
#define NUM_MARKER_IDENT   12
// Fill each string with 0, out to 8 characters.
marker_ident_t  marker_ident[ NUM_MARKER_IDENT ] =
{
   { "S_START\0", LNS_sprite }, // S_START to S_END
   { "S_END\0\0\0",   LNS_global },
   { "F_START\0", LNS_flat },   // F_START to F_END
   { "F_END\0\0\0",   LNS_global },
   { "C_START\0", LNS_colormap },  // C_START to C_END
   { "C_END\0\0\0",   LNS_global },
   { "SS_START",  LNS_sprite }, // SS_START to SS_END
   { "SS_END\0\0",    LNS_global },
   { "FF_START",  LNS_flat },   // FF_START to FF_END
   { "FF_END\0\0",    LNS_global },
   { "PP_START",  LNS_patch },  // PP_START to PP_END
   { "PP_END\0\0",    LNS_global },
};


// Make the name into numerical for easy compares.
void  numerical_name( const char * name, lump_name_t * numname )
{
    numname->namecode = 0;  // clear
    strncpy (numname->s, name, 8);
    // in case the name was a full 8 chars (has no 0 term)
    numname->s[8] = 0;
    // case insensitive compares
    strupr (numname->s);
    // numname.namecode can now be used for compares as uint64_t
}

// Return FC_ indicator
byte  W_filename_classify( const char * filename )
{
    const char * extp = strrchr( filename, '.' );
    if( extp )
    {
        // has a dot extension
        extp ++;
        if( strcasecmp( extp, "wad" ) == 0 )
            return FC_wad;
        if( strcasecmp( extp, "deh" ) == 0 )
            return FC_deh;
        if( strcasecmp( extp, "bex" ) == 0 )
            return FC_bex;
        if( strcasecmp( extp, "zip" ) == 0 )
            return FC_zip;
    }
    return FC_other;
}



// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//


#ifdef WADFILE_RELOAD
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

// [WDJ] FIXME: never set anymore
static lumpnum_t  reload_lumpnum = NO_LUMP;
static char*  reload_filename = NULL;
#endif

// [WDJ] Note: Avoid names that differ by only one letter.
static void    W_Load_dehacked_wad_lumps( int wadnum );
#ifdef ENABLE_UMAPINFO
static void    W_Load_umapinfo_wad_lumps( int wadnum );
#endif



//  Allocate a wadfile, setup the lumpinfo (directory) and
//  lumpcache, add the wadfile to the current active wadfiles
//
//  Now returns index into wadfiles[], you can get wadfile_t*
//  with:
//       wadfiles[<return value>]
//
//  return -1 in case of problem
//
// BP: Can now load dehacked files (ext .deh)
// Called by W_Init_MultipleFiles, P_AddWadFile.
int W_Load_WadFile ( const char * filename )
{
    // findfile requires a buffer of (at least) MAX_WADPATH
    char             filenamebuf[MAX_WADPATH];
    char *           msg;
    lumpinfo_t *     lumpinfo = NULL;
    lumpcache_t *    lumpcache;
    wadfile_t *      wadfile;
#ifdef HWRENDER    
    MipPatch_t *     grPatch;
#endif
    int              filenum;  // return value
    int              numlumps = 0;
    int              handle = -1;
    filestatus_e     fs;
    int              i, m;
    int              length;
    unsigned int     file_size;
    lump_namespace_e within_namespace = LNS_global;
   
    byte fc = W_filename_classify( filename );
    if( fc == FC_other )
        goto return_fail;

    //
    // check if limit of active wadfiles
    //
    filenum = numwadfiles;
    if( filenum >= MAX_WADFILES )
    {
        msg = "Maximum wad files reached";
        goto return_fail_msg;
    }

    dl_strncpy(filenamebuf, filename, MAX_WADPATH);

#ifdef ZIPWAD
    // When not libzip_present, then cannot have archive_open.
    if( archive_open )
    {
        handle = WZ_open_file_z( filename );
        if( handle == FH_none )
        {
            msg = "Cannot open";
            goto return_fail_msg;
        }

        file_size = WZ_filesize( filenamebuf );

        handle = -3;
    }
    else
#endif
    {
        struct stat      bufstat;

        // open wad file
        handle = open (filenamebuf, O_RDONLY|O_BINARY, 0666);
        if( handle == -1 )
        {
            // not in cur dir, must search
            nameonly(filenamebuf); // only search for the name

            // findfile returns dir+filename
            // Owner security permissions.
            fs = findfile(filenamebuf, NULL, false, /*OUT*/ filenamebuf);
            if( fs == FS_NOTFOUND )
            {
                msg = "Not found";
                goto return_fail_msg;
            }

            handle = open (filenamebuf, O_RDONLY|O_BINARY, 0666);
            if( handle == -1 )
            {
                msg = "Cannot open";
                goto return_fail_msg;
            }
        }
     
        // Get the filesize
        fstat(handle,&bufstat);
        file_size = bufstat.st_size;
    }

    if( fc == FC_zip )
    {
#ifdef ZIPWAD
# ifdef OPT_LIBZIP
        if( ! libzip_present )
        {
            msg = "Cannot read zip file, ziplib not present.";
                goto read_failure;
        }
# endif

        // Other files are keep open until shutdown,
        // but zip files are not accessed by handle.
        close( handle );
        handle = -2;
        numlumps = 0;
        lumpinfo = NULL;
#else
        msg = "Cannot read zip file, do not have ziplib option.";
            goto read_failure;
#endif
    }
    else

    // detect dehacked file with the "deh" extension, or bex files
    if( fc == FC_deh || fc == FC_bex )
    {
        // This code emulates a wadfile with one lump name "DEHACKED" 
        // at position 0 and size of the whole file.
        // This allow deh file to be copied by network and loaded at the
        // console, like wad files.
        numlumps = 1; 
        lumpinfo = Z_Malloc (sizeof(lumpinfo_t),PU_STATIC,NULL);
        lumpinfo->position = 0;
        lumpinfo->size = file_size;
        lumpinfo->lump_namespace = LNS_dehacked;
        memcpy(lumpinfo->name, "DEHACKED", 8);  // name array, no term
    }
    else if( fc == FC_wad )
    {
        // wad file
        wadinfo_t        header;
        lumpinfo_t*      lump_p;
        filelump_t*      fileinfo;
        filelump_t*      flp;
        int   rs;

        // read the header
#ifdef ZIPWAD       
        if( archive_open )
        {
            // read file in zip archive
            rs = WZ_read_archive_file( 0, sizeof(header), (byte*)&header );
            if( rs < 0 )
                goto read_failure;
        }
        else
#endif
        {
            rs = read (handle, &header, sizeof(header));
            if( rs < 0 )
                goto read_failure;
        }

        if (strncmp(header.identification,"IWAD",4))
        {
            // Homebrew levels?
            if (strncmp(header.identification,"PWAD",4))
            {
                msg = "Does not have IWAD or PWAD id";
                goto close_with_msg;
            }
            // ???modifiedgame = true;
        }
        header.numlumps = LE_SWAP32(header.numlumps);
        header.infotableofs = LE_SWAP32(header.infotableofs);

        // read wad file directory
        length = header.numlumps * sizeof(filelump_t);
        fileinfo = calloc (length, sizeof(*fileinfo));  // temp alloc, zeroed

        // Read fileinfo at offset specified in header.
#ifdef ZIPWAD       
        if( archive_open )
        {
            // read file (already open) in zip archive
            rs = WZ_read_archive_file( header.infotableofs, length, (byte*)fileinfo );
            if( rs < 0 )
                goto read_failure;
        }
        else
#endif
        {
            lseek (handle, header.infotableofs, SEEK_SET);
            rs = read (handle, fileinfo, length);
            if( rs < 0 )
                goto read_failure;
        }

        numlumps = header.numlumps;
        
        within_namespace = LNS_global;  // each wad starts in global namespace

        // fill in lumpinfo array for this wad
        flp = fileinfo;
        lump_p = lumpinfo = Z_Malloc (numlumps*sizeof(lumpinfo_t),PU_STATIC,NULL);
        for (i=0 ; i<numlumps ; i++, lump_p++, flp++)
        {
            // Make name compatible with compares using numerical_name.
            *((uint64_t*)&lump_p->name) = 0;  // clear
            strncpy (lump_p->name, flp->name, 8);  // pad 0
            // Check for namespace markers using clean lump name.
            for( m=0; m < NUM_MARKER_IDENT; m++ )
            {
               // Fast numerical name compare.
               if( *((uint64_t*)&lump_p->name) == *((uint64_t*)&marker_ident[m].markername) )
                  within_namespace = marker_ident[m].marker_namespace;
            }
            // No uppercase because no other port does it here, and it would
            // catch lowercase named lumps that otherwise would be ignored.
            lump_p->lump_namespace = within_namespace;
            lump_p->position = LE_SWAP32(flp->filepos);
            lump_p->size     = LE_SWAP32(flp->size);
        }
        free(fileinfo);
    }

    //
    //  link wad file to search files
    //
    wadfile = Z_Malloc (sizeof (wadfile_t),PU_STATIC,NULL);
    wadfile->filename = Z_StrDup(filenamebuf);
    wadfile->handle = handle;
    wadfile->numlumps = numlumps;
    wadfile->lumpinfo = lumpinfo;
    wadfile->filesize = file_size;

    //
    //  add the wadfile
    //
    wadfiles[filenum] = wadfile;
    numwadfiles++;

    //
    //  generate md5sum
    // 
    {
//#define DEBUG_MD5_TIME
#ifdef DEBUG_MD5_TIME
        // [WDJ] Do not know why timing the md5 calc was important, ever.
        int t;
        if( devparm >= 3 )
            t = I_GetTime();
#endif

#ifdef ZIPWAD
        if( archive_open )
        {
            // Need to get proper md5 because client may have same files,
            // but not in a zip archive.  They may have unzipped them.
            WZ_md5_stream( filenamebuf, wadfile->md5sum );
        }
        else
#endif
        {
            FILE * fhandle = fopen(filenamebuf, "rb");
            md5_stream (fhandle, wadfile->md5sum);
            fclose(fhandle);
        }

#ifdef DEBUG_MD5_TIME
        if( devparm >= 3 )
        {
            GenPrintf(EMSG_dev, "Load_WadFile %s: md5 calc took %f second\n",
                        filenamebuf, (float)(I_GetTime()-t)/TICRATE);
        }
#endif
    }
    
#ifdef ZIPWAD
    wadfile->classify = fc;
    wadfile->lumpcache = NULL;
# ifdef HWRENDER
    wadfile->hwrcache = NULL;
# endif
    // Immediately after the archive wadfile_t are the wadfile_t for files within the archive.
    wadfile->archive_num_wadfile = 0;  // number of wadfile in this archive
    wadfile->archive_parent = 0xFF;  // contained in archive wadfile index,  0xFF= normal file

    if( fc == FC_zip )
    {
        // New zip file.
        // This is indirect recursive call, that alters this wadfile (by filenum), so it must be setup first.
        int numfile = WZ_Load_zip_archive( filenamebuf, filenum );
        if( numfile <= 0 )
            goto return_fail;  // no files
       
        return filenum;
    }
    else if( archive_open )
    {
        // File is in zip archive, link to the parent zip archive.
        if( archive_filenum < filenum )  // file in zip archive
        {
            wadfile->archive_parent = archive_filenum;  // contained in archive wadfile index,  0xFF= normal file
            wadfiles[archive_filenum]->archive_num_wadfile++;
        }
    }
#endif

    // Load the file data
   
    //
    //  set up caching
    //
    length = numlumps * sizeof(lumpcache_t);
    lumpcache = Z_Malloc (length,PU_STATIC,NULL);

    memset (lumpcache, 0, length);
    wadfile->lumpcache = lumpcache;

#ifdef HWRENDER
    //faB: now allocates MipPatch info structures STATIC from the start,
    //     because these were causing a lot of fragmentation of the heap,
    //     considering they are never freed.
    length = numlumps * sizeof(MipPatch_t);
    grPatch = Z_Malloc (length, PU_HWRPATCHINFO, NULL);    //never freed
    // set mipmap.downloaded to false
    memset (grPatch, 0, length);
    for (i=0; i<numlumps; i++)
    {
        // store the software patch lump number for each MipPatch
        grPatch[i].patch_lumpnum = WADLUMP(filenum,i);  // form file/lump
    }
    wadfile->hwrcache = grPatch;
#endif

    GenPrintf(EMSG_info, "Added file %s (%i lumps)\n", filenamebuf, numlumps);
    W_Load_dehacked_wad_lumps( filenum );

#ifdef ENABLE_UMAPINFO   
    // [MB] 2023-01-10: Process UMAPINFO last for highest priority
    if ( EN_doom_etc )
    {
        // UMAPINFO is for Doom only (not suitable for Heretic and Hexen)
// [WDJ] Heretic people will want it.
        W_Load_umapinfo_wad_lumps( filenum );
    }
#endif
    return filenum;

read_failure:
    msg = "read failure";
   
close_with_msg:
    if( handle >= 0 )
        close( handle );
#ifdef ZIPWAD
    if( archive_open )
        WZ_close( FH_zip_file );
#endif

return_fail_msg:
    GenPrintf(EMSG_warn, "File %s: %s\n", filenamebuf, msg );
return_fail:
    return -1;  // fail
}



#ifdef WADFILE_RELOAD
// !!!NOT CHECKED WITH NEW WAD SYSTEM
//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the lump directory.
//
// Called from: P_SetupLevel
void W_Reload (void)
{
    wadinfo_t           header;
    int                 lumpcount;
    lumpinfo_t*         lump_p;
    int                 i;
    int                 handle;
    int                 length;
    int                 filenum, lumpnum;
    filelump_t*         fileinfo;
    filelump_t*         flp;
    lumpcache_t*        lumpcache;

    if (!reload_filename)
        return;

    handle = open (reload_filename,O_RDONLY | O_BINARY);
    if( handle == -1)
        I_Error ("W_Reload: couldn't open %s",reload_filename);

    read (handle, &header, sizeof(header));
    lumpcount = LE_SWAP32(header.numlumps);
    header.infotableofs = LE_SWAP32(header.infotableofs);
    length = lumpcount*sizeof(filelump_t);
    fileinfo = calloc (length, sizeof(*fileinfo));  // temp alloc, zeroed
    lseek (handle, header.infotableofs, SEEK_SET);
    read (handle, fileinfo, length);

    // Fill in lumpinfo
    filenum = WADFILENUM(reload_lumpnum);
    lumpnum = LUMPNUM(reload_lumpnum);
    lump_p = & wadfiles[filenum]->lumpinfo[ lumpnum ];
    lumpcache = wadfiles[filenum]->lumpcache;

    flp = fileinfo;
    // Only index lumpcache by lumpnum.
    for (i=lumpnum ; i<lumpnum+lumpcount ; i++)
    {
        if (lumpcache[i])
            Z_Free (lumpcache[i]);

        lump_p->position = LE_SWAP32(flp->filepos);
        lump_p->size = LE_SWAP32(flp->size);
        lump_p++;
        flp++;
    }

    close (handle);
    free(fileinfo);
}
#endif


//
// W_Init_MultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file must be found.
// Any load failure is indicated to the caller, for error handling.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
// Return 0 when any file load does not succeed
// Called by: D_DoomMain
int W_Init_MultipleFiles ( char ** filenames )
{
    int  rc = 1;

    // open all the files, load headers, and count lumps
    numwadfiles = 0;

    // will be realloced as lumps are added
    for ( ; *filenames ; filenames++)
    {
        if( W_Load_WadFile (*filenames) == -1 )
        {
            rc = 0;
            GenPrintf( EMSG_warn, "File not found: %s\n", *filenames );
        }
    }

    if (!numwadfiles)
    {
        I_SoftError ("W_Init_MultipleFiles: no files found\n");
        fatal_error = 1;
    }

    return rc;
}


//===========================================================================
//                                                        LUMP BASED ROUTINES
//===========================================================================

//
//  W_CheckNumForName
//  Lists are currently used by flats and colormaps.
//  These are faster as they skip checking some names.
//  Patches within textures use namespace to protect against trying to use
//  another lump of the same name (colormap) as a patch.

//  Return lump id, or NO_LUMP if name not found.
lumpnum_t  W_Check_Namespace (const char* name, lump_namespace_e within_namespace)
{
    lumpnum_t  alternate = NO_LUMP;
    int  i,j;
    lump_name_t name8;
    lumpinfo_t* lump_p;

    numerical_name( name, & name8 );

    // Scan wad files backwards so PWAD files take precedence.
    for (i = numwadfiles-1 ; i>=0; i--)
    {
        // Scan forward within a wad file.
        lump_p = wadfiles[i]->lumpinfo;
        if( ! lump_p )  continue;

        for (j = 0; j<wadfiles[i]->numlumps; j++,lump_p++)
        { 
            // Fast numerical name compare.
            if ( *(uint64_t *)lump_p->name == name8.namecode )
            {
                // Name matches.
                // Check if lump is from the wanted namespace.
                if( within_namespace == LNS_any
                    || lump_p->lump_namespace == within_namespace )
                {
                    // Return wad/lump identifier
                    return  WADLUMP(i,j);
                }
                // Wrong namespace.
                if( alternate == NO_LUMP )  // remember first lump found
                   alternate = WADLUMP(i,j);  // wad/lump identifier
            }
        }
    }
    // Not found.
    // Return first matching lump found in any namespace.
    if( alternate != NO_LUMP )
      return alternate;

    return NO_LUMP;
}

//  Return lump id, or NO_LUMP if name not found.
lumpnum_t  W_CheckNumForName (const char* name)
{
    return W_Check_Namespace( name, LNS_any );
}


//
//  Same as the original, but checks in one pwad only
//  (Used for sprites loading)
//    wadid :  the wad number
//    startlump : the lump number (without wad id) to start the search
//  Return lump number, or NO_LUMP.
lumpnum_t  W_CheckNumForNamePwad (const char* name, int wadid, int startlump)
{
    int  i;
    lump_name_t name8;
    lumpinfo_t* lump_p;

    // make the name into numerical for easy compares
    numerical_name( name, & name8 );

    //
    // scan forward
    // start at 'startlump', useful parameter when there are multiple
    //                       resources with the same name
    //
    if (startlump < wadfiles[wadid]->numlumps)
    {
        lump_p = wadfiles[wadid]->lumpinfo + startlump;
        if( ! lump_p )
            return NO_LUMP;

        for (i = startlump; i<wadfiles[wadid]->numlumps; i++,lump_p++)
        {
            // Fast numerical name compare.
            if ( *(uint64_t *)lump_p->name == name8.namecode )
            {
                return WADLUMP(wadid,i);
            }
        }
    }

    // not found.
    return NO_LUMP;
}



//
// W_GetNumForName
//   Calls W_CheckNumForName, but bombs out if not found.
//
lumpnum_t  W_GetNumForName (const char* name)
{
    lumpnum_t ln;

    ln = W_CheckNumForName (name);

    if( ! VALID_LUMP(ln) )
    {
#ifdef DEBUG_CHEXQUEST
        I_SoftError ("W_GetNumForName: %s not found!\n", name);	// [WDJ] 4/28/2009 Chexquest
#else
        I_Error ("W_GetNumForName: %s not found!\n", name);
#endif
    }

    return ln;
}

// Scan wads files forward, IWAD precedence.
lumpnum_t  W_CheckNumForNameFirst (const char* name)
{
    int i, j;
    lump_name_t name8;
    lumpinfo_t* lump_p;

    numerical_name( name, & name8 );

    // scan wad files forward, when original wad resources
    //  must take precedence
    for (i = 0; i<numwadfiles; i++)
    {
        lump_p = wadfiles[i]->lumpinfo;
        if( ! lump_p )  continue;

        for (j = 0; j<wadfiles[i]->numlumps; j++,lump_p++)
        {
            if ( *(uint64_t *)lump_p->name == name8.namecode )
            {
                return WADLUMP(i,j);
            }
        }
    }

    return NO_LUMP;
}

//
//  W_GetNumForNameFirst : like W_GetNumForName, but scans FORWARD
//                         so it gets resources from the original wad first
//  (this is used only to get S_START for now, in r_data.c)
lumpnum_t  W_GetNumForNameFirst (const char* name)
{
    lumpnum_t  ln;

    ln = W_CheckNumForNameFirst (name);
    if( ! VALID_LUMP(ln) )
        I_Error ("W_GetNumForNameFirst: %s not found!", name);

    return ln;
}

// W_name_prefix_detect
void report_detect( const char * who, uint16_t detect );  // BDTC_game_detection_e

// Scan all lump names of the wad.
// This detects lumps by name per the prefix_table, and reports in deh_detect.
// Return flags of which lump name prefixes were detected.
static
void  W_name_prefix_detect( int wadnum )
{
    uint16_t detect = 0;
    wadfile_t * wadf = wadfiles[wadnum];
    if( ! wadf )
        return;

    // Scan forward within a wad file.
    lumpinfo_t * lumpinfo = wadf->lumpinfo;
    if( ! lumpinfo )
        return;

    int j;
    for (j = 0; j<wadf->numlumps; j++)
    { 
        const char * lin = lumpinfo[j].name;
        int k;
        for( k=0; k<num_prefix_detect_table; k++ )
        {
            const prefix_detect_t * ptp = & prefix_detect_table[k];
            const char * prfx = ptp->prefix;
            if( prfx == NULL )
                break;

            // [WDJ] The strcasecmp cannot be trusted when the strings do not match.
            // This is because it is sorting lexigraphically, not detecting differences.
            int len = strlen( prfx );
            if( strncasecmp( lin, prfx, len ) == 0 )
            {
                const char c = lin[ strlen(prfx) ];
                switch( ptp->condition )
                {
                 case COND_numeric:
                    if( ! isdigit(c) )  continue;
                    break;
                 case COND_alpha:
                    if( ! isalpha(c) )  continue;
                    break;
                 case COND_none:
                 default:
                    break;
                }
                // Name prefix matches.
                detect |= ptp->detflag;
                if( verbose > 1 )
                {
                    char namestr[10]; // names are 6 char
                    char buf[40];
                    dl_strncpy(namestr, lin, 7);
                    snprintf( buf, 38, "Lump detect %s:", namestr );
                    buf[39] = 0;
                    report_detect( buf, ptp->detflag );
                }
            }
        }
    }

    deh_detect |= detect;
    if( detect )
        report_detect( "Lump detect", detect );
}


//
//  W_LumpLength
//   Returns the buffer size needed to load the given lump.
//
uint32_t  W_LumpLength (lumpnum_t lump)
{
#ifdef PARANOIA
    if( ! VALID_LUMP(lump) )
        I_Error("W_LumpLength: lump not exist\n");

    if (LUMPNUM(lump) >= wadfiles[WADFILENUM(lump)]->numlumps)
        I_Error("W_LumpLength: index %i >= numlumps", lump);
#endif
    lumpinfo_t * lump_p = wadfiles[WADFILENUM(lump)]->lumpinfo;
    if( lump_p == NULL )
        return 0;

    return lump_p[ LUMPNUM(lump) ].size;
}



//
// W_ReadLumpHeader : read 'size' bytes of lump
//                    sometimes just the header is needed
//
//Fab:02-08-98: now returns the number of bytes read (should == size)
int  W_ReadLumpHeader ( lumpnum_t     lump,
                        void*         dest,
                        int           size )
{
    lumpinfo_t* lif;
    wadfile_t * wf;
    int  ln1, wn1;
    int  handle;
    int  bytesread = 0;

    ln1 = LUMPNUM(lump);
    wn1 = WADFILENUM(lump);
    wf = wadfiles[ wn1 ];
   
#ifdef PARANOIA
//    if (lump<0) I_Error("W_ReadLumpHeader: lump not exist\n");
    if ( ! VALID_LUMP(lump) )
        I_Error("W_ReadLumpHeader: Invalid lump id\n");

    if( ln1 >= wf->numlumps )
        I_Error ("W_ReadLumpHeader: %i >= numlumps",lump);
#endif

    if( wf->lumpinfo == NULL )
        return 0;

    lif = & wf->lumpinfo[ ln1 ];

    // empty resource (usually markers like S_START, F_END ..)
    if (lif->size==0)
        return 0;

#ifdef LOADING_DISK_ICON
    // the good ole 'loading' disc icon TODO: restore it :)
    I_BeginRead ();
    // After this point must go thorugh I_EndRead() before can return.
#endif

    handle = wf->handle; //lif->handle;

#ifdef WADFILE_RELOAD
    if (lif->handle == -1)
    {
        // reloadable file, so use open / read / close
        handle = open (reload_filename,O_RDONLY|O_BINARY,0666);
        if( handle == -1 )
        {
            I_SoftError ("W_ReadLumpHeader: couldn't open %s\n", reload_filename);
            goto done;
        }
    }
#endif

    // 0 size means read all the lump
    if (!size || size>lif->size)
        size = lif->size;

    // Read lump at offset.
#ifdef ZIPWAD
    if( wf->archive_num_wadfile )
    {
        // An archive, cannot read lump.
        return 0;
    }
    else if( wf->archive_parent < 0xFF )  // valid parent archive
    {
        // From file in zip archive.
        bytesread = WZ_read_wadfile_from_archive_file_offset( wn1, wf, lif->position, size, /*OUT*/ dest );
    }
    else
#endif
    {
        lseek (handle, lif->position, SEEK_SET);
        bytesread = read (handle, dest, size);
    }

#ifdef WADFILE_RELOAD
    if (lif->handle == -1)
        close (handle);
#endif

#ifdef WADFILE_RELOAD
done:
#endif
#ifdef LOADING_DISK_ICON
    I_EndRead ();
#endif
    return bytesread;
}


//
//  W_ReadLump
//  Loads the lump into the given buffer,
//   which must be >= W_LumpLength().
//
//added:06-02-98: now calls W_ReadLumpHeader() with full lump size.
//                0 size means the size of the lump, see W_ReadLumpHeader
void W_ReadLump ( lumpnum_t     lump,
                  void*         dest )
{
    W_ReadLumpHeader (lump, dest, 0);
}


// ==========================================================================
// W_CacheLumpNum
// ==========================================================================
// [WDJ] Indicates cache miss, new lump read requires endian fixing.
boolean lump_read;	// set by W_CacheLumpNum

void* W_CacheLumpNum ( lumpnum_t lumpnum, int ztag )
{
    lumpcache_t*  lumpcache;

    //SoM: 4/8/2000: Don't keep doing operations to the lump variable!
    unsigned int  llump = LUMPNUM(lumpnum);
    unsigned int  lfile = WADFILENUM(lumpnum);

#ifdef DEBUG_CHEXQUEST
   // [WDJ] Crashes in chexquest with black screen, cannot debug
   if( ! VALID_LUMP(lumpnum) ) {
      lump_read = 0;  // no data
       // [WDJ] prevent SIGSEGV in chexquest
      I_SoftError ("W_CacheLumpNum: not VALID_LUMP passed!\n");
      return NULL;
    }
#endif   
#ifdef PARANOIA
    // check return value of a previous W_CheckNumForName()
    //SoM: 4/8/2000: Do better checking. No more SIGSEGV's!
    if( ! VALID_LUMP(lumpnum) )  // [WDJ] must be first to protect use as index
      I_Error ("W_CacheLumpNum: not VALID_LUMP passed!\n");
    if (lfile >= numwadfiles)
      I_Error("W_CacheLumpNum: %i >= numwadfiles(%i)\n", lfile, numwadfiles);
    if (llump >= wadfiles[lfile]->numlumps)
      I_Error ("W_CacheLumpNum: %i >= numlumps", llump);
#endif

    lumpcache = wadfiles[lfile]->lumpcache;
    if (!lumpcache[llump])
    {
        // read the lump in

        //debug_Printf ("cache miss on lump %i\n",lump);
        uint32_t lumplen = W_LumpLength(lumpnum);
        if( lumplen )
        {
            byte* ptr = Z_Malloc ( lumplen, ztag, &lumpcache[llump]);
            W_ReadLumpHeader( lumpnum, ptr, 0 );   // read whole lump
//        W_ReadLumpHeader (lump, lumpcache[llump], 0);   // read whole lump
            lump_read = 1; // cache miss, read lump, caller must apply endian fix
        }
        else
            return NULL;  // not found
    }
    else
    {
        //debug_Printf ("cache hit on lump %i\n",lump);
        // [WDJ] Do not degrade lump to PU_CACHE while it is in use.
        if( ztag == PU_CACHE )   ztag = PU_CACHE_DEFAULT;
        Z_ChangeTag (lumpcache[llump], ztag);
        lump_read = 0;  // cache hit, cache already has endian fixes
    }

    return lumpcache[llump];
}


// ==========================================================================
// W_CacheLumpName
// ==========================================================================
void* W_CacheLumpName ( const char* name, int ztag )
{
    return W_CacheLumpNum (W_GetNumForName(name), ztag);
}



// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current rendermode. For software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, the
// heap memory cache then act as a 'level 2' cache just after the graphics
// card memory.


//  pl : a patch list, maybe offset into a patch list
void load_patch_list( load_patch_t * pl )
{
    while( pl->patch_owner ) {
        // software_render will store a patch allocation.
        // hardware render stores a cache ptr.
        *(pl->patch_owner) = W_CachePatchName(pl->name, PU_LOCK_SB);
        pl++;
    }
}

//  pl : a patch list, maybe offset into a patch list
void release_patch_list( load_patch_t * pl )
{
    while( pl->patch_owner ) {
        W_release_patch( *(pl->patch_owner) );
        pl++;
    }
}

//  pp : an array of patch_t ptr
//  count : number of patches to release
void release_patch_array( patch_t ** pp, int count )
{
    while( count-- ) {
        // Hardware render and software render releases are different.
        if( *pp )
            W_release_patch( *pp );
        pp++;
    }
}


//
// Cache a patch into heap memory, convert the patch format as necessary
//

// [WDJ] When there is another lump with the same name as a patch, this will
// sometimes get that lump and convert the header as if it was a patch.
// This should only get lumps that are patches.

// Cache the patch with endian conversion
// [WDJ] Only read patches using this function, hardware render too.
// [WDJ] Removed inline because is also called from in hardware/hw_cache.c.
void* W_CachePatchNum_Endian ( lumpnum_t lump, int ztag )
{
// __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#ifdef __BIG_ENDIAN__
    patch_t * patch = W_CacheLumpNum(lump, ztag);
    // [WDJ] If newly read patch then fix endian.
    if( lump_read )
    {
        patch->height = (uint16_t)( LE_SWAP16(patch->height) );
        patch->width = (uint16_t)( LE_SWAP16(patch->width) );
        patch->topoffset = LE_SWAP16(patch->topoffset);
        patch->leftoffset = LE_SWAP16(patch->leftoffset);
        {
            // [WDJ] columnofs[ 0 .. width-1 ]
            // The patch structure only shows 8, but there can be many more.
            int i = patch->width - 1;
            for( ; i>=0; i-- )
               patch->columnofs[i] = LE_SWAP32( patch->columnofs[i] );
        }
    }
    return patch;
#else
    // [WDJ] Optimized version for little-endian, much faster
    return W_CacheLumpNum(lump, ztag);
#endif
}



// Called from many draw functions
void* W_CachePatchNum ( lumpnum_t lumpnum, int ztag )
{
    MipPatch_t*   grPatch;

#ifdef HWRENDER
    if( ! HWR_patchstore ) {
        return W_CachePatchNum_Endian ( lumpnum, ztag );
    }
   
    // hardware render
    // Patches are stored in HWR patch format.

#ifdef PARANOIA
    // check the return value of a previous W_CheckNumForName()
    if ( ( ! VALID_LUMP(lumpnum) )
         || (LUMPNUM(lumpnum) >= wadfiles[WADFILENUM(lumpnum)]->numlumps) )
        I_Error ("W_CachePatchNum: %i >= numlumps", LUMPNUM(lumpnum));
#endif

    grPatch = &(wadfiles[WADFILENUM(lumpnum)]->hwrcache[LUMPNUM(lumpnum)]);

    if( ! grPatch->mipmap.GR_data ) 
    {   // first time init grPatch fields
        // we need patch w,h,offset,...
        patch_t* tmp_patch = W_CachePatchNum_Endian(grPatch->patch_lumpnum, PU_LUMP); // temp use
        // default no TF_Opaquetrans
        HWR_MakePatch ( tmp_patch, grPatch, &grPatch->mipmap, 0);
        Z_Free (tmp_patch);
        // HWR_MakePatch makes GR_data as PU_HWRCACHE
    }

    // return MipPatch_t, which can be casted to (patch_t) with valid patch header info
    return (void*)grPatch;
#else
    // Software renderer only, simplified
    return W_CachePatchNum_Endian( lump, ztag );
#endif
}

#ifdef HWRENDER
// [WDJ] Called from hardware render for special mapped sprites
void* W_CacheMappedPatchNum ( lumpnum_t lumpnum, uint32_t drawflags )
{
    MipPatch_t*   grPatch;

#ifdef PARANOIA
    // check the return value of a previous W_CheckNumForName()
    if ( ( ! VALID_LUMP(lumpnum) )
         || (LUMPNUM(lumpnum) >= wadfiles[WADFILENUM(lumpnum)]->numlumps) )
        I_Error ("W_CacheMappedPatchNum: %i >= numlumps", LUMPNUM(lumpnum));
#endif

    grPatch = &(wadfiles[WADFILENUM(lumpnum)]->hwrcache[LUMPNUM(lumpnum)]);

    if( ! grPatch->mipmap.GR_data )
    {   // first time init grPatch fields
        // we need patch w,h,offset,...
        patch_t *tmp_patch = W_CachePatchNum_Endian(grPatch->patch_lumpnum, PU_LUMP); // temp use
        // pass TF_Opaquetrans
        HWR_MakePatch ( tmp_patch, grPatch, &grPatch->mipmap, drawflags);
        Z_Free (tmp_patch);
        // HWR_MakePatch makes GR_data as PU_HWRCACHE
    }

    // return MipPatch_t, which can be casted to (patch_t) with valid patch header info
    return (void*)grPatch;
}
#endif

// Release patches made with W_CachePatchNum, W_CachePatchName.
void W_release_patch( patch_t * patch )
{
#ifdef HWRENDER
    if( HWR_patchstore )
    {
        // Hardware render: the patches are fake, and are allocated in a large array
        MipPatch_t*  grPatch = (MipPatch_t*) patch; // sneaky HWR casting
        HWR_release_Patch( grPatch, &grPatch->mipmap );
        return;       
    }
#endif
   
    // Software render: the patches were allocated with Z_Malloc       
#ifdef PARANOIA
    if( ! verify_Z_Malloc(patch))
    {
        GenPrintf( EMSG_error, "Error W_release_patch: Not a memory block %x\n", *patch);
        return;
    }
#endif
    Z_ChangeTag( patch, PU_UNLOCK_CACHE );
}


// Find patch in LNS_patch namespace
void* W_CachePatchName ( const char* name, int ztag )
{
    int lumpid;
    // substitute known name for name not found
    lumpid = W_Check_Namespace( name, LNS_patch );
    if( ! VALID_LUMP(lumpid) )
    {
        name = "BRDR_MM";
        lumpid = W_Check_Namespace( name, LNS_patch );
        if( ! VALID_LUMP(lumpid) )
            I_Error ("W_CachePatchName: %s not found!\n", name);
    }
    return W_CachePatchNum( lumpid, ztag);
}


// convert raw heretic picture to legacy pic_t format
// Used for heretic: TITLE, HELP1, HELP2, ORDER, CREDIT, FINAL1, FINAL2, E2END
// Used for raven demo screen
void* W_CacheRawAsPic( lumpnum_t lumpnum, int width, int height, int ztag)
{
    // [WDJ] copy of CacheLumpNum with larger lump allocation,
    // read into pic, and no endian fixes
    lumpcache_t*  lumpcache;
    //SoM: 4/8/2000: Don't keep doing operations to the lump variable!
    unsigned int  llump = LUMPNUM(lumpnum);
    unsigned int  lfile = WADFILENUM(lumpnum);

#ifdef PARANOIA
    // check return value of a previous W_CheckNumForName()
    //SoM: 4/8/2000: Do better checking. No more SIGSEGV's!
    if( ! VALID_LUMP(lumpnum) )
      I_Error ("W_CacheRawAsPic: not VALID_LUMP passed!\n");
    if (lfile >= numwadfiles)
      I_Error("W_CacheRawAsPic: %i >= numwadfiles(%i)\n", lfile, numwadfiles);
    if (llump >= wadfiles[lfile]->numlumps)
      I_Error ("W_CacheRawAsPic: %i >= numlumps", llump);
#endif

    lumpcache = wadfiles[lfile]->lumpcache;
    if (!lumpcache[llump]) 	// cache miss
    {
        // read the lump in

        // Allocation is larger than what W_CacheLumpNum does
        pic_t* pic = Z_Malloc (W_LumpLength(lumpnum)+sizeof(pic_t),
                               ztag, &lumpcache[llump]);
        // read lump + pic into pic->data (instead of lumpcache)
        W_ReadLumpHeader( lumpnum, pic->data, 0 );
        // set pic info from caller parameters, (which are literals)
        pic->width = width;
        pic->height = height;
        pic->mode = PALETTE;
    }
    else
    {
        // [WDJ] Do not degrade lump to PU_CACHE while it is in use.
        if( ztag == PU_CACHE )
           ztag = PU_CACHE_DEFAULT;
        Z_ChangeTag (lumpcache[llump], ztag);
    }

    return lumpcache[llump];
}


// Cache and endian convert a pic_t
void* W_CachePicNum( lumpnum_t lumpnum, int ztag )
{
// __BIG_ENDIAN__ is defined on MAC compilers, not on WIN, nor LINUX
#ifdef __BIG_ENDIAN__
    pic_t * pt = W_CacheLumpNum ( lumpnum, ztag );
    // [WDJ] If newly read pic then fix endian.
    if( lump_read )
    {
        pt->height = (uint16_t)( LE_SWAP16(pt->height) );
        pt->width = (uint16_t)( LE_SWAP16(pt->width) );
//        pt->reserved = LE_SWAP16(pt->reserved);
    }
    return pt;
#else
    // [WDJ] Optimized version for little-endian, much faster
    return W_CacheLumpNum(lumpnum, ztag);
#endif
}

// Cache and endian convert a pic_t
void* W_CachePicName( const char* name, int tag )
{
    return W_CachePicNum( W_GetNumForName(name), tag);
}


// Search for all DEHACKED lump in all wads and load it.
static
void W_Load_dehacked_wad_lumps( int wadnum )
{
    lumpnum_t  clump = 0;
    
    // Look for lump names that would identify dehacked detect.
    W_name_prefix_detect( wadnum );
    
    // Initialize what is reported in Finalize
    DEH_Init_Load_Dehacked();

    while (1)
    { 
        clump = W_CheckNumForNamePwad("DEHACKED", wadnum, LUMPNUM(clump));
        if( ! VALID_LUMP(clump) )
            break;
        GenPrintf(EMSG_info, "Loading dehacked from %s\n",wadfiles[wadnum]->filename);
        DEH_LoadDehackedLump(clump);
        clump++;
    }

    DEH_Finalize();
}

#ifdef ENABLE_UMAPINFO
// [MB] 2023-01-10: Search for all UMAPINFO lumps in all wads and load them
static
void W_Load_umapinfo_wad_lumps( int wadnum )
{
    lumpnum_t  clump = 0;

    while (1)
    {
        clump = W_CheckNumForNamePwad("UMAPINFO", wadnum, LUMPNUM(clump));
        if( ! VALID_LUMP(clump) )
            break;
        GenPrintf(EMSG_info, "Loading UMAPINFO from %s\n",
                  wadfiles[wadnum]->filename);
        UMI_Load_umapinfo_lump(clump);
        clump++;
    }
}
#endif


// [WDJ] Return a sum unique to a lump, to detect replacements.
// The lumpptr must be to a Z_Malloc lump.
uint64_t  W_lump_checksum( void* lumpptr )
{
    // Work only with the lumpptr given, cannot trust that can get stats
    // on the exact same lump, it may be in multiple wads and pwads.
    // Very simple checksum over the size of the Z_Malloc block.
    int lumpsize = Z_Datasize( lumpptr );
    uint64_t  checksum = 0;
    int i;
    for( i=0; i<lumpsize; i++ )
      checksum += ((byte*)lumpptr)[i];
    return checksum;
}


#if 0
// Makes DOS assumptions
// --------------------------------------------------------------------------
// W_Profile
// --------------------------------------------------------------------------
//
/*     --------------------- UNUSED ------------------------
int             info[2500][10];
int             profilecount;

void W_Profile (void)
{
    int         i;
    memblock_t* block;
    void*       ptr;
    char        ch;
    FILE*       f;
    int         j;
    char        name[9];


    for (i=0 ; i<numlumps ; i++)
    {
        ptr = lumpcache[i];
        if (!ptr)
        {
            ch = ' ';
            continue;
        }
        else
        {
            block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
            if (block->memtag < PU_PURGELEVEL)
                ch = 'S';
            else
                ch = 'P';
        }
        info[i][profilecount] = ch;
    }
    profilecount++;

    f = fopen ("waddump.txt","w");
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
        memcpy (name,lumpinfo[i].name,8);

        for (j=0 ; j<8 ; j++)
            if (!name[j])
                break;

        for ( ; j<8 ; j++)
            name[j] = ' ';

        fprintf (f,"%s ",name);

        for (j=0 ; j<profilecount ; j++)
            fprintf (f,"    %c",info[i][j]);

        fprintf (f,"\n");
    }
    fclose (f);
}

// --------------------------------------------------------------------------
// W_AddFile : the old code kept for reference
// --------------------------------------------------------------------------
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//

int filelen (int handle)
{
    struct stat fileinfo;

    if (fstat (handle,&fileinfo) == -1)
        I_Error ("Error fstating");

    return fileinfo.st_size;
}


int W_AddFile (char *filename)
{
    wadinfo_t           header;
    lumpinfo_t*         lump_p;
    unsigned            i;
    int                 handle;
    int                 length;
    int                 startlump;
    filelump_t*         fileinfo;
    filelump_t          singleinfo;
    int                 storehandle;

    // open the file and add to directory

    // handle reload indicator.
    if (filename[0] == '~')
    {
        filename++;
        reload_filename = filename;
        reload_lumpnum = numlumps;
    }

    if ( (handle = open (filename,O_RDONLY | O_BINARY)) == -1)
    {
        CONS_Printf (" couldn't open %s\n",filename);
        return 0;
    }

    CONS_Printf (" adding %s\n",filename);
    startlump = numlumps;

    if (strcasecmp (filename+strlen(filename)-3, "wad") )
    {
        // single lump file
        fileinfo = &singleinfo;
        singleinfo.filepos = 0;
        singleinfo.size = LE_SWAP32(filelen(handle));
        FIL_ExtractFileBase (filename, singleinfo.name);
        numlumps++;
    }
    else
    {
        // WAD file
        read (handle, &header, sizeof(header));
        if (strncmp(header.identification,"IWAD",4))
        {
            // Homebrew levels?
            if (strncmp(header.identification,"PWAD",4))
            {
                I_Error ("Wad file %s doesn't have IWAD "
                         "or PWAD id\n", filename);
            }

            // ???modifiedgame = true;
        }
        header.numlumps = LE_SWAP32(header.numlumps);
        header.infotableofs = LE_SWAP32(header.infotableofs);
        length = header.numlumps*sizeof(filelump_t);
        fileinfo = alloca (length);
        lseek (handle, header.infotableofs, SEEK_SET);
        read (handle, fileinfo, length);
        numlumps += header.numlumps;
    }


    // Fill in lumpinfo
    lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
        I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    storehandle = reload_filename ? -1 : handle;

    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
    {
        lump_p->handle = storehandle;
        lump_p->position = LE_SWAP32(fileinfo->filepos);
        lump_p->size = LE_SWAP32(fileinfo->size);
        strncpy (lump_p->name, fileinfo->name, 8);
    }

    if (reload_filename)
        close (handle);

    return 1;
}
*/
#endif
