// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: command.c 1688 2024-06-23 20:41:30Z wesleyjohnson $
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
// $Log: command.c,v $
// Revision 1.15  2005/05/21 08:41:23  iori_
// May 19, 2005 - PlayerArmor FS function;  1.43 can be compiled again.
//
// Revision 1.14  2004/08/26 23:15:45  hurdler
// add FS functions in console (+ minor linux fixes)
//
// Revision 1.13  2003/05/30 22:44:08  hurdler
// add checkcvar function to FS
//
// Revision 1.12  2001/12/27 22:50:25  hurdler
// fix a colormap bug, add scrolling floor/ceiling in hw mode
//
// Revision 1.11  2001/02/24 13:35:19  bpereira
//
// Revision 1.10  2001/01/25 22:15:41  bpereira
// added heretic support
//
// Revision 1.9  2000/11/11 13:59:45  bpereira
// Revision 1.8  2000/11/02 19:49:35  bpereira
// Revision 1.7  2000/10/08 13:29:59  bpereira
// Revision 1.6  2000/09/28 20:57:14  bpereira
// Revision 1.5  2000/08/31 14:30:54  bpereira
// Revision 1.4  2000/08/03 17:57:41  bpereira
// Revision 1.3  2000/02/27 00:42:10  hurdler
// Revision 1.2  2000/02/26 00:28:42  hurdler
// Mostly bug fix (see borislog.txt 23-2-2000, 24-2-2000)
//
//
// DESCRIPTION:
//      parse and execute commands from console input/scripts/
//      and remote server.
//
//      handles console variables, which is a simplified version
//      of commands, each consvar can have a function called when
//      it is modified.. thus it acts nearly as commands.
//
//      code shamelessly inspired by the QuakeC sources, thanks Id :)
//
//-----------------------------------------------------------------------------


#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "console.h"
#include "z_zone.h"
#include "d_clisrv.h"
#include "d_netcmd.h"
#include "m_misc.h"
#include "m_fixed.h"
#include "byteptr.h"
#include "p_saveg.h"

// Hurdler: add FS functionnality to console command
#include "t_vari.h"
//void run_string(char * data);

//========
// protos.
//========
static boolean COM_Exists (const char * com_name);
static void    COM_ExecuteString (const char * text, boolean script, byte cfg);

static void    COM_Alias_f (void);
static void    COM_Echo_f (void);
static void    COM_Exec_f (void);
static void    COM_Wait_f (void);
static void    COM_Help_f (void);
static void    COM_Toggle_f (void);

static boolean CV_Var_Command ( byte cfg );
static char *  CV_StringValue (const char * var_name);
static consvar_t * consvar_vars;       // list of registered console variables

#define COM_TOKEN_MAX   1024
static char    com_token[COM_TOKEN_MAX];
static const char *  COM_Parse (const char * data, boolean script);

CV_PossibleValue_t CV_OnOff[] =    {{0,"Off"}, {1,"On"},    {0,NULL}};
CV_PossibleValue_t CV_YesNo[] =     {{0,"No"} , {1,"Yes"},   {0,NULL}};
CV_PossibleValue_t CV_uint16[]=   {{0,"MIN"}, {0xFFFF,"MAX"}, {0,NULL}};
CV_PossibleValue_t CV_Unsigned[]=   {{0,"MIN"}, {999999999,"MAX"}, {0,NULL}};
CV_PossibleValue_t CV_byte[]=   {{0,"MIN"}, {255,"MAX"}, {0,NULL}};

#define COM_BUF_SIZE    8192   // command buffer size

static int com_wait;       // one command per frame (for cmd sequences)


// command aliases
//
typedef struct cmdalias_s
{
    struct cmdalias_s * next;
    char * name;
    char * value;     // the command string to replace the alias
} cmd_alias_t;

static cmd_alias_t * com_alias; // aliases list


// =========================================================================
//                            COMMAND BUFFER
// =========================================================================


static vsbuf_t com_text;     // variable sized buffer


//  Add text (a NUL-terminated string) in the command buffer (for later execution)
//
void COM_BufAddText (const char * text)
{
  if (!VS_Print(&com_text, text))
    CONS_Printf ("Command buffer full!\n");
}


// Adds command text immediately after the current command
// Adds a \n to the text
//
void COM_BufInsertText (const char * text)
{
    char * temp;

    // copy off any commands still remaining in the exec buffer
    int templen = com_text.cursize;
    if (templen)
    {
      // add a trailing NUL (TODO why do we even allow non-string data in a vsbuf_t?)
      temp = Z_Malloc (templen + 1, PU_STATIC, NULL);
      temp[templen] = '\0';
      memcpy (temp, com_text.data, templen);
      VS_Clear (&com_text);
    }
    else
        temp = NULL;    // shut up compiler

    // add the entire text of the file (or alias)
    COM_BufAddText (text);

    // add the copied off data
    if (templen)
    {
      if (!VS_Print(&com_text, temp))
        CONS_Printf ("Command buffer full!!\n");

      Z_Free (temp);
    }
}


//  Flush (execute) console commands in buffer.
//
//   cfg : cv_config_e, to mark the cvar as to the source
//
//  Global: com_wait : does nothing until com_wait ticks down
//
// Called by: TryRunTics( CFG_none )
// Called by: P_Load_LevelInfo( CFG_none )
// Called by: M_LoadConfig( cfg )
// Was Called by: M_Choose_to_quit_Response, but that is disabled now.
void COM_BufExecute( byte cfg )
{
  int     i;
  boolean script = 1;
  char line[1024];

  if (com_wait)
  {
        com_wait--;
        return;
  }

  while (com_text.cursize)
  {
      // find a '\n' or ; line break
      char * text = (char *)com_text.data;
      boolean in_quote = false;

      // This is called without a clue as to what commands are present.
      // Scripts have quoted strings,
      // exec have quoted filenames with backslash: exec "c:\doomdir\".
      // The while loop continues into the exec which can have two levels
      // of quoted strings:
      //      alias zoom_in "fov 15; bind \"mouse 3\" zoom_out"
      script =
        ( ( strncmp(text,"exec",4) == 0 )
          ||( strncmp(text,"map",3) == 0 )
          ||( strncmp(text,"playdemo",8) == 0 )
          ||( strncmp(text,"addfile",7) == 0 )
          ||( strncmp(text,"loadconfig",10) == 0 )
          ||( strncmp(text,"saveconfig",10) == 0 )
          ) ? 0 : 1;  // has filename : is script with quoted strings

      for (i=0; i < com_text.cursize; i++)
      {
        register char ch = text[i];
        if (ch == '"') // non-escaped quote 
          in_quote = !in_quote;
        else if( in_quote )
        {
          if (script && (ch == '\\')) // escape sequence
          {
#if 1
              // [WDJ] Only doublequote and backslash really matter
              i += 1;  // skip it, because other parser does too
              continue;
#else
              switch (text[i+1])
              {
                case '\\': // backslash
                case '"':  // double quote
                case 't':  // tab
                case 'n':  // newline
                  i += 1;  // skip it
                  break;

                default:
                  // unknown sequence, parser will give an error later on.
                  break;
              }
              continue;
#endif	     
          }
        }
        else
        { // not in quoted string
          if (ch == ';') // semicolon separates commands
            break;
          if (ch == '\n' || ch == '\r') // always separate commands
            break;
        }
      }


      if( i > 1023 )  i = 1023;  // overrun of line
      memcpy (line, text, i);
      line[i] = 0;

      // flush the command text from the command buffer, _BEFORE_
      // executing, to avoid that 'recursive' aliases overflow the
      // command text buffer, in that case, new commands are inserted
      // at the beginning, in place of the actual, so it doesn't
      // overflow
      if (i == com_text.cursize)
      {
            // the last command was just flushed
            com_text.cursize = 0;
      }
      else
      {
            i++;
            com_text.cursize -= i;
            // Shuffle text, overlap copy.  Bug fix by Ryan bug_0626.
            memmove(text, text+i, com_text.cursize);
      }

      // execute the command line, marking the cvar values as to the source
      COM_ExecuteString( line, script, cfg );

      // delay following commands if a wait was encountered
      if (com_wait)
      {
            com_wait--;
            break;
      }
  }
}


// =========================================================================
//                            COMMAND EXECUTION
// =========================================================================

typedef struct xcommand_s
{
    const char *  name;
    struct xcommand_s * next;
    com_func_t         function;
    byte    cctype; // classification for help
} xcommand_t;

static  xcommand_t * com_commands = NULL;     // current commands


#define MAX_ARGS        80
static int  com_argc;
static char *  com_argv[MAX_ARGS];
static char *  com_null_string = "";
static const char * com_args = NULL;          // current command args or NULL


//  Initialize command buffer and add basic commands
//
// Called only once
void COM_Init (void)
{
    int i;
    for( i=0; i<MAX_ARGS; i++ )  com_argv[i] = com_null_string;
    com_argc = 0;

    // allocate command buffer
    VS_Alloc (&com_text, COM_BUF_SIZE);

    // add standard commands
    COM_AddCommand ("alias",COM_Alias_f, CC_command);
    COM_AddCommand ("echo", COM_Echo_f, CC_command);
    COM_AddCommand ("exec", COM_Exec_f, CC_command);
    COM_AddCommand ("wait", COM_Wait_f, CC_command);
    COM_AddCommand ("toggle", COM_Toggle_f, CC_command);
    COM_AddCommand ("help", COM_Help_f, CC_info);
    Register_NetXCmd(XD_NETVAR, Got_NetXCmd_NetVar);
}


// Returns how many args for last command
//
int COM_Argc (void)
{
    return com_argc;
}


// Returns string pointer for given argument number
//
char * COM_Argv (int arg)
{
    if ( arg >= com_argc || arg < 0 )
        return com_null_string;
    return com_argv[arg];
}

// get some args
// More efficient, but preserves read only interface
void  COM_Args( COM_args_t * comargs )
{
    int i;
    comargs->num = com_argc;
    for( i=0; i<4; i++ )
    {
        comargs->arg[i] = com_argv[i];
    }
}

#if 0
// [WDJ] Unused
// Returns string pointer of all command args
//
char * COM_Args (void)
{
    return com_args;
}
#endif


int COM_CheckParm (const char * check_str)
{
    int i;

    for (i = 1; i<com_argc; i++)
    {
        if( strcasecmp(check_str, com_argv[i]) == 0 )
            return i;
    }
    return 0;
}


// Parses the given string into command line tokens.
//
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.
static void COM_TokenizeString (const char * text, boolean script)
{
    int  i;

// clear the args from the last string
    for (i=0 ; i<com_argc ; i++)
    {
        Z_Free (com_argv[i]);
        com_argv[i] = com_null_string;  // never leave behind ptrs to old mem
    }

    com_argc = 0;
    com_args = NULL;

    while (1)
    {
// skip whitespace up to a /n
        while (*text && *text <= ' ' && *text != '\n')
            text++;

        if (*text == '\n')
        {   // a newline means end of command in buffer,
            // thus end of this command's args too
            text++;
            break;
        }

        if (!*text)
            return;

        if (com_argc == 1)
            com_args = text;

        text = COM_Parse (text, script);
        if (!text)
            return;

        if (com_argc < MAX_ARGS)
        {
            com_argv[com_argc] = Z_Malloc (strlen(com_token)+1, PU_STATIC, NULL);
            strcpy (com_argv[com_argc], com_token);
            com_argc++;
        }
    }

}


// Add a command before existing ones.
//
void COM_AddCommand( const char * name, com_func_t func, byte command_type )
{
    xcommand_t * cmd;

    // fail if the command is a variable name
    if (CV_StringValue(name)[0])
    {
        CONS_Printf ("%s is a variable name\n", name);
        return;
    }

    // fail if the command already exists
    for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
        if (!strcmp (name, cmd->name))
        {
            CONS_Printf ("Command %s already exists\n", name);
            return;
        }
    }

    cmd = Z_Malloc (sizeof(xcommand_t), PU_STATIC, NULL);
    cmd->name = name;
    cmd->function = func;
    cmd->cctype = command_type;
    cmd->next = com_commands;
    com_commands = cmd;
}


//  Returns true if a command by the name given exists
//
static boolean COM_Exists (const char * com_name)
{
    xcommand_t * cmd;

    for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
        if (!strcmp (com_name,cmd->name))
            return true;
    }

    return false;
}


//  Command completion using TAB key like '4dos'
//  Will skip 'skips' commands
//
//  partial : a partial keyword
const char * COM_CompleteCommand (const char * partial, int skips)
{
    xcommand_t * cmd;
    int len;

    len = strlen(partial);

    if (!len)
        return NULL;

// check functions
    for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
        if (!strncmp (partial,cmd->name, len))
        {
            if (!skips--)
                return cmd->name;
        }
    }

    return NULL;
}



// Parses a single line of text into arguments and tries to execute it.
// The text can come from the command buffer, a remote client, or stdin.
//
//   cfg : cv_config_e, to mark the cvar as to the source
// Called by: COM_BufExecute
static void COM_ExecuteString (const char * text, boolean script, byte cfg )
{
    xcommand_t * cmd;
    cmd_alias_t * a;

    COM_TokenizeString (text, script);

// execute the command line
    if (com_argc==0)
        return;     // no tokens

// check functions
    for (cmd=com_commands ; cmd ; cmd=cmd->next)
    {
        if (!strcmp (com_argv[0],cmd->name))
        {
            cmd->function ();
            return;
        }
    }

// check aliases
    for (a=com_alias ; a ; a=a->next)
    {
        if (!strcmp (com_argv[0], a->name))
        {
            COM_BufInsertText (a->value);
            return;
        }
    }

// check FraggleScript functions
    if (find_variable(com_argv[0])) // if this is a potential FS function, try to execute it
    {
//        run_string(text);
        return;
    }

    // check cvars
    // Hurdler: added at Ebola's request ;)
    // (don't flood the console in software mode with bad gr_xxx command)
    if (!CV_Var_Command( cfg ) && con_destlines)
    {
        CONS_Printf ("Unknown command '%s'\n", com_argv[0]);
    }
}



// =========================================================================
//                            SCRIPT COMMANDS
// =========================================================================


// alias command : a command name that replaces another command
//
static void COM_Alias_f (void)
{
    cmd_alias_t * a;
    char        cmd[1024];
    int         i;
    COM_args_t  carg;
    
    COM_Args( &carg );
   
    if ( carg.num < 3 )
    {
        CONS_Printf("alias <name> <command>\n");
        return;
    }

    a = Z_Malloc (sizeof(cmd_alias_t), PU_STATIC, NULL);
    a->next = com_alias;
    com_alias = a;

    a->name = Z_StrDup (carg.arg[1]);

// copy the rest of the command line
    cmd[0] = 0;     // start out with a null string
    for (i=2 ; i<carg.num ; i++)
    {
        register int n = 1020 - strlen( cmd );  // free space, with " " and "\n"
        strncat (cmd, COM_Argv(i), n);
        if (i != carg.num)
            strcat (cmd, " ");
    }
    strcat (cmd, "\n");

    a->value = Z_StrDup (cmd);
}


// Echo a line of text to console
//
static void COM_Echo_f (void)
{
    int     i;
    COM_args_t  carg;
  
    COM_Args( &carg );

    for (i=1 ; i<carg.num ; i++)
        CONS_Printf ("%s ",COM_Argv(i));
    CONS_Printf ("\n");
}


// Execute a script file
//
static void COM_Exec_f (void)
{
    byte*   buf=NULL;
    COM_args_t  carg;
   
    COM_Args( &carg );

    if (carg.num != 2)
    {
        CONS_Printf ("exec <filename> : run a script file\n");
        return;
    }

// load file

#ifdef DEBUG_EXEC_FILE_LENGTH
    int length;
    length = FIL_ReadFile (carg.arg[1], &buf);
    debug_Printf ("EXEC: file length = %d\n", length);
#else   
    FIL_ReadFile (carg.arg[1], &buf);
#endif

    if (!buf)
    {
        CONS_Printf ("couldn't execute file %s\n", carg.arg[1]);
        return;
    }

    CONS_Printf ("executing %s\n", carg.arg[1]);

// insert text file into the command buffer

    COM_BufInsertText((char *)buf);

// free buffer

    Z_Free(buf);
}


// Delay execution of the rest of the commands to the next frame,
// allows sequences of commands like "jump; fire; backward"
//
static void COM_Wait_f (void)
{
    COM_args_t  carg;
  
    COM_Args( &carg );
    if (carg.num>1)
        com_wait = atoi( carg.arg[1] );
    else
        com_wait = 1;   // 1 frame
}


// [WDJ] Categorized help.
typedef struct {
  const char * str;
  byte  cctype;  // cc type
  byte  varflag; // cvar flags
} help_cat_t;

#define NUM_HELP_CAT   13
static help_cat_t  helpcat_table[ NUM_HELP_CAT ] =
{
   {"INFO", CC_info, 0},
   {"CHEAT", CC_cheat, 0},
   {"COMMAND", CC_command, 0},
   {"SAVEGAME", CC_savegame, 0},
   {"CONFIG", CC_config, 0},
   {"CONTROL", CC_control, 0},
   {"FS", CC_fs, 0},  // fragglescript
   {"CHAT", CC_chat, 0},
   {"NET", CC_net, 0},
   {"CONSOLE", CC_console, 0},
   {"VAR", 0xFF, 0xFF},
   {"NETVAR", 0xFF, CV_NETVAR},
   {"CFGVAR", 0xFF, CV_SAVE},
};

static void COM_Help_f (void)
{
    xcommand_t * cmd;
    consvar_t * cvar;
    int i, k;
    uint32_t   varflag = 0;
    byte  cctype = 0;

    COM_args_t  carg;
    
    COM_Args( &carg );

    // [WDJ] Categorized help.
    if( carg.num < 2 )
    {
        CONS_Printf ("HELP <category>\n" );
        CONS_Printf ("   INFO CHEAT COMMAND SAVEGAME CONFIG CONTROL FS CHAT NET CONSOLE\n" );
        CONS_Printf ("   VAR NETVAR CFGVAR\n" );
        CONS_Printf ("HELP <varname>\n");
        return;
    }
   
    for( i = 1; i < carg.num; i++ )
    {
        for( k = 0; k < NUM_HELP_CAT; k++ )
        {
            if( strcasecmp( carg.arg[i], helpcat_table[k].str ) == 0 )
            {
                if( helpcat_table[k].cctype < 20 )
                {
                    cctype = helpcat_table[k].cctype;
                    break;
                }
                varflag |= helpcat_table[k].varflag;
                cctype = 0xF0; // var listing
            }
        }
    }

    if((carg.num>1) && (cctype == 0))
    {
        cvar = CV_FindVar (carg.arg[1]);
        if( cvar )
        {
            con_Printf("Variable %s:\n",cvar->name);
            con_Printf("  flags :");
            if( cvar->flags & CV_SAVE )
                con_Printf("AUTOSAVE ");
            if( cvar->flags & CV_FLOAT )
                con_Printf("FLOAT ");
            if( cvar->flags & CV_NETVAR )
                con_Printf("NETVAR ");
            if( cvar->flags & CV_CALL )
                con_Printf("ACTION ");
            con_Printf("\n");

            if( cvar->PossibleValue )
            {
                CV_PossibleValue_t * pv0 = cvar->PossibleValue;
                CV_PossibleValue_t * pv;
                if( strcasecmp(pv0->strvalue,"MIN") == 0 )
                {
                    // search for MAX
                    for( pv = pv0+1; pv->strvalue; pv++)
                    {
                        if( strcasecmp(pv->strvalue,"MAX") == 0 )
                            break;
                    }
                    con_Printf("  range from %d to %d\n", pv0->value, pv->value);
                }
                else
                {
                    con_Printf("  possible value :\n",cvar->name);
                    for( pv = pv0; pv->strvalue; pv++)
                    {
                        con_Printf("    %-2d : %s\n", pv->value, pv->strvalue);
                    }
                }
            }
        }
        else
            con_Printf("No Help for this command/variable\n");
       
        return;
    }
    
    i = 0; // cnt vars and commands
    if( cctype == 0 )
        varflag = 0xFFFF;  // all variables

    if( cctype < 20 )
    {
        // commands
        con_Printf("\2Commands\n");
        for (cmd=com_commands ; cmd ; cmd=cmd->next)
        {
          if( (cctype == 0) || (cctype == cmd->cctype) )
          {
            con_Printf("%s ",cmd->name);
            i++;
          }
        }
    }

    if( varflag )
    {
        // variable
        con_Printf("\2\nVariables\n");
        for (cvar=consvar_vars; cvar; cvar = cvar->next)
        {
          if( cvar->flags & varflag )
          {
            con_Printf("%s ",cvar->name);
            i++;
          }
        }
    }

    con_Printf("\2\nRead the console docs for more or type help <command or variable>\n");

    if( devparm > 1 )
            con_Printf("\2Total : %d\n",i);
}

static void COM_Toggle_f(void)
{
    consvar_t * cvar;
    COM_args_t  carg;
    
    COM_Args( &carg );

    if(carg.num!=2 && carg.num!=3)
    {
        CONS_Printf("Toggle <cvar_name> [-1]\n"
                    "Toggle the value of a cvar\n");
        return;
    }
    cvar = CV_FindVar (carg.arg[1]);
    if(!cvar)
    {
        CONS_Printf("%s is not a cvar\n", carg.arg[1]);
        return;
    }

    // netcvar don't change imediately
    cvar->flags |= CV_SHOWMODIF_ONCE;  // show modification, reset flag
    if( carg.num==3 )
        CV_ValueIncDec(cvar, atol( carg.arg[2] ));
    else
        CV_ValueIncDec(cvar,+1);
}

// =========================================================================
//                      VARIABLE SIZE BUFFERS
// =========================================================================

#define VSBUFMINSIZE   256

void VS_Alloc (vsbuf_t * buf, int initsize)
{
    if (initsize < VSBUFMINSIZE)
        initsize = VSBUFMINSIZE;
    buf->data = Z_Malloc (initsize, PU_STATIC, NULL);
    buf->maxsize = initsize;
    buf->cursize = 0;
    buf->allowoverflow = false;
}


void VS_Free (vsbuf_t * buf)
{
//  Z_Free (buf->data);
    buf->cursize = 0;
}


void VS_Clear (vsbuf_t * buf)
{
    buf->cursize = 0;
}


// Add length to the space in use.  Detect overflow.
void * VS_GetSpace (vsbuf_t * buf, int length)
{
    if (buf->cursize + length > buf->maxsize)
    {
        if (!buf->allowoverflow)
          return NULL;

        if (length > buf->maxsize)
          return NULL;

        buf->overflowed = true;
        CONS_Printf ("VS buffer overflow");
        VS_Clear (buf);
    }

    void * data = buf->data + buf->cursize;
    buf->cursize += length;

    return data;
}


#if 0
// [WDJ] Unused
//  Copy data at end of variable sized buffer
//
boolean VS_Write (vsbuf_t * buf, void * data, int length)
{
  void * dest = VS_GetSpace(buf, length);
  if (!dest)
    return false;

  memcpy(dest, data, length);
  return true;
}
#endif


//  Print text in variable size buffer, like VS_Write + trailing 0
//
boolean VS_Print (vsbuf_t * buf, const char * data)
{
  int len = strlen(data) + 1;
  int old_size = buf->cursize;  // VS_GetSpace modifies cursize
   
  // Remove trailing 0 before any consideration.
  // Otherwise the extra accumulates until garbage gets between the appends.
  if( old_size )
  {
      if( buf->data[old_size-1] == 0 )
         buf->cursize --;
  }

  // len-1 would be enough if there already is a trailing zero, but...
  byte * dest = (byte *)VS_GetSpace(buf, len);
  if (!dest)  goto fail_cleanup;
  
  memcpy(dest, data, len); 
  return true;
   
fail_cleanup:
  // Restore
  buf->cursize = old_size;
  return false;
}

// =========================================================================
//
//                           CONSOLE VARIABLES
//
//   console variables are a simple way of changing variables of the game
//   through the console or code, at run time.
//
//   console vars acts like simplified commands, because a function can be
//   attached to them, and called whenever a console var is modified
//
// =========================================================================

static char * cv_null_string = "";
byte    command_EV_param = 0;

static byte  OnChange_user_enable = 0;

static byte  CV_Pop_Config( consvar_t * cvar );
static void  CV_set_str_value( consvar_t * cvar, const char * valstr, byte call_enable, byte user_enable );


//  Search if a variable has been registered
//  returns true if given variable has been registered
//
consvar_t * CV_FindVar (const char * name)
{
    consvar_t * cvar;

    for (cvar=consvar_vars; cvar; cvar = cvar->next)
    {
        if ( !strcmp(name,cvar->name) )
            return cvar;
    }

    return NULL;
}


//  Build a unique Net Variable identifier number, that is used
//  in network packets instead of the fullname
//
uint16_t  CV_ComputeNetid (const char * s)
{
    uint16_t ret;
    static byte premiers[16] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53};
    int i;

    ret=0;
    i=0;
    while(*s)
    {
        ret += ((byte)(*s)) * ((unsigned int) premiers[i]);
        s++;
        i = (i+1)%16;
    }
    return ret;
}


//  Return the Net Variable, from it's identifier number
//
static consvar_t * CV_FindNetVar (uint16_t netid)
{
    consvar_t * cvar;

    for (cvar=consvar_vars; cvar; cvar = cvar->next)
    {
        if (cvar->netid==netid)
            return cvar;
    }

    return NULL;
}


//  Register a variable, that can be used later at the console
//
void CV_RegisterVar (consvar_t * cvar)
{
    // first check to see if it has already been defined
    if (CV_FindVar (cvar->name))
    {
        CONS_Printf ("Variable %s is already defined\n", cvar->name);
        return;
    }

    // check for overlap with a command
    if (COM_Exists (cvar->name))
    {
        CONS_Printf ("%s is a command name\n", cvar->name);
        return;
    }

    cvar->string = NULL;
    cvar->state = 0;

    // check net cvars
    if (cvar->flags & CV_NETVAR)
    {
        cvar->netid = CV_ComputeNetid (cvar->name);
        if (CV_FindNetVar(cvar->netid))
            I_Error("Variable %s has duplicate netid\n",cvar->name);
    }

    // link the cvar in
    if( !(cvar->flags & CV_HIDEN) )
    {
        cvar->next = consvar_vars;
        consvar_vars = cvar;
    }

#ifdef PARANOIA
    if ((cvar->flags & CV_NOINIT) && !(cvar->flags & CV_CALL))
        I_Error("variable %s has CV_NOINIT without CV_CALL\n",cvar->name);
    if ((cvar->flags & CV_CALL) && !cvar->func)
        I_Error("variable %s has CV_CALL without func",cvar->name);
#endif
    CV_set_str_value( cvar, cvar->defaultvalue,
                     ((cvar->flags & CV_NOINIT) == 0), // call_enable
                     1 );  // user_enable, default is a user setting
   

    // CV_set_str_value will have set this bit
    cvar->state &= ~CS_MODIFIED;
}


void CV_RegisterVar_list (consvar_t ** cvar_list )
{
    for( ; *cvar_list ; cvar_list++ ) // NULL terminated list
    {
        CV_RegisterVar( *cvar_list );
    }
}



//  Returns the string value of a console var
//
static char * CV_StringValue (const char * var_name)
{
    consvar_t * cvar;

    cvar = CV_FindVar (var_name);
    if (!cvar)
        return cv_null_string;
    return cvar->string;
}


//  Completes the name of a console var
//
const char * CV_CompleteVar (const char * partial, int skips)
{
    consvar_t * cvar;
    int len;

    len = strlen(partial);

    if (!len)
        return NULL;

    // check functions
    for (cvar=consvar_vars ; cvar ; cvar=cvar->next)
    {
        if (!strncmp (partial,cvar->name, len))
        {
            if (!skips--)
                return cvar->name;
        }
    }

    return NULL;
}

// [WDJ] Hard to tell yet which of the two methods has the least problems.
// For now, they both work, and are about the same size.
#define COMMAND_RECOVER_STRING
#ifdef COMMAND_RECOVER_STRING
static const char * cvar_string_min = NULL;
static const char * cvar_string_max = NULL;
#endif

// Free this string allocation, when it is not a PossibleValue const.
static
void  CV_Free_cvar_string_param( consvar_t * cvar, char * str )
{
#ifdef COMMAND_RECOVER_STRING
    // Check if is in bounds of allocated cvar strings.
    if( cvar_string_min
        && str >= cvar_string_min
        && str <= cvar_string_max )
    {
        // was allocated
        Z_Free( str );
    }
#else
    // It is Z_StrDup or a string from PossibleValue
    CV_PossibleValue_t * pv = cvar->PossibleValue;
    if( pv )
    {
        for( ; pv->strvalue ; pv++ )
        {
            if( str == pv->strvalue )
                return;  // was a ptr to a PossibleValue string
        }
    }

    // not in PossibleValue
    Z_Free( str );
#endif
}

// Free the cvar string allocation, when it is not a PossibleValue const.
void  CV_Free_cvar_string( consvar_t * cvar )
{
    if( cvar->string )
    {
        CV_Free_cvar_string_param( cvar, cvar->string );

        cvar->string = NULL;
    }
}

// Makes a copy of the string, and handles PossibleValue string values.
//   str : a reference to a string, it will be copied.
void  CV_Set_cvar_string( consvar_t * cvar, const char * str )
{
    CV_PossibleValue_t * pv;

    // Free an allocated existing string.
    CV_Free_cvar_string( cvar );

    // Check if str is Z_StrDup or a string from PossibleValue
    pv = cvar->PossibleValue;
    if( pv )
    {
        for( ; pv->strvalue ; pv++ )
        {
            // Only if it is a pointer to a PossibleValue string.
            if( str == pv->strvalue )
                goto update;  // just point to it, by reference
        }
    }

    // Have to copy it.
    str = Z_StrDup( str );
#ifdef COMMAND_RECOVER_STRING
    // Track range of allocated cvar strings.
    if( cvar_string_min == NULL || str < cvar_string_min )
        cvar_string_min = str;
    if( str > cvar_string_max )
        cvar_string_max = str;
#endif

update:
    // current cvar
    cvar->string = (char*) str;
    return;
}

// Get string for CV_PossibleValue_t
//  pv_value: a value in the CV_PossibleValue_t list
const char *  CV_get_possiblevalue_string( CV_PossibleValue_t * pv,  byte pv_value )
{
    while( pv->strvalue )
    {
        if( pv->value == pv_value )
            return pv->strvalue;
        pv++;
    }
    return NULL;
}

// Do the CV_CALL, with validity tests, and enforcing user_enable rules.
void  CV_cvar_call( consvar_t * cvar, byte user_enable )
{
    // Call the CV_CALL func to restore state dependent upon this setting.
    // Set the 'on change' code.
    if((cvar->flags & CV_CALL) && (cvar->func))
    {
        // Handle recursive OnChange calls. Propagate a valid user_enable.
        byte cfg = cvar->state & CS_CONFIG;
        OnChange_user_enable = user_enable && ((cfg >= CFG_main) && (cfg <= CFG_drawmode));
        cvar->func();
    }
}


// Set variable value, for user settings, save games, and network settings.
// Updates value and EV.
// Does NOT relay NETVAR to clients.
//  call_enable : when 0, blocks CV_CALL
//  user_enable : enable setting the string value which gets saved in config files.
static
void  CV_set_str_value( consvar_t * cvar, const char * valstr, byte call_enable, byte user_enable )
{
    char  value_str[64];  // print %d cannot exceed 64
    CV_PossibleValue_t * pv0, * pv;
    int  ival;
    byte is_a_number = 0;

#ifdef PARANOIA
    if( valstr == NULL )
    {
        I_SoftError( "CV_set_str_value passed NULL string: %s\n", cvar->name );
        return;
    }
#endif

    // [WDJ] If the value is a float, then all comparisons must be fixed_t.
    // Any PossibleValues would be fixed_t too.
    if( cvar->flags & CV_FLOAT )
    {
        // store as fixed_t
        double d = atof( valstr );
        ival = (int)(d * FRACUNIT);
    }
    else
    {
        ival = atoi(valstr);  // enum and integer values
    }

    if( ival )
    {
        is_a_number = 1;  // atoi or atof found a number
    }
    else
    {
        // Deal with the case where there are leading spaces.
        const char * c = valstr;
        while( *c == ' ' )  c++;  // skip spaces
        is_a_number = (*c >= '0' && *c <= '9');
    }

    pv0 = cvar->PossibleValue;
    if( pv0 )
    {
        if( strcasecmp(pv0->strvalue,"MIN") == 0 )
        {   // bounded cvar
            // search for MAX
            for( pv = pv0+1; pv->strvalue; pv++)
            {
                if( strcasecmp(pv->strvalue,"MAX") == 0 )
                    break;
            }

#ifdef PARANOIA
            if( pv->strvalue == NULL )
                I_Error("Bounded cvar \"%s\" without MAX !", cvar->name);
#endif
            // PossibleValue is MIN MAX, so value must be a number.
            if( ! is_a_number )  goto error;

            // [WDJ] Cannot print into const string.
            if(ival < pv0->value)  // MIN value
            {
                ival = pv0->value;
                sprintf(value_str,"%d", ival);
                valstr = value_str;
            }
            if(ival > pv->value)  // supposedly MAX value
            {
                ival = pv->value;
                sprintf(value_str,"%d", ival);
                valstr = value_str;
            }
        }
        else
        {
            // waw spaghetti programming ! :)

            // check for string match
            for( pv = pv0; pv->strvalue; pv++)
            {
                if( strcasecmp(pv->strvalue, valstr) == 0 )
                    goto found_possible_value;
            }

            // If valstr is not a number, then it cannot be used as a PossibleValue.
            if( ! is_a_number )  goto error;

            // check as PossibleValue number
            for( pv = pv0; pv->strvalue; pv++)
            {
                if( ival == pv->value )
                    goto found_possible_value;
            }
            goto error;

    found_possible_value:
            ival = pv->value;
            if( user_enable )
            {
                // [WDJ] Used to assume existing was a const string, whenever the new string
                // was a const string.  Cannot prove that assumption so call CV_Free.
                CV_Free_cvar_string( cvar );
                cvar->value = ival;
                // When value is from PossibleValue, string is a const char *.
                cvar->string = (char*) pv->strvalue;
            }
            goto finish;
        }
    }

    // CV_STRING has no temp values, and is used for network addresses.
    // Block it for security reasons, to prevent redirecting.
    // Only change the cvar string when user is making the change.
    if( user_enable )
    {
        // free the old value string, set the new value
        CV_Set_cvar_string( cvar, valstr );
    }

    // Update value when set by user, or if flagged as numeric value.
    // CV_uint16, CV_Unsigned values may not fit into EV.
    if( cvar->flags & (CV_FLOAT | CV_VALUE) )
    {
        if( ! is_a_number )  goto error;
        cvar->value = ival;
    }
    else if( user_enable )
    {
        cvar->value = ival;
    }


finish:
    // The SHOWMODIF is display of CV_Set, and not other set paths.
    if( cvar->flags & (CV_SHOWMODIF | CV_SHOWMODIF_ONCE) )
    {
        CONS_Printf("%s set to %s\n", cvar->name, valstr );
        cvar->flags &= ~CV_SHOWMODIF_ONCE;
    }
    DEBFILE(va("%s set to %s\n", cvar->name, cvar->string));

    cvar->state |= CS_MODIFIED;
    cvar->EV = ival;  // user setting of active value

    // raise 'on change' code
    if( call_enable )
        CV_cvar_call( cvar, user_enable );
    return;

error: // not found
    CONS_Printf("\"%s\" is not a possible value for \"%s\"\n", valstr, cvar->name);
    if( strcasecmp(cvar->defaultvalue, valstr) == 0 )
    { 
        I_SoftError("Variable %s default value \"%s\" is not a possible value\n",
                    cvar->name, cvar->defaultvalue);
    }
    return;
}

// Called after demo to restore the user settings.
// Copies value to EV.
void CV_Restore_User_Settings( void )
{
    consvar_t * cvar;

    // Check for modified cvar
    for (cvar=consvar_vars; cvar; cvar = cvar->next)
    {
        if( cvar->state & CS_EV_PROT )  // protected EV value
            continue;

        if((cvar->state & CS_CONFIG) > CFG_other )
        {
            // Undo a push of NETVAR
            CV_Pop_Config( cvar );  // CV_CALL
            cvar->state &= ~CS_EV_PARAM;
            continue;
        }

        if( cvar->flags & CV_VALUE )
        {
            cvar->value = atoi( cvar->string );
        }

        if( (cvar->EV != (byte)cvar->value)
            || (cvar->value >> 8)
            || (cvar->state & CS_EV_PARAM) )  // command line param in EV
        {
            cvar->EV = cvar->value;  // user setting of active value
            CV_cvar_call( cvar, 1 );
            cvar->state &= ~CS_EV_PARAM;
        }
    }
    command_EV_param = 0;
}


//
// Use XD_NETVAR argument :
//      2 byte for variable identification
//      then the value of the variable followed with a 0 byte (like str)
//
// Receive network game settings, or restore save game.
void Got_NetXCmd_NetVar(xcmd_t * xc)
{
    byte * bp = xc->curpos;	// macros READ,SKIP want byte*

    consvar_t * cvar = CV_FindNetVar(READU16(bp));  // netvar id
    char * svalue = (char *)bp;  // after netvar id

    while( *(bp++) ) {  // find 0 term
       if( bp > xc->endpos )  goto buff_overrun;  // bad string
    }
    xc->curpos = bp;	// return updated ptr only once

    if(cvar==NULL)
    {
        CONS_Printf("\2Netvar not found\n");
        return;
    }

    if( cvar->flags & (CV_FLOAT | CV_VALUE | CV_STRING))
    {
        // Netvar value will not fit in EV, so use NETVAR push.
        CV_Put_Config_string( cvar, CFG_netvar, svalue );
        // Current config is netvar setting (not saved).
    }
    else
    {
        // Put netvar value in EV.
        CV_set_str_value(cvar, svalue, 1, 0);  // CV_CALL, temp
        // Current config is netvar setting (not saved).
        // Not visible to menu. Menu displays the string.
    }
    return;

buff_overrun:
    xc->curpos = xc->endpos+2;  // indicate overrun
    return;
}


// Called by SV_Send_ServerConfig, P_Savegame_Save_game.
void CV_SaveNetVars(xcmd_t * xc)
{
    char buf[32];
    char * vp;
    consvar_t * cvar;
    byte * bp = xc->curpos;	// macros want byte*
    

    // We must send all NETVAR cvar, because on another machine,
    // some of them may have a different value.
    for (cvar=consvar_vars; cvar; cvar = cvar->next)
    {
        if( ! (cvar->flags & CV_NETVAR) )  continue;

        // Command line settings goto network games and savegames.
        // CV_STRING do not have temp values.
        if( cvar->state & CS_EV_PARAM )  // command line param in EV
        {
            // Send the EV param value instead.
            sprintf (buf, "%d", cvar->EV);
            vp = buf;
        }
        else if( cvar->flags & CV_VALUE )
        {
            // Value has precedence over the string.
            sprintf (buf, "%d", cvar->value);
            vp = buf;
        }
        else
        {
            vp = cvar->string;	       
        }
        // potential buffer overrun test
        if((bp + 2 + strlen(vp)) > xc->endpos )  goto buff_overrun;
        // Format:  netid uint16, var_string str0.
        WRITE16(bp,cvar->netid);
        bp = write_string(bp, vp);
    }
    xc->curpos = bp;	// return updated ptr only once
    return;

buff_overrun:
    I_SoftError( "Net Vars overrun available packet space\n" );
    return;
}

// Client: Receive server netvars state.  Server config.
void CV_LoadNetVars(xcmd_t * xc)
{
    consvar_t * cvar;

    // Read netvar from byte stream, identified by netid.
    for (cvar=consvar_vars; cvar; cvar = cvar->next)
    {
        if (cvar->flags & CV_NETVAR)
            Got_NetXCmd_NetVar( xc );
        // curpos on last read can go to endpos+1
        if(xc->curpos > xc->endpos+1)  goto buff_overrun;
    }
    return;

buff_overrun:
    I_SoftError( "Load Net Vars overran packet buffer\n" );
    return;
}

#define SET_BUFSIZE 128

// PUBLIC

// Sets a var to a string value.
// called by CV_Var_Command to handle "<varname> <value>" entered at the console
void CV_Set (consvar_t * cvar, const char * str_value)
{
    //changed = strcmp(var->string, value);
#ifdef PARANOIA
    if(!cvar)
        I_Error("CV_Set : no variable\n");

    // Not an error if cvar does not have string.
#endif

    if( cvar->string )
    {
      if( strcasecmp(cvar->string, str_value) == 0 )
        return; // no changes
    }

    if (netgame)
    {
      // in a netgame, certain cvars are handled differently
      if (cvar->flags & CV_NET_LOCK)
      {
        CONS_Printf("This variable cannot be changed during a netgame.\n");
        return;
      }

      if( cvar->flags & CV_NETVAR )
      {
        if (!server)
        {
            CONS_Printf("Only the server can change this variable.\n");
            return;
        }

        // Change user settings too, but want only one CV_CALL.
        CV_set_str_value(cvar, str_value, 0, 1); // no CALL, user

        // send the value of the variable
        byte buf[SET_BUFSIZE], *p; // macros want byte*
        p = buf;
        // Format:  netid uint16, var_string str0.
        WRITEU16(p, cvar->netid);
        p = write_stringn(p, str_value, SET_BUFSIZE-2-1);
        SV_Send_NetXCmd(XD_NETVAR, buf, (p - buf));  // as server
        // NetXCmd will set as netvar, CV_CALL, not user.
        // This NetXCmd is also used by savegame restore, so it cannot block server.
        return;
      }
    }

    // Single player
    CV_set_str_value(cvar, str_value, 1, 1);  // CALL, user
}


//  Expands value to string before calling CV_Set ()
//
void CV_SetValue (consvar_t * cvar, int value)
{
    char    val[32];

    sprintf (val, "%d", value);
    CV_Set (cvar, val);
}

// Set a command line parameter value (temporary).
// This should not affect owner saved values.
void CV_SetParam (consvar_t * cvar, int value)
{
    command_EV_param = 1;  // flag to undo these later
    cvar->EV = value;   // temp setting, during game play
    cvar->state |= CS_EV_PARAM;
    CV_cvar_call( cvar, 0 );  // not user
}

// If a OnChange func tries to change other values,
// this function should be used.
// It will determine the same user_enable.
void CV_Set_by_OnChange (consvar_t * cvar, int value)
{
    byte saved_user_enable = OnChange_user_enable;
    if( OnChange_user_enable )
    {
        CV_SetValue( cvar, value );
    }
    else
    {
        CV_SetParam( cvar, value );
    }
    OnChange_user_enable = saved_user_enable;
}



void CV_ValueIncDec (consvar_t * cvar, int increment)
{
    int   newvalue = cvar->value + increment;
    CV_PossibleValue_t *  pv0 = cvar->PossibleValue;  // array of

    if( pv0 )
    {
        // If first item in list is "MIN"
        if( strcmp( pv0->strvalue,"MIN") == 0 )
        {
            // MIN .. MAX
            int min_value = pv0->value;  // MIN value
            int max_value = INT_MAX;
            CV_PossibleValue_t *  pv;

            // Search the list for MAX value, or INC.
            for( pv = pv0; pv->strvalue ; pv++ )
            {
                if( strcmp(pv->strvalue,"INC") == 0 )
                {
                    // Has an INC
                    newvalue = cvar->value + (increment * pv->value);
                }
                else
                {
                    max_value = pv->value;  // last value is assumed "MAX"
                }
            }

            if( newvalue < min_value )
            {
                // To accomodate negative increment.
                newvalue += max_value - min_value + 1;   // add the max+1
            }
            newvalue = min_value
             + ((newvalue - min_value) % (max_value - min_value + 1));

            CV_SetValue(cvar,newvalue);
        }
        else
        {
            // List of Values
            int max, currentindice=-1;

            // this code do not support more than same value for differant PossibleValue
            for( max = 0; ; max++ )
            {
                if( pv0[max].strvalue == NULL )  break;  // end of list
                if( pv0[max].value == cvar->value )
                    currentindice = max;
            }
            // max is at NULL, has count of possible value list
#ifdef PARANOIA
            if( currentindice == -1 )
            {
                I_SoftError("CV_ValueIncDec : current value %d not found in possible value\n", cvar->value);
                return;
            }
#endif
            // calculate position in possiblevalue
            // max is list count
            // To accommodate neg increment, add extra list count.
            // Modulo result back into the possible value range,
            int newindice = ( currentindice + increment + max) % (max);
            CV_Set(cvar, pv0[newindice].strvalue);
        }
    }
    else
    {
        CV_SetValue(cvar,newvalue);
    }
}


// =================
// Pushed cvar values

typedef struct cv_pushed_s {
    struct cv_pushed_s * next;
    consvar_t  * parent; 
    char *  string;  // value in string
    int32_t  value;  // for int and fixed_t
    byte     state;  // cv_state_e
} cv_pushed_t;

cv_pushed_t *  cvar_pushed_list = NULL;  // malloc


// Frees the string and the pushed record.
static
void  release_pushed_cvar( cv_pushed_t * pp_rel )
{
    // unlink from pushed list
    if( cvar_pushed_list == pp_rel )
    {
        cvar_pushed_list = pp_rel->next;
    }
    else
    {
        // find the pp_rel in the list.
        cv_pushed_t * pp;
        for( pp = cvar_pushed_list; pp ; pp = pp->next )
        {
            if( pp->next == pp_rel )
            {
                // found it, unlink it.	       
                pp->next = pp_rel->next;
                break;
            }
        }
    }

    // The string must be freed.
    if( pp_rel->string )
        CV_Free_cvar_string_param( pp_rel->parent, pp_rel->string );

    free( pp_rel );
}

static
cv_pushed_t * create_pushed_cvar( consvar_t * parent_cvar )
{
    cv_pushed_t * pp = (cv_pushed_t*) malloc( sizeof(cv_pushed_t) );
    if( pp )
    {
        // link into the push list
        pp->next = cvar_pushed_list;
        cvar_pushed_list = pp;

        pp->parent = parent_cvar;
    }
    return pp;
}

// Search the pushed cv for a matching config.
static 
cv_pushed_t *  find_pushed_cvar( consvar_t * cvar, byte cfg )
{
    cv_pushed_t * pp;
    for( pp = cvar_pushed_list; pp ; pp = pp->next )
    {
        if( (pp->parent == cvar)
            && ((pp->state & CS_CONFIG) == cfg) )
        {
             return pp;
        }
    }
    return NULL;  // not found
}

// update the CS_PUSHED flag
static
void  update_pushed_cvar( consvar_t * cvar )
{
    cv_pushed_t * pp;
    for( pp = cvar_pushed_list; pp ; pp = pp->next )
    {
        if( pp->parent == cvar )
        {
             cvar->state |= CS_PUSHED;
             return;
        }
    }
    // none found
    cvar->state &= ~CS_PUSHED;
}

//  new_cfg : the new config that is causing the push
static
void  CV_Push_Config( consvar_t * cvar, byte new_cfg )
{
    cv_pushed_t * pp = create_pushed_cvar( cvar );
    if( ! pp )
        return;

    // save cvar values
    pp->string = cvar->string;  // move the string
    cvar->string = NULL;
    pp->value = cvar->value;
    pp->state = cvar->state;

    // update the current cvar
    cvar->state = (cvar->state & ~CS_CONFIG) | new_cfg | CS_PUSHED;
}

// return 1 if pop succeeded, 0 if no pop.
static
byte  CV_Pop_Config( consvar_t * cvar )
{
    cv_pushed_t * pp;   
    byte old_config = (cvar->state & CS_CONFIG);
    while( --old_config )
    {
        pp = find_pushed_cvar( cvar, old_config );
        if( pp )  goto restore_cvar;
    }
    return 0;
   
restore_cvar:
    // restore cvar values
    // Move the pushed string to the cvar.
    CV_Free_cvar_string( cvar );
    cvar->string = pp->string;  // move string, as pushed cvar will be released
    pp->string = NULL;  // because the string in the pushed record will be freed.

    // If this happens during a demo or other usage, protections have already been applied.
    cvar->EV = pp->value;
    cvar->value = pp->value;
    cvar->state = pp->state;

    release_pushed_cvar( pp );
    update_pushed_cvar( cvar );
   
    CV_cvar_call( cvar, 1 );  // Pop brings user settings back into force.
    return 1;
}

// Public

// Get the values of a pushed cvar, into the temp cvar.
//   pushed_cvar : copy of the cvar values, is assumed to be uninitialized,
//                 any lingering string value will not be freed
//   temp_cvar : an uninitialized cvar to receive the value
//               If NULL, then is just a check on existance.
// Return false if not found.
boolean  CV_Get_Pushed_cvar( consvar_t * cvar, byte cfg, /*OUT*/ consvar_t * temp_cvar )
{
    cv_pushed_t * pp = find_pushed_cvar( cvar, cfg );
    if( !pp )
        return false;

    if( temp_cvar )
    {
        memcpy( temp_cvar, cvar, sizeof(consvar_t) );  // setup values

        temp_cvar->string = NULL;  // must be Z_StrDup, not copied
        CV_Set_cvar_string( temp_cvar, pp->string );   // may get edited

        temp_cvar->value = pp->value;
        temp_cvar->state = pp->state;
        // EV is not needed
    }
    return true;
}


// Put the values in the temp cvar, into the pushed or current cvar.
//   temp_cvar : copy of the cvar values, cannot be NULL
// The string value of temp_cvar will be stolen.
void  CV_Put_Config_cvar( consvar_t * cvar, byte cfg, /*IN*/ consvar_t * temp_cvar )
{
    cv_pushed_t * pp;
    byte current_cfg = cvar->state & CS_CONFIG;
   
    if( cfg == current_cfg )
        goto update_cvar;  // put to the current cvar

    if( cfg > current_cfg )
    {
        // push the current cvar
        CV_Push_Config( cvar, cfg ); // change current to cfg
        goto update_cvar;  // put to the new current cvar
    }
   
    // assert (cfg < current_cfg)
    // Put to the pushed cvar record.
    pp = find_pushed_cvar( cvar, cfg );
    if( pp )
    {
        // Free the existing string value first.
        CV_Free_cvar_string_param( cvar, pp->string );
    }
    else
    {
        // Not found, make a new one.
        pp = create_pushed_cvar( cvar );
        if( pp == NULL )  return;
    }
    // assert (pp->string == NULL or invalid)
    // Move the temp_cvar string to the pushed record.
    pp->string = temp_cvar->string;
    temp_cvar->string = NULL;

    pp->value = temp_cvar->value;
    pp->state = (temp_cvar->state & ~CS_CONFIG) | cfg;
    cvar->state |= CS_PUSHED;  // Mark existance of pushed cfg.
    return;

update_cvar:   
    // Update current cvar
    CV_Free_cvar_string( cvar );
    cvar->string = temp_cvar->string; // move the string
    temp_cvar->string = NULL;

    // If this happens during a demo or other usage, protections have already been applied.
    cvar->value = temp_cvar->value;
    cvar->EV = temp_cvar->value;
    // preserve CS_PUSHED
    cvar->state = (cvar->state & ~CS_CONFIG) | cfg | (temp_cvar->state & CS_MODIFIED);

    // Current cvar always does CV_CALL.
    CV_cvar_call( cvar, 1 );  // user_enable detected in new config.
    return;
}


// Return the string value of the config var, current or pushed.
// Return NULL if not found.
const char *  CV_Get_Config_string( consvar_t * cvar, byte cfg )
{
    // Check the current cvar first.
    if( (cvar->state & CS_CONFIG) == cfg )
        return cvar->string;

    if( cvar->state & CS_PUSHED )
    {
        cv_pushed_t * pp = find_pushed_cvar( cvar, cfg );
        if( pp )
            return pp->string;
    }

    return NULL;  // not found
}

// Put the string value to the pushed or current cvar.
// This will create or push, as needed.
//   str : str value, will be copied.  Will set numeric value too.
void  CV_Put_Config_string( consvar_t * cvar, byte cfg, const char * str )
{
    consvar_t  new_cvar;

    // Copy the parent cvar, need PossibleValue and flags.
    memcpy( &new_cvar, cvar, sizeof(consvar_t) );
    new_cvar.string = NULL;
    if( (cvar->state & CS_CONFIG) > cfg )
    {
        // Create as pushed cvar value.
        // Kill any effects that only the current cvar should perform.
        // Pushed cvar does not save these flags, flags will be gotten from parent.
        new_cvar.flags &= ~( CV_CALL | CV_NETVAR | CV_SHOWMODIF | CV_SHOWMODIF_ONCE );
    }
   
    // Copy the new value into the temp cvar.
    CV_set_str_value( &new_cvar, str, 0, 1 );

    // Create the cvar value, even if it is pushed and not current.
    // This will steal the string value from temp_cvar.  Do not need to Z_Free it.
    CV_Put_Config_cvar( cvar, cfg, &new_cvar );  // does CV_CALL
}



// Remove the cvar value for the config.
void  CV_Delete_Config_cvar( consvar_t * cvar, byte cfg )
{
    // Check current cvar first
    if( (cvar->state & CS_CONFIG) == cfg )
    {
        // Remove the current cvar
        if( CV_Pop_Config( cvar ) == 0 )
        {
            // No pushed value found
            cvar->state &= ~CS_CONFIG; // set config to 0	
        }
    }
    else
    {
        // Remove the pushed cvar.
        cv_pushed_t * pp = find_pushed_cvar( cvar, cfg );
        if( pp )
        {
            release_pushed_cvar( pp );  // free the string too
        }
    }
}


// Clear all values that came from the config file.
void  CV_Clear_Config( byte cfg )
{
    consvar_t * cv;

    // Clear from pushed list
    cv_pushed_t * pp = cvar_pushed_list;
    while( pp )
    {
        cv_pushed_t * ppn = pp->next;
        if((pp->state & CS_CONFIG) == cfg )
        {
            cv = pp->parent;  // get it before releasing
            release_pushed_cvar( pp );  // free the string too
            update_pushed_cvar( cv );  // update CS_PUSHED
        }
        pp = ppn;
    }

    // Clear from current cv vars.
    for( cv = CV_IteratorFirst(); cv ; cv = CV_Iterator(cv))
    {
        if( (cv->state & CS_CONFIG) == cfg )
            CV_Pop_Config(cv);
    }
}


// Check for drawmode CV variables.
// Return true if any value of the config is current, or pushed.
boolean CV_Config_check( byte cfg )
{
    consvar_t * cv;
    for( cv = CV_IteratorFirst(); cv ; cv = CV_Iterator( cv ) )
    {
        if( (cv->flags & CV_SAVE) == 0 )   continue;

        if( (cv->state & CS_CONFIG) == cfg )
            return true;	

        if( cv->state & CS_PUSHED )
        {
            if( CV_Get_Pushed_cvar( cv, cfg, NULL ) )
                return true;
        }
    }
    return false;
}



// =================
//


//  Allow display of variable content or change from the console
//
//  Returns false if the passed command was not recognised as
//  console variable.
//
//   cfg : cv_config_e
static boolean CV_Var_Command ( byte cfg )
{
    consvar_t * cvar;
    const char * tstr;
    COM_args_t  carg;
    int tval;
    
    COM_Args( &carg );

    // check variables
    cvar = CV_FindVar ( carg.arg[0] );
    if(!cvar)
        return false;

    // perform a variable print or set
    if ( carg.num == 1 )  goto show_value;

    // Set value
    cfg &= CS_CONFIG;  // only config selection
    if( cfg )
    {
        if( cvar->flags & CV_CFG1 )
        {
            // A restricted var cannot be loaded from the other config files.
            if( cfg != CFG_main )
                return false;
        }
        if( cvar->flags & CV_NETVAR )
        {
            // A NETVAR cannot be loaded from the drawmode config file.
            if( cfg == CFG_drawmode )
                return false;
        }

        byte old_cfg = cvar->state & CS_CONFIG;
        if( old_cfg )
        {
            // current cvar values are not the default values.
            if( old_cfg < cfg )
            {
                // Push the current value
                CV_Push_Config( cvar, cfg );
            }
            else if( old_cfg > cfg )
            {
                // It likely was pushed already.
                // Do not use CV_Set, because this will not become the current value.
                CV_Put_Config_string( cvar, cfg, carg.arg[1] );
                return true;
            }
        }

        // Record which config file the setting comes from.
        cvar->state &= ~CS_CONFIG;
        cvar->state |= cfg; 
    }

    CV_Set (cvar, carg.arg[1] );
    return true;

show_value:
    if( cvar->flags & CV_STRING )  goto std_show_str;
    if( cvar->flags & CV_VALUE )
    {
        if( cvar->value == atoi(cvar->string) )  goto std_show_str;
        tval = cvar->value;
    }
    else if( (cvar->state & CS_EV_PARAM)  // command line param in EV
        || (cvar->EV != (byte)cvar->value) )
    {
        tval = cvar->EV;
    }
    else goto std_show_str;
   
    if( cvar->PossibleValue )
    {
        // Search the PossibleValue for the value.
        CV_PossibleValue_t * pv;
        for( pv = cvar->PossibleValue; pv->strvalue; pv++)
        {
            if( pv->value == tval )
            {
                tstr = pv->strvalue;
                goto show_by_str;
            }
        }
    }

    // show by value
    CONS_Printf ("\"%s\" is \"%i\" config \"%s\" default is \"%s\"\n",
                 cvar->name, tval, cvar->string, cvar->defaultvalue);
    return true;

show_by_str:
    CONS_Printf ("\"%s\" is \"%s\" config \"%s\" default is \"%s\"\n",
                 cvar->name, tstr, cvar->string, cvar->defaultvalue);
    return true;

std_show_str:
    CONS_Printf ("\"%s\" is \"%s\" default is \"%s\"\n",
                 cvar->name, cvar->string, cvar->defaultvalue);
    return true;
}



//  Support for saving the console variables that have the CV_SAVE flag set.
//  This has less splitting of the logic between three functions,
//  and does not require passing a FILE ptr around.
consvar_t *  CV_IteratorFirst( void )
{
    return consvar_vars;
}

consvar_t *  CV_Iterator( consvar_t * cv )
{
    return cv->next;
}



//============================================================================
//                            SCRIPT PARSE
//============================================================================

//  Parse a token out of a string, handles script files too
//  returns the data pointer after the token
//  Do not mangle filenames, set script only where strings might have '\' escapes.
static const char * COM_Parse (const char * data, boolean script)
{
    int c;
    int len = 0;
    com_token[0] = '\0';

    if (!data)
        return NULL;

// skip whitespace
skipwhite:
    while ( (c = *data) <= ' ')
    {
        if (!c)
            return NULL;            // end of file;
        data++;
    }

// skip // comments
    // Also may be Linux filename: //home/user/.legacy
    if ( script && (c == '/' && data[1] == '/'))
    {
        while (*data && *data != '\n')
            data++;
        goto skipwhite;
    }


// handle quoted strings specially
    if (c == '"')
    {
        data++;
        while ( len < COM_TOKEN_MAX-1 )
        {
            c = *data++;
            if (!c)
            {
              // NUL in the middle of a quoted string. Missing closing quote?
              CONS_Printf("Error: Quoted string ended prematurely.\n");
              goto term_done;
            }

            if (c == '"') // closing quote
              goto term_done;

            if ( script && (c == '\\')) // c-like escape sequence
            {
              switch (*data)
              {
              case '\\':  // backslash
                com_token[len++] = '\\'; break;

              case '"':  // double quote
                com_token[len++] = '"'; break;

              case 't':  // tab
                com_token[len++] = '\t'; break;

              case 'n':  // newline
                com_token[len++] = '\n'; break;

              default:
                CONS_Printf("Error: Unknown escape sequence '\\%c'\n", *data);
                break;
              }

              data++;
              continue;
            }

            // normal char
            com_token[len++] = c;
        }
    }

// parse single characters
    // Also ':' can appear in WIN path names
    if (script && (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':'))
    {
      if( len >= COM_TOKEN_MAX-2 )  goto term_done;
      com_token[len++] = c;
      data++;
      goto term_done;
    }

// parse a regular word
    do
    {
      if( len >= COM_TOKEN_MAX-2 )  goto term_done;
      com_token[len++] = c;
      data++;
      c = *data;
      if (script && (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':'))
        break;
    } while (c > ' ');

term_done:   
    com_token[len] = '\0';
    return data;
}
