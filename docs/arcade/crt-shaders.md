# CRT shaders (OpenGL)

Read before touching `sdl/ogl_shader.c`, `sdl/crt/`, `sdl/ogl_crt_glsl.h`, `tools/glsl2c.py`,
`cv_grshader`, or `ogl_read_front_hook` in `r_opengl.c`.

## What it is

`gr_shader` (**OpenGL 3D Card Options → Shaders → CRT Shader**) runs one of four libretro CRT
shaders over the finished frame, just before the buffer swap in `OglSdl_FinishUpdate`. Everything
drawn that frame (3D view, HUD, menus, console, wipe) goes through it once. Default `Off`: a new
cvar's default is what every cabinet runs until a `-devmode` session saves otherwise.

The shaders are taken verbatim from libretro `glsl-shaders/crt/shaders/` and live in `sdl/crt/`,
with a README naming author and licence. `tools/glsl2c.py` turns them into C strings in
`sdl/ogl_crt_glsl.h`, so the binary needs no files beside it. Re-run it after editing a `.glsl`.
Its list order is the cvar's value order and must match `grshader_cons_t` in `hw_main.c`.
crt-easymode and crt-aperture were left out because their headers say "GPL" with no version.

## The pass (`OGL_Shader_Present`)

1. `glCopyTexSubImage2D` of the back buffer into `src_tex` (screen size, NPOT, which GL 2.0 allows).
2. Box filter down by a whole-number factor, `vid.height / 200` clamped to 1..10, into `low_tex`
   through an FBO. The shaders treat **each input texel as one line of the tube**. Given the
   full-size frame they would draw one scanline per monitor row, which is invisible, so the
   downsample is what makes the lines game-sized. The factor is whole so every line covers the
   same number of rows; a fractional one beats against the pixel grid. The box shader is generated
   per factor with every tap unrolled, since the Pi's GPU cannot run loops, and uses bilinear taps
   to read two pixels per axis each.
   Without FBOs the shader samples `src_tex` directly but is still *told* the small size, so the
   line count is the same; only the averaging is lost.
3. The CRT shader, full screen, `TextureSize = InputSize = low size`, `OutputSize = screen`,
   identity `MVPMatrix`, vertices in clip space.

All GL state is bracketed with `glPushAttrib(GL_ALL_ATTRIB_BITS)` / `glPushClientAttrib`, because
`SetBlend` caches state in `cur_polyflags` (see `screen-wipe.md`). The program and FBO binding are
not attribute state and are reset by hand (`UseProgram(0)`, `BindFramebuffer(0)`). Conventional
client arrays are disabled inside the bracket because they alias generic attribute 0.

## libretro conventions

- One file, compiled twice with `#define VERTEX` / `#define FRAGMENT`, prefixed `#version 120`
  (GL 2.1, what the Pi's vc4 driver offers). The shaders choose `attribute`/`varying`/`texture2D`
  themselves below 1.30.
- Attributes are bound by name before linking: `VertexCoord` 0, `TexCoord` 1, `COLOR` 2.
- **`PARAMETER_UNIFORM` must be defined.** The other mode, where each parameter falls back to a
  `#define`, is not usable: crt-geom has `#define lum 0.0` *and* a local variable called `lum`,
  which fails to compile. RetroArch always uses uniforms, so that path is never exercised
  upstream. Each `#pragma parameter NAME "label" default min max step` line is parsed and its
  default set once after linking (`crt_set_parameters`); uniform values belong to the program.
- `filter_linear` from each shader's RetroArch preset is the `linear` column in `glsl2c.py`.

## The wipe

The wipe captures its outgoing frame with `ReadScreenRect(..., from_front)`. With a shader on,
the front buffer holds the *filtered* picture, which would then be filtered a second time during
the melt and jump at the end. `ogl_read_front_hook` (`r_opengl.c`) lets the shader hand over
`src_tex`, the unfiltered copy of the frame on screen, via `glGetTexImage`.
`crt_frame_saved` is cleared on any frame the shader did not run, so the hook falls back to
reading the front buffer.

## Re-entry

A shader build failure is reported with `GenPrintf`, and the console can redraw and call
`I_FinishUpdate` from inside that print, which re-enters `OGL_Shader_Present` before the failure
is recorded. This happened on the first run of crt-geom: unbounded recursion and a segfault. Build
state is now settled *before* building (`crt_prog_state = 2`, `box_factor`), and a `busy` flag
guards the builds.

## Contexts

`OglSdl_SetMode` destroys and recreates the GL context on every mode change, so
`OGL_Shader_Context_Lost` forgets every GL name (not deleted: they died with the context). Programs
rebuild lazily on the next frame.

## Status

`gr_shader_status` (`hw_main.c`): 0 ok or off, 1 the driver has no GLSL or lacks an entry point,
2 the selected shader failed to build. The Shaders page shows 1 and 2 in red. Details go to the
log as `CRT shader ...` lines.

## Verified

All four compile, link and run on the laptop's Radeon (Mesa, GLSL 4.60) under the offscreen driver,
and the game exits cleanly with each. The look, the frame rate cost and the Pi's vc4 driver have
not been checked; the Pi is normally run in software mode anyway, where there is no shader.

## Texture names: never glGenTextures

The first build called `glGenTextures` for its two textures and was unusable on the cabinet: the
title screen came up upside down and cropped ("DOOW"), and starting a game froze. **The renderer
does not allocate its texture names through GL.** `r_opengl.c` counts up from `no_texture_id`
(`next_texture_id++`) and binds whatever number comes next, so a name GL handed out was soon
bound by the renderer for a patch, and its upload replaced the frame copy with that patch. The
shader then drew the patch over the whole screen. The names are now fixed constants far above
anything the counter reaches (`CRT_TEX_SRC`, `CRT_TEX_LOW`); the compatibility profile creates a
texture on first bind. The same applies to any future code that owns a GL texture here.

It passed the first round of checks because those only confirmed the shaders compiled and the game
exited. Nothing looked at the picture. It was reproduced and verified under Xvfb at 1366x768 with
the cabinet's own config, grabbing the X screen with `import -window root`. The game's own
screenshot is taken before the shader runs, so it cannot show this.
