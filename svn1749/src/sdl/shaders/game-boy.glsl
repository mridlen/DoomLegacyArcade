/*
    game-boy -- the original Game Boy's four shades of green, for Doom Legacy
    Arcade, with an ordered dither so gradients survive the four levels.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    Runs on the downsampled frame (about 200 lines, see ogl_shader.c), drawn
    with nearest sampling so each game-sized pixel is a solid block, and the
    dither works on those blocks rather than on screen pixels.
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
uniform vec2 TextureSize;
varying vec2 uv;

void main()
{
    vec2 cell = floor(uv * TextureSize);
    vec3 c = texture2D(Texture, (cell + 0.5) / TextureSize).rgb;
    // Brightened well up: Doom is dark, and the darkest shade is nearly black,
    // so without this most of a level lands on it.
    float l = clamp(pow(dot(c, vec3(0.299, 0.587, 0.114)), 0.6) * 1.25, 0.0, 1.0);

    // 2x2 Bayer threshold, spread over one step of the four levels.
    vec2 m = mod(cell, 2.0);
    float bayer = (m.x * 2.0 + m.y * 3.0 - 4.0 * m.x * m.y);  // 0 2 3 1
    float level = floor(clamp(l * 3.0 + (bayer + 0.5) / 4.0 - 0.5, 0.0, 3.0));

    vec3 shade = vec3(0.059, 0.220, 0.059);                      // #0f380f
    if( level > 0.5 )  shade = vec3(0.188, 0.384, 0.188);        // #306230
    if( level > 1.5 )  shade = vec3(0.545, 0.675, 0.059);        // #8bac0f
    if( level > 2.5 )  shade = vec3(0.608, 0.737, 0.059);        // #9bbc0f
    gl_FragColor = vec4(shade, 1.0);
}
#endif
