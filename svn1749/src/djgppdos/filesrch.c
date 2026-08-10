#include <stdio.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "d_netfil.h"
#include "m_misc.h"

#define MAX_SRCHPATH (MAX_WADPATH * 2)

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
    filestatus_e retval = FS_NOTFOUND;
    DIR ** dirhandle_stack;  // (malloc)
    DIR * dirhandle;
    struct dirent *dent;
    struct stat fstat;
    int * index_stack;  // each directory in the searchpath  (malloc)
    int depth=0;
    int cur_index, remspace;  // searchpath
    char searchpath[MAX_SRCHPATH];

    if( maxsearchdepth < 1 )
       maxsearchdepth = 0;
    dirhandle_stack = (DIR**) malloc( (maxsearchdepth+1) * sizeof( DIR*));
    if( dirhandle_stack == NULL )   goto error1_exit;
    index_stack = (int*) malloc( (maxsearchdepth+1) * sizeof(int));
    if( index_stack == NULL )   goto error2_exit;

    strncpy( searchpath, startpath, MAX_SRCHPATH-1);
    searchpath[MAX_SRCHPATH-1] = '\0';
    cur_index = strlen( searchpath) + 1;

    dirhandle = opendir( searchpath);
    if( dirhandle == NULL )  goto error3_exit;

    // Initial stack
    index_stack[0] = cur_index;
    dirhandle_stack[0] = dirhandle;

    if(searchpath[cur_index-2] != '\\')
    {
        searchpath[cur_index-1] = '\\';
        searchpath[cur_index] = 0;
    }
    else
    {
        cur_index--;
    }

    for(;;)
    {
        searchpath[cur_index]=0;
        dent = readdir( dirhandle );  // next dir entry
        if( !dent)  // done with dir
        {
            closedir( dirhandle );
            // Pop stack to previous directory.
            if( depth == 0 )  break;  // done
            cur_index = index_stack[--depth];
            dirhandle = dirhandle_stack[depth];
            continue;
        }

        if( dent->d_name[0]=='.' )
        {
            // ignore the "." and ".." entries, we don't want to scan uptree
            if( dent->d_name[1]=='\0' )  continue;
            if( dent->d_name[1]=='.' && dent->d_name[2]=='\0' )  continue;
        }

        // Form file name for stat.
        remspace = (MAX_SRCHPATH - 1) - cur_index;
        strncpy(&searchpath[cur_index], dent->d_name, remspace);

        if( stat(searchpath,&fstat) < 0) // do we want to follow symlinks? if not: change it to lstat
        {
            // was the file (re)moved? can't stat it
            continue;
        }

        if( S_ISDIR(fstat.st_mode) )
        {
            if( depth >= maxsearchdepth )  continue;  // depth limited

            dirhandle = opendir(searchpath);
            if( dirhandle == NULL )
            {
                // can't open it... maybe no read-permissions
                // go back to previous dir
                cur_index = index_stack[depth];
                dirhandle = dirhandle_stack[depth];
                continue;
            }

            // Push new directory to stack.
            cur_index = strlen(searchpath) + 1;
            index_stack[++depth] = cur_index;
            dirhandle_stack[depth] = dirhandle;

            searchpath[cur_index-1]='\\';
            searchpath[cur_index]=0;
        }
        else if ( strcasecmp(filename, dent->d_name) == 0 )
        {
            // Found the filename.
            retval = checkfile_md5(searchpath, wantedmd5sum);
            if( retval != FS_FOUND )  continue;

            // FS_FOUND, Return the found file.
            if(completepath)
            {
                strncpy(completepath, searchpath, MAX_WADPATH-1);
                completepath[MAX_WADPATH-1] = '\0';
            }
            // Close the open directories.
            for(; depth >= 0; closedir(dirhandle_stack[depth--]));
            break;
        }
    }

error3_exit:
    free(index_stack);
error2_exit:
    free(dirhandle_stack);
error1_exit:

    return retval;
}
