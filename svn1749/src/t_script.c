// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_script.c 1474 2019-10-15 12:34:14Z wesleyjohnson $
//
// Copyright(C) 2000 Simon Howard
// Copyright (C) 2001-2011 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// $Log: t_script.c,v $
// Revision 1.3  2004/09/17 23:04:49  darkwolf95
// playerkeysb (see comment), waittic and clocktic
//
// Revision 1.2  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------
//
// scripting.
//
// delayed scripts, running scripts, console cmds etc in here
// the interface between FraggleScript and the rest of the game
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
//#include "c_net.h"
//#include "c_runcmd.h"
#include "g_game.h"
#include "r_state.h"
#include "p_info.h"
#include "p_mobj.h"
#include "p_spec.h"
#include "p_setup.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_script.h"
#include "t_parse.h"
#include "t_vari.h"
#include "t_func.h"

void clear_runningscripts( void );

//                  script tree:
//
//                     global_script
//                  /                 \.
//           hubscript                 thingscript
//          /         \                  /     \.
//    levelscript    [levelscript]    ... scripts ...
//     /      \          /      \.
//  ... scripts...   ... scripts ...
//

// the level script is just the stuff put in the wad,
// which the other scripts are derivatives of
script_t  fs_levelscript;



// the thing script
//script_t thingscript;

// the individual scripts
//script_t *scripts[MAXSCRIPTS];       // the scripts

runningscript_t  fs_runningscripts;        // first in chain

mobj_t *  fs_run_trigger; // the trigger parameter on RunScript

//     T_Init()
//
//    called at program start

void T_Init_FS( void )
{
  T_Init_variables();
  T_Init_functions();
}

//
// T_Clear_Scripts()
//
// called at level start, clears all scripts
//
void T_Clear_Scripts( void )
{
  int i;
  
  // stop runningscripts
  clear_runningscripts();

  // clear the levelscript
  fs_levelscript.data = Z_Malloc(5, PU_LEVEL, 0);  // empty data
  fs_levelscript.data[0] = '\0';
  
  fs_levelscript.scriptnum = -1;
  fs_levelscript.parent = &hub_script;

  // clear levelscript variables
  
  for(i=0; i<VARIABLESLOTS; i++)
  {
      fs_levelscript.variables[i] = NULL;
  }
}

void T_LoadThingScript( void )
{
/*  char *scriptlump;
  lumpnum_t lumpnum;
  int lumplen;
  
  if(thingscript.data)
    Z_Free(thingscript.data);

  // load lump into thingscript.data

  // get lumpnum, lumplen
  
  lumpnum = W_CheckNumForName("THINGSCR");
  if( ! VALID_LUMP(lumpnum) )
    return;
  
  lumplen = W_LumpLength(lumpnum);

  // alloc space for lump and copy lump data into it
  
  thingscript.data = Z_Malloc(lumplen+10, PU_STATIC, 0);
  scriptlump = W_CacheLumpNum(lumpnum, PU_CACHE);

  memcpy(thingscript.data, scriptlump, lumplen);

  // add '\0' to end of string

  thingscript.data[lumplen] = '\0';

  // preprocess script

  preprocess(&thingscript);

  // run script

  thingscript.trigger = players[0].mo;
  run_script(&thingscript);  */
}



void T_PreprocessScripts( void )
{
  // run the levelscript first
  // get the other scripts
  
  // levelscript started by player 0 'superplayer'
  fs_levelscript.trigger = players[0].mo;
  
  preprocess(&fs_levelscript);
  run_script(&fs_levelscript);

  // load and run the thing script
  T_LoadThingScript();
}



void T_RunScript(int scriptnum, mobj_t * t_trigger )
{
  script_t *script;

  if(scriptnum<0 || scriptnum>=MAXSCRIPTS) return;

  // use the level's child script[ scriptnum ]
  script = fs_levelscript.children[scriptnum];
  if(!script) return;
 
  fs_run_trigger = t_trigger;
  script->trigger = t_trigger;    // save trigger in script
  
  run_script(script);
}


#if 0
// T_RunThingScript:
// identical to T_RunScript but runs a script
// from the thingscript list rather than the
// levelscript list

void T_RunThingScript(int n, mobj_t * t_trigger )
{
/*  script_t *script;
  
  if(scriptnum<0 || scriptnum>=MAXSCRIPTS) return;

  // use the things's script[ scriptnum ]
  script = thingscript.children[scriptnum];
  if(!script) return;
 
  fs_run_trigger = t_trigger;
  script->trigger = t_trigger;    // save trigger in script
  
  run_script(script);*/
}
#endif



// console scripting debugging commands

void COM_T_DumpScript_f (void)
{
  script_t *script;
  
  if(COM_Argc() < 2)
  {
      CONS_Printf("usage: T_DumpScript <scriptnum>\n");
      return;
  }

  if(!strcmp(COM_Argv(1), "global"))
    script = &fs_levelscript;
  else
    script = fs_levelscript.children[atoi(COM_Argv(1))];
  
  if(!script)
  {
      CONS_Printf("script '%s' not defined.\n", COM_Argv(1));
      return;
  }
  
  CONS_Printf("%s\n", script->data);
}



void COM_T_RunScript_f (void)
{
  int sn;
  
  if(COM_Argc() < 2)
  {
      CONS_Printf("Usage: T_RunScript <script>\n");
      return;
  }
  
  sn = atoi(COM_Argv(1));
  
  if(!fs_levelscript.children[sn])
  {
      CONS_Printf("script not defined\n");
      return;
  }
  
  T_RunScript(sn, consoleplayer_ptr->mo );
}



/************************
         PAUSING SCRIPTS
 ************************/

runningscript_t *freelist=NULL;      // maintain a freelist for speed

// Does not return NULL
runningscript_t * new_runningscript( void )
{
  // check the freelist
  if(freelist)
  {
      runningscript_t *returnv=freelist;
      freelist = freelist->next;
      return returnv;
  }
  
  // alloc static: can be used in other levels too
  return Z_Malloc(sizeof(runningscript_t), PU_STATIC, 0);
}



static void free_runningscript(runningscript_t *runscr)
{
  // add to freelist
  runscr->next = freelist;
  freelist = runscr;
}


// Return true when finished, false while waiting
static boolean wait_finished(runningscript_t *script)
{
  switch(script->wait_type)
  {
    case WT_none: break;	      // always finished
    case WT_scriptwait:               // waiting for script to finish
      {
        runningscript_t *current;
        for(current = fs_runningscripts.next; current; current = current->next)
        {
            if(current == script) continue;  // ignore this script
            if(current->script->scriptnum == script->wait_data)
              goto ret_wait;  // script still running
        }
      }
      break;  // finished

    case WT_delay:                          // just count down
      {
        return --script->wait_data <= 0;
      }
    
    case WT_tagwait:
      {
        int secnum = -1;

        while ((secnum = P_FindSectorFromTag(script->wait_data, secnum)) >= 0)
        {
            sector_t *sec = &sectors[secnum];
            if(sec->floordata || sec->ceilingdata || sec->lightingdata)
              goto ret_wait;  // not finished
        }
      }
      break;  // finished

    default: break;
  }
  return true;  // default finished

ret_wait:
  return false;  // waiting
}




void T_DelayedScripts( void )
{
  runningscript_t *current, *next;
  int i;

  if(!info_scripts) return;       // no level scripts
  
  current = fs_runningscripts.next;
  
  while(current)
  {
      if(wait_finished(current))
      {
          // copy out the script variables from the
          // runningscript_t

          for(i=0; i<VARIABLESLOTS; i++)
            current->script->variables[i] = current->variables[i];
          current->script->trigger = current->trigger; // copy trigger

          // continue the script

          continue_script(current->script, current->savepoint);

          // unhook from chain and free

          current->prev->next = current->next;
          if(current->next) current->next->prev = current->prev;
          next = current->next;   // save before freeing
          free_runningscript(current);
      }
      else
        next = current->next;
      current = next;   // continue to next in chain
  }
                
}


static runningscript_t * T_SaveCurrentScript( void )
{
  runningscript_t *runscr;
  int i;

  runscr = new_runningscript();
  runscr->script = fs_current_script;
  runscr->savepoint = fs_src_cp;

  // leave to other functions to set wait_type: default to WT_none
  runscr->wait_type = WT_none;

  // hook into chain at start
  runscr->next = fs_runningscripts.next;
  runscr->prev = &fs_runningscripts;
  runscr->prev->next = runscr;
  if(runscr->next)
    runscr->next->prev = runscr;
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
  {
      runscr->variables[i] = fs_current_script->variables[i];
      
      // remove all the variables from the script variable list
      // to prevent them being removed when the script stops

      while(fs_current_script->variables[i]
            && fs_current_script->variables[i]->type != FSVT_label)
      {
        fs_current_script->variables[i] =
          fs_current_script->variables[i]->next;
      }
  }
  runscr->trigger = fs_current_script->trigger;      // save trigger
  
  fs_killscript = true;      // stop the script

  return runscr;
}




// script function
void SF_Wait( void )
{
  runningscript_t *runscr;

  if(t_argc != 1)  goto err_numarg;

  runscr = T_SaveCurrentScript();

  runscr->wait_type = WT_delay;
  runscr->wait_data = (intvalue(t_argv[0]) * 35) / 100;
done:
  return;

err_numarg:
  wrong_num_arg( "Wait", 1);
  goto done;
}

//if you want to wait on tics instead of "real" time
void SF_WaitTic( void )
{
  runningscript_t *runscr;

  if(t_argc != 1)  goto err_numarg;

  runscr = T_SaveCurrentScript();

  runscr->wait_type = WT_delay;
  runscr->wait_data = intvalue(t_argv[0]);
done:
  return;

err_numarg:
  wrong_num_arg( "WaitTic", 1);
  goto done;
}

// wait for sector with particular tag to stop moving
void SF_TagWait( void )
{
  runningscript_t *runscr;

  if(t_argc != 1)  goto err_numarg;

  runscr = T_SaveCurrentScript();

  runscr->wait_type = WT_tagwait;
  runscr->wait_data = intvalue(t_argv[0]);
done:
  return;

err_numarg:
  wrong_num_arg( "TagWait", 1);
  goto done;
}




// wait for a script to finish
void SF_ScriptWait( void )
{
  runningscript_t *runscr;

  if(t_argc != 1)  goto err_numarg;

  runscr = T_SaveCurrentScript();

  runscr->wait_type = WT_scriptwait;
  runscr->wait_data = intvalue(t_argv[0]);
done:
  return;

err_numarg:
  wrong_num_arg( "ScriptWait", 1);
  goto done;
}




//extern mobj_t * fs_trigger_obj;           // in t_func.c

void SF_StartScript( void )
{
  runningscript_t *runscr;
  script_t *script;
  int i, snum;
  
  if(t_argc != 1)  goto err_numarg;

  snum = intvalue(t_argv[0]);
  script = fs_levelscript.children[snum];
  if(!script)  goto err_noscript;
  
  runscr = new_runningscript();
  runscr->script = script;
  runscr->savepoint = script->data; // start at beginning
  runscr->wait_type = WT_none;      // start straight away

  // hook into chain at start
  
  runscr->next = fs_runningscripts.next;
  runscr->prev = &fs_runningscripts;
  runscr->prev->next = runscr;
  if(runscr->next)
    runscr->next->prev = runscr;
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
  {
      runscr->variables[i] = script->variables[i];
      
      // in case we are starting another current_script:
      // remove all the variables from the script variable list
      // we only start with the basic labels
      while(runscr->variables[i] &&
            runscr->variables[i]->type != FSVT_label)
        runscr->variables[i] =
          runscr->variables[i]->next;
  }
  // copy trigger
  runscr->trigger = fs_current_script->trigger;
done:
  return;

err_numarg:
  wrong_num_arg( "StartScript", 1);
  goto done;

err_noscript:
  script_error("StartScript: script %i not defined\n", snum);
  goto done;
}



// int ScriptRunning( int scriptnumber )
// Return 1 when script (scriptnumber) is running
void SF_ScriptRunning( void )
{
  runningscript_t *current;
  int snum;

  if(t_argc != 1)  goto err_numarg;

  snum = intvalue(t_argv[0]);

  t_return.type = FSVT_int;
  t_return.value.i = 0;  // default, script not found

  for(current=fs_runningscripts.next; current; current=current->next)
  {
      if(current->script->scriptnum == snum)
      {
          t_return.value.i = 1;  // found, and running
          goto done;
      }
  }

done:
  return;

err_numarg:
  wrong_num_arg( "ScriptRunning", 1);
  goto done;
}




// running scripts

void COM_T_Running_f (void)
{
  runningscript_t *current;
  
  current = fs_runningscripts.next;
  
  CONS_Printf("running scripts\n");
  
  if(!current)
    CONS_Printf("no running scripts.\n");
  
  while(current)
  {
      CONS_Printf("%i:", current->script->scriptnum);
      switch(current->wait_type)
      {
        case WT_none:
          CONS_Printf("waiting for nothing?\n");
          break;
        case WT_delay:
          CONS_Printf("delay %i tics\n", current->wait_data);
          break;
        case WT_tagwait:
          CONS_Printf("waiting for tag %i\n", current->wait_data);
          break;
        case WT_scriptwait:
          CONS_Printf("waiting for script %i\n", current->wait_data);
          break;
        default:
          CONS_Printf("unknown wait type \n");
          break;
      }
      current = current->next;
  }
}




void clear_runningscripts( void )
{
  runningscript_t *runscr, *next;
  
  runscr = fs_runningscripts.next;
  
  // free the whole chain
  while(runscr)
  {
      next = runscr->next;
      free_runningscript(runscr);
      runscr = next;
  }
  fs_runningscripts.next = NULL;
}


mobj_t * MobjForSvalue(fs_value_t svalue)
{
  int intval;
  
  if(svalue.type == FSVT_mobj)
    return svalue.value.mobj;
  
  // this requires some creativity. We use the intvalue
  // as the thing number of a thing in the level.
  
  intval = intvalue(svalue);        
  if(intval < 0 || intval >= nummapthings || !mapthings[intval].mobj)
      goto err_mapthing;

  return mapthings[intval].mobj;

err_mapthing:
  script_error("no levelthing %i\n", intval);
  return NULL;
}





/*********************
            ADD SCRIPT
 *********************/

// when the level is first loaded, all the
// scripts are simply stored in the levelscript.
// before the level starts, this script is
// preprocessed and run like any other. This allows
// the individual scripts to be derived from the
// levelscript. When the interpreter detects the
// 'script' keyword this function is called

void spec_script( void )
{
  int scriptnum;
  int datasize;
  script_t *script;

  if(!fs_current_section)  goto err_nosection;
  
  // presume that the first token is "script"
  if(num_tokens < 2)  goto err_numtokens;

  scriptnum = intvalue(evaluate_expression(1, num_tokens-1));
  if(scriptnum < 0)  goto err_scriptnum;

  script = Z_Malloc(sizeof(script_t), PU_LEVEL, 0);

  // add to scripts list of parent
  fs_current_script->children[scriptnum] = script;
  
  // copy script data
  // workout script size: -2 to ignore { and }
  datasize = fs_current_section->end - fs_current_section->start - 2;

  // alloc extra 10 for safety
  script->data = Z_Malloc(datasize+10, PU_LEVEL, 0);
 
  // copy from parent script (fs_levelscript) 
  // ignore first char which is {
  memcpy(script->data, fs_current_section->start+1, datasize);
  // tack on a 0 to end the string
  script->data[datasize] = '\0';
  
  script->scriptnum = scriptnum;
  script->parent = fs_current_script; // remember parent
  
  // preprocess the script now
  preprocess(script);
    
  // restore current_script: usefully stored in new script
  fs_current_script = script->parent;

  // fs_src_cp may also be changed, but is changed below anyway
  
  // we dont want to run the script, only add it
  // jump past the script in parsing
  fs_src_cp = fs_current_section->end + 1;
done:
  return;

err_nosection:
  script_error("need separators for script\n");
  goto done;

err_numtokens:
  script_error("need script number\n");
  goto done;

err_scriptnum:
  script_error("invalid script number\n");
  goto done;
}




/****** scripting command list *******/

void T_Register_Commands( void )
{
#ifdef FRAGGLESCRIPT
  COM_AddCommand("fs_dumpscript",  COM_T_DumpScript_f, CC_fs);
  COM_AddCommand("fs_runscript",   COM_T_RunScript_f, CC_fs);
  COM_AddCommand("fs_running",     COM_T_Running_f, CC_fs);
  // for old wads with bind keys (like Chex newmaps)
  COM_AddCommand("t_runscript",   COM_T_RunScript_f, CC_fs);
  COM_AddCommand("t_running",     COM_T_Running_f, CC_fs);
//  COM_AddCommand("t_dumpscript",  COM_T_DumpScript_f, CC_fs);
#endif
}

