# CMake configuration file for Doom Legacy 1.x
# SDL interface module

set(SRC_sdl
  dosstr.c
  endtxt.c
  filesrch.c
  hwsym_sdl.c
  i_cdmus.c
  i_main.c
  i_net.c
  i_sound.c
  i_system.c
  i_video.c
  ogl_sdl.c)

add_library(media_sdl ${SRC_sdl})
