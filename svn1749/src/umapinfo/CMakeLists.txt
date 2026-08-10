# CMake configuration file for Doom Legacy 1.x
# umapinfo interface module

set(SRC_umapinfo
  doom_umi_api.c
  doom_umi_lexer.c
  doom_umi_parser.c
  doom_umi_memory.c
  doom_umi_keywords.c

add_library(umapinfo ${SRC_umapinfo})
