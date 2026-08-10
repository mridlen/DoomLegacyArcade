// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: command.h 1686 2024-06-20 09:45:56Z wesleyjohnson $
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
// $Log: command.h,v $
// Revision 1.8  2003/05/30 22:44:08  hurdler
// add checkcvar function to FS
//
// Revision 1.7  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.6  2000/11/11 13:59:45  bpereira
// Revision 1.5  2000/10/08 13:29:59  bpereira
// Revision 1.4  2000/08/31 14:30:55  bpereira
// Revision 1.3  2000/04/16 18:38:06  bpereira
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
//   Command line processing for console.
//   CV variable support, saving, loading of config.
//
//-----------------------------------------------------------------------------


#ifndef COMMAND_H
#define COMMAND_H

#include "doomtype.h"

//===================================
// Command buffer & command execution
//===================================

typedef void (*com_func_t) (void);

typedef enum {
  // need 0 as noop
  CC_info = 1,
  CC_cheat,   // cheats
  CC_command, // general commands
  CC_savegame, // savegames
  CC_config,  // config files
  CC_control, // keys, bind
  CC_fs, // fragglescript
  CC_chat, // chat
  CC_net, // network
  CC_console // console
} command_type_e;
void  COM_AddCommand (const char * name, com_func_t func, byte command_type );

typedef struct {
  byte   num;     // number of actual args
  char * arg[4];  // first 4
} COM_args_t;

// get some args
void  COM_Args( COM_args_t * comargs );

// Any args
int     COM_Argc (void);
char *  COM_Argv (int arg);   // if argv>argc, returns empty string
int     COM_CheckParm (const char * check_str); // like M_CheckParm :)

// match existing command or NULL
const char *  COM_CompleteCommand (const char * partial, int skips);

// insert at queu (at end of other command)
void    COM_BufAddText (const char * text);

// insert in head (before other command)
void    COM_BufInsertText (const char * text);

// Execute commands in buffer, flush them
void    COM_BufExecute( byte cs_config );

// setup command buffer, at game tartup
void    COM_Init (void);


// ======================
// Variable sized buffers
// ======================

typedef struct vsbuf_s
{
    boolean allowoverflow;  // if false, do a I_Error
    boolean overflowed;     // set to true if the buffer size failed
    byte *  data;
    int     maxsize;
    int     cursize;
} vsbuf_t;

void VS_Alloc (vsbuf_t * buf, int initsize);
void VS_Free  (vsbuf_t * buf);
void VS_Clear (vsbuf_t * buf);
void * VS_GetSpace (vsbuf_t * buf, int length);
boolean VS_Write (vsbuf_t * buf, void * data, int length);
// strcats onto the buf
boolean VS_Print (vsbuf_t * buf, const char * data);

// ======================


//==================
// Console variables
//==================
// console vars are variables that can be changed through code or console,
// at RUN TIME. They can also act as simplified commands, because a
// function can be attached to a console var, which is called whenever the
// variable is modified (using flag CV_CALL).

// flags for console vars

typedef enum
{
    CV_SAVE   = 0x01, // save to config when quit game
    CV_CALL   = 0x02, // call function on change
    CV_NETVAR = 0x04, // send it when change (see logboris.txt at 12-4-2000)
    CV_NOINIT = 0x08, // dont call function when var is registered (1st set)
    CV_FLOAT  = 0x10, // the value is fixed 16:16, where unit is FRACUNIT
                      // (allow user to enter 0.45 for ex)
                      // WARNING: currently only supports set with CV_Set()
    CV_VALUE  = 0x20, // Value is too large for EV, but not a string.
    CV_STRING = 0x40, // String value.
    CV_NET_LOCK = 0x100, // some variable can't be changed in network but is not netvar (ex: splitscreen)
    CV_HIDEN   = 0x200,   // variable is not part of the cvar list so cannot be accessed by the console
                          // can only be set when we have the pointer to hit 
                          // used on the menu
    CV_CFG1   = 0x2000,  // Restricted to main config file.
    CV_SHOWMODIF = 0x4000,  // say something when modified
    CV_SHOWMODIF_ONCE = 0x8000,  // same, but resets this flag to 0 after showing, set in toggle
} cv_flags_e;

typedef enum
{
    CS_CONFIG   = 0x07, // the config file source, 3 bits
    CS_PUSHED   = 0x08, // a value from another config has been pushed
    CS_EV_PROT  = 0x10, // protect the EV value
    CS_EV_PARAM = 0x40, // A command line param is in EV.
    CS_MODIFIED = 0x80, // this bit is set when cvar is modified
} cv_state_e;

typedef enum
{
// Configfile Value sources.
    CFG_none,     // not loaded
    CFG_main,     // the main config file
    CFG_drawmode, // the drawmode config file
    CFG_other,    // some other config value
    CFG_netvar,   // pushed by network setting
// Searches
//    CFG_all = 0x40,  // match all
    CFG_null = 0x4f, // match none
} cv_config_e;

typedef struct {
    int   value;
    const char * strvalue;
} CV_PossibleValue_t;

// [WDJ] CV_PossibleValue supports the following structures.
// MIN .. MAX : First element has label MIN.  Last element is maximum value.
// MIN INC .. MAX : Label INC is the increment.
// List of values : Next or previous value on the list.

// [WDJ] Ptrs together for better packing. Beware many consts of this type.
typedef struct consvar_s  consvar_t;
typedef struct consvar_s
{
// Declare in consvar_t instance.  If this order is altered, about 60 instances have to be fixed.
    const char * name;
    const char * defaultvalue;
    uint32_t flags;            // flags see cv_flags_e above
    CV_PossibleValue_t * PossibleValue;  // table of possible values
    void    (*func) (void);    // called on change, if CV_CALL set, optional
// Undeclared
    int32_t  value;            // for int and fixed_t
    uint16_t netid;            // hashed netid for net send and receive
                               // used only with CV_NETVAR
    byte     state;  // cv_state_e
    byte     EV;  // [WDJ] byte value, set from value changes, set from demos.
       // This saves user settings from being changed by demos.
       // Do not make it anything except byte.  Byte is efficient for most
       // enables, and enum. Two bytes of space are free due to alignment.
       // For most user settings this is slightly easier to manage than
       // creating more EN vars.  For the exceptions, create a setting function
       // to pass consvar settings to EN vars.
    char *  string;      // value in string
       // Saved user config is in string.
       // When pointing to a PossibleValue, it will need to be a const char *.
       // Otherwise, it is allocated with Z_Alloc, Z_Free.
    consvar_t * next;
} consvar_t;

extern CV_PossibleValue_t CV_OnOff[];
extern CV_PossibleValue_t CV_YesNo[];
extern CV_PossibleValue_t CV_Unsigned[];
extern CV_PossibleValue_t CV_uint16[];
extern CV_PossibleValue_t CV_byte[];
// register a variable for use at the console
void  CV_RegisterVar (consvar_t * variable);
void  CV_RegisterVar_list (consvar_t ** cvar_list );  // terminate with NULL

// returns the name of the nearest console variable name found
//  partial : partial variable name
const char * CV_CompleteVar (const char * partial, int skips);

// Sets a var to a string value.
void  CV_Set (consvar_t * var, const char * str_value);

// expands value to a string and calls CV_Set
void  CV_SetValue (consvar_t * var, int value);

// Set a command line parameter value.
void  CV_SetParam (consvar_t * var, int value);
extern byte command_EV_param;

// Makes a copy of the string, and handles PossibleValue string values.
//   str : a reference to a string, it will by copied.
void  CV_Set_cvar_string( consvar_t * cvar, const char * str );
void  CV_Free_cvar_string( consvar_t * cvar );

// Get string for CV_PossibleValue_t
//  pv_value: a value in the CV_PossibleValue_t list
const char *  CV_get_possiblevalue_string( CV_PossibleValue_t * pv,  byte pv_value );

// If a OnChange func tries to change other values,
// this function should be used.
void CV_Set_by_OnChange (consvar_t * cvar, int value);

// it a setvalue but with a modulo at the maximum
void  CV_ValueIncDec (consvar_t * cvar, int increment);

// Do the CV_CALL, with validity tests, and enforcing user_enable rules.
void  CV_cvar_call( consvar_t * cvar, byte user_enable );

// Called after demo to restore the user settings.
void  CV_Restore_User_Settings( void );

// Iterator for saving variables
consvar_t *  CV_IteratorFirst( void );
consvar_t *  CV_Iterator( consvar_t * cv );

consvar_t * CV_FindVar (const char * name);

// Return the string value of the config var, current or pushed.
// Return NULL if not found.
const char *  CV_Get_Config_string( consvar_t * cvar, byte c_config );

// Get the values of a pushed cvar, into the temp cvar.
boolean  CV_Get_Pushed_cvar( consvar_t * cvar, byte c_config, /*OUT*/ consvar_t * temp_cvar );

// Put the values in the temp cvar, into the pushed or current cvar.
void  CV_Put_Config_cvar( consvar_t * cvar, byte c_config, /*IN*/ consvar_t * temp_cvar );

// Put the string value to the pushed or current cvar.
// This will create or push, as needed.
//   str : str value, will be copied.  Will set numeric value too.
void  CV_Put_Config_string( consvar_t * cvar, byte cfg, const char * str );

// Remove the cvar value for the config.
void  CV_Delete_Config_cvar( consvar_t * cvar, byte c_config );

// Clear all values from the config file.
void  CV_Clear_Config( byte cs_config );

// Return true if any value of the config is current, or pushed.
boolean CV_Config_check( byte cfg );


#endif // COMMAND_H
