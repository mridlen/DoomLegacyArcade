// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: w_zip.c 1456 2019-09-11 12:26:00Z wesleyjohnson $
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

#include "doomdef.h"
#ifdef ZIPWAD

#include <fcntl.h>
  // open
#include <unistd.h>
  // close, read, lseek

#include <zip.h>
  // ziplib

#include "doomincl.h"
#include "doomtype.h"
#include "w_wad.h"
#include "z_zone.h"
#include "md5.h"


// [WDJ] 2020
// Use ziplib to access zip archives (xx.zip).
// It uses zlib to expand the compressed files.

// Check for libzip_ver >= 1.2 which has zip_fseek.
// A loaded libzip may not have zip_fseek.
#if ((LIBZIP_VERSION_MAJOR < 1) || (LIBZIP_VERSION_MAJOR == 1 && LIBZIP_VERSION_MINOR < 2)) || defined(OPT_LIBZIP)
// Generate WZ_zip_fseek.
# define GEN_ZIP_SEEK
#endif


#ifdef OPT_LIBZIP
#include <dlfcn.h>
  // dlopen

// Indirections to libzip functions.
static void (*DL_zip_stat_init)(zip_stat_t *);
#define zip_stat_init  (*DL_zip_stat_init)

static int (*DL_zip_stat_index)(zip_t *, zip_uint64_t, zip_flags_t, zip_stat_t *);
#define zip_stat_index  (*DL_zip_stat_index)

static int (*DL_zip_stat)(zip_t *, const char *, zip_flags_t, zip_stat_t *);
#define zip_stat  (*DL_zip_stat)

static zip_int64_t (*DL_zip_name_locate)(zip_t *, const char *, zip_flags_t);
#define zip_name_locate  (*DL_zip_name_locate)

static zip_t * (*DL_zip_open)(const char *, int, int *);
#define zip_open  (*DL_zip_open)

static void (*DL_zip_discard)(zip_t *);
#define zip_discard  (*DL_zip_discard)

static zip_file_t * (*DL_zip_fopen)(zip_t *, const char *, zip_flags_t);
#define zip_fopen  (*DL_zip_fopen)

static int (*DL_zip_fclose)(zip_file_t *);
#define zip_fclose  (*DL_zip_fclose)

static zip_int64_t (*DL_zip_fread)(zip_file_t *, void *, zip_uint64_t);
#define zip_fread  (*DL_zip_fread)

static zip_error_t * (*DL_zip_file_get_error)(zip_file_t *);
#define zip_file_get_error  (*DL_zip_file_get_error)

static void (*DL_zip_error_init_with_code)(zip_error_t *, int);
#define zip_error_init_with_code  (*DL_zip_error_init_with_code)

static zip_error_t *  (*DL_zip_get_error)( zip_t * );
#define zip_get_error  (*DL_zip_get_error)  

static const char * (*DL_zip_error_strerror)(zip_error_t *);
#define zip_error_strerror  (*DL_zip_error_strerror)

static int (*DL_zip_fseek)( zip_file_t *, zip_int64_t, int/*SEEK_SET*/ );
#define zip_fseek  (*DL_zip_fseek)


byte  libzip_present = 0;

#ifdef LINUX
# define LIBZIP_NAME   "libzip.so"
#else
# define LIBZIP_NAME   "libzip.so"
#endif

// Set libzip_present=0 when libzip is not available.
void WZ_available( void )
{
    // Test for libzip being loaded.
    void * lzp = dlopen( LIBZIP_NAME, RTLD_LAZY );
    // No reason to close it as it would dec the reference count.
    
    libzip_present = ( lzp != NULL );

    if( ! libzip_present )  return;
   
    // Get ptrs for libzip functions.
    DL_zip_stat_init = dlsym( lzp, "zip_stat_init" );
    DL_zip_stat_index = dlsym( lzp, "zip_stat_index" );
    DL_zip_stat = dlsym( lzp, "zip_stat" );
    DL_zip_name_locate = dlsym( lzp, "zip_name_locate" );
    DL_zip_open = dlsym( lzp, "zip_open" );
    DL_zip_discard = dlsym( lzp, "zip_discard" );
    DL_zip_fopen = dlsym( lzp, "zip_fopen" );
    DL_zip_fclose = dlsym( lzp, "zip_fclose" );
    DL_zip_fread = dlsym( lzp, "zip_fread" );

    if( (DL_zip_stat_init == NULL) || (DL_zip_stat_index == NULL)
        || (DL_zip_stat == NULL) || (DL_zip_name_locate == NULL)
        || (DL_zip_open == NULL) || (DL_zip_discard == NULL)
        || (DL_zip_fopen == NULL) || (DL_zip_fclose == NULL)
        || (DL_zip_fread == NULL)  )
    {
        libzip_present = 0;
        return;
    }
   
    DL_zip_file_get_error = dlsym( lzp, "zip_file_get_error" );
    DL_zip_error_init_with_code = dlsym( lzp, "zip_error_init_with_code" );
    DL_zip_get_error = dlsym( lzp, "zip_get_error" );
    DL_zip_error_strerror = dlsym( lzp, "zip_error_strerror" );

    // [WDJ] zip_fseek requires ziplib >= 1.2.0.
    DL_zip_fseek = dlsym( lzp, "zip_fseek" );  // might not be present
}
#undef ZIPLIB_NAME
#endif

//  filebuf : a buffer of length MAX_WADPATH
// Return filename with extension, otherwise NULL.
char *  WZ_make_name_with_extension( const char * filename, const char * extension, /*OUT*/ char * buf )
{
    strncpy( buf, filename, MAX_WADPATH-1 );
    buf[MAX_WADPATH-1] = 0;

    char * extp = strrchr( buf, '.' );
    if( ! extp )
    {
        // append extension
        extp = buf + strlen( buf );
        if( extp > (buf + (MAX_WADPATH - 5)) )
            return NULL;

        extp[0] = '.';
    }
    
    if( extp > (buf + (MAX_WADPATH - 5)) )
        return NULL;

    strcpy( extp+1, extension );
    return buf;
}

char  archive_filename[MAX_WADPATH];


void  WZ_save_archive_name( const char * filename )
{
    strncpy( archive_filename, filename, MAX_WADPATH-1 );
    archive_filename[MAX_WADPATH-1] = 0;
}

// Return archive filename, otherwise NULL.
char *  WZ_make_archive_name( const char * filename )
{
    return  WZ_make_name_with_extension( filename, "zip", /*OUT*/ archive_filename );
}



// Return 0 when exact match.
// Return 1 when filename1 is zip file name.
// Return 2 when filename2 is zip file name.
// Return >= 4 when other difference.
byte WZ_filename_cmp( const char * filename1, const char * filename2 )
{
    int l1, l2;
    if( strcasecmp( filename1, filename2 ) )
        return 0;  // exact match

    l1 = strlen( filename1 ) - 3;  // should be the '.'
    l2 = strlen( filename2 ) - 3;
    if( (l2 != l1) || (l1 < 1) )
        return 9;  // cannot match extensions

    if( strncasecmp( filename1, filename2, l1 ) )
        return 5;  // different name

    if( (strncasecmp( filename1 + l1, ".wad", 3 ) == 0 )
        && (strncasecmp( filename2 + l1, ".zip", 3 ) == 0 ) )
    {
        return 2;  // filename2 has ZIP extension
    }
    else if( (strncasecmp( filename1 + l1, ".zip", 3 ) == 0 )
        && (strncasecmp( filename2 + l1, ".wad", 3 ) == 0 ) )
    {
        return 1;  // filename1 has ZIP extension
    }
   
    return 4; // match fail
}

// Can only have one archive open at any time.
// Do not support archive in archive.
static zip_t * archive_z = NULL;
static zip_file_t * file_z = NULL;
static FILE *  file_n = NULL;

byte  archive_open = 0;
byte  archive_filenum = 0;  // filenum assigned by W_Load_WadFile
byte  file_filenum = 0;


const char *  WZ_error_from_archive_z( void )
{
    zip_error_t * error_z = zip_get_error( archive_z );
    return  zip_error_strerror( error_z );
}

#ifdef GEN_ZIP_SEEK
// Does not have zip_fseek
zip_uint64_t  position_z;
char *        filename_z = NULL;

// Return -1 when fail.
static
int  WZ_zip_fseek( uint32_t offset )
{
    byte bb[1024];

#ifdef OPT_LIBZIP
    if( DL_zip_fseek )
    {
        // [WDJ] zip_fseek requires ziplib >= 1.2.0
        zip_int8_t zf = (*DL_zip_fseek)( file_z, offset, SEEK_SET );
        if( zf == 0 )
            return 0;

        // [WDJ] Sometimes the zip_fseek fails on a perfectly good file.
        position_z = offset + 10;  // force close/open
    }
#endif
   
    // Does not have zip_fseek
    if( offset < position_z )
    {
        // Re-open the file to position it to 0 again.
        zip_fclose( file_z );
        file_z = zip_fopen( archive_z, filename_z, ZIP_FL_NOCASE | ZIP_FL_NODIR );
        position_z = 0;
    }

    while( position_z < (zip_uint64_t) offset )
    {
        zip_uint64_t read_count = ((zip_uint64_t) offset) - position_z;
        if( read_count > sizeof(bb) )  read_count = sizeof(bb);  // buf size
        int rs = zip_fread( file_z, bb, read_count );  // discard
        if( rs < 0 )
        {
            position_z = (zip_uint64_t) -1;
            return rs;
        }

        position_z += rs;
    }
    return 0;
}
#endif


void  WZ_open_archive( const char * archive_name )
{
    int zip_err_code;
    
#ifdef OPT_LIBZIP
    if( ! libzip_present )
        return;
#endif

    if( archive_z )  // Only allow one archive open at a time.
    {
        zip_discard( archive_z );  // does not save changes, no errors
        archive_open = 0;
    }

    archive_z = zip_open( archive_name, ZIP_RDONLY, &zip_err_code );
    if( archive_z == NULL )
    {
        zip_error_t  zip_error;
        zip_error_init_with_code( &zip_error, zip_err_code );
        GenPrintf(EMSG_warn, "Zip file %s: %s\n", archive_name, zip_error_strerror( &zip_error ) );
        return;
    }

    archive_open = 1;
}

void  WZ_close_archive( void )
{
      if( archive_z )
      {
//        zip_close( archive_z );
          zip_discard( archive_z );  // does not save changes, no errors
          archive_z = NULL;
      }
      archive_open = 0;
}

byte  WZ_open_file_z( const char * filename )
{
    if( file_z )
        zip_fclose( file_z );

#ifdef GEN_ZIP_SEEK
    // Tracking for WZ_zip_fseek
    if( filename_z )
        free( filename_z );
    filename_z = strdup( filename );
    position_z = 0;
#endif

    file_z = zip_fopen( archive_z, filename, ZIP_FL_NOCASE | ZIP_FL_NODIR );
    if( file_z )
        return FH_zip_file;

    GenPrintf(EMSG_warn, "Open %s: %s\n", filename, WZ_error_from_archive_z() );
    return FH_none;
}

static
void  WZ_close_file_z( void )
{
    if( file_z )
    {
        zip_fclose( file_z );
        file_z = NULL;
    }

#ifdef GEN_ZIP_SEEK
    // Tracking for WZ_zip_fseek
    position_z = 0;
    free( filename_z ); // can be NULL
    filename_z = NULL;
#endif

    file_filenum = 0xFF;
}


// Open a file.
// Return the local handle.
byte  WZ_open( const char * filename )
{
    byte fc = W_filename_classify( filename );
    if( fc == FC_zip )
    {
        WZ_open_archive( filename );
        return FH_zip_archive;
    }
   
    if( archive_z )
    {
        return  WZ_open_file_z( filename );
    }
    else
    {
        file_n = fopen( filename, "rb" );
        if( file_n )
            return FH_file;
    }
    return FH_none;
}

// Close a file.
void  WZ_close( byte handle )
{
    byte fh = handle & FH_mask;
    if( fh == FH_zip_archive )
    {
        WZ_close_archive();
    }
    else if( fh == FH_zip_file )
    {
        WZ_close_file_z();
    }
    else if( file_n )
    {
        fclose( file_n );
        file_n = NULL;
    }
}

void  WZ_close_all( void )
{
    WZ_close( 0 );
    WZ_close_file_z();
    WZ_close_archive();
}


// Position a file.
// Return seek status, -1 on error.
int  WZ_seek( byte handle, uint32_t offset )
{
    int rs = 0;
    byte fh = handle & FH_mask;
    if( (fh == FH_zip_file) && file_z )
    {
#ifdef GEN_ZIP_SEEK
        // Does not have zip_fseek, use work-around.
        rs = WZ_zip_fseek( offset );
#else
        // [WDJ] zip_fseek requires ziplib >= 1.2.0
        rs = zip_fseek( file_z, offset, SEEK_SET );
#endif
    }
    else if( file_n )
    {
        rs = fseek( file_n, offset, SEEK_SET );
    }
    return rs;
}

// Read a file.
// Return num bytes read, -1 on error.
int  WZ_read( byte handle, uint32_t read_count, /*OUT*/ byte * dest )
{
    int rs = 0;
    byte fh = handle & FH_mask;
    if( (fh == FH_zip_file) && file_z )
    {
        rs = zip_fread( file_z, dest, read_count );

#ifdef GEN_ZIP_SEEK
        // Tracking for WZ_zip_fseek
        if( rs < 0 )
        {
            position_z = (zip_uint64_t) -1;
            return rs;
        }

        position_z += rs;  // update position for fseek
#endif

    }
    else if( file_n )
    {
        rs = fread( dest, read_count, 1, file_n );  // 1 item of size read_count
        // returns num of items read (not bytes)
        if( rs > 0 )
            rs = read_count;
    }
    return rs;
}



// ---- ZIP file handling

//   archive_name : zip archive to search
//                  when NULL, search the currently open archive
// Return FS_FOUND when file found.
// Return FS_NOTFOUND when file not found.
// Return other FS when other error.
byte  WZ_find_file_in_archive( const char * filename, const char * archive_name )
{
    byte result = FS_NOTFOUND;

    if( archive_name )
    {
        // Close the current archive, and open this archive.
        WZ_open_archive( archive_name );
    }

    if( archive_z )
    {
        zip_int64_t zi = zip_name_locate( archive_z, filename, ZIP_FL_NOCASE | ZIP_FL_NODIR ); // index
        if( zi >= 0 )
            result = FS_FOUND;

//        if( archive_name )       
//            WZ_close_archive( );
    }
    return result;
}


unsigned int  WZ_filesize( const char * filename )
{
    zip_stat_t  zipstat;
    unsigned int filesize = 0;

#ifdef OPT_LIBZIP
    if( ! libzip_present )
        return 0;
#endif
    // Get info about file in archive.
// by name or by index
    int r = zip_stat( archive_z, filename, ZIP_FL_NOCASE | ZIP_FL_NODIR, /*OUT*/ &zipstat );
    if( r < 0 )
    {
        // on failure the error code is in archive_z        
        GenPrintf( EMSG_warn, "WZ_filesize %s: %s\n", filename, WZ_error_from_archive_z() );
        return -1;
    }
   
    if( zipstat.valid & ZIP_STAT_SIZE )
        filesize = zipstat.size;
#if 0   
    if( zipstat.valid & ZIP_STAT_INDEX )
        index = zipstat.index;  // index within archive
#endif
   
    return filesize;
}



// Read from file in archive_z.
//  offset : read at offset, optional
//  read_size : in bytes
//  dest : dest buffer
// return bytes read, or -1 when error
int WZ_read_archive_file( uint32_t offset, uint32_t read_size, /*OUT*/ byte * dest )
{
//    zip_file_t *  file_z;
    const char * msg;
    int num_read = -1;
   
#ifdef OPT_LIBZIP
    if( ! libzip_present )
        return -1;
#endif

    if( archive_z == NULL )
        return -1;

    if( file_z == NULL )
        return -1;

    if( offset )  // ???
    {
        int se;
#ifdef GEN_ZIP_SEEK
        // Does not have zip_fseek, use work-around.
        se = WZ_zip_fseek( offset );
#else
        // [WDJ] zip_fseek requires ziplib >= 1.2.0
        se = zip_fseek( file_z, offset, SEEK_SET );
#endif
        if( se < 0 )
        {
            msg = "Seek";
            goto print_err;
        }
    }

    if( read_size )
    {
        num_read = zip_fread( file_z, dest, read_size );
        if( num_read < 0 )
            goto file_err;

#ifdef GEN_ZIP_SEEK
        // Tracking for WZ_zip_fseek
        position_z += num_read;
#endif
    }

    return num_read;

file_err:
    {
        zip_error_t * error_z = zip_file_get_error( file_z );
        msg = zip_error_strerror( error_z );
    }
#ifdef GEN_ZIP_SEEK
    position_z = (zip_uint64_t) -1;
#endif
    goto print_err;
   
print_err:
    GenPrintf(EMSG_warn, "WZ_Read %s error: offset=%i, size=%i, %s\n", wadfiles[file_filenum]->filename, offset, read_size, msg );
    return -1;
}

//  fn : wadfile filenum
//  wf : read file wadfile_t, in an archive
//  offset : read at offset
//  read_size : in bytes
//  dest : dest buffer
// return bytes read
int WZ_read_wadfile_from_archive_file_offset( byte fn, wadfile_t * wf, uint32_t offset, uint32_t read_size, /*OUT*/ byte * dest )
{
    wadfile_t * archive_wf = wadfiles[ wf->archive_parent ];
   
#ifdef OPT_LIBZIP
    if( ! libzip_present )
        return 0;
#endif

    if( (archive_z == NULL) || (wf->archive_parent != archive_filenum) )
    {
        // Close any current archive, and open the correct one.
        WZ_close_file_z();  // file position is bogus if not closed.
        WZ_open_archive( archive_wf->filename );  // archive_z
        if( archive_z == NULL )
            return 0;
    }
   
    if( (file_z == NULL) || (file_filenum != fn) )
    {
        WZ_open_file_z( wf->filename );  // must perform seek fixes
        if( file_z == NULL )
            return 0;

        file_filenum = fn;
    }

    int br = WZ_read_archive_file( offset, read_size, /*OUT*/ dest );
//    int br = WZ_read_archive_file( WZC_OPEN | WZC_SEEK | WZC_CLOSE, wf->filename, offset, read_size, /*OUT*/ dest );

//    WZ_close_archive();
    return br;
}


// Important: BLOCKSIZE must be a multiple of 64.
#define MD5_BLOCKSIZE 4096

// This is same as md5_stream, rewritten to access zip files.
// For zip file, compute the MD5 message digest.
//
// digest_block : the md5 digest, as 16 byte array
//  Return FS_FOUND when success.
filestatus_e  WZ_md5_stream( const char * filename, byte * digest_block )
{
    zip_file_t *  file_z;
    struct md5_ctx  ctx;
    char buffer[MD5_BLOCKSIZE + 72];
    size_t readcnt;
    zip_int64_t  n;

#ifdef OPT_LIBZIP
    if( ! libzip_present )
        return FS_INVALID;
#endif
    if( archive_z == NULL )
        return FS_FILEERR;

#ifdef GEN_ZIP_SEEK
    position_z = 0;
#endif
    file_z = zip_fopen( archive_z, filename, ZIP_FL_NODIR );
    if( file_z == NULL )
        return FS_NOTFOUND; // on failure the error code is in archive_z

    // Initialize the computation context.
    md5_init_ctx( &ctx );

    // Iterate over full file contents.
    for(;;)
    {
        //  Read the zip file in MD5_BLOCKSIZE bytes.
        //  Each call of the computation processes a entire whole buffer.
        readcnt = 0;

        // Read a block.
        do
        {
            // Does not trust getting a full buffer in one read.
            n = zip_fread( file_z, buffer + readcnt, MD5_BLOCKSIZE - readcnt );
            if( n < 0 )
                goto file_err;

            // If end of file is reached, end the loop.
            if( n == 0 )
                goto eof_reached;  // have not reached MD5_BLOCKSIZE

            readcnt += n;
        }
        while( readcnt < MD5_BLOCKSIZE );

        // Process buffer with MD5_BLOCKSIZE bytes.
        // Note that modulo( MD5_BLOCKSIZE, 64 ) == 0
        md5_process_block( buffer, MD5_BLOCKSIZE, &ctx );
    }

eof_reached:
    // Add the last odd buffer size, if necessary.
    if( readcnt > 0 )
        md5_process_bytes( buffer, readcnt, &ctx );

    // Construct result in desired memory.
    md5_finish_ctx (&ctx, digest_block);

#ifdef GEN_ZIP_SEEK
    position_z = (zip_uint64_t) -1;
#endif
    zip_fclose( file_z );
    return FS_FOUND;

file_err:
#ifdef GEN_ZIP_SEEK
    position_z = (zip_uint64_t) -1;
#endif
    zip_fclose( file_z );
    return FS_FILEERR;
}


// ---- ZIP archive access


// Zip file. Process all files in directory.
// Calls wad handler function.
//  as_archive_filenum : the wadfile index assigned to the archive
// Return number of wadfile processed.
// Return -1 when problem.
int  WZ_Load_zip_archive( const char * filename, int as_archive_filenum )
{
    zip_stat_t  zipstat;
    int file_count = 0;  // nothing loaded
    int i;

#ifdef OPT_LIBZIP
    if( ! libzip_present )
        return -1;
#endif
   
    zip_stat_init( &zipstat );

    // Open zip
    WZ_open_archive( filename );  // archive_z
    if( archive_z == NULL )
        return -1;

    archive_filenum = as_archive_filenum;
    for( i=0; i<1024; i++ )
    {
        // by name or by index
        int r = zip_stat_index( archive_z, i, 0, /*OUT*/ &zipstat );
        if( r < 0 )
            break;  // end of directory (one way or another).

        // Recursive call of Load Wadfile.
        int fn = W_Load_WadFile( zipstat.name );  // wadfile index
        if( fn > 0 )
            file_count++;
    }

    // close archive_z
    WZ_close_archive();
    return file_count;
}

#endif

