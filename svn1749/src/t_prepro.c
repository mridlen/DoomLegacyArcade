// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_prepro.c 1419 2019-01-29 08:01:42Z wesleyjohnson $
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
// $Log: t_prepro.c,v $
// Revision 1.1  2000/11/02 17:57:28  stroggonmeth
// FraggleScript files...
//
//
//--------------------------------------------------------------------------
//
// Preprocessor.
//
// The preprocessor must be called when the script is first loaded.
// It performs 2 functions:
//      1: blank out comments (which could be misinterpreted)
//      2: makes a list of all the sections held within {} braces
//      3: 'dry' runs the script: goes thru each statement and
//         sets the types of all the fs_section_t's in the script
//      4: Saves locations of all goto() labels
//
// the system of fs_section_t's is pretty horrible really, but it works
// and its probably the only way i can think of of saving scripts
// half-way thru running
//
// By Simon Howard
//
//---------------------------------------------------------------------------

/* includes ************************/

#include <stdio.h>
#include <string.h>

#include "doomincl.h"
  // debugfile
//#include "command.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

// clear the script: section and variable slots

void clear_script( void )
{
  int i;
  
  for(i=0; i<SECTIONSLOTS; i++)
    fs_current_script->sections[i] = NULL;
  
  for(i=0; i<VARIABLESLOTS; i++)
    fs_current_script->variables[i] = NULL;

  // clear child scripts
  
  for(i=0; i<MAXSCRIPTS; i++)
    fs_current_script->children[i] = NULL;
}

/*********** {} sections *************/

// during preprocessing all of the {} sections
// are found. these are stored in a hash table
// according to their offset in the script. 
// functions here deal with creating new fs_section_t's
// and finding them from a given offset.

#define section_hash(b)           \
       ( (int) ( (b) - fs_current_script->data) % SECTIONSLOTS)

fs_section_t *new_section(char *brace)
{
  int n;
  fs_section_t *newsec;
  
  // create section
  // make level so its cleared at start of new level
  
  newsec = Z_Malloc(sizeof(fs_section_t), PU_LEVEL, 0);
  newsec->start = brace;
  
  // hook it into the hashchain
  
  n = section_hash(brace);
  newsec->next = fs_current_script->sections[n];
  fs_current_script->sections[n] = newsec;
  
  return newsec;
}

// find a fs_section_t from the location of the starting { brace
fs_section_t* find_section_start(char *brace)
{
  int n = section_hash(brace);
  fs_section_t *current;
  
  current = fs_current_script->sections[n];
  
  // use the hash table: check the appropriate hash chain
  
  while(current)
    {
      if(current->start == brace)
        return current;
      current = current->next;
    }
  
  return NULL;    // not found
}

// find a fs_section_t from the location of the ending } brace
fs_section_t* find_section_end(char *brace)
{
  int n;
  
  // hash table is no use, they are hashed according to
  // the offset of the starting brace
  
  // we have to go through every entry to find from the
  // ending brace
  
  for(n=0; n<SECTIONSLOTS; n++)      // check all sections in all chains
    {
      fs_section_t *current = fs_current_script->sections[n];
      
      while(current)
        {
          if(current->end == brace)
            return current;        // found it
          current = current->next;
        }
    }
  
  return NULL;    // not found
}

/********** labels ****************/

// labels are also found during the
// preprocessing. these are of the form
//
//      label_name:
//
// and are used for the goto function.
// goto labels are stored as variables.

                // from parse.c
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
                     ( (c)<='9' && (c)>='0') || ( (c)=='_') )

        // create a new label. pass the location inside the script
fs_variable_t* new_label(char *labelptr)
{
  fs_variable_t *newlabel;   // labels are stored as variables
  char labelname[256];
  char *temp, *temp2;
  
  // copy the label name from the script up to ':'
  for(temp=labelptr, temp2 = labelname; *temp!=':'; temp++, temp2++)
    *temp2 = *temp;
  *temp2 = '\0';  // end string
  
  newlabel = new_variable(fs_current_script, labelname, FSVT_label);
  
  // put neccesary data in the label
  
  newlabel->value.labelptr = labelptr;
  
  return newlabel;
}

/*********** main loop **************/

// This works by recursion. when a { opening
// brace is found, another instance of the
// function is called for the data inside
// the {} section.
// At the same time, the sections are noted
// down and hashed. Goto() labels are noted
// down, and comments are blanked out

char* process_find_char(char *data, char find)
{
  while(*data)
  {
      if(*data==find) return data;
      if(*data=='\"')       // found a quote: ignore stuff in it
      {
          data++;
          while(*data && *data != '\"')
          {
              // escape sequence ?
              if(*data=='\\') data++;
              data++;
          }
          // error: end of script in a constant
          if(!*data) return NULL;
      }

      // comments: blank out

      if(*data=='/' && *(data+1)=='*')        // /* -- */ comment
      {
          while(*data && (*data != '*' || *(data+1) != '/') )
          {
              *data=' '; data++;
          }
          if(*data)
            *data = *(data+1) = ' ';   // blank the last bit
          else
          {
              fs_src_cp = data;
              // script terminated in comment
              script_error("script terminated inside comment\n");
          }
      }
      if(*data=='/' && *(data+1)=='/')        // // -- comment
      {
        while(*data != '\n')
          {
            *data=' '; data++;       // blank out
          }
      }

      // labels

      if(*data==':'  // ':' -- a label
         && fs_current_script->scriptnum != -1)   // not levelscript
      {
          char *labelptr = data-1;

          while(!isop(*labelptr)) labelptr--;
          new_label(labelptr+1);
      }
      
      if(*data=='{')  // { -- } sections: add 'em
      {
          fs_section_t *newsec = new_section(data);

          newsec->type = FSST_empty;
          // find the ending } and save
          newsec->end = process_find_char(data+1, '}');
          if(!newsec->end)
            {                // brace not found
              fs_src_cp = data;
              script_error("section error: no ending brace\n");
              return NULL;
            }
          // continue from the end of the section
          data = newsec->end;
      }
      data++;
  }

  return NULL;
}


/*********** second stage parsing ************/

// second stage preprocessing considers the script
// in terms of tokens rather than as plain data.
//
// we 'dry' run the script: go thru each statement and
// collect types for fs_section_t
//
// this is an important thing to do, it cannot be done
// at runtime for 2 reasons:
//      1. gotos() jumping inside loops will pass thru
//         the end of the loop
//      2. savegames. loading a script saved inside a
//         loop will let it pass thru the loop
//
// this is basically a cut-down version of the normal
// parsing loop.

void get_tokens(char *);         // t_parse.c

void dry_run_script( void )
{
  // save some stuff
  char *old_src_cp = fs_src_cp;
  fs_section_t *old_current_section = fs_current_section;
  
  char *end = fs_current_script->data + fs_current_script->len;
  char *token_alloc;
  
  fs_killscript = false;
  
  // allocate space for the tokens
  token_alloc = Z_Malloc(fs_current_script->len + T_MAXTOKENS, PU_STATIC, 0);
  
  fs_src_cp = fs_current_script->data;
  
  while(fs_src_cp < end && *fs_src_cp)
  {
      tokens[0] = token_alloc;
      get_tokens(fs_src_cp);
      
      if(fs_killscript) break;
      if(!num_tokens) continue;
      
      if(fs_current_section && tokentype[0] == TT_function)
      {
          if(!strcmp(tokens[0], "if"))
          {
              fs_current_section->type = FSST_if;
              continue;
          }
          else if(!strcmp(tokens[0], "elseif"))
          {
              fs_current_section->type = FSST_elseif;
              continue;
          }
          else if(!strcmp(tokens[0], "else"))
          {
              fs_current_section->type = FSST_else;
              continue;
          }
          else if(!strcmp(tokens[0], "while") ||
                  !strcmp(tokens[0], "for"))
          {
              fs_current_section->type = FSST_loop;
              fs_current_section->data.data_loop.loopstart = fs_linestart_cp;
              continue;
          }
      }
  }
  
  Z_Free(token_alloc);
  
  // restore stuff
  fs_current_section = old_current_section;
  fs_src_cp = old_src_cp;
}

/***************** main preprocess function ******************/

// set up current_script, script->len
// just call all the other functions

void preprocess(script_t *script)
{
#ifdef DEBUGFILE
  if(debugfile)
    fprintf(debugfile,"  preprocess script %i\n", script->scriptnum);
#endif

  fs_current_script = script;
  script->len = strlen(script->data);
  
  clear_script();

#ifdef DEBUGFILE
  if(debugfile)
    fprintf(debugfile, "    run thru script\n");
#endif
  
  process_find_char(script->data, 0);  // fill in everything
  
#ifdef DEBUGFILE
  if(debugfile)
    fprintf(debugfile, "    dry run script\n");
#endif
  
  dry_run_script();
}

/************ includes ******************/

// FraggleScript allows 'including' of other lumps.
// we divert input from the current_script (normally
// levelscript) to a seperate lump. This of course
// first needs to be preprocessed to remove comments
// etc.

void parse_data(char *data, char *end); // t_parse.c

// parse an 'include' lump

void parse_include(char *lumpname)
{
  lumpnum_t lumpnum;
  char *temp;
  char *lump, *end;
  char *saved_src_cp;
  int  len;
  
  lumpnum = W_GetNumForName(lumpname);
  if( ! VALID_LUMP(lumpnum) )
  {
      script_error("include lump '%s' not found!\n", lumpname);
      return;
  }
  
  lump = W_CacheLumpNum(lumpnum, PU_STATIC);
  
  // realloc bigger for NULL at end
  // SoM: REALLOC does not seem to work! So we alloc here and copy....
  //lump = Z_Realloc(lump, W_LumpLength(lumpnum)+10, PU_STATIC, NULL);
  temp = lump;
  len = W_LumpLength(lumpnum);
  lump = Z_Malloc( len + 10, PU_STATIC, NULL);
  memcpy(lump, temp, len);
  
  saved_src_cp = fs_src_cp;    // save fs_src_cp during include
  fs_src_cp = lump;
  end = lump + len;
  *end = 0;
  
  // preprocess the include
  // we assume that it does not include sections or labels or 
  // other nasty things
  process_find_char(lump, 0);
  
  // now parse the lump
  parse_data(lump, end);
  
  // restore fs_src_cp
  fs_src_cp = saved_src_cp;
  
  // free the lump
  Z_Free(lump);
  Z_Free(temp);
}

//---------------------------------------------------------------------------
