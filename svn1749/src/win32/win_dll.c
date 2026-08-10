// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_dll.c 1257 2016-09-20 17:14:21Z wesleyjohnson $
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
// $Log: win_dll.c,v $
// Revision 1.11  2002/01/21 23:28:45  judgecutor
// Added HW3S_I_StartSound low-level fuction for arbitrary managing of 3D sources
//
// Revision 1.10  2001/08/07 00:54:40  hurdler
// MD2 implementation is getting better but still need lots of work
//
// Revision 1.9  2001/05/27 13:42:48  bpereira
//
// Revision 1.8  2001/04/04 20:19:07  judgecutor
// Added support for the 3D Sound
//
// Revision 1.7  2001/02/24 13:35:23  bpereira
//
// Revision 1.6  2001/01/05 18:19:48  hurdler
// add renderer version checking
//
// Revision 1.5  2000/10/04 16:27:02  hurdler
// Implement texture memory stats
//
// Revision 1.4  2000/08/10 14:19:56  hurdler
// add waitvbl
//
// Revision 1.3  2000/04/16 18:38:07  bpereira
//
//
// DESCRIPTION:
//      load and initialise the 3D driver DLL
//
//-----------------------------------------------------------------------------

// Because of WINVER redefine, doomtype.h (via doomincl.h) is before any
// other include that might define WINVER
#include "doomincl.h"

#include "hardware/hw_drv.h"        // get the standard 3D Driver DLL exports prototypes

#include "hardware/hw3dsdrv.h"      // get the 3D sound driver DLL export prototypes

#include "win_dll.h"
#include "win_main.h"       // I_GetLastErrorMsgBox()


// m_misc.h
char *va(char *format, ...);


// ==========================================================================
// STANDARD 3D DRIVER DLL FOR DOOM LEGACY
// ==========================================================================

// note : the 3D driver loading should be put somewhere else..

HINSTANCE hwdInstance = NULL;

loadfunc_t hwdFuncTable[] = {
    {"_Init@4",            &hwdriver.pfnInit},
    {"_Shutdown@0",        &hwdriver.pfnShutdown},
    {"_GetModeList@8",     &hwdriver.pfnGetModeList},
    {"_SetPalette@8",      &hwdriver.pfnSetPalette},
    {"_FinishUpdate@4",    &hwdriver.pfnFinishUpdate},
    {"_Draw2DLine@12",     &hwdriver.pfnDraw2DLine},
    {"_DrawPolygon@16",    &hwdriver.pfnDrawPolygon},
    {"_SetBlend@4",        &hwdriver.pfnSetBlend},
    {"_ClearBuffer@12",    &hwdriver.pfnClearBuffer},
    {"_SetTexture@4",      &hwdriver.pfnSetTexture},
    {"_ReadRect@24",       &hwdriver.pfnReadRect},
    {"_GClipRect@20",      &hwdriver.pfnGClipRect},
    {"_ClearMipMapCache@0",&hwdriver.pfnClearMipMapCache},
    {"_SetSpecialState@8", &hwdriver.pfnSetSpecialState},
    {"_DrawMD2@16",        &hwdriver.pfnDrawMD2},
    {"_SetTransform@4",    &hwdriver.pfnSetTransform},
    {"_GetTextureUsed@0",  &hwdriver.pfnGetTextureUsed},
    {"_GetRenderVersion@0",&hwdriver.pfnGetRenderVersion},
    {NULL,NULL}
};

#ifdef HW3SOUND
HINSTANCE hwsInstance = NULL;

loadfunc_t hwsFuncTable[] = {
    {"_Startup@8",              &hw3ds_driver.pfnStartup},
    {"_Shutdown@0",             &hw3ds_driver.pfnShutdown},
    {"_GetHW3DSVersion@0",      &hw3ds_driver.pfnGetHW3DSVersion},
    {"_Add3DSource@8",          &hw3ds_driver.pfnAdd3DSource},
    {"_Add2DSource@4",          &hw3ds_driver.pfnAdd2DSource},
    {"_StartSource@4",          &hw3ds_driver.pfnStartSource},
    {"_StopSource@4",           &hw3ds_driver.pfnStopSource},
    {"_BeginFrameUpdate@0",     &hw3ds_driver.pfnBeginFrameUpdate},
    {"_EndFrameUpdate@0",       &hw3ds_driver.pfnEndFrameUpdate},
    {"_IsPlaying@4",            &hw3ds_driver.pfnIsPlaying},
    {"_UpdateListener@4",       &hw3ds_driver.pfnUpdateListener},
    {"_SetGlobalSfxVolume@4",   &hw3ds_driver.pfnSetGlobalSfxVolume},
    {"_Update2DSoundParms@12",  &hw3ds_driver.pfnUpdate2DSoundParms},
    {"_Update3DSource@8",       &hw3ds_driver.pfnUpdate3DSource},
    {"_UpdateSourceVolume@8",   &hw3ds_driver.pfnUpdateSourceVolume},
    {"_SetCone@8",              &hw3ds_driver.pfnSetCone},
    {"_Reload3DSource@8",       &hw3ds_driver.pfnReload3DSource},
    {"_KillSource@4",           &hw3ds_driver.pfnKillSource},
    {NULL, NULL}
};
#endif

BOOL Init3DDriver (char* dllName)
{
    hwdInstance = LoadDLL (dllName, hwdFuncTable);
    return (hwdInstance != NULL);
}

void Shutdown3DDriver (void)
{
    UnloadDLL (&hwdInstance);
}

#ifdef HW3SOUND
BOOL Init3DSDriver(char *dllName)
{
    hwsInstance = LoadDLL (dllName, hwsFuncTable);
    return (hwsInstance != NULL);
}

void Shutdown3DSDriver (void)
{
    UnloadDLL (&hwsInstance);
}
#endif

// --------------------------------------------------------------------------
// Load a DLL, returns the HINSTANCE handle or NULL
// --------------------------------------------------------------------------
HINSTANCE LoadDLL (char* dllName, loadfunc_t* funcTable)
{
    void*       funcPtr;
    loadfunc_t* loadfunc;
    HINSTANCE   hInstance;

    if ((hInstance = LoadLibrary (dllName)) != NULL)
    {
        // get function pointers for all functions we use
        for (loadfunc = funcTable; loadfunc->fnName!=NULL; loadfunc++)
        {
            funcPtr = GetProcAddress (hInstance, loadfunc->fnName);
            if (!funcPtr) {
                //I_GetLastErrorMsgBox ();
                MessageBox( NULL, va("The '%s' haven't the good specification (function %s missing)\n\n"
                                     "You must use dll from the same zip of this exe\n", dllName, loadfunc->fnName), "Error", MB_OK|MB_ICONINFORMATION );
                return FALSE;
            }
            // store function address
            *((void**)loadfunc->fnPointer) = funcPtr;
        }
    }
    else
    {
        MessageBox( NULL, va("LoadLibrary() FAILED : couldn't load '%s'\r\n", dllName), "Warning", MB_OK|MB_ICONINFORMATION );
        //I_GetLastErrorMsgBox ();
    }

    return hInstance;
}


// --------------------------------------------------------------------------
// Unload the DLL
// --------------------------------------------------------------------------
void UnloadDLL (HINSTANCE* pInstance)
{
    if (FreeLibrary (*pInstance))
        *pInstance = NULL;
    else
        I_GetLastErrorMsgBox ();
}
