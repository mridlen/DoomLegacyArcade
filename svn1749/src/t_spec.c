// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_spec.c 1368 2017-11-01 01:17:48Z wesleyjohnson $
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
// $Log: t_spec.c,v $
// Revision 1.5  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.4  2002/01/12 02:21:36  stroggonmeth
//
// Revision 1.3  2001/03/13 22:14:20  stroggonmeth
// Long time no commit. 3D floors, FraggleScript, portals, ect.
//
// Revision 1.2  2000/11/03 11:48:40  hurdler
// Fix compiling problem under win32 with 3D-Floors and FragglScript (to verify!)
//
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------
//
// 'Special' stuff
//
// if(), int statements, etc.
//
// By Simon Howard
//
//----------------------------------------------------------------------------

/* includes ************************/

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_vari.h"


// ending brace found in parsing

void spec_brace( void )
{
  if(script_debug)
    CONS_Printf("brace\n");
  
  if(fs_bracetype != BRACKET_close)  // only deal with closing } braces
    goto done;
  
  // if() requires nothing to be done
  if(fs_current_section->type == FSST_if
     || fs_current_section->type == FSST_else)   goto done;
  
  // if a loop, jump back to the start of the loop
  if(fs_current_section->type == FSST_loop)
  {
      fs_src_cp = fs_current_section->data.data_loop.loopstart;
      goto done;
  }
done:
  return;
}

// 'if' statement
int spec_if( void )
{
  int endtoken;
  fs_value_t eval;
  int testval;
  
  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)  goto err_endparen;
  
  // 2 to skip past the 'if' and '('
  eval = evaluate_expression(2, endtoken-1);
  testval = intvalue(eval);
  
  if(fs_current_section
     && fs_bracetype == BRACKET_open
     && endtoken == num_tokens-1)
  {
    // {} braces
    if( !testval )       // test false, skip to end of section
      fs_src_cp = fs_current_section->end+1;
  }
  else if( testval )
  {
      // test true
      if(endtoken == num_tokens-1)  goto done; // nothing to do ?
      evaluate_expression(endtoken+1, num_tokens-1);
  }
done:
  return( testval );  // to skip else and elseif

err_endparen:
  script_error("If: missing end paren\n");
  return 0;
}


int spec_elseif(boolean lastif)
{
  int endtoken;
  fs_value_t eval;
  int testval;

  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)  goto err_endparen;

  // when IF was true, do not execute ELSEIF
  if(lastif)
  {
    fs_src_cp = fs_current_section->end+1;
    return true;
  }
  // 2 to skip past the 'elseif' and '('
  eval = evaluate_expression(2, endtoken-1);
  testval = intvalue(eval);
  
  if(fs_current_section
     && fs_bracetype == BRACKET_open
     && endtoken == num_tokens-1)
  {
      // {} braces
      if( !testval )       // test false, skip to end of section
        fs_src_cp = fs_current_section->end+1;
  }
  else    // elseif() without {} braces
  {
      if( testval )
      {
        if(endtoken == num_tokens-1)  goto done;  // nothing to do
        evaluate_expression(endtoken+1, num_tokens-1);
      }
  }
done:
  return( testval );  // to skip else and elseif

err_endparen:
  script_error("ElseIf: missing end paren\n");
  return 0;
}


void spec_else(boolean lastif)
{
  // when IF was true, do not execute ELSE
  if(lastif)
    fs_src_cp = fs_current_section->end+1;
}


// while() loop
void spec_while( void )
{
  int endtoken;
  fs_value_t eval;
  int testval;

  if(!fs_current_section)  goto err_section;
  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)  goto err_endparen;
  
  eval = evaluate_expression(2, endtoken-1);
  testval = intvalue(eval);
  
  // break out of loop when test fails
  if( !testval )
      fs_src_cp = fs_current_section->end+1;
done:
  return;

err_section:
  script_error("Loop: missing {} section\n");
  goto done;

err_endparen:
  script_error("Loop: missing end paren\n");
  goto done;
}

// for() loop
void spec_for( void )
{
  fs_value_t eval;
  int start;
  int comma1, comma2;     // token numbers of the separating commas
  int testval;
  
  if(!fs_current_section)  goto err_section;
  
  // is a valid section
  
  start = 2;     // skip "for" and "(": start on third token(2)
  
  // find the separating commas first
  
  if( (comma1 = find_operator(start,    num_tokens-1, ",")) == -1
      || (comma2 = find_operator(comma1+1, num_tokens-1, ",")) == -1)  goto err_syntax;
  
  // are we looping back from a previous loop?
  if(fs_current_section == fs_prev_section)
  {
      // do the loop 'action' (third argument)
      evaluate_expression(comma2+1, num_tokens-2);
      
      // check if we should run the loop again (second argument)
      eval = evaluate_expression(comma1+1, comma2-1);
      testval = intvalue(eval);
      if( !testval )
      {
          // test fail, stop looping
          fs_src_cp = fs_current_section->end + 1;
      }
  }
  else
  {
      // first time: starting the loop
      // just evaluate the starting expression (first arg)
      evaluate_expression(start, comma1-1);
  }
done:
  return;
   
err_section:
  script_error("For: missing {} section\n");
  goto done;

err_syntax:
  script_error("For: syntax\n");
  goto done;
}

/**************************** Variable Creation ****************************/

int newvar_type;
script_t *newvar_script;

// called for each individual variable in a statement
//  newvar_type must be set

static void create_variable(int start, int stop)
{
  if(fs_killscript)  goto done;
  
  if(tokentype[start] != TT_name)   goto err_varname;
  
  // check if already exists, only checking
  // the current script
  if( variable_for_name(newvar_script, tokens[start]) )  goto done;  // already one
  
  new_variable(newvar_script, tokens[start], newvar_type);
  
  if(stop != start)
      evaluate_expression(start, stop);
done:
  return;
   
err_varname:
  script_error("Var: invalid name for variable: '%s'\n", tokens[start+1]);
  goto done;
}

// divide a statement (without type prefix) into individual
// variables to create them using create_variable

static void parse_var_line(int start)
{
  int starttoken = start, endtoken;
  
  while(1)
  {
      if(fs_killscript)   goto done;
      endtoken = find_operator(starttoken, num_tokens-1, ",");
      if(endtoken == -1) break;
      create_variable(starttoken, endtoken-1);
      starttoken = endtoken+1;  //start next after end of this one
  }
  // dont forget the last one
  create_variable(starttoken, num_tokens-1);
done:
  return;
}

boolean spec_variable( void )
{
  int start = 0;

  newvar_type = -1;                 // init to -1
  newvar_script = fs_current_script;   // use current script

  // check for 'hub' keyword to make a hub variable
  if(!strcmp(tokens[start], "hub"))
  {
      newvar_script = &hub_script;
      start++;  // skip first token
  }

  // now find variable type
  if(!strcmp(tokens[start], "const"))
  {
      newvar_type = FSVT_const;
      start++;
  }
  else if(!strcmp(tokens[start], "string"))
  {
      newvar_type = FSVT_string;
      start++;
  }
  else if(!strcmp(tokens[start], "int"))
  {
      newvar_type = FSVT_int;
      start++;
  }
  else if(!strcmp(tokens[start], "mobj"))
  {
      newvar_type = FSVT_mobj;
      start++;
  }
  else if(!strcmp(tokens[start], "script"))     // check for script creation
  {
      spec_script();
      return true;       // used tokens
  }
  else if(!strcmp(tokens[start], "float") || !strcmp(tokens[start], "fixed"))
  {
      newvar_type = FSVT_fixed;
      start++;
  }
  else if(!strcmp(tokens[start], "array")) // arrays
  {
     newvar_type = FSVT_array;
     start++;
  }


  // other variable types could be added: eg float

  if(newvar_type != -1)   // are we creating a new variable?
  {
      parse_var_line(start);
      return true;       // used tokens
  }

  return false; // not used: try normal parsing
}

