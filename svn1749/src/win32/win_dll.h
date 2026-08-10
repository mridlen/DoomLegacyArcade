// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: win_dll.h 1035 2013-08-14 00:38:40Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
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
// $Log: win_dll.h,v $
// Revision 1.3  2001/04/04 20:19:07  judgecutor
// Added support for the 3D Sound
//
// Revision 1.2  2000/02/27 00:42:12  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:33  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//      load/unload a DLL at run-time
//
//-----------------------------------------------------------------------------

#ifndef WIN_DLL_H
#define WIN_DLL_H

#include <windows.h>

// this is made extern so win_dbg.c can log the load address
// of the dynamically loaded DLL functions
typedef struct loadfunc_s {
    char*   fnName;
    void*   fnPointer;
} loadfunc_t;
extern HINSTANCE  hwdInstance;
extern loadfunc_t hwdFuncTable[];

//for win_dbg.c
extern HINSTANCE hwdInstance;
extern loadfunc_t hwdFuncTable[];

#ifdef HW3SOUND
extern HINSTANCE hwsInstance;
extern loadfunc_t hwsFuncTable[];
#endif

    
HINSTANCE LoadDLL (char* dllName, loadfunc_t* funcTable);
void UnloadDLL (HINSTANCE* pInstance);

BOOL Init3DDriver (char* dllName);
void Shutdown3DDriver (void);

#ifdef HW3SOUND
BOOL Init3DSDriver(char *dllName);
void Shutdown3DSDriver(void);
#endif

#endif
