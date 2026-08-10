// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: t_array.c 1618 2022-01-24 21:01:46Z wesleyjohnson $
//
// Copyright(C) 2000 James Haley
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
//
// $Log: t_array.c,v $
// Revision 1.1  2004/07/27 08:22:01  exl
// Add fs arrys files
//
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//
//   Array support for FraggleScript
//
//   By James Haley, with special thanks to SoM
//
//--------------------------------------------------------------------------

#include <string.h>
  // memcpy, memset

#include "z_zone.h"
#include "t_array.h"
#include "t_vari.h"
//#include "p_enemy.h"

// Save list for arrays -- put ALL arrays spawned into this list,
// and they'll live until the end of the level. This is very lazy
// garbage collection, but its the only real solution given the
// state of the FS source (the word "haphazard" comes to mind...)
#ifdef FS_ARRAYLIST_STRUCTHEAD
array_t fs_arraylist =
{
   NULL,
   0,
   0,
   NULL,
};
#else
fs_array_t * fs_arraylist = NULL;
#endif

// add an array into the array save list
void T_Add_FSArray(fs_array_t *array)
{
   fs_array_t *temp;

   // always insert at head of list
#ifdef FS_ARRAYLIST_STRUCTHEAD
   temp = fs_arraylist.next;
   fs_arraylist.next = array;
#else
   temp = fs_arraylist;
   fs_arraylist = array;
#endif
   array->next = temp;
}

static void * initsave_levelclear = NULL;  // indicates when PU_LEVEL cleared
  
// call from P_SetupLevel and P_Savegame_Save_game
// Clears added array values but not base of fs_arraylist
void T_Init_FSArrayList(void)
{
   // Z_Malloc of arrays is PU_LEVEL, but does not pass a user ptr, so level
   // clear will have released all this memory without informing the owners.
   if( initsave_levelclear )	// level not cleared, as in load saved game
   {
       fs_array_t * sfap, * sfap_nxt;
       // enable to test if this is happening
//       debug_Printf( "T_Init_FSArrayList: clearing array list\n" );
#ifdef FS_ARRAYLIST_STRUCTHEAD
       sfap = fs_arraylist.next;
#else      
       sfap = fs_arraylist;
#endif
       for( ; sfap; sfap=sfap_nxt )
       {
          if( sfap->values )   Z_Free( sfap->values );
          sfap_nxt = sfap->next;  // get next before deallocate
          Z_Free( sfap );
       }
   }
   else
   {
       // will trip when level is cleared
       Z_Malloc( 1, PU_LEVEL, &initsave_levelclear );
   }
#ifdef FS_ARRAYLIST_STRUCTHEAD
   fs_arraylist.next = NULL;
#else      
   fs_arraylist = NULL;
#endif
}

#if 0
// Clears all array values including base of fs_arraylist
void T_Init_FSArrays(void)
{
   // Z_Malloc of arrays does not pass a user ptr, so level clear
   // will not have destroyed these arrays.
   T_Init_FSArrayList();	// clear fs_arraylist.next
   if( fs_arraylist.values )  Z_Free( fs_arraylist.values );
   fs_arraylist.values = NULL;
   fs_arraylist.saveindex = 0;
   fs_arraylist.length = 0;
}
#endif

// SF Handler functions for calling from scripts

//
// SF_NewArray
// 
//  Create a new fs_array_t and initialize it with values
//
//  Implements: array newArray(...)
//
// array functions in t_array.c

// NewArray( ... )
// copy parameters to array
// array or string parameters are ignored
void SF_NewArray(void)
{
   int i;
   fs_array_t *newArray;

   if(!t_argc)  goto done; // empty, do nothing

   // allocate a fs_array_t
   newArray = Z_Malloc(sizeof(fs_array_t), PU_LEVEL, NULL);

   // init all fields to zero
   memset(newArray, 0, sizeof(fs_array_t));
   
   // allocate t_argc number of values, set length
   newArray->values = Z_Malloc(t_argc*sizeof(fs_value_t), PU_LEVEL, NULL);
   memset(newArray->values, 0, t_argc*sizeof(fs_value_t));
   
   newArray->length = t_argc;

   for(i=0; i<t_argc; i++)
   {
      // strings, arrays are ignored
      if(t_argv[i].type == FSVT_string || t_argv[i].type == FSVT_array)
         continue;

      // copy all the argument values into the local array
      memcpy(&(newArray->values[i]), &t_argv[i], sizeof(fs_value_t));
   }

   T_Add_FSArray(newArray); // add the new array to the save list
   
   t_return.type = FSVT_array;
   // t_return is an internal value which may not be captured in
   // an svariable_t, so we don't count it as a reference --
   // in the cases of immediate value usage, the garbage collector
   // won't have a chance to free it until it has been used
   t_return.value.a = newArray;
done:
   return;
}



//
// SF_NewEmptyArray
// 
//  Create a new fs_array_t and initialize it with a standard value
//
// NewEmptyArray( int numberelements , int elementtype )
// elementtype: 0= int 0, 1= fixed 0, 2= mobj NULL
void SF_NewEmptyArray(void)
{
   int i;
   fs_array_t *newArray;
   fs_value_t	newval;

   if(t_argc < 2)  goto done; // empty, do nothing

   // bad types
   if(t_argv[0].type != FSVT_int || t_argv[1].type != FSVT_int)  goto err_argtype;
   // elementtype out of bounds
   if(t_argv[1].value.i < 0 || t_argv[1].value.i > 2)  goto err_elementtype;

   // allocate a fs_array_t
   newArray = Z_Malloc(sizeof(fs_array_t), PU_LEVEL, NULL);

   // init all fields to zero
   memset(newArray, 0, sizeof(fs_array_t));
   
   // allocate t_argc number of values, set length
   newArray->values = Z_Malloc(t_argv[0].value.i*sizeof(fs_value_t), PU_LEVEL, NULL);
   memset(newArray->values, 0, t_argv[0].value.i*sizeof(fs_value_t));

   newArray->length = t_argv[0].value.i;

   // initialize each value
   switch(t_argv[1].value.i)
   {
                case 0:
                        newval.type = FSVT_int;
                        newval.value.i = 0;
                        break;
                case 1:
                        newval.type = FSVT_fixed;
                        newval.value.f = 0;
                        break;
                case 2:
                        newval.type = FSVT_mobj;
                        newval.value.mobj = NULL;
                        break;
   }

   for(i=0; i<t_argv[0].value.i; i++)
   {
      // Copy the new element into the array
          memcpy(&(newArray->values[i]), &newval, sizeof(fs_value_t));
   }

   T_Add_FSArray(newArray); // add the new array to the save list
   
   t_return.type = FSVT_array;
   // t_return is an internal value which may not be captured in
   // an svariable_t, so we don't count it as a reference --
   // in the cases of immediate value usage, the garbage collector
   // won't have a chance to free it until it has been used
   t_return.value.a = newArray;
done:
   return;

err_argtype:
   script_error("NewEmptyArray: expected integer\n");
   goto done;

err_elementtype:
   script_error("NewEmptyArray: elementtype can be 0,1,2\n");
   goto done;
}


//
// SF_ArrayCopyInto
//
// Copies the values from one array into the values of another.
// Arrays must be non-empty and must be of equal length.
//
// void ArrayCopyInto(array source, array target)
void SF_ArrayCopyInto(void)
{
   unsigned int i;
   fs_array_t *source, *target;
   
   if(t_argc != 2)  goto err_numarg;

   if(t_argv[0].type != FSVT_array || t_argv[1].type != FSVT_array)  goto err_argtype;

   source = t_argv[0].value.a;
   target = t_argv[1].value.a;

   if(!source || !target)  goto err_array_empty;
#if 0
   // [WDJ] Improvement: Allow as long as target is large enough for source
   if(source->length > target->length)  goto err_array_length;
#else
   // Previous, exact same length only
   if(source->length != target->length)  goto err_array_length;
#endif

   for(i=0; i<source->length; i++)
   {
      memcpy(&(target->values[i]), &(source->values[i]), 
             sizeof(fs_value_t));
   }
done:
   return;
   
err_numarg:
   wrong_num_arg("ArrayCopyInto", 2);
   goto done;

err_argtype:
   script_error("ArrayCopyInto: requires array parameters\n");
   goto done;

err_array_empty:
   script_error("ArrayCopyInto: requires non-empty arrays\n");
   goto done;

err_array_length:
   script_error("ArrayCopyInto: requires arrays of equal length\n");
   goto done;
}

//
// SF_ArrayElementAt
//
// Retrieves a value at a specific index
//
// This function is somewhat unique as it has a polymorphic
// return type :)
//
// polytype ArrayElementAt(array x, int index)
// Returns int or fixed or mobj type
void SF_ArrayElementAt(void)
{
   unsigned int index;
   
   if(t_argc != 2)  goto err_numarg;
   if(t_argv[0].type != FSVT_array || !t_argv[0].value.a)  goto err_argtype;

   // get index from second arg
   index = intvalue(t_argv[1]);
   if( index >= t_argv[0].value.a->length )  goto err_index;

   // copy full fs_value_t to t_return
   memcpy(&t_return, &(t_argv[0].value.a->values[index]), 
          sizeof(fs_value_t));
done:
   return;

err_numarg:
   wrong_num_arg("ArrayElementAt", 2);
   goto done;

err_argtype:
   script_error("ArrayElementAt: requires non-empty array\n");
   goto done;

err_index:
   script_error("ArrayElementAt: array index exceeds array\n");
   goto done;
}

//
// SF_ArraySetElementAt
//
// Sets a specific value in an array
//
// void ArrayElementAt(array x, polytype value, int index)
// value can be int or fixed or mobj type
// Arrays can be of mixed type.
void SF_ArraySetElementAt(void)
{
   unsigned int index;
   
   if(t_argc != 3)  goto err_numarg;

   if(t_argv[0].type != FSVT_array || !t_argv[0].value.a)  goto err_argtype;

   // get index from third arg this time...
   index = intvalue(t_argv[2]);
   if( index >= t_argv[0].value.a->length )  goto err_index;

   // type checking on value arg: restricted types
   if(t_argv[1].type == FSVT_array || t_argv[1].type == FSVT_string)  goto err_valuetype;

   // copy full fs_value_t into array at given index
   memcpy(&(t_argv[0].value.a->values[index]), &t_argv[1],
          sizeof(fs_value_t));
done:
   return;

err_numarg:
   wrong_num_arg("ArraySetElementAt", 3);
   goto done;

err_argtype:
   script_error("ArraySetElementAt: requires non-empty array\n");
   goto done;

err_index:
   script_error("ArraySetElementAt: array index exceeds array\n");
   goto done;

err_valuetype:
   script_error("ArraySetElementAt: cannot store array, string values\n" );
   goto done;
}

//
// SF_ArrayLength
//
// Retrieves the length of an array
//
// int ArrayLength( array x )
void SF_ArrayLength(void)
{
   if(!t_argc)  goto err_numarg;
   if(t_argv[0].type != FSVT_array)  goto err_argtype;

   t_return.type = FSVT_int;
   if(!t_argv[0].value.a)
      t_return.value.i = 0;
   else
      t_return.value.i = t_argv[0].value.a->length;
done:
   return;

err_numarg:
   wrong_num_arg("ArrayLength", 1);
   goto done;

err_argtype:
   script_error("ArrayLength: requires array parameter\n");
   goto done;
}
