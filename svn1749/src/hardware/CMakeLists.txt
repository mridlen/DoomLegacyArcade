# CMake configuration file for Doom Legacy 1.x
# OpenGL module

set(SRC_hardware_renderer
  hw_bsp.c
  hw_cache.c
  hw_draw.c
  hw_light.c
  hw_main.c
  hw_md2.c
  hw_trick.c
  r_opengl/r_opengl.c)

add_library(hardware_renderer ${SRC_hardware_renderer})
