// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Vid_vesa.c 1650 2023-10-25 02:25:41Z wesleyjohnson $
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
//
// $Log: Vid_vesa.c,v $
// Revision 1.5  2001/03/03 06:17:34  bpereira
//
// Revision 1.4  2001/01/25 22:15:45  bpereira
// added heretic support
//
// Revision 1.3  2000/08/31 14:30:57  bpereira
// Revision 1.2  2000/02/27 00:42:11  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      extended vesa VESA2.0 video modes i/o.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#include "doomincl.h"
  // MAXVIDWIDTH, MAXVIDHEIGHT
  // I_Error(), CONS_Printf
//#include "i_system.h"
#include "Vid_vesa.h"
#include "screen.h"

#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <sys/segments.h>
#include <sys/nearptr.h>

#include "console.h"
#include "command.h"            //added:21-03-98: vid_xxx commands
#include "v_video.h"
#include "i_video.h"


// PROTOS
static vmode_t *VID_GetModePtr (modenum_t modenum);
static int  VID_VesaGetModeInfo (int modenum, byte gmi_req_bitpp);
void VID_VesaGetExtraModes ( byte xm_req_bitpp );
int  VID_VesaInitMode (viddef_t *lvid, vmode_t *currentmode_p);

void VID_Command_NumModes_f (void);
void VID_Command_ModeInfo_f (void);
void VID_Command_ModeList_f (void);
void VID_Command_Mode_f (void);


#define VBEVERSION      2       // we need vesa2 or higher

// -----------------------------------------------------
#define MASK_LINEAR(addr)     (addr & 0x000FFFFF)
#define RM_TO_LINEAR(addr)    (((addr & 0xFFFF0000) >> 12) + (addr & 0xFFFF))
#define RM_OFFSET(addr)       (addr & 0xF)
#define RM_SEGMENT(addr)      ((addr >> 4) & 0xFFFF)
// -----------------------------------------------------

static int totalvidmem;
static byte  vesa_bitpp;
static vmode_t      vesa_modes[MAX_VESA_MODES] = {{NULL, NULL}};
static vesa_extra_t vesa_extra[MAX_VESA_MODES];

static int VGA_InitMode (viddef_t *lvid, vmode_t *currentmode_p);

//this is the only supported non-vesa mode : standard 320x200x256c.
#define NUM_VGA_VIDMODES  1
// There is only 1 vidmode, but the size of the array must include mode 0.
static vmode_t      vga_vidmodes[NUM_VGA_VIDMODES+1] = {
  { // 0 mode, HIDDEN
    & vga_vidmodes[1],  // next
    "Initial",
    INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT,
    INITIAL_WINDOW_WIDTH, 1,     // rowbytes, bytes per pixel
    MODE_window, 1,  // windowed, numpages
    NULL,
    VGA_InitMode
  },
  {
    NULL,  // next
    "320x200",
    320, 200,  //(200.0/320.0)*(320.0/240.0),
    320, 1,    // rowbytes, bytes per pixel
    MODE_window, 1,
    NULL,
    VGA_InitMode
  }
};

static char         names[MAX_VESA_MODES][10];

//----------------------------i_video.c------------------------------------
// these ones should go to i_video.c, but I prefer keep them away from the
// doom sources until the vesa stuff is ok.
int     num_all_vidmodes = 0;   // total number of all vidmodes
int     num_full_vidmodes = 0;  // total number of video modes, vga, vesa1, vesa2
vmode_t * all_vidmodes = NULL;  // start of videomodes list (window and fullscreen)
vmode_t * full_vidmodes = NULL; // start of fullscreen vidmodes
vmode_t * currentmode_p = NULL; // the current active videomode.
//----------------------------i_video.c------------------------------------


// table des modes videos.
// seul le mode 320x200x256c standard VGA est support‚ sans le VESA.
// ce mode est le mode num‚ro 0 dans la liste.
typedef struct
{
    int modenum;            // vesa vbe2.0 modenum
    int mode_attributes;
    int winasegment;
    int winbsegment;
    int bytes_per_scanline; // bytes per logical scanline (+16)
    int win;                // window number (A=0, B=1)
    int win_size;           // window size (+6)
    int granularity;        // how finely i can set the window in vid mem (+4)
    int width, height;      // displayed width and height (+18, +20)
    int bits_per_pixel;     // er, better be 8, 15, 16, 24, or 32 (+25)
    int bytes_per_pixel;    // er, better be 1, 2, or 4
    int memory_model;       // and better be 4 or 6, packed or direct color (+27)
    int num_pages;          // number of complete frame buffer pages (+29)
    int red_width;          // the # of bits in the red component (+31)
    int red_pos;            // the bit position of the red component (+32)
    int green_width;        // etc.. (+33)
    int green_pos;          // (+34)
    int blue_width;         // (+35)
    int blue_pos;           // (+36)
    int pptr;
    int pagesize;
    int numpages;
} modeinfo_t;

static vbeinfoblock_t vesainfo;
static vesamodeinfo_t vesamodeinfo;

// ------------------------------------------------------------------------
// DOS stuff
// ------------------------------------------------------------------------
static unsigned long conventional_memory = -1;

void map_in_conventional_memory(void)
{
    if (conventional_memory == -1)
    {
        if (__djgpp_nearptr_enable())
        {
            conventional_memory = __djgpp_conventional_base;
        }
    }
}

// Converts a flat 32 bit ptr to a realmode 0x12345 type ptr (seg<<4 + offs)
unsigned int ptr2real(void *ptr)
{
    map_in_conventional_memory();
    return (int)ptr - conventional_memory;
}

// Converts 0x12345 (seg<<4+offs) realmode ptr to a flat 32bit ptr
void *real2ptr(unsigned int real)
{
    map_in_conventional_memory();
    return (void *) (real + conventional_memory);
}


/* ======================================================================== */
// Add the standard VGA video modes to the video modes list.
/* ======================================================================== */
// mode 0 and 1 are used for windowed console startup.
void VID_InitVGAModes(void)
{
    // do not include Mode 0 (INITIAL) in count
    all_vidmodes = &vga_vidmodes[0];
    num_all_vidmodes = NUM_VGA_VIDMODES;  // count
    num_full_vidmodes = 0;
}

static void append_full_vidmodes( vmode_t newmodes, int nummodes )
{
    full_vidmodes = newmodes;
    vga_vidmodes[NUM_VGA_VIDMODES].next = newmodes;  // link in last entry

    num_full_vidmodes += nummodes;
    num_all_vidmodes += nummodes;
}


static inline
int  VID_NumModes(void)
{
    return ( vid.fullscreen )? num_full_vidmodes : num_all_vidmodes;
}

//   request_drawmode : vid_drawmode_e
//   request_fullscreen : true if want fullscreen modes
//   request_bitpp : bits per pixel
// Return true if there are viable modes.
boolean  VID_Query_Modelist( byte request_drawmode, byte request_fullscreen, byte request_bitpp )
{
    // Require modelist before rendermode is set.
    if( request_drawmode >= DRM_opengl )
        return false;
   
    if( request_fullscreen )
    {
        full_vidmodes = NULL;
        currentmode_p = NULL;
        num_full_vidmodes = 0;
        // get available display modes for the device
        VID_VesaGetExtraModes ( request_bitpp );
        if( num_full_vidmodes )  return true;
    }
    else
    {
        if( num_all_vidmodes )
            return true;
    }
    return false;
}

// modetype is of modetype_e
range_t  VID_ModeRange( byte modetype )
{
    range_t  mrange = { 1, 1 };
    // INITIAL_WINDOW mode 0 is not included
    if(modetype == MODE_fullscreen)
    {   // fullscreen  2..
        // do not include window modes
        mrange.first = NUM_VGA_VIDMODES+1;
        mrange.last = mrange.first + num_full_vidmodes;
    }
    else
    {   // window   1..
        mrange.last = num_all_vidmodes;
    }
    return mrange;
}


//added:21-03-98: return info on video mode
static
char * VID_ModeInfo (modenum_t modenum, char **ppheader)
{
    static char *badmodestr = "Bad video mode number\n";
    vmode_t     *pv;

    pv = VID_GetModePtr (modenum);

    if (!pv)
    {
        if (ppheader)
            *ppheader = NULL;
        return badmodestr;
    }
    else
    {
        //if (ppheader)
        //    *ppheader = pv->header;
        return pv->name;
    }
}



//added:03-02-98: return a video mode number from the dimensions
//   rmodetype : modetype_e
// Returns MODE_NOP when none found
modenum_t  VID_GetModeForSize( int rw, int rh, byte rmodetype )
{
    modenum_t  modenum = { MODE_NOP, 0 };
    int mi = 1;  // first window mode
    int tdist = INT_MAX;
    int bestdist;
    vmode_t * pv = all_vidmodes;

    if( modetype == MODE_fullscreen )
    {
        if( num_full_vidmodes == 0 )  goto done;
        // do not include window modes
        mi = NUM_VGA_VIDMODES+1;  // fullscreen modes start after
        pv = full_vidmodes;
    }
    for ( ; pv!=NULL; pv=pv->next )
    {
        tdist = abs(pv->width - rw) + abs(pv->height - rh);
        // find closest dist
        if( bestdist > tdist )
        {
            bestdist = tdist;
            if( tdist == 0 )  break;   // found exact match
        }
        mi++;
    }
    modenum.index = mi;
    modenum.modetype = rmodetype;

done:
    return modenum;
}


/* ======================================================================== */
//
/* ======================================================================== */
// only called once
void VID_Init (void)
{
    COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f, CC_info);
    COM_AddCommand ("vid_modeinfo", VID_Command_ModeInfo_f, CC_info);
    COM_AddCommand ("vid_modelist", VID_Command_ModeList_f, CC_info);
    COM_AddCommand ("vid_mode", VID_Command_Mode_f, CC_config);
}

// Get Fullscreen, VESA modes, append to VGA window modes
int VID_GetModes ( byte request_drawmode, byte select_bitpp )
{
    // setup the video modes list,
    // note that mode 0 must always be VGA mode 0x13
    full_vidmodes = NULL;
    currentmode_p = NULL;
    num_full_vidmodes = 0;
   
    // setup the vesa_modes list
    VID_VesaGetExtraModes ( select_bitpp );

    if (num_full_vidmodes==0)
        goto no_modes;
   
    // the game boots in 320x200 standard VGA, but
    // we need a highcolor mode to run the game in highcolor
    if( select_bitpp > 8 && num_full_vidmodes == 0)
    {
        GenPrintf( EMSG_info, "No highcolor VESA2 video mode found, cannot run in highcolor.\n");
//        VID_VesaGetExtraModes ( 8 );
        goto no_modes; // must return correct signaling to v_video logic
    }
    return 1;

no_modes:
    return FAIL_select;
}


// ========================================================================
// Returns a vmode_t from the video modes list, given a video mode number.
// ========================================================================
static
vmode_t * VID_GetModePtr (modenum_t modenum)
{
    // first mode in all_vidmodes is the HIDDEN INITIAL_WINDOW
    vmode_t * pv = all_vidmodes;  // window
    int mi = modenum.index;      // 0..

    if ( modenum.modetype == MODE_fullscreen )
    {
        pv = full_vidmodes;
        mi = modenum.index - NUM_VGA_VIDMODES;  // 2..
    }
    if (!pv || mi < 0 )  goto fail;

    while (mi--)
    {
        pv = pv->next;
        if (!pv)  goto fail;
    }
    return pv;
fail:
    return NULL;
}


modestat_t  VID_GetMode_Stat( modenum_t modenum )
{
    modestat_t  ms;

    // fullscreen and window modes  1..
    vmode_t * pv = VID_GetModePtr(modenum);
    if( pv )
    {
        ms.width = pv->width;
        ms.height = pv->height;
        ms.type = MODE_either;
        ms.mark = "";
    }
    else
    {
        ms.type = MODE_NOP;
        ms.width = ms.height = 0;
        ms.mark = NULL;
    }
    return ms;
}


//added:30-01-98:return the name of a video mode
char * VID_GetModeName( modenum_t modenum )
{
    // fullscreen and window modes  1..
    vmode_t *pv = VID_GetModePtr(modenum);
    return (pv)? pv->name : NULL;
}


// ========================================================================
// Sets a video mode
// ========================================================================
// Returns FAIL_end, FAIL_create, of status_return_e, 1 on success;
int VID_SetMode (modenum_t modenum)
{
    int     stat;
    vmode_t *newmode_p, *oldmode_p;
    viddef_t oldvid = vid;   // to back out
    boolean  set_fullscreen = (modenum.modetype == MODE_fullscreen);
    range_t  range = VID_ModeRange( modenum.modetype );


    if ((modenum.index > range.last) || (modenum.index < range.first))
    {
        if (currentmode_p == NULL)
        {
            modenum.index = 0;    // revert to the default base vid mode
        }
        else
        {
            I_SoftError ("Unknown video mode\n");
            return  FAIL_end;
        }
    }

    newmode_p = VID_GetModePtr (modenum);
    if( newmode_p == NULL )
       return  FAIL_end;

    // dont switch to the same display mode
    if (newmode_p == currentmode_p)   goto done;

    // initialize the new mode
    oldmode_p = currentmode_p;
    currentmode_p = newmode_p;

    vid.draw_ready = 0;  // disable print reaching console

    // initialize vidbuffer size for setmode_func
    vid.width  = currentmode_p->width;
    vid.height = currentmode_p->height;
    vid.bytepp = currentmode_p->bytesperpixel;
    vid.bitpp = (vid.bytepp==1)? 8:15;

    stat = (*currentmode_p->setmode_func) (&vid, currentmode_p);
      // sets vid.direct, vid.buffer, vid.display, vid.ybytes, vid.screen_size, vid.screen1
    if (stat < 0)
    {
        if (stat == FAIL_create)
        {
            // hardware could not setup mode
            //if (VID_SetMode (vid.modenum) < 0)
            //    I_Error ("VID_SetMode: couldn't set video mode (hard failure)");
            I_SoftError("Couldn't set video mode %d\n", modenum);
        }
        else
        if (stat == FAIL_memory)
        {
            I_SoftError ("Not enough mem for VID_SetMode...\n");
        }
        // not enough memory; just put things back the way they were
        currentmode_p = oldmode_p;
        // cannot just copy oldvid because of buffer pointers that
        // are no longer valid
        vid.width  = oldvid.width;
        vid.height = oldvid.height;
        vid.bytepp = oldvid.bytepp;
        vid.bitpp = oldvid.bitpp;
        (*currentmode_p->setmode_func) (&vid, currentmode_p);
        return FAIL_create;
    }

    vid.widthbytes = vid.width * vid.bytepp;
    vid.drawmode = (vid.bytepp==1)? DRAW8PAL:DRAW15;
    vid.direct_rowbytes = currentmode_p->rowbytes;
    vid.direct_size = vid.direct_rowbytes * vid.height;
    vid.modenum = modenum;
    vid.fullscreen = set_fullscreen;

#ifdef DEBUG
    debug_Printf("after VID_SetMode\n");
    debug_Printf("vid.width    %d\n",vid.width);
    debug_Printf("vid.height   %d\n",vid.height);
    debug_Printf("vid.buffer   %x\n",vid.buffer);
    debug_Printf("vid.rowbytes %d\n",vid.rowbytes);
    debug_Printf("vid.numpages %d\n",vid.numpages);
    debug_Printf("vid.recalc   %d\n",vid.recalc);
    debug_Printf("vid.direct   %x\n",vid.direct);
#endif
    //debug
    //if (vid.rowbytes != vid.width)
    //    I_Error("vidrowbytes (%d) <> vidwidth(%d)\n",vid.rowbytes,vid.width);

    //printf ("%s\n", VID_ModeInfo (vid.modenum, NULL));

    //added:20-01-98: recalc all tables and realloc buffers based on
    //                vid values.
    vid.recalc = 1;

 done:
    return 1;
}



// converts a segm:offs 32bit pair to a 32bit flat ptr
void *VID_ExtraFarToLinear (void *ptr)
{
    int     temp;

    temp = (int)ptr;
    return real2ptr (((temp & 0xFFFF0000) >> 12) + (temp & 0xFFFF));
}



// ========================================================================
// Helper function for VID_VesaGetExtraModes
// In:  vesa mode number, from the vesa videomodenumbers list
// Out: false, if no info for given modenum
// ========================================================================
static
int VID_VesaGetModeInfo (int vesa_modenum, byte gmi_req_bitpp)
{
    int     bytes_per_pixel;
    int     i;
    __dpmi_regs regs;

    for (i=0; i<sizeof(vesamodeinfo_t); i++)
        _farpokeb(_dos_ds, MASK_LINEAR(__tb)+i, 0);

    regs.x.ax = 0x4f01;
    regs.x.di = RM_OFFSET(__tb);
    regs.x.es = RM_SEGMENT(__tb);
    regs.x.cx = vesa_modenum;
    __dpmi_int(0x10, &regs);
    if (regs.h.ah)
        return false;
    else
    {
        dosmemget (MASK_LINEAR(__tb), sizeof(vesamodeinfo_t), &vesamodeinfo);

        bytes_per_pixel = (vesamodeinfo.BitsPerPixel+7)/8;

#if 1
        if (vesamodeinfo.BitsPerPixel != gmi_req_bitpp)
            return false;
#else
        // we add either highcolor or lowcolor video modes, not the two
        if (highcolor && (vesamodeinfo.BitsPerPixel != 15))
            return false;
        if (!highcolor && (vesamodeinfo.BitsPerPixel != 8))
            return false;
#endif

#if 1       
        if ((bytes_per_pixel > 4) ||
#else
        if ((bytes_per_pixel > 2) ||
#endif
            (vesamodeinfo.XResolution > MAXVIDWIDTH) ||
            (vesamodeinfo.YResolution > MAXVIDHEIGHT))
        {
            return false;
        }

        // we only want color graphics modes that are supported by the hardware
        if ((vesamodeinfo.ModeAttributes &
            (MODE_SUPPORTED_IN_HW | COLOR_MODE | GRAPHICS_MODE) ) !=
             (MODE_SUPPORTED_IN_HW | COLOR_MODE | GRAPHICS_MODE))
        {
            return false;
        }

        // we only work with linear frame buffers, except for 320x200,
        // which is linear when banked at 0xA000
        if (!(vesamodeinfo.ModeAttributes & LINEAR_FRAME_BUFFER))
        {
            if ((vesamodeinfo.XResolution != 320) ||
                (vesamodeinfo.YResolution != 200))
            {
                return false;
            }
        }

        // pagesize
        if ((vesamodeinfo.BytesPerScanLine * vesamodeinfo.YResolution)
             > totalvidmem)
        {
            return false;
        }

        vesamodeinfo.NumberOfImagePages = 1;


#ifdef DEBUG
        debug_Printf("VID: (VESA) info for mode 0x%x\n", modeinfo.modenum);
        debug_Printf("  mode attrib = 0x%0x\n", modeinfo.mode_attributes);
        debug_Printf("  win a attrib = 0x%0x\n", *(unsigned char*)(infobuf+2));
        debug_Printf("  win b attrib = 0x%0x\n", *(unsigned char*)(infobuf+3));
        debug_Printf("  win a seg 0x%0x\n", (int) modeinfo.winasegment);
        debug_Printf("  win b seg 0x%0x\n", (int) modeinfo.winbsegment);
        debug_Printf("  bytes per scanline = %d\n",
                modeinfo.bytes_per_scanline);
        debug_Printf("  width = %d, height = %d\n", modeinfo.width,
                modeinfo.height);
        debug_Printf("  win = %c\n", 'A' + modeinfo.win);
        debug_Printf("  win granularity = %d\n", modeinfo.granularity);
        debug_Printf("  win size = %d\n", modeinfo.win_size);
        debug_Printf("  bits per pixel = %d\n", modeinfo.bits_per_pixel);
        debug_Printf("  bytes per pixel = %d\n", modeinfo.bytes_per_pixel);
        debug_Printf("  memory model = 0x%x\n", modeinfo.memory_model);
        debug_Printf("  num pages = %d\n", modeinfo.num_pages);
        debug_Printf("  red width = %d\n", modeinfo.red_width);
        debug_Printf("  red pos = %d\n", modeinfo.red_pos);
        debug_Printf("  green width = %d\n", modeinfo.green_width);
        debug_Printf("  green pos = %d\n", modeinfo.green_pos);
        debug_Printf("  blue width = %d\n", modeinfo.blue_width);
        debug_Printf("  blue pos = %d\n", modeinfo.blue_pos);
        debug_Printf("  phys mem = %x\n", modeinfo.pptr);
#endif
    }

    return true;
}


// ========================================================================
// Get extended VESA modes information, keep the ones that we support,
// so they'll be available in the game Video menu.
// ========================================================================
#define MAXVESADESC 100
static char vesadesc[MAXVESADESC] = "";

// append modes to modelist
void VID_VesaGetExtraModes ( byte select_bitpp )
{
    int             i;
    int             nummodes;
    unsigned long   addr;
    __dpmi_meminfo  phys_mem_info;
    unsigned long   mode_ptr;
    __dpmi_regs     regs;

    // make a copy of the video modes list! else trash in __tb
    unsigned short  vmode[MAX_VESA_MODES+1];
    int             numvmodes;
    unsigned short  vesamode;

    // new ugly stuff...
    for (i=0; i<sizeof(vbeinfoblock_t); i++)
       _farpokeb(_dos_ds, MASK_LINEAR(__tb)+i, 0);

    dosmemput("VBE2", 4, MASK_LINEAR(__tb));

    // see if VESA support is available
    regs.x.ax = 0x4f00;
    regs.x.di = RM_OFFSET(__tb);
    regs.x.es = RM_SEGMENT(__tb);
    __dpmi_int(0x10, &regs);
    if (regs.h.ah)
        goto no_vesa;

    dosmemget(MASK_LINEAR(__tb), sizeof(vbeinfoblock_t), &vesainfo);

    if (strncmp(vesainfo.VESASignature, "VESA", 4) != 0)
        goto no_vesa;

    if (vesainfo.VESAVersion < (VBEVERSION<<8))
        goto wrong_vesa;

    //
    // vesa version number
    //
    CONS_Printf ("%4.4s %d.%d (", vesainfo.VESASignature,
                                  vesainfo.VESAVersion>>8,
                                  vesainfo.VESAVersion&0xFF);

    //
    // vesa description string
    //
    i = 0;
    addr = RM_TO_LINEAR(vesainfo.OemStringPtr);
    _farsetsel(_dos_ds);
    while (_farnspeekb(addr) != 0)
    {
        vesadesc[i++] = _farnspeekb(addr++);
        if (i == MAXVESADESC-1)
            break;
    }
    vesadesc[i]=0;
    CONS_Printf ("%s)\n",vesadesc);

    totalvidmem = vesainfo.TotalMemory << 16;

    vesa_bitpp = select_bitpp;

   //
   // find 8 bit modes
   //
    numvmodes = 0;
    mode_ptr = RM_TO_LINEAR(vesainfo.VideoModePtr);
    while ((vmode[numvmodes] = _farpeekw(_dos_ds, mode_ptr)) != 0xFFFF)
    {
        numvmodes++;
        if( numvmodes == MAX_VESA_MODES )
            break;
        mode_ptr += 2;
    }
    vmode[numvmodes] = 0xffff;

    nummodes = 0;       // number of video modes accepted for the game

    numvmodes=0;  // go again through vmodes table
    while ( ((vesamode=vmode[numvmodes++]) != 0xFFFF) && (nummodes < MAX_VESA_MODES) )
    {
        //fill the modeinfo struct.
        if (VID_VesaGetModeInfo (vesamode, select_bitpp))
        {
            vesa_modes[nummodes].next = &vesa_modes[nummodes+1];
            if (vesamodeinfo.XResolution > 999)
            {
                if (vesamodeinfo.YResolution > 999)
                {
                    sprintf (&names[nummodes][0], "%4dx%4d", vesamodeinfo.XResolution,
                                                             vesamodeinfo.YResolution);
                    names[nummodes][9] = 0;
                }
                else
                {
                    sprintf (&names[nummodes][0], "%4dx%3d", vesamodeinfo.XResolution,
                                                             vesamodeinfo.YResolution);
                    names[nummodes][8] = 0;
                }
            }
            else
            {
                if (vesamodeinfo.YResolution > 999)
                {
                    sprintf (&names[nummodes][0], "%3dx%4d", vesamodeinfo.XResolution,
                                                             vesamodeinfo.YResolution);
                    names[nummodes][8] = 0;
                }
                else
                {
                    sprintf (&names[nummodes][0], "%3dx%3d", vesamodeinfo.XResolution,
                                                             vesamodeinfo.YResolution);
                    names[nummodes][7] = 0;
                }
            }

            vesa_modes[nummodes].name = &names[nummodes][0];
            vesa_modes[nummodes].width = vesamodeinfo.XResolution;
            vesa_modes[nummodes].height = vesamodeinfo.YResolution;

            //added:20-01-98:aspect ratio to be implemented...
            vesa_modes[nummodes].rowbytes = vesamodeinfo.BytesPerScanLine;
            vesa_modes[nummodes].modetype = MODE_fullscreen;
            vesa_modes[nummodes].extradata = &vesa_extra[nummodes];

            if (vesamodeinfo.ModeAttributes & LINEAR_FRAME_BUFFER)
            {
            // add linear bit to mode for linear modes
                vesa_extra[nummodes].vesamode = vesamode | LINEAR_MODE;
                vesa_modes[nummodes].numpages = 1; //vesamodeinfo.NumberOfImagePages;

                phys_mem_info.address = (int)vesamodeinfo.PhysBasePtr;
                phys_mem_info.size = 0x400000;

                // returns -1 on error
                if (__dpmi_physical_address_mapping(&phys_mem_info))
                {
                    //skip this mode, it doesnt work
                    continue;
                }

                // if physical mapping was ok... convert the selector:offset
                vesa_extra[nummodes].linearmem =
                           real2ptr (phys_mem_info.address);

                // lock the region
                __dpmi_lock_linear_region (&phys_mem_info);
            }
            else
            {
            // banked at 0xA0000
                vesa_extra[nummodes].vesamode = vesamode;
                //vesa_extra[nummodes].pages[0] = 0;
                vesa_extra[nummodes].linearmem =
                           real2ptr (vesamodeinfo.WinASegment<<4);

                vesa_modes[nummodes].numpages = 1; //modeinfo.numpages;
            }

            vesa_modes[nummodes].bytesperpixel = (vesamodeinfo.BitsPerPixel+1)/8;

            nummodes++;
        }
    }

    if (nummodes)
    {
        // append the vesa modes to the modelist
        vesa_modes[nummodes-1].next = NULL; //full_vidmodes;
        append_full_vidmodes( &vesa_modes[0], nummodes );
    }
    return;
   
no_vesa:
    CONS_Printf ("No VESA driver\n");
    return;

wrong_vesa:
    CONS_Printf ("VESA VBE %d.0 not available\n", VBEVERSION);
    return;
}


// ========================================================================
// Free the video buffer of the last video mode,
// allocate a new buffer for the video mode to set.
// ========================================================================
boolean VID_FreeAndAllocVidbuffer (viddef_t *lvid)
{
    int  vidbuffersize;

    // Must agree with FinishUpdate, which uses VID_BlitLinearScreen
#if 1 
    // screen size same as video buffer, simple copy
    lvid->ybytes = lvid->direct_rowbytes;
    lvid->screen_size = lvid->direct_size;
#else
    // minimal screen buffer, must copy by line (VID_BlitLinearScreen)
    lvid->ybytes = lvid->widthbytes;
    lvid->screen_size = lvid->ybytes * lvid->height;
#endif
    vidbuffersize = (lvid->screen_size * NUMSCREENS);

    // free allocated buffer for previous video mode
    if (lvid->buffer!=NULL)
    {
        free(lvid->buffer);
    }

    // allocate the new screen buffer
    lvid->buffer = (byte *) malloc(vidbuffersize);
    lvid->display = lvid->buffer;  // display = buffer, screen[0]
    if( lvid->buffer == NULL )
    {
        lvid->screen1 = NULL;
        return false;
    }
    lvid->screen1 = lvid->buffer + lvid->screen_size;

    // initially clear the video buffer
    memset (lvid->buffer, 0, vidbuffersize);

#ifdef DEBUG
 debug_Printf("VID_FreeAndAllocVidbuffer done, vidbuffersize: %x\n",vidbuffersize);
#endif
    return true;
}


// ========================================================================
// Set video mode routine for STANDARD VGA MODES (NO HIGHCOLOR)
// Out: 1 ok, negative FAIL codes
// ========================================================================
static
int VGA_InitMode (viddef_t *lvid, vmode_t *currentmode_p)
{
    __dpmi_regs   regs;

    if (!VID_FreeAndAllocVidbuffer (lvid))
       return FAIL_memory;  //no mem

    //added:26-01-98: should clear video mem here

    //set mode 0x13
    regs.h.ah = 0;
    regs.h.al = 0x13;
    __dpmi_int(0x10, &regs);

    // here it is the standard VGA 64k window, not an LFB
    // (you could have 320x200x256c with LFB in the vesa modes)
    lvid->direct = (byte *) real2ptr (0xa0000);
    lvid->numpages = 1;
    lvid->bytepp = currentmode_p->bytesperpixel;

    return 1;
}


// ========================================================================
// Set video mode routine for VESA video modes, see VID_SetMode()
// Out: 1 ok, FAIL errors (negative)
// ========================================================================
int VID_VesaInitMode (viddef_t *lvid, vmode_t *currentmode_p)
{
    vesa_extra_t    *extra;
    __dpmi_regs     regs;

    extra = currentmode_p->extradata;

#ifdef DEBUG
 debug_Printf("VID_VesaInitMode...\n");
 debug_Printf(" currentmode_p->name %s\n",currentmode_p->name);
 debug_Printf("               width %d\n",currentmode_p->width);
 debug_Printf("               height %d\n",currentmode_p->height);
 debug_Printf("               rowbytes %d\n",currentmode_p->rowbytes);
 debug_Printf("               windowed %d\n",currentmode_p->windowed);
 debug_Printf("               numpages %d\n",currentmode_p->numpages);
 debug_Printf(" currentmode_p->extradata :\n");
 debug_Printf("                ->vesamode %x\n",extra->vesamode);
 debug_Printf("                ->linearmem %x\n\n",extra->linearmem);
#endif

    //added:20-01-98:no page flipping now... TO DO!!!
    lvid->numpages = 1;

    // clean up any old vid buffer lying around, alloc new if needed
    if (!VID_FreeAndAllocVidbuffer (lvid))
       return FAIL_memory;                  //no mem


    //added:20-01-98: should clear video mem here


    // set the mode
    regs.x.ax = 0x4f02;
    regs.x.bx = extra->vesamode;
    __dpmi_int (0x10, &regs);

    if (regs.x.ax != 0x4f)
        return FAIL_create;               // could not set mode

//added:20-01-98: should setup wait_vsync flag, currentpage here...
//                plus check for display_enable bit

//added:20-01-98: here we should set the page if page flipping...

    // points to LFB, or the start of VGA mem.
    lvid->direct = extra->linearmem;
    lvid->bytepp = currentmode_p->bytesperpixel;

    return 1;
}

// ========================================================================
//                     VIDEO MODE CONSOLE COMMANDS
// ========================================================================


//  vid_nummodes
//
//added:21-03-98:
void VID_Command_NumModes_f (void)
{
    int nummodes = VID_NumModes ();
    CONS_Printf ("%d video mode(s) available(s)\n", nummodes);
}


//  vid_modeinfo <modenum>
//
void VID_Command_ModeInfo_f (void)
{
    vmode_t *   pv;
    modenum_t   mn;

    if (COM_Argc()!=2)
        mn = vid.modenum;          // describe the current mode
    else
    {
        mn.index = atoi (COM_Argv(1));   //    .. the given mode number
        mn.modetype = vid.modenum.modetype;  // keep same
    }

    if (mn.index >= VID_NumModes())
    {
        CONS_Printf ("No such video mode\n");
        return;
    }

    pv = VID_GetModePtr (mn);

    CONS_Printf ("%s\n", VID_ModeInfo (mn, NULL));
    CONS_Printf ("width : %d\n"
                 "height: %d\n"
                 "bytes per scanline: %d\n"
                 "bytes per pixel: %d\n"
                 "numpages: %d\n",
                 pv->width,
                 pv->height,
                 pv->rowbytes,
                 pv->bytesperpixel,
                 pv->numpages );
}


//  vid_modelist
//
void VID_Command_ModeList_f (void)
{
    int         i, nummodes;
    modenum_t   mn;
    char *      pinfo, * pheader;
    vmode_t *   pv;
    boolean     na;

    na = false;
    mn.modetype = vid.modenum.modetype;  // keep same

    nummodes = VID_NumModes ();
    for (i=0 ; i<nummodes ; i++)
    {
        mn.index = i;
        pv = VID_GetModePtr (mn);
        pinfo = VID_ModeInfo (mn, &pheader);

        if (i==0 || pv->bytesperpixel==1)
            CONS_Printf ("%d: %s\n", i, pinfo);
        else
            CONS_Printf ("%d: %s (hicolor)\n", i, pinfo);
    }

}


//  vid_mode <modenum>
//
void VID_Command_Mode_f (void)
{
    modenum_t  mn;

    if (COM_Argc()!=2)
    {
        CONS_Printf ("vid_mode <modenum> : set video mode\n");
        return;
    }

    mn.index = atoi(COM_Argv(1));
    mn.modetype = vid.modenum.modetype;  // keep same

    if (mn.index >= VID_NumModes())
        CONS_Printf ("No such video mode\n");
    else
        // request vid mode change
        setmodeneeded = mn;
}
