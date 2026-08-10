// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: filesrch.c 1257 2016-09-20 17:14:21Z wesleyjohnson $
//
// Copyright (C) 1998-2016 by DooM Legacy Team.
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
//-----------------------------------------------------------------------------

// Because of WINVER redefine, doomtype.h (via doomincl.h) is before any
// other include that might define WINVER
#include "doomincl.h"

#include <stdio.h>
#include <string.h>
#include <direct.h>
  // io.h -> access, _findnext, _A_SUBDIR, chdir

#if 0
#include <fcntl.h>
#include <sys/stat.h>
#ifndef __WIN32__
#include <unistd.h>
#else
#include <windows.h>
#endif
#endif

#include "d_netfil.h"
#include "m_misc.h"

//
// sys_filesearch:
//
//  filename : the filename to be found
//  wantedmd5sum : NULL for no md5 check
//  completepath : when not NULL, return the full path and name
//      must be a buffer of MAX_WADPATH
//  maxsearchdepth : dir depth, when 0 only search given directory
// return FS_NOTFOUND
//        FS_MD5SUMBAD
//        FS_FOUND

filestatus_e  sys_filesearch( const char * filename, const char * startpath,
                              const byte * wantedmd5sum, int maxsearchdepth,
                              /*OUT*/ char * completepath )
{
    filestatus_e fs;
    struct _finddata_t dta;
    int    searchhandle;
    
    searchhandle=access(filename,4);
    if(searchhandle!=-1)
    {
        // take care of gmt timestamp conversion
        fs = checkfile_md5(filename,wantedmd5sum);
        if( (fs==FS_FOUND) && completepath )
	{
	    // completepath may be same buffer as filename
	    char orig_name[MAX_WADPATH];
	    strncpy( orig_name, filename, MAX_WADPATH-1 );
	    orig_name[MAX_WADPATH-1] = '\0';
	    cat_filename(completepath, dta.name, orig_name);
	}
        return fs;
    }

    if( maxsearchdepth < 1 )
        return FS_NOTFOUND;

    // Search subdirectories
    if((searchhandle=_findfirst("*.*",&dta))!=-1)
    {
        do
        {
            if((dta.name[0]!='.') && (dta.attrib & _A_SUBDIR ))
            {
                if( chdir(dta.name)==0 ) { // can fail if we haven't the right
                    fs = sys_filesearch( filename, NULL, wantedmd5sum,
					 maxsearchdepth-1,
                                         /*OUT*/ completepath );
                    chdir("..");
                    if( fs == FS_FOUND )
                    {
                        _findclose(searchhandle);
                        return fs;
                    }
                }
            }
        } while(_findnext(searchhandle,&dta)==0);
    }
    _findclose(searchhandle);
    return FS_NOTFOUND;
}

