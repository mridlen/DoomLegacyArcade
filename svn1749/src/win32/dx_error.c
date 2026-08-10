// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: dx_error.c 1257 2016-09-20 17:14:21Z wesleyjohnson $
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
// $Log: dx_error.c,v $
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      DirectX error messages
//      adapted from DirectX6 sample code
//
//-----------------------------------------------------------------------------

#ifndef __MINGW32__
// disable when have broken D3D.  The MS headers are broken under MinGW.
#define HAVE_D3D
#endif

// Because of WINVER redefine, doomtype.h (via doomincl.h) is before any
// other include that might define WINVER
#include "doomincl.h"

#include <windows.h>
#include <stdarg.h>

// We can use 0x0300, 0x0500, 0x0600, or 0x0700, or can default
#define DIRECTDRAW_VERSION  0x0300
#include <ddraw.h>
  // DDERR_
#define DIRECT3D_VERSION  0x0300
#ifdef HAVE_D3D
//#include <d3d.h>
  // D3DERR_
#include <d3drmwin.h>
  // d3drm, ddraw, d3d
//#include <d3drm.h>
  // D3DRMERR_
#endif

#ifdef __MINGW32__
// blocks duplicate definition of LPDIRECTFULLDUPLEX
#define _IDirectSoundFullDuplex_
#endif
#include <dsound.h>
  // DSERR_

#include "dx_error.h"

// -----------------
// DXErrorMessageBox
// Displays a message box containing the given formatted string.
// -----------------
/*
void __cdecl DXErrorMessageBox (HRESULT error) LPSTR fmt, ... )
{
    char buff[256];
    va_list args;
    
    va_start(args, fmt);
    wvsprintf(buff, fmt, args);
    va_end(args);
    
    lstrcat(buff, "\r\n");
    MessageBox( NULL, buff, "DirectX Error:", MB_ICONEXCLAMATION + MB_OK );
}*/


// ---------------
// DXErrorToString
// Returns a pointer to a string describing the given DD, D3D or D3DRM error code.
// ---------------
char* DXErrorToString (HRESULT error)
{
    char * emsg = "";
    switch(error) {
        case DD_OK:
            /* Also includes D3D_OK and D3DRM_OK */
            emsg = "No error.";
        case DDERR_ALREADYINITIALIZED:
            emsg = "This object is already initialized.";
        case DDERR_BLTFASTCANTCLIP:
            emsg = "emsg = if a clipper object is attached to the source surface passed into a BltFast call.";
        case DDERR_CANNOTATTACHSURFACE:
            emsg = "This surface can not be attached to the requested surface.";
        case DDERR_CANNOTDETACHSURFACE:
            emsg = "This surface can not be detached from the requested surface.";
        case DDERR_CANTCREATEDC:
            emsg = "Windows can not create any more DCs.";
        case DDERR_CANTDUPLICATE:
            emsg = "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";
        case DDERR_CLIPPERISUSINGHWND:
            emsg = "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";
        case DDERR_COLORKEYNOTSET:
            emsg = "No src color key specified for this operation.";
        case DDERR_CURRENTLYNOTAVAIL:
            emsg = "Support is currently not available.";
        case DDERR_DIRECTDRAWALREADYCREATED:
            emsg = "A DirectDraw object representing this driver has already been created for this process.";
        case DDERR_EXCEPTION:
            emsg = "An exception was encountered while performing the requested operation.";
        case DDERR_EXCLUSIVEMODEALREADYSET:
            emsg = "An attempt was made to set the cooperative level when it was already set to exclusive.";
        case DDERR_GENERIC:
            emsg = "Generic failure.";
        case DDERR_HEIGHTALIGN:
            emsg = "Height of rectangle provided is not a multiple of reqd alignment.";
        case DDERR_HWNDALREADYSET:
            emsg = "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";
        case DDERR_HWNDSUBCLASSED:
            emsg = "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";
        case DDERR_IMPLICITLYCREATED:
            emsg = "This surface can not be restored because it is an implicitly created surface.";
        case DDERR_INCOMPATIBLEPRIMARY:
            emsg = "Unable to match primary surface creation request with existing primary surface.";
        case DDERR_INVALIDCAPS:
            emsg = "One or more of the caps bits passed to the callback are incorrect.";
        case DDERR_INVALIDCLIPLIST:
            emsg = "DirectDraw does not support the provided cliplist.";
        case DDERR_INVALIDDIRECTDRAWGUID:
            emsg = "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";
        case DDERR_INVALIDMODE:
            emsg = "DirectDraw does not support the requested mode.";
        case DDERR_INVALIDOBJECT:
            emsg = "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";
        case DDERR_INVALIDPARAMS:
            emsg = "One or more of the parameters passed to the function are incorrect.";
        case DDERR_INVALIDPIXELFORMAT:
            emsg = "The pixel format was invalid as specified.";
        case DDERR_INVALIDPOSITION:
            emsg = "emsg = ed when the position of the overlay on the destination is no longer legal for that destination.";
        case DDERR_INVALIDRECT:
            emsg = "Rectangle provided was invalid.";
        case DDERR_LOCKEDSURFACES:
            emsg = "Operation could not be carried out because one or more surfaces are locked.";
        case DDERR_NO3D:
            emsg = "There is no 3D present.";
        case DDERR_NOALPHAHW:
            emsg = "Operation could not be carried out because there is no alpha accleration hardware present or available.";
        case DDERR_NOBLTHW:
            emsg = "No blitter hardware present.";
        case DDERR_NOCLIPLIST:
            emsg = "No cliplist available.";
        case DDERR_NOCLIPPERATTACHED:
            emsg = "No clipper object attached to surface object.";
        case DDERR_NOCOLORCONVHW:
            emsg = "Operation could not be carried out because there is no color conversion hardware present or available.";
        case DDERR_NOCOLORKEY:
            emsg = "Surface doesn't currently have a color key";
        case DDERR_NOCOLORKEYHW:
            emsg = "Operation could not be carried out because there is no hardware support of the destination color key.";
        case DDERR_NOCOOPERATIVELEVELSET:
            emsg = "Create function called without DirectDraw object method SetCooperativeLevel being called.";
        case DDERR_NODC:
            emsg = "No DC was ever created for this surface.";
        case DDERR_NODDROPSHW:
            emsg = "No DirectDraw ROP hardware.";
        case DDERR_NODIRECTDRAWHW:
            emsg = "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";
        case DDERR_NOEMULATION:
            emsg = "Software emulation not available.";
        case DDERR_NOEXCLUSIVEMODE:
            emsg = "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";
        case DDERR_NOFLIPHW:
            emsg = "Flipping visible surfaces is not supported.";
        case DDERR_NOGDI:
            emsg = "There is no GDI present.";
        case DDERR_NOHWND:
            emsg = "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";
        case DDERR_NOMIRRORHW:
            emsg = "Operation could not be carried out because there is no hardware present or available.";
        case DDERR_NOOVERLAYDEST:
            emsg = "emsg = ed when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";
        case DDERR_NOOVERLAYHW:
            emsg = "Operation could not be carried out because there is no overlay hardware present or available.";
        case DDERR_NOPALETTEATTACHED:
            emsg = "No palette object attached to this surface.";
        case DDERR_NOPALETTEHW:
            emsg = "No hardware support for 16 or 256 color palettes.";
        case DDERR_NORASTEROPHW:
            emsg = "Operation could not be carried out because there is no appropriate raster op hardware present or available.";
        case DDERR_NOROTATIONHW:
            emsg = "Operation could not be carried out because there is no rotation hardware present or available.";
        case DDERR_NOSTRETCHHW:
            emsg = "Operation could not be carried out because there is no hardware support for stretching.";
        case DDERR_NOT4BITCOLOR:
            emsg = "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";
        case DDERR_NOT4BITCOLORINDEX:
            emsg = "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";
        case DDERR_NOT8BITCOLOR:
            emsg = "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";
        case DDERR_NOTAOVERLAYSURFACE:
            emsg = "emsg = ed when an overlay member is called for a non-overlay surface.";
        case DDERR_NOTEXTUREHW:
            emsg = "Operation could not be carried out because there is no texture mapping hardware present or available.";
        case DDERR_NOTFLIPPABLE:
            emsg = "An attempt has been made to flip a surface that is not flippable.";
        case DDERR_NOTFOUND:
            emsg = "Requested item was not found.";
        case DDERR_NOTLOCKED:
            emsg = "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";
        case DDERR_NOTPALETTIZED:
            emsg = "The surface being used is not a palette-based surface.";
        case DDERR_NOVSYNCHW:
            emsg = "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";
        case DDERR_NOZBUFFERHW:
            emsg = "Operation could not be carried out because there is no hardware support for zbuffer blitting.";
        case DDERR_NOZOVERLAYHW:
            emsg = "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";
        case DDERR_OUTOFCAPS:
            emsg = "The hardware needed for the requested operation has already been allocated.";
        case DDERR_OUTOFMEMORY:
            emsg = "There is not enough memory to perform the operation.";
        case DDERR_OUTOFVIDEOMEMORY:
            emsg = "DirectDraw does not have enough memory to perform the operation.";
        case DDERR_OVERLAYCANTCLIP:
            emsg = "The hardware does not support clipped overlays.";
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            emsg = "Can only have ony color key active at one time for overlays.";
        case DDERR_OVERLAYNOTVISIBLE:
            emsg = "emsg = ed when GetOverlayPosition is called on a hidden overlay.";
        case DDERR_PALETTEBUSY:
            emsg = "Access to this palette is being refused because the palette is already locked by another thread.";
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            emsg = "This process already has created a primary surface.";
        case DDERR_REGIONTOOSMALL:
            emsg = "Region passed to Clipper::GetClipList is too small.";
        case DDERR_SURFACEALREADYATTACHED:
            emsg = "This surface is already attached to the surface it is being attached to.";
        case DDERR_SURFACEALREADYDEPENDENT:
            emsg = "This surface is already a dependency of the surface it is being made a dependency of.";
        case DDERR_SURFACEBUSY:
            emsg = "Access to this surface is being refused because the surface is already locked by another thread.";
        case DDERR_SURFACEISOBSCURED:
            emsg = "Access to surface refused because the surface is obscured.";
        case DDERR_SURFACELOST:
            emsg = "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";
        case DDERR_SURFACENOTATTACHED:
            emsg = "The requested surface is not attached.";
        case DDERR_TOOBIGHEIGHT:
            emsg = "Height requested by DirectDraw is too large.";
        case DDERR_TOOBIGSIZE:
            emsg = "Size requested by DirectDraw is too large, but the individual height and width are OK.";
        case DDERR_TOOBIGWIDTH:
            emsg = "Width requested by DirectDraw is too large.";
        case DDERR_UNSUPPORTED:
            emsg = "Function call not supported.";
        case DDERR_UNSUPPORTEDFORMAT:
            emsg = "FOURCC format requested is unsupported by DirectDraw.";
        case DDERR_UNSUPPORTEDMASK:
            emsg = "Bitmask in the pixel format requested is unsupported by DirectDraw.";
        case DDERR_VERTICALBLANKINPROGRESS:
            emsg = "Vertical blank is in progress.";
        case DDERR_WASSTILLDRAWING:
            emsg = "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";
        case DDERR_WRONGMODE:
            emsg = "This surface can not be restored because it was created in a different mode.";
        case DDERR_XALIGN:
            emsg = "Rectangle provided was not horizontally aligned on required boundary.";
#ifdef HAVE_D3D
        case D3DERR_BADMAJORVERSION:
            emsg = "D3DERR_BADMAJORVERSION";
        case D3DERR_BADMINORVERSION:
            emsg = "D3DERR_BADMINORVERSION";
        case D3DERR_EXECUTE_LOCKED:
            emsg = "D3DERR_EXECUTE_LOCKED";
        case D3DERR_EXECUTE_NOT_LOCKED:
            emsg = "D3DERR_EXECUTE_NOT_LOCKED";
        case D3DERR_EXECUTE_CREATE_FAILED:
            emsg = "D3DERR_EXECUTE_CREATE_FAILED";
        case D3DERR_EXECUTE_DESTROY_FAILED:
            emsg = "D3DERR_EXECUTE_DESTROY_FAILED";
        case D3DERR_EXECUTE_LOCK_FAILED:
            emsg = "D3DERR_EXECUTE_LOCK_FAILED";
        case D3DERR_EXECUTE_UNLOCK_FAILED:
            emsg = "D3DERR_EXECUTE_UNLOCK_FAILED";
        case D3DERR_EXECUTE_FAILED:
            emsg = "D3DERR_EXECUTE_FAILED";
        case D3DERR_EXECUTE_CLIPPED_FAILED:
            emsg = "D3DERR_EXECUTE_CLIPPED_FAILED";
        case D3DERR_TEXTURE_NO_SUPPORT:
            emsg = "D3DERR_TEXTURE_NO_SUPPORT";
        case D3DERR_TEXTURE_NOT_LOCKED:
            emsg = "D3DERR_TEXTURE_NOT_LOCKED";
        case D3DERR_TEXTURE_LOCKED:
            emsg = "D3DERR_TEXTURELOCKED";
        case D3DERR_TEXTURE_CREATE_FAILED:
            emsg = "D3DERR_TEXTURE_CREATE_FAILED";
        case D3DERR_TEXTURE_DESTROY_FAILED:
            emsg = "D3DERR_TEXTURE_DESTROY_FAILED";
        case D3DERR_TEXTURE_LOCK_FAILED:
            emsg = "D3DERR_TEXTURE_LOCK_FAILED";
        case D3DERR_TEXTURE_UNLOCK_FAILED:
            emsg = "D3DERR_TEXTURE_UNLOCK_FAILED";
        case D3DERR_TEXTURE_LOAD_FAILED:
            emsg = "D3DERR_TEXTURE_LOAD_FAILED";
        case D3DERR_MATRIX_CREATE_FAILED:
            emsg = "D3DERR_MATRIX_CREATE_FAILED";
        case D3DERR_MATRIX_DESTROY_FAILED:
            emsg = "D3DERR_MATRIX_DESTROY_FAILED";
        case D3DERR_MATRIX_SETDATA_FAILED:
            emsg = "D3DERR_MATRIX_SETDATA_FAILED";
        case D3DERR_SETVIEWPORTDATA_FAILED:
            emsg = "D3DERR_SETVIEWPORTDATA_FAILED";
        case D3DERR_MATERIAL_CREATE_FAILED:
            emsg = "D3DERR_MATERIAL_CREATE_FAILED";
        case D3DERR_MATERIAL_DESTROY_FAILED:
            emsg = "D3DERR_MATERIAL_DESTROY_FAILED";
        case D3DERR_MATERIAL_SETDATA_FAILED:
            emsg = "D3DERR_MATERIAL_SETDATA_FAILED";
        case D3DERR_LIGHT_SET_FAILED:
            emsg = "D3DERR_LIGHT_SET_FAILED";
        case D3DRMERR_BADOBJECT:
            emsg = "D3DRMERR_BADOBJECT";
        case D3DRMERR_BADTYPE:
            emsg = "D3DRMERR_BADTYPE";
        case D3DRMERR_BADALLOC:
            emsg = "D3DRMERR_BADALLOC";
        case D3DRMERR_FACEUSED:
            emsg = "D3DRMERR_FACEUSED";
        case D3DRMERR_NOTFOUND:
            emsg = "D3DRMERR_NOTFOUND";
        case D3DRMERR_NOTDONEYET:
            emsg = "D3DRMERR_NOTDONEYET";
        case D3DRMERR_FILENOTFOUND:
            emsg = "The file was not found.";
        case D3DRMERR_BADFILE:
            emsg = "D3DRMERR_BADFILE";
        case D3DRMERR_BADDEVICE:
            emsg = "D3DRMERR_BADDEVICE";
        case D3DRMERR_BADVALUE:
            emsg = "D3DRMERR_BADVALUE";
        case D3DRMERR_BADMAJORVERSION:
            emsg = "D3DRMERR_BADMAJORVERSION";
        case D3DRMERR_BADMINORVERSION:
            emsg = "D3DRMERR_BADMINORVERSION";
        case D3DRMERR_UNABLETOEXECUTE:
            emsg = "D3DRMERR_UNABLETOEXECUTE";
#endif

        //
        // DirectSound errors
        //
        case DSERR_ALLOCATED:
            emsg = "The request failed because resources, such as a priority level, were already in use by another caller.";
        case DSERR_ALREADYINITIALIZED:
            emsg = "The object is already initialized.";
        case DSERR_BADFORMAT:
            emsg = "The specified wave format is not supported.";
        case DSERR_BUFFERLOST:
            emsg = "The buffer memory has been lost and must be restored.";
        case DSERR_CONTROLUNAVAIL:
            emsg = "The control (volume, pan, and so forth) requested by the caller is not available.";
        case DSERR_INVALIDCALL:
            emsg = "This function is not valid for the current state of this object.";
        case DSERR_NOAGGREGATION:
            emsg = "The object does not support aggregation.";
        case DSERR_NODRIVER:
            emsg = "No sound driver is available for use.";
        case DSERR_NOINTERFACE:
            emsg = "The requested COM interface is not available.";
        case DSERR_OTHERAPPHASPRIO:
            emsg = "Another application has a higher priority level, preventing this call from succeeding";
        case DSERR_PRIOLEVELNEEDED:
            emsg = "The caller does not have the priority level required for the function to succeed.";
        case DSERR_UNINITIALIZED:
            emsg = "The IDirectSound::Initialize method has not been called or has not been called successfully before other methods were called.";
/* duplicate valuse ?!?
        case DSERR_GENERIC  :
            emsg = "An undetermined error occurred inside the DirectSound subsystem.";  
        case DSERR_INVALIDPARAM :
            emsg = "An invalid parameter was passed to the emsg = ing function.";
        case DSERR_OUTOFMEMORY :
             emsg = "The DirectSound subsystem could not allocate sufficient memory to complete the caller's request.";
        case DSERR_UNSUPPORTED :
             emsg = "The function called is not supported at this time.";
*/
        default:
            emsg = "Unrecognized error value.";
    }
    return emsg;
}
