# CMake configuration file for Doom Legacy 1.x

project(DoomLegacy C)
cmake_minimum_required(VERSION 2.8)


# prevent bad practices
if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
   message(FATAL_ERROR "In-tree build attempt detected, aborting. Set your build dir outside your source dir, delete CMakeCache.txt from source root and try again.")
endif()

# Setup source
message( STATUS "Project dir:" ${PROJECT_SOURCE_DIR} )
if( EXISTS ${PROJECT_SOURCE_DIR}/d_main.c )
  message( STATUS "Source dir: " ${PROJECT_SOURCE_DIR} )
  set(SRCDIR ${PROJECT_SOURCE_DIR})
else()
  set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
  message( STATUS "Test Source dir:" ${SRC_DIR} )
  if( EXISTS ${SRC_DIR} )
    # Source directory
    set(CMAKE_CURRENT_SOURCE_DIR  ${SRC_DIR} )
    message( STATUS "Source dir: " ${SRC_DIR} )
  endif()
endif()

if( NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/d_main.c )
  message( FATAL_ERROR "Source file not found: " ${CMAKE_CURRENT_SOURCE_DIR} "/d_main.c" )
endif()
message( STATUS "Working source directory: ${CMAKE_CURRENT_SOURCE_DIR}" )

# cmake options
# more informative compilation
set(CMAKE_VERBOSE_MAKEFILE ON)
# Linux/Win32 cross-compilation
#set(CMAKE_TOOLCHAIN_FILE "toolchain_win32.cmake")  
# make a debug build
#set(CMAKE_BUILD_TYPE "debug")


# some build options (TODO implement)
option(OPT_SVN "Are you building out of an SVN working copy?" OFF)
option(OPT_SDL_MIXER "Use SDL_mixer for music?" ON)

if( OPT_SDL_MIXER )
  find_package( SDL_mixer REQUIRED )
endif()


# ZLIB is used for compressed extended nodes (OPTIONAL).
find_package( ZLIB )

if(WIN32)
  # FIXME: Add dynamic loading for WIN32
  set(OPT_DYN_ZLIB OFF)
else()
  if( ZLIB_FOUND )
    option(OPT_DYN_ZLIB "Compile for Dynamic zlib loading?" OFF)
  endif()
endif()


# LIBZIP is used for reading zipped wads (OPTIONAL).
# FIXME: libzip is not found
#find_package( LIBZIP )
if(WIN32)
  # FIXME
  find_library( LIBZIP  libzip )
  # FIXME: Add dynamic loading for WIN32
  set(OPT_DYN_LIBZIP OFF)
else()
  find_library( LIBZIP  libzip  HINTS /usr/lib /usr/local/lib )
  if( LIBZIP_FOUND )
    option(OPT_DYN_LIBZIP "Compile for Dynamic libzip loading?" OFF)
  endif()
endif()



# where should the built executables and libraries go?
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# build types
# TODO use add_compile_options()?
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-result -O3 -ffast-math -fno-strict-aliasing")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall") # TODO -ggdb?
if(CMAKE_BUILD_TYPE STREQUAL "debug")
  add_definitions("-DDEBUG_WINDOWED")
endif()


# system libraries
find_library(LIB_math m)
set(LIBS ${LIB_math}) 

if(WIN32)
  # add windows-specific stuff
  add_definitions("-DWIN32 -DWIN_LARGE_MEM")
  find_library(LIB_wsock32 wsock32)
  set(LIBS ${LIBS} ${LIB_wsock32}) 
else()
  # generic unix-specific stuff
  add_definitions("-DLINUX")
endif()

include_directories(.)

# system multimedia interface (only SDL for now)
set(SMIF "SDL")
if(SMIF STREQUAL "SDL")
  # external packages
  find_package(SDL REQUIRED)
  find_package(SDL_mixer REQUIRED)
  find_package(OpenGL REQUIRED)
  set(LIBS ${LIBS}
    ${SDL_LIBRARY}
    ${SDL_MIXER_LIBRARIES}
    ${OPENGL_LIBRARIES})
  include_directories(
    ${SDL_INCLUDE_DIR}
    ${SDL_MIXER_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR})

  add_definitions("-DSDL -DHWRENDER -DHAVE_MIXER -DCDMUS")
  # SDL media interface source
#  add_subdirectory(sdl)
  add_subdirectory(${SRC_DIR}/sdl)
endif()

if( ZLIB_FOUND )
  message( STATUS  "Found ZLIB" )
  include_directories( ${ZLIB_INCLUDE_DIR} )
  if( OPT_DYN_ZLIB )
    # Dynamic loading
    add_definitions( "-DHAVE_ZLIB=3" )
  else()
    add_definitions( "-DHAVE_ZLIB=1" )
    set( LIBS ${LIBS}
        ${ZLIB_LIBRARIES} )
  endif()
endif()

if( LIBZIP_FOUND )
  message( STATUS  "Found LIBZIP" )
  include_directories( ${LIBZIP_INCLUDE_DIR} )
  if( OPT_DYN_LIBZIP )
    # Dynamic loading
    add_definitions( "-DHAVE_LIBZIP=3" )
  else()
    add_definitions( "-DHAVE_LIBZIP=1" )
    set( LIBS ${LIBS}
        ${LIBZIP_LIBRARIES} )
  endif()
endif()


# hardware renderer source
add_subdirectory(${SRC_DIR}/hardware)

# game engine source
message( STATUS "Current source directory: ${CMAKE_CURRENT_SOURCE_DIR}" )
set(SRC
  tables.c info.c dstrings.c
  screen.c v_video.c
  r_draw.c r_plane.c r_segs.c r_sky.c r_things.c r_splats.c r_bsp.c r_data.c r_main.c
  z_zone.c
  p_sight.c p_mobj.c p_enemy.c p_user.c p_inter.c p_pspr.c
  p_lights.c p_ceilng.c p_doors.c p_plats.c p_floor.c p_spec.c
  p_switch.c p_genlin.c p_telept.c p_tick.c p_fab.c p_info.c p_setup.c
  p_blockmap.c p_extnodes.c
  p_map.c p_maputl.c
  p_heretic.c p_hsight.c
  p_chex.c
  sb_bar.c hu_stuff.c
  st_lib.c st_stuff.c
  t_array.c t_func.c t_oper.c t_parse.c t_prepro.c t_script.c t_spec.c t_vari.c
  sounds.c qmus2mid.c s_sound.c s_amb.c mserv.c
  b_game.c b_look.c b_node.c b_search.c
  g_state.c g_input.c g_game.c
  f_finale.c f_wipe.c
  wi_stuff.c
  am_map.c
  md5.c
  m_menu.c m_misc.c m_argv.c m_bbox.c m_fixed.c m_swap.c m_cheat.c m_random.c
  console.c command.c
  p_saveg.c
  w_wad.c w_zip.c dehacked.c
  d_netcmd.c d_clisrv.c d_net.c d_netfil.c i_tcp.c
  d_items.c d_main.c)

add_executable(doomlegacy ${SRC})
target_link_libraries(doomlegacy
  media_sdl
  hardware_renderer
  ${LIBS})
