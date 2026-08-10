# Toolchain file for Windows cross-compilation on Linux using CMake.
# Usage: cd build; cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain_win32.cmake ../trunk

#INCLUDE(CMakeForceCompiler)

# this one is important
SET(CMAKE_SYSTEM_NAME Windows)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
set(GNU_HOST "i686-w64-mingw32")

#CMAKE_FORCE_C_COMPILER(${GNU_HOST}-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER(${GNU_HOST}-g++ GNU)
SET(CMAKE_C_COMPILER ${GNU_HOST}-gcc)
SET(CMAKE_RC_COMPILER ${GNU_HOST}-windres)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH "/usr/${GNU_HOST}")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
