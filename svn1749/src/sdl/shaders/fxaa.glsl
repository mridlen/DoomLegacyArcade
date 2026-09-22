/*
    fxaa -- fast approximate anti-aliasing, for Doom Legacy Arcade.
    Written for this project from Timothy Lottes' published FXAA method
    (the short "console" form): find the edge direction from the luma of the
    four diagonal neighbours, then blur along it.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    Runs on the full-size frame.  Needs linear sampling.
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

#define REDUCE_MIN  (1.0/128.0)
#define REDUCE_MUL  (1.0/8.0)
#define SPAN_MAX    8.0

void main()
{
    vec2 px = 1.0 / TextureSize;
    vec3 luma = vec3(0.299, 0.587, 0.114);
    vec3 rgbNW = texture2D(Texture, uv + vec2(-1.0, -1.0) * px).rgb;
    vec3 rgbNE = texture2D(Texture, uv + vec2( 1.0, -1.0) * px).rgb;
    vec3 rgbSW = texture2D(Texture, uv + vec2(-1.0,  1.0) * px).rgb;
    vec3 rgbSE = texture2D(Texture, uv + vec2( 1.0,  1.0) * px).rgb;
    vec3 rgbM  = texture2D(Texture, uv).rgb;
    float lNW = dot(rgbNW, luma);
    float lNE = dot(rgbNE, luma);
    float lSW = dot(rgbSW, luma);
    float lSE = dot(rgbSE, luma);
    float lM  = dot(rgbM,  luma);
    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));

    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y =  ((lNW + lSW) - (lNE + lSE));
    float dirReduce = max((lNW + lNE + lSW + lSE) * (0.25 * REDUCE_MUL), REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-SPAN_MAX), vec2(SPAN_MAX)) * px;

    vec3 rgbA = 0.5 * (texture2D(Texture, uv + dir * (1.0/3.0 - 0.5)).rgb
                     + texture2D(Texture, uv + dir * (2.0/3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture2D(Texture, uv - dir * 0.5).rgb
                                   + texture2D(Texture, uv + dir * 0.5).rgb);
    float lB = dot(rgbB, luma);
    gl_FragColor = vec4((lB < lMin || lB > lMax) ? rgbA : rgbB, 1.0);
}
#endif
