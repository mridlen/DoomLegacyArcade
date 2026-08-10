// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_oper.c 1572 2021-01-28 09:25:24Z wesleyjohnson $
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
// $Log: t_oper.c,v $
// Revision 1.5  2002/08/27 21:15:24  hurdler
// Fix the little problem with the "+" oper and strings
//
// Revision 1.4  2002/07/28 17:11:33  hurdler
// Change version number to reflect the big changes since v.30
//
// Revision 1.3  2001/03/21 18:24:56  stroggonmeth
// Misc changes and fixes. Code cleanup
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
// Operators
//
// Handler code for all the operators. The 'other half'
// of the parsing.
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
#include "t_vari.h"
#include "t_script.h"

fs_value_t OPequals(int, int, int);           // =

fs_value_t OPplus(int, int, int);             // +
fs_value_t OPminus(int, int, int);            // -
fs_value_t OPmultiply(int, int, int);         // *
fs_value_t OPdivide(int, int, int);           // /
fs_value_t OPremainder(int, int, int);        // %

fs_value_t OPor(int, int, int);               // ||
fs_value_t OPand(int, int, int);              // &&
fs_value_t OPnot(int, int, int);              // !

fs_value_t OPor_bin(int, int, int);           // |
fs_value_t OPand_bin(int, int, int);          // &
fs_value_t OPnot_bin(int, int, int);          // ~

fs_value_t OPcmp(int, int, int);              // ==
fs_value_t OPnotcmp(int, int, int);           // !=
fs_value_t OPlessthan(int, int, int);         // <
fs_value_t OPgreaterthan(int, int, int);      // >

fs_value_t OPincrement(int, int, int);        // ++
fs_value_t OPdecrement(int, int, int);        // --

fs_value_t OPlessthanorequal(int, int, int);  // <=
fs_value_t OPgreaterthanorequal(int, int, int); // >=

fs_value_t OPstructure(int, int, int);    // in t_vari.c

operator_t operators[]=
{
  {"=",   OPequals,               D_backward},
  {"||",  OPor,                   D_forward},
  {"&&",  OPand,                  D_forward},
  {"|",   OPor_bin,               D_forward},
  {"&",   OPand_bin,              D_forward},
  {"==",  OPcmp,                  D_forward},
  {"!=",  OPnotcmp,               D_forward},
  {"<",   OPlessthan,             D_forward},
  {">",   OPgreaterthan,          D_forward},
  {"<=",  OPlessthanorequal,      D_forward},
  {">=",  OPgreaterthanorequal,   D_forward},
  {"+",   OPplus,                 D_forward},
  {"-",   OPminus,                D_forward},
  {"*",   OPmultiply,             D_forward},
  {"/",   OPdivide,               D_forward},
  {"%",   OPremainder,            D_forward},
  {"!",   OPnot,                  D_forward},
  {"++",  OPincrement,            D_forward},
  {"--",  OPdecrement,            D_forward},
  {".",   OPstructure,            D_forward},
};

int num_operators = sizeof(operators) / sizeof(operator_t);

/***************** logic *********************/

// = operator

fs_value_t OPequals(int start, int n, int stop)
{
  fs_variable_t *var;
  fs_value_t evaluated;
  
  var = find_variable(tokens[start]);
  if( ! var)  goto err_novar;

  evaluated = evaluate_expression(n+1, stop);
  setvariablevalue(var, evaluated);
  return evaluated;

err_novar:
  script_error("unknown variable '%s'\n", tokens[start]);
  return nullvar;
}


fs_value_t OPor(int start, int n, int stop)
{
  fs_value_t returnvar;
  fs_value_t eval;
  
  // if first is true, do not evaluate the second
  
  returnvar.type = FSVT_int;
  eval = evaluate_expression(start, n-1);
  if(intvalue(eval))
      returnvar.value.i = true;
  else
  {
      eval = evaluate_expression(n+1, stop);
      returnvar.value.i = !!intvalue(eval);
  }
  return returnvar;
}


fs_value_t OPand(int start, int n, int stop)
{
  fs_value_t returnvar;
  fs_value_t eval;

  // if first is false, do not eval second
  
  returnvar.type = FSVT_int;
  eval = evaluate_expression(start, n-1);
  
  if(!intvalue(eval) )
      returnvar.value.i = false;
  else
  {
      eval = evaluate_expression(n+1, stop);
      returnvar.value.i = !!intvalue(eval);
  }
  return returnvar;
}

fs_value_t OPcmp(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = FSVT_int;        // always an int returned
  
  if(left.type == FSVT_string && right.type == FSVT_string)
  {
      returnvar.value.i = !strcmp(left.value.s, right.value.s);
      return returnvar;
  }

  if(left.type == FSVT_fixed || right.type == FSVT_fixed)
  {
      returnvar.value.i = fixedvalue(left) == fixedvalue(right);
      return returnvar;
  }

  if(left.type == FSVT_mobj || right.type == FSVT_mobj)
  {
    if(left.type == FSVT_mobj && right.type == FSVT_mobj)
      returnvar.value.i = left.value.mobj == right.value.mobj;
    else if(left.type == FSVT_mobj)
      returnvar.value.i = (left.value.mobj == MobjForSvalue(right)) ? 1 : 0;
    else if(right.type == FSVT_mobj)
      returnvar.value.i = (MobjForSvalue(left) == right.value.mobj) ? 1 : 0;

    return returnvar;
  }

  returnvar.value.i = intvalue(left) == intvalue(right);
  return returnvar;
}

fs_value_t OPnotcmp(int start, int n, int stop)
{
  fs_value_t returnvar;
  
  returnvar = OPcmp(start, n, stop);
  returnvar.value.i = !returnvar.value.i;
  
  return returnvar;
}

fs_value_t OPlessthan(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = FSVT_int;

  if(left.type == FSVT_fixed || right.type == FSVT_fixed)
    returnvar.value.i = fixedvalue(left) < fixedvalue(right);
  else
    returnvar.value.i = intvalue(left) < intvalue(right);

  return returnvar;
}

fs_value_t OPgreaterthan(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = FSVT_int;

  if(left.type == FSVT_fixed || right.type == FSVT_fixed)
    returnvar.value.i = fixedvalue(left) > fixedvalue(right);
  else
    returnvar.value.i = intvalue(left) > intvalue(right);

  return returnvar;
}

fs_value_t OPnot(int start, int n, int stop)
{
  fs_value_t right, returnvar;
  
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = FSVT_int;
  returnvar.value.i = !intvalue(right);
  return returnvar;
}

/************** simple math ***************/

fs_value_t OPplus(int start, int n, int stop)
{
    fs_value_t left, right, returnvar;
  
    left = evaluate_expression(start, n-1);
    right = evaluate_expression(n+1, stop);

    if (left.type == FSVT_string)
    {
        char *tmp;
        if (right.type == FSVT_string)
        {
            tmp = Z_Malloc(strlen(left.value.s) + strlen(right.value.s) + 1, PU_LEVEL, 0);
            sprintf(tmp, "%s%s", left.value.s, right.value.s);
        }
        else if (right.type == FSVT_fixed)
        {
            tmp = Z_Malloc(strlen(left.value.s) + 12, PU_LEVEL, 0);
            sprintf(tmp, "%s%4.4f", left.value.s, FIXED_TO_FLOAT(right.value.f));
        }
        else
        {
            tmp = Z_Malloc(strlen(left.value.s) + 12, PU_LEVEL, 0);
            sprintf(tmp, "%s%d", left.value.s, intvalue(right));
        }
        returnvar.type = FSVT_string;
        returnvar.value.s = tmp;
    }
    else if(left.type == FSVT_fixed || right.type == FSVT_fixed)
    {
        returnvar.type = FSVT_fixed;
        returnvar.value.f = fixedvalue(left) + fixedvalue(right);
    }
    else
    {
        returnvar.type = FSVT_int;
        returnvar.value.i = intvalue(left) + intvalue(right);
    }
    return returnvar;
}

fs_value_t OPminus(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  // do they mean minus as in '-1' rather than '2-1'?
  if(start == n)
  {
      // kinda hack, hehe
      left.value.i = 0; left.type = FSVT_int;
      right = evaluate_expression(n+1, stop);
  }
  else
  {
      left = evaluate_expression(start, n-1);
      right = evaluate_expression(n+1, stop);
  }
  
  if(left.type == FSVT_fixed || right.type == FSVT_fixed)
  {
      returnvar.type = FSVT_fixed;
      returnvar.value.f = fixedvalue(left) - fixedvalue(right);
  }
  else
  {
      returnvar.type = FSVT_int;
      returnvar.value.i = intvalue(left) - intvalue(right);
  }

  return returnvar;
}

fs_value_t OPmultiply(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  if(left.type == FSVT_fixed || right.type == FSVT_fixed)
  {
      returnvar.type = FSVT_fixed;
      returnvar.value.f = FixedMul(fixedvalue(left), fixedvalue(right));
  }
  else
  {
      returnvar.type = FSVT_int;
      returnvar.value.i = intvalue(left) * intvalue(right);
  }

  return returnvar;
}

fs_value_t OPdivide(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
//  if(left.type == FSVT_fixed || right.type == FSVT_fixed)
    {
      fixed_t fr = fixedvalue(right);

      if(fr == 0)
      {
          returnvar.type = FSVT_fixed;
          returnvar.value.f = 0x80000000;
          script_error("divide by zero\n");
      }
      else
      {
          returnvar.type = FSVT_fixed;
          returnvar.value.f = FixedDiv(fixedvalue(left), fr);
      }
    }
/*  else
    {
      int ir;

      if(!(ir = intvalue(right)))
        script_error("divide by zero\n");
      else
	{
          returnvar.type = FSVT_int;
          returnvar.value.i = intvalue(left) / ir;
	}
    }*/

  return returnvar;
}

fs_value_t OPremainder(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  int ir;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  ir = intvalue(right);
  if(ir == 0)
  {
      returnvar.type = FSVT_int;
      returnvar.value.f = 0;
      script_error("divide by zero\n");
  }
  else
  {
      returnvar.type = FSVT_int;
      returnvar.value.i = intvalue(left) % ir;
  }
  return returnvar;
}

        /********** binary operators **************/

// binary or |
fs_value_t OPor_bin(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = FSVT_int;
  returnvar.value.i = intvalue(left) | intvalue(right);
  return returnvar;
}


// binary and &

fs_value_t OPand_bin(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  
  returnvar.type = FSVT_int;
  returnvar.value.i = intvalue(left) & intvalue(right);
  return returnvar;
}



// ++
fs_value_t OPincrement(int start, int n, int stop)
{
  fs_value_t origvalue, value;
  fs_variable_t *var;
      
  value.type = FSVT_int;
  if(start == n)          // ++n
  {
      var = find_variable(tokens[stop]);
      if(!var)  goto err_novar_stop;
      origvalue = getvariablevalue(var);
      
      value.value.i = intvalue(origvalue) + 1;
      setvariablevalue(var, value);
      
      return value;
  }
  else if(stop == n)     // n++
  {
      var = find_variable(tokens[start]);
      if(!var)  goto err_novar;
      origvalue = getvariablevalue(var);
      
      value.value.i = intvalue(origvalue) + 1;
      setvariablevalue(var, value);
      
      return origvalue;
  }
  
  script_error("incorrect arguments to ++ operator\n");
  return nullvar;

err_novar_stop:
  start = stop;  // just for error message
err_novar:
  script_error("unknown variable '%s'\n", tokens[start]);
  return nullvar;
}

// --
fs_value_t OPdecrement(int start, int n, int stop)
{
  fs_value_t origvalue, value;
  fs_variable_t *var;
      
  value.type = FSVT_int;
  if(start == n)          // --n
  {
      var = find_variable(tokens[stop]);
      if(!var)  goto err_novar_stop;
      origvalue = getvariablevalue(var);
      
      value.value.i = intvalue(origvalue) - 1;
      value.type = FSVT_int;
      setvariablevalue(var, value);
      
      return value;
  }
  else if(stop == n)   // n--
  {
      var = find_variable(tokens[start]);
      if(!var)  goto err_novar;
      origvalue = getvariablevalue(var);
      
      value.type = FSVT_int;
      value.value.i = intvalue(origvalue) - 1;
      setvariablevalue(var, value);
      
      return origvalue;
  }
  
  script_error("incorrect arguments to ++ operator\n");
  return nullvar;

err_novar_stop:
  start = stop;  // just for error message
err_novar:
  script_error("unknown variable '%s'\n", tokens[start]);
  return nullvar;
}


// Thank you Quasar!
fs_value_t OPlessthanorequal(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;

  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  returnvar.type = FSVT_int;
  returnvar.value.i = intvalue(left) <= intvalue(right);
  return returnvar;
}


fs_value_t OPgreaterthanorequal(int start, int n, int stop)
{
  fs_value_t left, right, returnvar;
  left = evaluate_expression(start, n-1);
  right = evaluate_expression(n+1, stop);
  returnvar.type = FSVT_int;
  returnvar.value.i = intvalue(left) >= intvalue(right);
  return returnvar;
}

