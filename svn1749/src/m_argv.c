// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: m_argv.c 1414 2018-12-06 22:01:48Z wesleyjohnson $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: m_argv.c,v $
// Revision 1.5  2001/12/26 17:24:46  hurdler
// Update Linux version
//
// Revision 1.4  2001/08/12 15:21:04  bpereira
// see my log
//
// Revision 1.3  2000/03/29 19:39:48  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//    Multiple parameters
//
//
//-----------------------------------------------------------------------------


#include <string.h>

#include "doomincl.h"
#include "command.h"

// set by port main; may be replaced
int             myargc;
char**          myargv;

static int      myarg_alloc = 0;
static int      found;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
int M_CheckParm (const char *check)
{
    int         i;

    for (i = 1;i<myargc;i++)
    {
        if ( !strcasecmp(check, myargv[i]) )
        {
            found=i;
            return i;
        }
    }
    found=0;
    return 0;
}

// return true if there is available parameters
boolean M_IsNextParm(void)
{
    if(found>0 && found+1<myargc && myargv[found+1][0] != '-' && myargv[found+1][0] != '+')
        return true;
    return false;
}

// return the next parameter after a M_CheckParm
// NULL if not found use M_IsNext to find if there is a parameter
char *M_GetNextParm(void)
{
    if(M_IsNextParm())
    {
        found++;
        return myargv[found];
    }
    return NULL;
}

// push to COM all parameters begining by '+'
void M_PushSpecialParameters( void )
{
    int     i;
    char    s[256];
    boolean onetime=false;

    for (i = 1;i<myargc;i++)
    {
        if ( myargv[i][0]=='+' )
        {
            strcpy(s,&myargv[i][1]);
            i++;

            // get the parameter of the command too
            for(;i<myargc && myargv[i][0]!='+' && myargv[i][0]!='-' ;i++)
            {
                strcat(s," ");
                if(!onetime) { strcat(s,"\"");onetime=true; }
                strcat(s,myargv[i]);
            }
            if( onetime )    { strcat(s,"\"");onetime=false; }
            strcat(s,"\n");

            // push it
            COM_BufAddText (s);
            i--;
        }
    }
}




// [WDJ] generic expand of arg list
// Insert new args at atindex with existing CMDLINE ARGS before and after
char ** prev_argv = NULL;
int     prev_argc = 0;
int     prev_alloc = 0;   // to determine if prev_argv was allocated

// realloc the argv memory to a new size
static void expand_argv_memory( int total_req )
{
    // alloc too small
    // In this case, myargv is known to be from malloc
    myargv = realloc(myargv, sizeof(char *)*total_req);
    if( !myargv )
      I_Error("argv: not enough memory");
    if((total_req - myarg_alloc) > 0 )  // sometimes request is for less
    {
        // clear the added argv, but not the occupied argv
        memset( &myargv[myarg_alloc], 0,
                sizeof(char *) * (total_req - myarg_alloc));
    }
    myarg_alloc = total_req;
}

// expand the arg array to new request
static void expand_args( int total_req, int atindex )
{
    int k;

    // save current args for finish_expand
    prev_argv = myargv;
    prev_argc = myargc;
    prev_alloc = myarg_alloc;

    myargv = NULL;  // to protect prev_argv
    // always realloc new, to be separate from prev_argv
    expand_argv_memory( total_req );
    // keep arg[0] at [0], copy args upto atindex
    if( atindex <= 0 )  atindex = 1;
    myargc = 0;
    for (k = 0; k < atindex; k++)  // before insert
       myargv[myargc++] = prev_argv[k];
}


// include the prev args
static void finish_expand( int fromindex )
{
    int k;
    int newcnt = (prev_argc - fromindex) + myargc;

    if( prev_argv )
    {
        if( newcnt > myarg_alloc )
            expand_argv_memory( newcnt );
        // copy command line args after insert
        for (k = fromindex; k < prev_argc; k++)
            myargv[myargc++] = prev_argv[k];

        if( prev_alloc )  // was from malloc
        {
            free( prev_argv );
            prev_argv = NULL;
        }
    }
}

#define FILEARGS_INC     16

//
// Find a Response File
//
void M_FindResponseFile (void)
{
    int myi;
    char * myp;

    for (myi = 1; myi < myargc; myi++)
    {
        myp = myargv[myi];
        // because args after myi are from file, this is recursive
        if (myp[0] == '@')
        {
            FILE *          handle;
            size_t          size;
            int             k;
            boolean         inquote = false;
            char * file_mem;  // alloc mem, will be in use but this ptr will be lost
            char * infile;

            // READ THE RESPONSE FILE INTO MEMORY
            handle = fopen (&myp[1],"rb");
            if (!handle)
            {
                I_Error ("\nResponse file %s not found !", &myp[1]);
                exit(1);
            }
            CONS_Printf("Found response file %s!\n", &myp[1]);
            fseek (handle,0,SEEK_END);
            size = ftell(handle);
            fseek (handle,0,SEEK_SET);
            file_mem = malloc (size);
            fread (file_mem, size, 1, handle);
            fclose (handle);

            // Keep ARGV[0] and args upto this arg
            expand_args( myargc + FILEARGS_INC, myi );

            infile = file_mem;
            for( k = 0; k < size ; )
            {
                inquote = infile[k] == '"';
                if ( inquote )       // strip enclosing double-quote
                    k++;
                myargv[myargc++] = &infile[k];
                while (k < size && ( (inquote && infile[k]!='"') 
                                  || (!inquote && infile[k]> ' ') ) )
                    k++;
                infile[k] = 0;
                while(k < size && (infile[k]<= ' '))
                    k++;
            }

            // DISPLAY ARGS
            CONS_Printf("%d command-line args:\n",myargc);
            for (k=1;k<myargc;k++)
                CONS_Printf("%s\n",myargv[k]);

            break;
        }
    }
}


#ifdef LAUNCHER
// bounds for addparm memory maintenance, to avoid memory leaks
char * addparm_low = NULL;
char * addparm_high = NULL;
int    addparm_count = 0;


// alloc memory for addparam, keep bounds
//  s1 : source string ptr, to add to parameters
//  count : count of characters of source string
//  atindex : parameter num
static void  M_Arg_string( const char * s1, int count, int atindex )
{
#ifdef __MINGW32__
    char * sp = malloc( count+1 );  // malloc
    strncpy( sp, s1, count );
    sp[count] = 0;
#else
    char * sp = strndup( s1, count );  // malloc
#endif
    if( addparm_low )
    {
        // bounds on memory alloc
        if( addparm_low > sp )   addparm_low = sp;
        if( addparm_high < sp )   addparm_high = sp;
    }
    else
    {
        // init
        addparm_low = sp;
        addparm_high = sp;
    }
    myargv[atindex] = sp;
    addparm_count ++;
}


void M_Remove_Param( int i )
{
    int k;
    char * rmp = myargv[i];
    
    if( i <=0 || i >= myargc )  return;
   
    if( addparm_low && rmp >= addparm_low && rmp <= addparm_high )
    {
        free( rmp );  // is within our addparm memory
        addparm_count --;
    }
    // shuffle args down
    for (k = i; k < myargc-1; k++)
        myargv[k] = myargv[k+1];
    myargc --;
    myargv[myargc] = NULL;  // end of args
}


void M_Remove_matching_Param( const char * p1, const char * p2 )
{
    while( M_CheckParm ( p1 ) )  // if already exists, then erase it
    {
        // Remove the param and the associated operand
        M_Remove_Param( found );  // shuffles next arg down to found
        if( p2 )
        {
            while( myargv[found] && myargv[found][0] != '-'
                   && myargv[found][0] != '+' && myargv[found][0] != '@' )
                M_Remove_Param( found );
        }
    }
}


// add a param from Launcher
//  p1 : parameter string (one or more parameters)
//  p2 : optional additional parameter string (NULL)
void M_Add_Param( const char * p1, const char * p2 )
{
    const char * pa, * ps;

    M_Remove_matching_Param( p1, p2 );  // if already exists, then erase it

    // save previous argv for finish_expand
    // and add some extra argv alloc
    expand_args( myargc + 8, 1 );
    // insert starting at index 1
    pa = p1;  // first insert p1 string
    for( myargc = 1; ;  )
    {
        if( myargc >= myarg_alloc )
           expand_argv_memory( myargc + 8 );
        ps = strchr( pa, ' ' );  // more than one param in the string
        if( ps )  // found a delimiter
        {
            M_Arg_string( pa, ps-pa, myargc );
            myargc ++;
            pa = ps;
            while( *pa == ' ' )  pa++;  // skip space (1 or more)
            if( *pa )   continue;
            // end of string
        }
        else
        {
            // copy the rest of the string
            M_Arg_string( pa, strlen(pa), myargc );
            myargc ++;
        }
        if( ! p2 )  break;
        pa = p2;  // continue with insert of p2 string
        p2 = NULL;  // to exit the loop next time
    }
    finish_expand( 1 );  // command line args after the insert
}


// add two param from Launcher, or remove them if p2==NULL or empty string
void M_Change_2Param( const char * p1, const char * p2 )
{
    if( p2 && p2[0] )  // not an empty string
        M_Add_Param( p1, p2 );
    else
        M_Remove_matching_Param( p1, "" );
}


// Clear all param from Add_Param
void M_Clear_Add_Param( void )
{
    int i = addparm_count;
    while( i-- )
        M_Remove_Param( 1 );
    addparm_count = 0;
}
#endif
