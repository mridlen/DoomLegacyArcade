// [Arcade] Post-process shaders for the OpenGL drawmode.
// See docs/arcade/shaders.md.

#ifndef OGL_SHADER_H
#define OGL_SHADER_H

// Run the selected shader (cv_grshader) over the finished back buffer.
// Call immediately before the buffer swap.
void OGL_Shader_Present( void );

// Put the unfiltered frame back into the back buffer.
// Call immediately after the buffer swap.
void OGL_Shader_After_Swap( void );

// The GL context was replaced: every GL object this module held is gone.
void OGL_Shader_Context_Lost( void );

#endif
