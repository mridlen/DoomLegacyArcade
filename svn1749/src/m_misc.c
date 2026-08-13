// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: m_misc.c 1685 2024-06-20 09:45:15Z wesleyjohnson $
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
// $Log: m_misc.c,v $
// Revision 1.10  2004/04/18 12:40:14  hurdler
// Jive's request for saving screenshots
//
// Revision 1.9  2003/10/15 14:09:47  darkwolf95
// Fixed screenshots filename bug
//
// Revision 1.8  2001/03/03 06:17:33  bpereira
// Revision 1.7  2001/02/24 13:35:20  bpereira
//
// Revision 1.6  2001/01/25 22:15:42  bpereira
// added heretic support
//
// Revision 1.5  2000/10/08 13:30:01  bpereira
// Revision 1.4  2000/09/28 20:57:15  bpereira
// Revision 1.3  2000/04/16 18:38:07  bpereira
//
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      Default Config File.
//      PCX Screenshots.
//      File i/o
//      Common used routines
//
//-----------------------------------------------------------------------------


#include <fcntl.h>
#include <unistd.h>

#include "doomincl.h"
#include "g_game.h"
#include "m_misc.h"
#include "hu_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_video.h"
#include "d_main.h"
#include "m_argv.h"
#include "m_swap.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================


//
// FIL_WriteFile
//

boolean FIL_WriteFile ( char const*   name,
                        void*         source,
                        int           length )
{
    int         handle;
    int         count;

    handle = open ( name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

    if (handle == -1)
        return false;

    count = write (handle, source, length);
    close (handle);

    if (count < length)
        return false;

    return true;
}

//
// FIL_ReadFile : return length, 0 on error
//
//Fab:26-04-98:
//  appends a zero byte at the end
int FIL_ReadFile ( char const*   name,
                   byte**        buffer )
{
    int    handle, count, length;
    struct stat fileinfo;
    byte   *buf;

    handle = open (name, O_RDONLY | O_BINARY, 0666);
    if (handle == -1)
        return 0;

    if (fstat (handle,&fileinfo) == -1)
        return 0;

    length = fileinfo.st_size;
    buf = Z_Malloc (length+1, PU_STATIC, 0);
    count = read (handle, buf, length);
    close (handle);

    if (count < length)
        return 0;

    //Fab:26-04-98:append 0 byte for script text files
    buf[length]=0;

    *buffer = buf;
    return length;
}

// Extended Read and Write of buffers.

int FIL_ExtFile_Open ( ExtFIL_t * ft,  char const* name, boolean write_flag )
{
    ft->stat_error = STAT_OPEN;
    ft->bufcnt = 0;  // buffer empty
    ft->handle =
      open ( name,
             ( (write_flag)? O_WRONLY | O_CREAT | O_TRUNC | O_BINARY // write
                            :O_RDONLY | O_BINARY  // read
             ), 0666);
    if( ft->handle < 0) // file not found, or not created
        ft->stat_error = ft->handle; // error
    return ft->stat_error;
}

int FIL_ExtWriteFile ( ExtFIL_t * ft, size_t length )
{
    int count = write (ft->handle, ft->buffer, length);
    if( count != length )  // did not write all of length (disk full)
       ft->stat_error = ERR_RW;  // something negative, not -1
    return ft->stat_error;
}

int FIL_ExtReadFile ( ExtFIL_t * ft, size_t length )
{
    // check for done reading
    if( ft->stat_error < STAT_OPEN )  // ERR or EOF
        goto done;
    // still have data to read
    // append to existing data    
    int count = read (ft->handle, ft->buffer+ft->bufcnt, length);
    // It is not an error if read returns less than asked, it may have
    // been interupted or other things.  Return of 0 is end-of-file.
    if( count == -1 ) // error
    {
        ft->stat_error = ERR_RW; // read err
        goto done;
    }
   
    ft->bufcnt += count;
    if( count == 0 ) // EOF
        ft->stat_error = STAT_EOF;

done:   
    return ft->stat_error;
}

void FIL_ExtFile_Close ( ExtFIL_t * ft )
{
    if( ft->handle >= 0 )  // protect against second call when errors
    {
        close (ft->handle);
        ft->handle = -127;
        ft->stat_error = STAT_CLOSED;
    }
}


//
// checks if needed, and add default extension to filename
// in path[MAX_WADPATH]
void FIL_DefaultExtension (char *path, const char *extension)
{
    char    *src;
    // [WDJ] assume MAX_WADPATH buffer
    int  plen = strlen(path);
    if( plen > (MAX_WADPATH - 4) )   return;  // too long to add extension

  // search for '.' from end to begin, add .EXT only when not found
    src = path + plen - 1;

    while (*src != '/' && src != path)
    {
        if (*src == '.')
            return;                 // it has an extension
        src--;
    }

    strcat (path, extension);
}


// Point to start of the filename in longer string
char * FIL_Filename_of( char * nstr )
{
    int i;
    // point to start of filename only
    for (i = strlen(nstr) - 1; i >= 0; i--)
    {
      if (nstr[i] == '\\' || nstr[i] == '/' || nstr[i] == ':')
        break;
    }
    return &nstr[i+1];
}

#if 0
// [WDJ] Unused, was only used in old W_AddFile, makes DOS assumptions
// Uppercase only
//  Creates a resource name (max 8 chars 0 padded) from a file path
//
void FIL_ExtractFileBase ( char*  path,  char* dest )
{
    char*       src;
    int         length;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path
           && *(src-1) != '\\'
           && *(src-1) != '/')
    {
        src--;
    }

    // copy up to eight characters
    memset (dest,0,8);
    length = 0;

    while (*src && *src != '.')
    {
        if (++length == 9)
            I_Error ("Filename base of %s >8 chars",path);

        *dest++ = toupper((int)*src++);
    }
}
#endif


//  Returns true if a filename extension is found
//  There are no '.' in wad resource name
//
boolean FIL_CheckExtension (const char * in)
{
    while (*in++)
    {
        if (*in=='.')
            return true;
    }

    return false;
}


// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
// owned, malloc
char * configfile_main = NULL;
char * configfile_drawmode = NULL;
// true once config.cfg loaded, and values are present.
// Bit 0x80 is used to flag that some configfile was loaded.
// Bit 0x40 is used to flag empty CFG_main.
static byte config_loaded = 0;  // bit per cfg
static const byte  config_load_bit[ 4 ] = { 0, 0x01, 0x02, 0x04 };  // bit masks

void  M_Set_configfile_main( const char * filename )
{
    free( configfile_main );
    configfile_main = strdup( filename );
    config_loaded &= ~config_load_bit[CFG_main];  // clear flag bit
}

// This table will work even on compilers that do not
// combine identical const strings.
// index by drawmode : drawmode_e
const char * configfile_drawmode_designator[] = {
  "--", // no drawmode configfile
  "8p",  // DRM_8pal
  "15",  // DRM_15
  "16",  // DRM_16
  "24",  // DRM_24
  "32",  // DRM_32
  "bp",  // DRM_explicit_bpp,
  "n",   // DRM_native
#ifdef HWRENDER
  "gl",  // DRM_opengl
  "mg",  // DRM_minigl
#ifdef SMIF_WIN_NATIVE
  "wg",  // DRM_glide
  "wd",  // DRM_d3d
#endif
#endif
};

// name is usually limited to 8 char
#define  NAMEBUF_LEN  24

//  drawmode : drawmode_sel_t
void  M_Set_configfile_drawmode( byte drawmode )
{
    char cfgbuf[ MAX_WADPATH ];
    char namebuf[ NAMEBUF_LEN ];

    // Form drawmode config filename.
    // example: /home/user/.legacy/configgl.cfg
    const char * cfdd_str = configfile_drawmode_designator[ drawmode ];
    snprintf( namebuf, NAMEBUF_LEN-1, DRAWMODE_CONFIGFILENAME, cfdd_str );
    cat_filename( cfgbuf, legacyhome, namebuf );

    free( configfile_drawmode );
    configfile_drawmode = strdup( cfgbuf );
    config_loaded &= ~config_load_bit[CFG_drawmode];  // clear flag bit
}


byte  M_Have_configfile_drawmode( void )
{
    return  config_loaded & config_load_bit[ CFG_drawmode ];
}

void  M_Set_configfile_drawmode_present( void )
{
    config_loaded |= config_load_bit[ CFG_drawmode ];
}


// Save config file, without disturbing configfile settings.
void Command_SaveConfig_f (void)
{
    char cfgname[MAX_WADPATH];
    COM_args_t  carg;
    
    COM_Args( &carg );

    if (carg.num!=2)
    {
        CONS_Printf("saveconfig <filename[.cfg]> : save config to a file\n");
        return;
    }
    dl_strncpy(cfgname, carg.arg[1], MAX_WADPATH);
    if( cfgname[0] == 0 )  goto failed;

    FIL_DefaultExtension (cfgname,".cfg");

    M_SaveConfig( CFG_main, cfgname );

    // Also save the drawmode configfile, with NULL name check.
    M_SaveConfig( CFG_drawmode, configfile_drawmode );

    // Check that the file now exists.
    FILE * f = fopen (cfgname, "r");
    if( ! f ) goto failed;
    fclose( f );
   
    CONS_Printf("config saved as %s\n", cfgname );  // actual name saved
    return;

failed:
    I_SoftError("Could not save game config file %s\n", cfgname );
    return;
}

// Replace config values from a new config file.
void Command_LoadConfig_f (void)
{
    char cfgname[MAX_WADPATH];
    COM_args_t  carg;
    byte  namearg = 1;
    unsigned char  other_flag = 0;
    
    COM_Args( &carg );
    if( carg.num < 2 || carg.num > 3 )  goto bad_syntax;

    if( carg.arg[1][0] == '-' )
    {
        other_flag = carg.arg[1][1];
        namearg = 2;
    }
    else if( carg.arg[2][0] == '-' )
    {
        other_flag = carg.arg[2][1];
    }

    dl_strncpy(cfgname, carg.arg[namearg], MAX_WADPATH);
    if( cfgname[0] == 0 )  goto failed;

    FIL_DefaultExtension (cfgname,".cfg");
    //  for create, don't check if file exists

    // The original intent was to load a complete config file, replacing all.
    other_flag = tolower( other_flag );
    if( other_flag == 'a' )  // append
    {
        // Append config file settings.
        if( access(cfgname, R_OK ) < 0 )  goto failed;
        // Load additional config settings.  Do not save them to the main config.
        M_LoadConfig( CFG_other, cfgname );
    }
    else
    {
        M_ClearConfig( CFG_main );
        // Load config sets main config filename.
        M_Set_configfile_main( cfgname );
        // At program end, it will overwrite this config file with all the CFG_main settings.
        M_LoadConfig( CFG_main, cfgname );
    }
    return;

failed:
    I_SoftError("Could not load game config file %s\n", cfgname );
    return;
   
bad_syntax:
    CONS_Printf("loadconfig (-A) <filename[.cfg]> : load config from a file\n");
    return;
}

void Command_ChangeConfig_f (void)
{
    COM_args_t  carg;
    
    COM_Args( &carg );

    if (carg.num!=2)
    {
        CONS_Printf("changeconfig <filename[.cfg]> : save current config and load another\n");
        return;
    }

#if 1
    // Why be indirect when there is a direct method, with all the parameters.
    M_SaveAllConfig();
#else
    COM_BufAddText (va("saveconfig \"%s\"\n", configfile_main));
#endif
#if 1
    // Replace the main config with a different config file.
    M_ClearConfig( CFG_main );  // cleanup old values
#endif
    // indirect invoke of LoadConfig above.
    COM_BufAddText (va("loadconfig \"%s\"\n", carg.arg[1])); // -> configfile
}


// Clear the config
//   cfg : cv_config_e, source config file ident
void M_ClearConfig( byte cfg )
{
    config_loaded &= ~config_load_bit[cfg];  // clear flag bit
    CV_Clear_Config( cfg );
}

//
// Load a config file
//
//   cfg : cv_config_e, source config file ident
void M_LoadConfig( byte cfg, const char * cfgfile )
{
    FILE * fr;

    //  cfgfile is initialised by d_main when searching for the wad ?!
    if( ! cfgfile )  return;

    // Check that it can be opened.
    fr = fopen ( cfgfile, "r" );
    if( ! fr )
    {
        if( cfg == CFG_main )
            config_loaded |= 0x40;  // set flag bit, so can create CFG_main later
        return;
    }
    fclose( fr );
   
    COM_BufExecute( CFG_none );  // Clear buffer of any COM commands, before Loading

    // load config, make sure those commands doesnt require the screen..
    CONS_Printf("\n");
    COM_BufInsertText (va("exec \"%s\"\n", cfgfile));
    COM_BufExecute( cfg );       // make sure initial settings are done

    // make sure I_Quit() will write back the correct config
    // (do not write back the config if it crash before)
    config_loaded |= 0x80 | config_load_bit[cfg];  // set flag bit, and specific bit
}



//   cfg : cv_config_e
void M_SaveConfig( byte cfg, const char * cfgfile )
{
    FILE * fw;
    consvar_t * cv;
    // When CFG_main, also save CFG_none vars
    byte cfg2;

    if( ! cfgfile )
        return;

    // make sure not to write back the config until
    //  it's been correctly loaded
    if( ! config_loaded )
        return;

    // Write this config file if one was loaded,
    // or if there are some values of that config now.
    if( (cfg != CFG_main) // CFG_main always gets saved, has all CFG_none var too.
        && ! (config_loaded & config_load_bit[cfg])  // cfg was NOT loaded
        && ! CV_Config_check( cfg ) ) // NOT any of the cfg is (current or pushed).
        return;

    fw = fopen (cfgfile, "w");
    if (!fw)
    {
        I_SoftError("Could not save game config file %s\n", cfgfile);
        return;
    }

    // header message
    fprintf (fw, "// Doom Legacy configuration file.\n");

    //FIXME: save key aliases if ever implemented..

    // Save CV variables
    // The main configfile also gets the uninitialized variables (CFG_none).
    // There are no CFG_null variables.
    cfg2 = (cfg == CFG_main) ? CFG_none : CFG_null;
    for( cv = CV_IteratorFirst(); cv ; cv = CV_Iterator( cv ) )
    {
        if( cv->flags & CV_SAVE )
        {
            const char * str = NULL;
            byte cm = cv->state & CS_CONFIG;
            if( (cm == cfg) || (cm == cfg2) )
            {
                str = cv->string;
            }
            else if( cv->state & CS_PUSHED )
            {
                str = CV_Get_Config_string( cv, cfg );
            }
            else  continue;  // not current and not pushed

            if( ! str ) continue;
            // Save the cvar string
            fprintf( fw, "%s \"%s\"\n", cv->name, str );
        }
    }

    if( cfg == CFG_main )
        G_SaveKeySetting(fw);

    fclose (fw);
}

//  Save all game config here
void M_SaveAllConfig( void )
{
    // [Arcade] The cabinet does not persist settings: whatever a player
    // changes in the menus lasts only for that session, and the next launch
    // reloads the operator's baseline.  Run with -devmode to set that
    // baseline -- only then is the config written back.
    // High scores and record demos are separate files and are unaffected.
    if( ! devmode )
        return;

    M_SaveConfig( CFG_main, configfile_main );

    M_SaveConfig( CFG_drawmode, configfile_drawmode );
}



// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================

// Make filename for screen shot.
// return 1 when have filename.
byte  M_Make_Screenshot_Filename( char * lbmname, const char * ext )
{
    int i;
    char wadname[MAX_WADPATH];
    char * s;
    char * wn;
    char * vernum;
    const char * savedir;

    // Defaults
    savedir = ".";
    switch( gamemode )
    {
     case heretic :
       wn = "HRTC";
       break;
     case chexquest1 :
       wn = "CHXQ";
       break;
     default :
       wn = "DOOM";
       break;
    }

    if (cv_screenshot_dir.string[0]) // Hurdler: Jive's request (save file in other directory)
    {
        savedir = (const char *) cv_screenshot_dir.string;
        for (i=1; ; i++) // seach the first "real" wad file (also skip iwad).
        {
            char * wadfile = startupwadfiles[i];
            if( ! wadfile )  break;
            // Examine extension
            int pos = strlen(wadfile) - 4;
            if ((pos >= 0) && !strncmp(&wadfile[pos], ".wad", 4))
            {
                // Wad file name copied to screenshot name.
                strcpy(wadname, wadfile);
                wadname[pos] = '\0';  // eliminate wad extension
                wn = wadname;
                // Eliminate wad directory name
                s = strrchr(wn, '/');
                if( s )  wn = s + 1;
#if defined( SMIF_PC_DOS) || defined( WIN32 ) || defined( SMIF_OS2_NATIVE )
                s = strrchr(wn, '\\');  // DOS
                if( s )  wn = s + 1;
#endif
                break;
            }
        }
#if defined( __GNUC__ ) && ( __GNUC__ >= 12 )
	// [WDJ] GCC introduced their second guessing of every usage in ver 7.1, and since then
	// every programmer has to circumvent it.
	// With every version of GCC, I have to fix this code again, and it was never broken in the first place.
        // This truncation is intentional as we cannot ever supply an infinite buffer for arbitrary names.
	// Because we support other compilers, which behave better, this cannot just be thrown in the compile flags.
//# pragma GCC diagnostic push
# pragma GCC diagnostic ignored	  "-Wformat-truncation"
        snprintf(lbmname, MAX_WADPATH-1, "%s/%s0000.%s", cv_screenshot_dir.string, wn, ext );
//# pragma GCC diagnostic pop
#else
        snprintf(lbmname, MAX_WADPATH-1, "%s/%s0000.%s", cv_screenshot_dir.string, wn, ext );
#endif
        lbmname[MAX_WADPATH-1] = 0;
    }
    else
    {
        sprintf(lbmname, "%s0000.%s", wn, ext );
    }

    vernum = strrchr( lbmname, '.') - 4;
    if( ( strlen( lbmname ) > (MAX_WADPATH-2) )
        || vernum == NULL )
    {
        CONS_Printf("Screenshot directory or name too long: %s\n", lbmname );
        return 0;
    }

    if( access( savedir, F_OK ) < 0 )
    {
        CONS_Printf("Screenshot directory error: %s\n", savedir);
        return 0;
    }

    // Find unused file name version.
    for (i=0 ; i<10000; i++)
    {
        vernum[0] = '0' + ((i/1000) % 10);
        vernum[1] = '0' + ((i/100) % 10);
        vernum[2] = '0' + ((i/10) % 10);
        vernum[3] = '0' + ((i/1) % 10);
        if (access(lbmname, F_OK) == -1)
            return 1;      // file doesn't exist
    }

    CONS_Printf("Screenshot: Failed to find unused filename: %s\n", lbmname);
    return 0;
}


// Graphic File types
// pcx
// ppm
// Targa
// bmp (windows and DOS)
// png (requires LIBPNG)

#ifdef SMIF_PC_DOS

// PCX file format
#pragma pack(1)
typedef struct
{
    uint8_t       manufacturer;
    uint8_t       version;
    uint8_t       encoding;
    uint8_t       bits_per_pixel;

    uint16_t      xmin;
    uint16_t      ymin;
    uint16_t      xmax;
    uint16_t      ymax;

    uint16_t      hres;
    uint16_t      vres;

    uint8_t       palette[48];

    uint8_t       reserved;
    uint8_t       color_planes;
    uint16_t      bytes_per_line;
    uint16_t      palette_type;

    uint8_t       filler[58];
    uint8_t       data;           // unbounded
} pcx_t;
#pragma pack()


//
// WritePCXfile
//
boolean Write_PCXfile ( const char * file_name, int width, int height, byte* data, byte* palette )
{
    int         i;
    int         length;
    boolean     br;
    pcx_t*      pcx;
    byte*       pack;

    pcx = Z_Malloc (width*height*2+1000, PU_STATIC, NULL);

    pcx->manufacturer = 0x0a;           // PCX id
    pcx->version = 5;                   // 256 color
    pcx->encoding = 1;                  // uncompressed
    pcx->bits_per_pixel = 8;            // 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    // [WDJ] The PCX format must be little-endian, must swap when big-endian
    pcx->xmax = (uint16_t)( LE_SWAP16(width-1) );
    pcx->ymax = (uint16_t)( LE_SWAP16(height-1) );
    pcx->hres = (uint16_t)( LE_SWAP16(width) );
    pcx->vres = (uint16_t)( LE_SWAP16(height) );
    memset (pcx->palette,0,sizeof(pcx->palette));
    pcx->color_planes = 1;              // chunky image
    pcx->bytes_per_line = (uint16_t)( LE_SWAP16(width) );
    pcx->palette_type = (uint16_t)( LE_SWAP16(1) );   // Color (2=grey scale)
    memset (pcx->filler,0,sizeof(pcx->filler));


    // pack the image
    pack = &pcx->data;

    for (i=0 ; i<width*height ; i++)
    {
        if ( (*data & 0xc0) != 0xc0)
            *pack++ = *data++;
        else
        {
            *pack++ = 0xc1;
            *pack++ = *data++;
        }
    }

    // write the palette
    *pack++ = 0x0c;     // palette ID byte
    for (i=0 ; i<768 ; i++)
        *pack++ = *palette++;

    // write output file
    length = pack - (byte *)pcx;
    br = FIL_WriteFile (file_name, pcx, length);

    Z_Free (pcx);
    return br;
}

#endif



// --------------------------------------------------------------------------
// save screenshots with TGA format
// --------------------------------------------------------------------------
// This will not be packed under Linux, GNU, or WIN32, unless it is explicit.
#pragma pack(1)
typedef struct {  // sizeof() = 18
  byte      id_field_length;
  byte      color_map_type;
  byte      image_type;
  int16_t   c_map_origin;
  int16_t   c_map_length;
  byte      c_map_depth;
  int16_t   x_origin;
  int16_t   y_origin;
  uint16_t  width;
  uint16_t  height;
  byte      image_pix_size;  // 16, 24, 32
  byte      image_descriptor;
} TGAHeader_t;
#pragma pack()

boolean  Write_TGA( const char * filename, int width, int height, int bitpp, byte* colormap, byte* buffer, size_t size )
{
    int fd;
    size_t count = 0;
    TGAHeader_t tga_hdr;

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (fd < 0)
        return false;

    memset(&tga_hdr, 0, sizeof(tga_hdr));
    // TGA format is little-endian
    tga_hdr.width = (uint16_t)( LE_SWAP16(width) );
    tga_hdr.height = (uint16_t)( LE_SWAP16(height) );
    tga_hdr.image_pix_size = bitpp;  // normal, 24 bits per pixel
    tga_hdr.image_type = 2;  // Uncompressed, RGB
    tga_hdr.image_descriptor = 0x20;  // bit 5, origin in upper left-hand corner
   
    if( colormap )
    {
        tga_hdr.image_type = 1;  // Uncompressed, colormap image
        tga_hdr.color_map_type = 1;
        tga_hdr.c_map_origin = 0;
        tga_hdr.c_map_length = LE_SWAP16( 256 );
        tga_hdr.c_map_depth = 24;
    }
   
    count = write(fd, &tga_hdr, sizeof(TGAHeader_t));
    if( count != sizeof(TGAHeader_t) )  goto write_fail;

    if( colormap )
    {
        count = write(fd, colormap, 256*3);
        if( count != 256*3 )  goto write_fail;
    }

    count = write(fd, buffer, size);
    if( count != size )  goto write_fail;

    close(fd);
    return true;

write_fail:
    close(fd);
    return false;
}


// indexed by drawmode:  DRAW8PAL, DRAW15, DRAW16, DRAW24, DRAW32

//
// M_ScreenShot
//
void M_ScreenShot (void)
{
    char   filename[MAX_WADPATH];
    // vid : from video setup
    byte*  bufs;  // source buffer ( screen[2] or hw_bufr )
    byte*  hw_bufr = NULL;  // allocated
    byte*  bufw = NULL;  // allocated
    byte*  bp;
    int i;
    int num_pixels = vid.width * vid.height;
    size_t  bufsize;
    byte   wr_bytepp = 3;
    byte   src_bitpp;
    boolean  br = false;

#ifdef HWRENDER
    if (rendermode!=render_soft)
    {
        // Hardware draw.
        // Save as Targa format.
        hw_bufr = HWR_Get_Screenshot( & src_bitpp );
        if( ! hw_bufr )  goto done;
        bufs = hw_bufr;
    }
    else
#endif
    {
        // Software draw.
        // munge planar buffer to bufs
        bufs = screens[2];  // Take screenshot to screens[2]
        I_ReadScreen (bufs);
        src_bitpp = vid.bitpp;
   
        if( vid.ybytes != vid.width )
        {
            // eliminate padding in the buffer
            byte *dest, *src;
            dest = src = &bufs[0];
            for( i=1; i<vid.height; i++ )
            {
                src += vid.ybytes;
                dest += vid.widthbytes;
                // overlapping copy
                memmove(dest, src, vid.width);
            }
        }
    }

#ifdef SMIF_PC_DOS
    if( (vid.drawmode == DRAW8PAL) && (cv_screenshot_type.EV == 8) )
    {
        // Save screenshot in PCX format

        if( ! M_Make_Screenshot_Filename( filename, "pcx" ) )
            return;

        GenPrintf( EMSG_ver, "Save PCX: %s\n", filename );
       
        // save the pcx file
        br = Write_PCXfile ( filename, vid.width, vid.height, bufs,
                            W_CacheLumpName ("PLAYPAL",PU_CACHE));
        goto done;
    }
#endif
   
    // Software draw, Targa format.
    // Targa allows 24 bit, 32 bit (with alpha), and 16 bit (5,5,5) formats.

    if( ! M_Make_Screenshot_Filename( filename, "tga" ) )
        return;

    GenPrintf( EMSG_ver, "Save Targa: %s\n", filename );
//    printf("Write Targa %s, drawmode=%i, wr_bytepp= %i, bitpp= %i\n", filename, vid.drawmode, wr_bytepp, wr_bytepp*8 );
    bufsize = (size_t)num_pixels * wr_bytepp;
    bufw = malloc( bufsize );
    if (!bufw)  goto done;

    bp = bufw;

    // conversions
    switch( src_bitpp )
    {
     case 8:
        {
            // PAL 8 bit format.
            // To Targa 8 bit, color mapped.
            byte  pal24[256*3];
            RGBA_t * pal32 = pLocalPalette;

            // Convert palette from RGBA to 24bit RGB.
            bp = pal24;
            for (i=0; i<256; i++)
            {
                *(bp++) = pal32[i].s.blue;
                *(bp++) = pal32[i].s.green;
                *(bp++) = pal32[i].s.red;
            }
      
            br = Write_TGA( filename, vid.width, vid.height, 8, pal24, bufs, num_pixels );
            goto done;
        }

#ifdef ENABLE_DRAW15
     case 15:
        {
            // Screen (5,5,5) format.
            uint16_t * r16 = (uint16_t*) bufs;
            if(cv_screenshot_type.EV == 1)  // Full
            {
                for (i=0; i<num_pixels; i++)
                {
                    // Convert 15bit 555 RGB to 24 bit RGB.
                    uint16_t rgb555 = *(r16++);
                    *(bp++) = (rgb555 & 0x001F) << 3;
                    *(bp++) = (rgb555 & 0x03E0) >> (5-3);
                    *(bp++) = (rgb555 & 0x7C00) >> (10-3);
                }
                wr_bytepp = 3;
            }
            else
            {   // compact
                // To Targa 16 bit, (5,5,5) format.
                uint16_t * w16 = (uint16_t*) bufw;
                for (i=0; i<num_pixels; i++)
                {
                    // Convert 15bit 555 RGB to Targa 15 bit RGB.
                    // Set alpha channel (0x8000)
                    *(w16++) = *(r16++) | 0x8000;
                }
                wr_bytepp = 2;
            }
        }
        break;
#endif
#if defined( ENABLE_DRAW16 ) || defined( HWRENDER )
     // HWRENDER Glide uses this.
     case 16:
        {
            // Screen (5,6,5) format.
            uint16_t * r16 = (uint16_t*) bufs;
            if(cv_screenshot_type.EV == 1)  // Full
            {
                // To Targa 24 bit format.
                for (i=0; i<num_pixels; i++)
                {
                    // Convert 16bit 565 RGB to 24 bit RGB.
                    uint16_t rgb565 = *(r16++);
                    *(bp++) = (rgb565 & 0x001F) << 3;
                    *(bp++) = (rgb565 & 0x07E0) >> (5-2);
                    *(bp++) = (rgb565 & 0xF800) >> (11-3);
                }
                wr_bytepp = 3;
            }
            else
            {   // compact
                // To Targa 16 bit, (5,5,5) format.
                uint16_t * w16 = (uint16_t*) bufw;
                for (i=0; i<num_pixels; i++)
                {
                    // Convert 16bit 565 RGB to Targa 15 bit RGB.
                    uint16_t rgb565 = *(r16++);
                    // Set alpha channel (0x8000)
                    *(w16++) = (rgb565 & 0x001F) | ((rgb565 >> 1) & 0x7FE0) | 0x8000;
                }
                wr_bytepp = 2;
            }
        }
        break;
#endif
#if defined( ENABLE_DRAW24 ) || defined( HWRENDER )
     // HWRENDER OpenGL uses this.
     case 24:
        // Screen 3 byte format.
        // Already in Targa 3 byte format.
        memcpy( bufw, bufs, bufsize );
        break;
#endif
#ifdef ENABLE_DRAW32
     case 32:
        // Screen 4 byte format.
        for (i=0; i<num_pixels; i++)
        {
            *(bp++) = *(bufs++);
            *(bp++) = *(bufs++);
            *(bp++) = *(bufs++);
            bufs++;  // alpha
        }
        break;
#endif
     default:
        goto done;
    }

    br = Write_TGA( filename, vid.width, vid.height, wr_bytepp*8, NULL, bufw, bufsize );

done:   
    if( bufw )  free(bufw);
    if( hw_bufr )  free(hw_bufr);

    if( br )
        CONS_Printf("screen shot %s saved\n", filename);
    else
        //CONS_Printf("Couldn't create screen shot\n");
        CONS_Printf("%s\n", filename);
}


// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================


//  Temporary varargs for COM_Buf and CONS_Printf usage
//  COM_BufAddText( va( "format", args ) )
//
// Buffer returned by va(), for every caller
#define VA_BUF_SIZE 1024
static char  va_buffer[VA_BUF_SIZE];
//
char*   va(const char *format, ...)
{
    va_list      argptr;

    va_start(argptr, format);
    vsnprintf(va_buffer, VA_BUF_SIZE, format, argptr);
    va_buffer[VA_BUF_SIZE-1] = '\0'; // term, when length limited
    va_end(argptr);

    return va_buffer;
}


// creates a copy of a string, null-terminated
// returns ptr to the new duplicate string
//
char *Z_StrDup (const char *in)
{
    char    *out;

    out = Z_Malloc (strlen(in)+1, PU_STATIC, NULL);
    strcpy (out, in);
    return out;
}

// dest must be filename buffer of MAX_WADPATH
// If directory dn does not end in '/', then a separator will be included.
void cat_filename( char * dest, const char * dn, const char * fn )
{
    const char * format = "%s%s";
    int dnlen = strlen( dn );
    if( dnlen )
    {
        // if directory does not have '/' then include one in format
        char ch = dn[ dnlen-1 ]; // last char
        if( ! ( ch == '/' || ch == '\\' ))   format = "%s/%s";
    }
    snprintf(dest, MAX_WADPATH-1, format, dn, fn);
    dest[MAX_WADPATH-1] = '\0';
}

#if 0
// [WDJ] No longer used
// s1=s2+s3+s1
void strcatbf(char *s1, const char *s2, const char *s3)
{
    char tmp[1024];

    strcpy(tmp,s1);
    strcpy(s1,s2);
    strcat(s1,s3);
    strcat(s1,tmp);
}
#endif

// A strncpy that gcc10 does not complain about.
// This truncates the src when the dest buffer is full.
// Will always write the terminating 0.
// Does not pad the dest, like strncpy does.
//  destsize: 1..  must have room for term 0
void dl_strncpy( char * dest, const char * src, int destsize )
{
    char * ep = dest + destsize - 1;
    while( (dest < ep) && (* src) )
    {
        * (dest++) = * (src++);
    }
    * dest = 0;
}

//#if defined( __MINGW32__ ) || defined( __WATCOM__ ) || defined( SMIF_X11 )
#if 1
// For systems that are missing strcasestr
char * dl_strcasestr( const char * haystack,  const char * needle )
{
    // Empty needle will match anything.
    int vcnt = strlen( haystack ) - strlen( needle );
    while( vcnt-- >= 0 )
    {
        const char * e = needle;
        const char * h = haystack;

        for(;;)
        {
            unsigned char ce = * (e ++);
            unsigned char ch = * (h ++);
            if( ce == 0 )  goto found;
            if( ch == 0 )  goto not_found;  // haystack shorter than needle
            if( tolower(ce) != tolower(ch) )  break;
        }
        haystack++;
    }

not_found:
    // not found
    return NULL;
   
found:
    return (char*) haystack;
}
#endif


#ifdef MBF21
// MBF21 requires handling numeric bits, in base 10, hex, octal.
uint32_t str_to_uint32( const char * str )
{
    uint32_t val = 0;
    if(! (sscanf( str, "0x%x", &val ) == 1)
      || (sscanf( str, "0X%X", &val ) == 1)
      || (sscanf( str, "0%o", &val ) == 1) )
    {
        sscanf( str, "%u", &val ) ;
    }
    return val;
}
#endif
