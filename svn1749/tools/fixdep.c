//!!C.ANSI
// Author: Wesley D. Johnson
// Date: 2015-5-8
// Copyright: GPL
// Overview: 
//   Fix dep file to include the object directory.
//   Usage:
//      $$ fixdep objdir  source.dep
//   The target is systems that do not have a convienent sed, such as DOS and Windows.
//   Handling unix systems is included for completeness. 
//:END:

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __WIN32__
# include <direct.h>
#else
# include <unistd.h>
#endif

int stat = 0;

FILE * depfile = NULL;

#define NAMELEN 256
char objstr[ NAMELEN ];
char filename_in[ NAMELEN ];

// To buffer one dep line.
#define BUFLEN  2048
char buf1[ BUFLEN ];


void help( void )
{
    printf( "Usage:\n  fixdep  objdir  depfile\n" );
    printf( "  objdir: must be a string with object directory.\n" );
    printf( "  depfile: is the dep file to be altered.\n" );
}

#ifdef __WIN32__
#define DOS_NAME_STYLE   256
#endif

#ifdef __OS2__
#define DOS_NAME_STYLE   256
#endif

#ifdef PC_DOS
#define DOS_NAME_STYLE   8
#endif

// The typical dep file is 2K to 20K, text lines, continued with backslash.
#define NUM_LINES_CONTENT  8
typedef struct linebuf_s {
  struct linebuf_s *   next;
  char *  content[ NUM_LINES_CONTENT ];  // lines
} linebuf_t;

linebuf_t  head;
linebuf_t * tail;
int  tail_index;
int  linecount = 0;


void  init_linebuf( void )
{
    memset( &head, 0, sizeof( linebuf_t ) );
    head.next = NULL;
    tail = &head;
    tail_index = 0;
    linecount = 0;
}

// Store the srcline at the current tail.
void  put_linebuf( const char * srcline )
{
    if( tail_index > (NUM_LINES_CONTENT-1) )
    {
        // Next container.
        tail->next = (linebuf_t*) malloc( sizeof(linebuf_t) );
        if( tail->next == NULL )  goto memory_error;
        tail = tail->next;
        // Init the new container.
        memset( tail, 0, sizeof( linebuf_t ) );
        tail->next = NULL;
        // First content in the new container.
        tail_index = 0;
    }
    // Store the line in the current container context.
    tail->content[ tail_index ++ ] = strdup( srcline );
    linecount++;
    return;

memory_error:
    printf( "Out of memory: linecount = %i\n", linecount );
    exit(3);
}

// Sequentially get lines from the head.
static linebuf_t * get_linebuf_ptr = NULL;
static int         get_linebuf_index = 0;

char *  get_linebuf( void )
{
   if( get_linebuf_ptr == NULL )
   {
       // Init to head of linebuf storage.
       get_linebuf_ptr = &head;
       get_linebuf_index = 0;
   }
   else if( get_linebuf_index > (NUM_LINES_CONTENT-1) )
   {
       // Next storage unit
       get_linebuf_ptr = get_linebuf_ptr->next;
       get_linebuf_index = 0;
   }

   if( get_linebuf_ptr == NULL )
   {
       return NULL;
   }
   else if( get_linebuf_ptr->next == NULL )
   {
       // At tail
       if( get_linebuf_index >= tail_index )
           return NULL;  // end of lines 
   }
   
   return  get_linebuf_ptr->content[ get_linebuf_index++ ];
}

void fix_line( void )
{
    int olen = strlen( objstr );
    int blen = strlen( buf1 );

    if( buf1[0] == ' ' )  return;
    // Verify that is ".o" line
    if( strstr( &buf1[0], ".o" ) == NULL )  return;
    if( olen + blen > BUFLEN-1 )
    {
        printf( "Line too long, truncated\n" );
        blen = BUFLEN-1 - olen;  // truncate does least damage
        buf1[ blen ] = 0;
        stat = -5;
    } 
    memmove( &buf1[olen], &buf1[0], blen+1 );
    memcpy( &buf1[0], objstr, olen );
    buf1[ BUFLEN-1 ] = 0;
}


int main(int argc, char * argv[])
{
    stat = 0;
    init_linebuf();

    // Get the string with object directory.
    if( argc < 2 )
    {
        help();
        exit(1);
    }
    strncpy( objstr, argv[1], NAMELEN-1 );
    objstr[ NAMELEN-1 ] = 0;
    
    // Get the dep file name.
    strncpy( filename_in, argv[2], NAMELEN-1 );
    filename_in[ NAMELEN-1 ] = 0;

    if( access( filename_in, R_OK ) != 0 )
    {
        printf( "Dep file does not exist: %s\n", filename_in );
        help();
        exit(1);
    }

    // Read the dep file.
    depfile = fopen( filename_in, "r" );
    if( depfile == NULL )
    {
        printf( "Dep file not readable: %s\n", filename_in );
        goto error_exit;
    }

    while( fgets( buf1, BUFLEN-1, depfile ) )
    {
        if( buf1[0] > 32 )
          fix_line();
        put_linebuf( buf1 );
    }

    fclose(depfile);
    depfile = NULL;

    depfile = fopen( filename_in, "w" );
    if( depfile == NULL )
    {
        printf( "Dep file not writable: %s\n", filename_in );
        goto error_exit;
    }

    // Write the output
    for(;;)
    {
        char * outline = get_linebuf();
        if( outline == NULL )  break;
        fputs( outline, depfile );
    }

    fclose(depfile);
    if( stat < 0 )
       exit(stat);
    return 0;

error_exit:
    if( depfile )
      fclose(depfile);
    exit(1);
}
