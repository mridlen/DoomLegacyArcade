/*
    software-look -- snap every pixel to the game's own 256 colour palette,
    for Doom Legacy Arcade.  OpenGL's smooth lighting then bands the way the
    software renderer's colormaps do.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    Palette is a 512x512 lookup built by ogl_shader.c from PLAYPAL: 64 levels
    per channel, laid out as an 8x8 grid of 64x64 red/green tiles, one tile
    per blue level, each texel holding the nearest palette colour.
*/

#if defined(VERTEX)
attribute vec4 VertexCoord;
attribute vec4 TexCoord;
varying vec2 uv;
uniform mat4 MVPMatrix;
void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    uv = TexCoord.xy;
}

#elif defined(FRAGMENT)
uniform sampler2D Texture;
uniform sampler2D Palette;
varying vec2 uv;

void main()
{
    vec3 q = floor(clamp(texture2D(Texture, uv).rgb, 0.0, 1.0) * 63.0 + 0.5);
    vec2 tile = vec2(mod(q.b, 8.0), floor(q.b / 8.0));
    vec2 p = (tile * 64.0 + q.rg + 0.5) / 512.0;
    gl_FragColor = vec4(texture2D(Palette, p).rgb, 1.0);
}
#endif
