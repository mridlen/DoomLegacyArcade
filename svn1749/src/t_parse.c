// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// $Id: t_parse.c 1368 2017-11-01 01:17:48Z wesleyjohnson $
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
// $Log: t_parse.c,v $
// Revision 1.9  2005/05/21 08:41:23  iori_
// May 19, 2005 - PlayerArmor FS function;  1.43 can be compiled again.
//
// Revision 1.8  2004/08/26 23:15:41  hurdler
// add FS functions in console (+ minor linux fixes)
//
// Revision 1.7  2004/07/27 08:19:37  exl
// New fmod, fs functions, bugfix or 2, patrol nodes
//
// Revision 1.6  2003/07/13 13:16:15  hurdler
// Revision 1.5  2001/08/14 00:36:26  hurdler
// Revision 1.4  2001/05/03 21:22:25  hurdler
// remove some warnings
//
// Revision 1.3  2000/11/04 16:23:44  bpereira
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
// Parsing.
//
// Takes lines of code, or groups of lines and runs them.
// The main core of FraggleScript
//
// By Simon Howard
//
//----------------------------------------------------------------------------

/* includes ************************/

#include "doomincl.h"
#include "doomstat.h"
#include "command.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "g_game.h"

#include "t_parse.h"
#include "t_prepro.h"
#include "t_spec.h"
#include "t_oper.h"
#include "t_vari.h"
#include "t_func.h"

void parse_script( void );
void parse_data(char *data, char *end);
fs_value_t evaluate_expression(int start, int stop);

script_t * fs_current_script;       // the current script
mobj_t * fs_trigger_obj;         // object which triggered script
int fs_killscript;               // when set to true, stop the script quickly

char *tokens[T_MAXTOKENS];
tokentype_t tokentype[T_MAXTOKENS];
int num_tokens = 0;
byte script_debug = false;

fs_value_t nullvar = { FSVT_int, {0} };    // null var for empty return

/************ Divide into tokens **************/

char * fs_linestart_cp;          // start of line
char * fs_src_cp;              // current point reached in script
fs_section_t * fs_current_section;  // the section (if any) found in parsing the line
fs_section_t * fs_prev_section;     // the section from the previous statement
int fs_bracetype;                // BRACKET_open or BRACKET_close

        // inline for speed
#define isnum(c) ( ((c)>='0' && (c)<='9') || (c)=='.' )
        // isop: is an 'operator' character, eg '=', '%'
#define isop(c)   !( ( (c)<='Z' && (c)>='A') || ( (c)<='z' && (c)>='a') || \
                     ( (c)<='9' && (c)>='0') || ( (c)=='_') )

        // for simplicity:
#define tt (tokentype[num_tokens-1])
#define tok (tokens[num_tokens-1])

static void add_char(char c);

// next_token: end this token, go onto the next
static void next_token( void )
{
    if (tok[0] || tt == TT_string)
    {
        num_tokens++;
        tokens[num_tokens - 1] = tokens[num_tokens - 2] + strlen(tokens[num_tokens - 2]) + 1;
        tok[0] = 0;
    }

    // get to the next token, ignoring spaces, newlines,
    // useless chars, comments etc
    while (*fs_src_cp && (*fs_src_cp == ' ' || *fs_src_cp < 32))
        fs_src_cp++;
    if (!*fs_src_cp)  goto end_of_char;  // end-of-script
    // 11/8 comments moved to new preprocessor

    if (num_tokens > 1 && *fs_src_cp == '(' && tokentype[num_tokens - 2] == TT_name)
        tokentype[num_tokens - 2] = TT_function;

    switch ( *fs_src_cp )
    {
     case '{':
        fs_bracetype = BRACKET_open;
        fs_current_section = find_section_start(fs_src_cp);
        if (!fs_current_section)  goto err_nosection;
        break;
     case '}':    // closing brace
        fs_bracetype = BRACKET_close;
        fs_current_section = find_section_end(fs_src_cp);
        if (!fs_current_section)  goto err_nosection;
        break;
     case ':':     // label
        // ignore the label : reset
        num_tokens = 1;
        tokens[0][0] = 0;
        tt = TT_name;
        fs_src_cp++;        // ignore
        break;
     case '\"':
        tt = TT_string;
        if (tokentype[num_tokens - 2] == TT_string)
            num_tokens--;       // join strings
        fs_src_cp++;
        break;
     default:
        tt = isop(*fs_src_cp) ? TT_operator
           : isnum(*fs_src_cp) ? TT_number : TT_name;
        break;
    }
done:   
    return;

end_of_char:
    if (tokens[0][0])
    {
        CONS_Printf("%s %i %i\n", tokens[0], fs_src_cp - fs_current_script->data, fs_current_script->len);
        // line contains text, but no semicolon: an error
        script_error("missing ';'\n");
    }
    // empty line, end of command-list
    goto done;

err_nosection:
    script_error("section not found!\n");
    goto done;
}

// return an escape sequence (prefixed by a '\')
// do not use all C escape sequences
static char escape_sequence(char c)
{
    if (c == 'n')
        return '\n';
    if (c == '\\')
        return '\\';
    if (c == '"')
        return '"';
    if (c == '?')
        return '?';
    if (c == 'a')
        return '\a';    // alert beep
    if (c == 't')
        return '\t';    //tab
//  if(c == 'z') return *FC_TRANS;    // translucent toggle

    // font colours
    //Hurdler: fix for Legacy text color
    if (c >= '0' && c <= '9')
        return /*128 + */ (c - '0');

    return c;
}

// add_char: add one character to the current token
static void add_char(char c)
{
    char *out = tok + strlen(tok);

    *out++ = c;
    *out = 0;
}

// get_tokens.
// Take a string, break it into tokens.

// individual tokens are stored inside the tokens[] array
// tokentype is also used to hold the type for each token:

//   TT_name: a piece of text which starts with an alphabet letter.
//         probably a variable name. Some are converted into
//         function types later on in find_brackets
//   TT_number: a number. like '12' or '1337'
//   TT_operator: an operator such as '&&' or '+'. All FraggleScript
//             operators are either one character, or two character
//             (if 2 character, 2 of the same char or ending in '=')
//   TT_string: a text string that was enclosed in quote "" marks in
//           the original text
//   TT_unset: shouldn't ever end up being set really.
//   TT_function: a function name (found in second stage parsing)

void get_tokens(char *s)
{
    fs_src_cp = s;
    num_tokens = 1;
    tokens[0][0] = 0;
    tt = TT_name;

    fs_current_section = NULL;     // default to no section found

    next_token();
    fs_linestart_cp = fs_src_cp;  // save the start

    if (*fs_src_cp)
    {
        while (1)
        {
            if (fs_killscript)  goto done;
            if (fs_current_section)
            {
                // a { or } section brace has been found
                break;  // stop parsing now
            }
            else if (tt != TT_string)
            {
                if (*fs_src_cp == ';')
                    break;      // check for end of command ';'
            }

            switch (tt)
            {
                case TT_unset:
                case TT_string:
                    while (*fs_src_cp != '\"')      // dedicated loop for speed
                    {
                        if (*fs_src_cp == '\\')     // escape sequences
                        {
                            fs_src_cp++;
                            add_char(escape_sequence(*fs_src_cp));
                        }
                        else
                            add_char(*fs_src_cp);
                        fs_src_cp++;
                    }
                    fs_src_cp++;
                    next_token();       // end of this token
                    continue;

                case TT_operator:
                    // all 2-character operators either end in '=' or
                    // are 2 of the same character
                    // do not allow 2-characters for brackets '(' ')'
                    // which are still being considered as operators

                    // TT_operators are only 2-char max, do not need
                    // a seperate loop

                    if ((*tok && *fs_src_cp != '=' && *fs_src_cp != *tok) || *tok == '(' || *tok == ')')
                    {
                        // end of TT_operator
                        next_token();
                        continue;
                    }
                    add_char(*fs_src_cp);
                    break;

                case TT_number:

                    // add while number chars are read

                    while (isnum(*fs_src_cp))       // dedicated loop
                        add_char(*fs_src_cp++);
                    next_token();
                    continue;

                case TT_name:

                    // add the chars

                    while (!isop(*fs_src_cp))       // dedicated loop
                        add_char(*fs_src_cp++);
                    next_token();
                    continue;

                default:
                    break;      // shut up compiler

            }
            fs_src_cp++;
        }
    }
    // check for empty last token

    if (!tok[0])
    {
        num_tokens = num_tokens - 1;
    }

    fs_src_cp++;
done:
    return;
}

void print_tokens( void )             // DEBUG
{
    int i;
    for (i = 0; i < num_tokens; i++)
    {
        CONS_Printf("\n'%s' \t\t --", tokens[i]);
        switch (tokentype[i])
        {
            case TT_string:
                CONS_Printf("string");
                break;
            case TT_operator:
                CONS_Printf("operator");
                break;
            case TT_name:
                CONS_Printf("name");
                break;
            case TT_number:
                CONS_Printf("number");
                break;
            case TT_unset:
                CONS_Printf("duh");
                break;
            case TT_function:
                CONS_Printf("function name");
                break;
        }
    }
    CONS_Printf("\n");
    if (fs_current_section)
        CONS_Printf("current section: offset %i\n", (int) (fs_current_section->start - fs_current_script->data));
}

// run_script
//
// the function called by t_script.c

void run_script(script_t * script)
{
    // set current script
    fs_current_script = script;

    // start at the beginning of the script
    fs_src_cp = fs_current_script->data;

    fs_current_script->lastiftrue = false;

    parse_script();     // run it
}

void continue_script(script_t * script, char *continue_point)
{
    fs_current_script = script;

    // continue from place specified
    fs_src_cp = continue_point;

    parse_script();     // run
}

void parse_script( void )
{
    // check for valid fs_src_cp
    if (fs_src_cp < fs_current_script->data || fs_src_cp > fs_current_script->data + fs_current_script->len)  goto err_parseptr;

    fs_trigger_obj = fs_current_script->trigger;      // set trigger

    parse_data(fs_current_script->data, fs_current_script->data + fs_current_script->len);

    // dont clear global vars!
    if (fs_current_script->scriptnum != -1)
        clear_variables(fs_current_script);        // free variables

    fs_current_script->lastiftrue = false;
done:
    return;

err_parseptr:
    script_error("parse_script: trying to continue from point outside script!\n");
    goto done;
}

/*
void run_string(char *data)
{
    extern script_t levelscript;
    static char buffer[4096];

    snprintf(buffer, 4096, "%s;\n", data);
    script_t script;

    memset(&script, 0, sizeof(script));

    script.data = buffer;
    script.scriptnum = -1; // dummy value
    script.len = strlen(script.data);
    script.variables[0] = NULL;
    script.sections[0] = NULL;
    script.parent = &levelscript;
    script.children[0] = NULL;
    script.trigger = players[0].mo;
    script.lastiftrue = false;

    run_script(&script);
    fs_current_script = &levelscript;
}
*/

void parse_data(char *data, char *end)
{
    char *token_alloc;          // allocated memory for tokens

    fs_killscript = false; // dont kill the script straight away

    // allocate space for the tokens
    token_alloc = Z_Malloc(fs_current_script->len + T_MAXTOKENS, PU_STATIC, 0);

    fs_prev_section = NULL;        // clear it

    while (*fs_src_cp)      // go through the script executing each statement
    {
        if (fs_src_cp > end)  // past end of script?
            break;

        // reset the tokens before getting the next line
        tokens[0] = token_alloc;

        fs_prev_section = fs_current_section; // store from prev. statement

        // get the line and tokens
        get_tokens(fs_src_cp);

        if (fs_killscript)
            break;

        if (!num_tokens)
        {
            if (fs_current_section)        // no tokens but a brace
            {
                // possible } at end of loop:
                // refer to spec.c
                spec_brace();
            }

            continue;   // continue to next statement
        }

        if (script_debug)
            print_tokens();     // debug
        run_statement();        // run the statement
    }
    Z_Free(token_alloc);
}

void run_statement( void )
{
    // decide what to do with it

    // NB this stuff is a bit hardcoded:
    //    it could be nicer really but i'm
    //    aiming for speed

    // if() and while() will be mistaken for functions
    // during token processing
    if (tokentype[0] == TT_function)
    {
        if (!strcmp(tokens[0], "if"))
        {
            fs_current_script->lastiftrue = spec_if()? true : false;
            return;
        }
        else if (!strcmp(tokens[0], "elseif"))
        {
            if (!fs_prev_section || (fs_prev_section->type != FSST_if && fs_prev_section->type != FSST_elseif))
                goto err_elseif_without_if;
            fs_current_script->lastiftrue = spec_elseif(fs_current_script->lastiftrue) ? true : false;
            return;
        }
        else if (!strcmp(tokens[0], "else"))
        {
            if (!fs_prev_section || (fs_prev_section->type != FSST_if && fs_prev_section->type != FSST_elseif))
                goto err_else_without_if;
            spec_else(fs_current_script->lastiftrue);
            fs_current_script->lastiftrue = true;
            return;
        }
        else if (!strcmp(tokens[0], "while"))
        {
            spec_while();
            return;
        }
        else if (!strcmp(tokens[0], "for"))
        {
            spec_for();
            return;
        }
    }
    else if (tokentype[0] == TT_name)
    {
        // NB: goto is a function so is not here

        // Hurdler: else is a special case (no more need to add () after else)
        if (!strcmp(tokens[0], "else"))
        {
            if (!fs_prev_section
                || (fs_prev_section->type != FSST_if && fs_prev_section->type != FSST_elseif))
                goto err_else_without_if;
            spec_else(fs_current_script->lastiftrue);
            fs_current_script->lastiftrue = true;
            return;
        }

        // if a variable declaration, return now
        if (spec_variable())
            return;
    }

    // just a plain expression
    evaluate_expression(0, num_tokens - 1);
done:
    return;

err_elseif_without_if:
    script_error("elseif without if!\n");
    goto done;

err_else_without_if:
    script_error("else without if!\n");
    goto done;
}

/***************** Evaluating Expressions ************************/

// find a token, ignoring things in brackets
int find_operator(int start, int stop, const char *value)
{
    int i;
    int bracketlevel = 0;

    for (i = start; i <= stop; i++)
    {
        // only interested in operators
        if (tokentype[i] != TT_operator)
            continue;

        // use bracketlevel to check the number of brackets
        // which we are inside
        bracketlevel += tokens[i][0] == '(' ? 1 : tokens[i][0] == ')' ? -1 : 0;

        // only check when we are not in brackets
        if (!bracketlevel && !strcmp(value, tokens[i]))
            return i;
    }

    return -1;
}

// go through tokens the same as find_operator, but backwards
int find_operator_backwards(int start, int stop, const char *value)
{
    int i;
    int bracketlevel = 0;

    for (i = stop; i >= start; i--)     // check backwards
    {
        // operators only

        if (tokentype[i] != TT_operator)
            continue;

        // use bracketlevel to check the number of brackets
        // which we are inside

        bracketlevel += tokens[i][0] == '(' ? -1 : tokens[i][0] == ')' ? 1 : 0;

        // only check when we are not in brackets
        // if we find what we want, return it

        if (!bracketlevel && !strcmp(value, tokens[i]))
            return i;
    }

    return -1;
}

// simple_evaluate is used once evalute_expression gets to the level
// where it is evaluating just one token

// converts TT_number tokens into fs_value_ts and returns
// the same with TT_string tokens
// TT_name tokens are considered to be variables and
// attempts are made to find the value of that variable
// command tokens are executed (does not return a fs_value_t)

extern fs_value_t nullvar;

static fs_value_t simple_evaluate(int n)
{
    fs_value_t returnvar;
    fs_variable_t *var;

    switch (tokentype[n])
    {
        case TT_string:
            returnvar.type = FSVT_string;
            returnvar.value.s = tokens[n];
            break;

        case TT_number:
            if (strchr(tokens[n], '.'))
            {
                returnvar.type = FSVT_fixed;
                returnvar.value.f = atof(tokens[n]) * FRACUNIT;
            }
            else
            {
                returnvar.type = FSVT_int;
                returnvar.value.i = atoi(tokens[n]);
            }
            break;

        case TT_name:
            var = find_variable(tokens[n]);
            if (!var)  goto err_unknownvar;
            return getvariablevalue(var);

        default:
            goto done_null;
    }
    return returnvar;

done_null:
    return nullvar;

err_unknownvar:
    script_error("unknown variable '%s'\n", tokens[n]);
    goto done_null;
}

// pointless_brackets : checks to see if there are brackets surrounding
// an expression. eg. "(2+4)" is the same as just "2+4"
//
// Because of the recursive nature of evaluate_expression, this function is
// necessary as evaluating expressions such as "2*(2+4)" will inevitably
// lead to evaluating "(2+4)"

static void pointless_brackets(int *start, int *stop)
{
    int bracket_level, i;

    // check that the start and end are brackets

    while (tokens[*start][0] == '(' && tokens[*stop][0] == ')')
    {

        bracket_level = 0;

        // confirm there are pointless brackets..
        // if they are, bracket_level will only get to 0
        // at the last token
        // check up to <*stop rather than <=*stop to ignore
        // the last token

        for (i = *start; i < *stop; i++)
        {
            if (tokentype[i] != TT_operator)
                continue;       // ops only
            bracket_level += (tokens[i][0] == '(');
            bracket_level -= (tokens[i][0] == ')');
            if (bracket_level == 0)
                return; // stop if braces stop before end
        }

        // move both brackets in
        *start = *start + 1;
        *stop = *stop - 1;
    }
}

// evaluate_expresion : is the basic function used to evaluate
// a FraggleScript expression.
// start and stop denote the tokens which are to be evaluated.
//
// Works by recursion: it finds operators in the expression
// (checking for each in turn), then splits the expression into
// 2 parts, left and right of the operator found.
// The handler function for that particular operator is then
// called, which in turn calls evaluate_expression again to
// evaluate each side. When it reaches the level of being asked
// to evaluate just 1 token, it calls simple_evaluate

fs_value_t evaluate_expression(int start, int stop)
{
    int i, n;

    if (fs_killscript)  goto done_null; // killing the script

    // possible pointless brackets
    if (tokentype[start] == TT_operator && tokentype[stop] == TT_operator)
        pointless_brackets(&start, &stop);

    if (start == stop)  // only 1 thing to evaluate
    {
        return simple_evaluate(start);
    }

    // go through each operator in order of precedence

    for (i = 0; i < num_operators; i++)
    {
        // check backwards for the token. it has to be
        // done backwards for left-to-right reading: eg so
        // 5-3-2 is (5-3)-2 not 5-(3-2)

        if (-1 !=
            (n = (
                  (operators[i].direction == D_forward) ? find_operator_backwards : find_operator
                  ) (start, stop, operators[i].string)
             )
            )
        {
            // CONS_Printf("operator %s, %i-%i-%i\n", operators[count].string, start, n, stop);

            // call the operator function and evaluate this chunk of tokens
            return operators[i].handler(start, n, stop);
        }
    }

    if (tokentype[start] == TT_function)
        return evaluate_function(start, stop);

    // error ?
    {
#define ERRSTR_LEN  1023       
        char errstr[ERRSTR_LEN+1] = "";
        int  len = 0;

        for (i = start; i <= stop; i++)
        {
            len += 1 + strlen( tokens[i] );
            if( len > ERRSTR_LEN ) break;
            strcat( errstr, " " );
            strcat( errstr, tokens[i] );
        }
        errstr[ERRSTR_LEN] = '\0';

        script_error("could not evaluate expression: %s\n", errstr);
    }
done_null:   
    return nullvar;
}

void script_error(const char *fmt, ...)
{
    va_list  ap;

    if (fs_killscript)
        return; //already killing script

    if (fs_current_script->scriptnum == -1)
        CONS_Printf("global");
    else
        CONS_Printf("%i", fs_current_script->scriptnum);

    // find the line number

    if (fs_src_cp >= fs_current_script->data && fs_src_cp <= fs_current_script->data + fs_current_script->len)
    {
        int linenum = 1;
        char *temp;
        for (temp = fs_current_script->data; temp < fs_linestart_cp; temp++)
        {
            if (*temp == '\n')
                linenum++;      // count EOLs
        }
        CONS_Printf(", %i", linenum);
    }
    CONS_Printf(": ");

    va_start(ap, fmt);
    GenPrintf_va(EMSG_error, fmt, ap);
    va_end(ap);

    // make a noise
    S_StartSound(sfx_pldeth);

    fs_killscript = true;
}

//
// sf: string value of an fs_value_t
//
const char * stringvalue(fs_value_t v)
{
#define STRVAL_BUFLEN  255
    static char buffer[STRVAL_BUFLEN+1];

    switch (v.type)
    {
        case FSVT_string:
            return v.value.s;

        case FSVT_mobj:
            return "map object";

        case FSVT_fixed:
        {
            double val = ((double) v.value.f / FRACUNIT);
            snprintf(buffer, STRVAL_BUFLEN, "%g", val);
        }

        case FSVT_array:
            return "array";

        case FSVT_int:
        default:
            snprintf(buffer, STRVAL_BUFLEN, "%d", v.value.i);
    }
    buffer[STRVAL_BUFLEN] = '\0';
    return buffer;
}

