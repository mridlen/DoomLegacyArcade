//-----------------------------------------------------------------------------
//
// $Id: doom_umi_parser.c 1610 2023-10-06 09:40:13Z wesleyjohnson $
//
// Copyright (C) 2023 by DooM Legacy Team.
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

#ifndef DOOM_UMI_PARSER_H
#define DOOM_UMI_PARSER_H  1

/* Header file for parser */
// All headers files are an interface.  Do not need another API.


/* Parser return code suitable for API */
extern int umi_parser_result;

void umi_parser_init(void);
int  umi_parser_parse(void);

void umi_parser_error( const char * msg );


#endif  /* DOOM_UMI_PARSER_H */
/* EOF */
