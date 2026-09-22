/*
    vhs -- a worn videotape, for Doom Legacy Arcade.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    What a VHS picture does, roughly in the order it is done here:
    - the lines wobble sideways a little, and a noisy tracking band drifts
      up the screen, tearing the lines it crosses;
    - the bottom few lines are torn sideways (head switching);
    - brightness keeps about 3 MHz of detail, colour far less, and the colour
      lands a little to the right of the picture it belongs to;
    - colour is washed out, blacks are lifted, and there is snow.

    Runs on the full-size frame and measures everything as a fraction of the
    screen, so it looks the same at any resolution.  Needs linear sampling.
    Time is in seconds, from ogl_shader.c.
*/

#pragma parameter chromaSmear  "Colour smear"          1.0  0.0 3.0 0.1
#pragma parameter wobble       "Line wobble"           1.0  0.0 3.0 0.1
#pragma parameter tracking     "Tracking band"         1.0  0.0 1.0 1.0
#pragma parameter snow         "Snow"                  0.05 0.0 0.3 0.01
#pragma parameter saturation   "Saturation"            0.8  0.0 1.5 0.05

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
uniform float Time;
uniform float chromaSmear;
uniform float wobble;
uniform float tracking;
uniform float snow;
uniform float saturation;
varying vec2 uv;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 rgb2yiq(vec3 c)
{
    return vec3(dot(c, vec3(0.299,  0.587,  0.114)),
                dot(c, vec3(0.596, -0.274, -0.322)),
                dot(c, vec3(0.211, -0.523,  0.312)));
}

vec3 yiq2rgb(vec3 y)
{
    return vec3(y.x + 0.956 * y.y + 0.621 * y.z,
                y.x - 0.272 * y.y - 0.647 * y.z,
                y.x - 1.106 * y.y + 1.703 * y.z);
}

void main()
{
    // uv.y is 0 at the bottom of the screen.
    float line = floor(uv.y * 240.0);
    float tic = floor(Time * 30.0);
    vec2 st = uv;

    // Wobble: a slow wave down the screen, plus a little per-line jitter.
    float shift = sin(uv.y * 25.0 + Time * 1.7) * 0.0007
                + (hash(vec2(line, tic)) - 0.5) * 0.0010;
    shift *= wobble;

    // Tracking band, drifting up and wrapping every 14 seconds.
    float band = 0.0;
    if( tracking > 0.5 )
    {
        float d = uv.y - fract(Time / 14.0) * 1.2 + 0.1;
        band = smoothstep(0.035, 0.0, abs(d));
        shift += band * (hash(vec2(line, tic + 7.0)) - 0.5) * 0.03;
    }

    // Head switching: the bottom lines tear to the right.
    float head = smoothstep(0.018, 0.0, uv.y);
    shift += head * (0.01 + 0.01 * hash(vec2(line, tic + 3.0)));

    st.x += shift;

    // Brightness: about 1/300 of the width.  Colour: about 1/60, shifted right.
    float lw = 1.0 / 300.0;
    float cw = chromaSmear / 60.0;
    vec3 c0 = texture2D(Texture, st + vec2(-lw, 0.0)).rgb;
    vec3 c1 = texture2D(Texture, st).rgb;
    vec3 c2 = texture2D(Texture, st + vec2( lw, 0.0)).rgb;
    float y = rgb2yiq(c0 * 0.25 + c1 * 0.5 + c2 * 0.25).x;

    vec2 iq = vec2(0.0);
    float cs = -0.35 * cw;  // colour trails behind
    iq += rgb2yiq(texture2D(Texture, st + vec2(cs - cw,        0.0)).rgb).yz;
    iq += rgb2yiq(texture2D(Texture, st + vec2(cs - cw * 0.5,  0.0)).rgb).yz;
    iq += rgb2yiq(texture2D(Texture, st + vec2(cs,             0.0)).rgb).yz;
    iq += rgb2yiq(texture2D(Texture, st + vec2(cs + cw * 0.5,  0.0)).rgb).yz;
    iq += rgb2yiq(texture2D(Texture, st + vec2(cs + cw,        0.0)).rgb).yz;
    iq *= 0.2 * saturation;

    vec3 rgb = yiq2rgb(vec3(y, iq));

    // Snow, heavier in the tracking band and the torn lines.
    float n = hash(floor(uv * vec2(640.0, 480.0)) + vec2(tic * 1.3, tic * 0.7)) - 0.5;
    rgb += n * (snow + band * 0.5 + head * 0.3);
    // A few bright streaks in the band.
    rgb += band * step(0.985, hash(vec2(floor(uv.x * 160.0), line + tic))) * 0.6;

    // Lifted blacks, softer whites.
    rgb = rgb * 0.9 + 0.04;
    gl_FragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
#endif
