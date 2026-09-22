/*
    colour -- whole-screen colour looks for Doom Legacy Arcade: greyscale,
    sepia and night vision.  One file, compiled once per look with LOOK
    defined by ogl_shader.c (1 greyscale, 2 sepia, 3 night vision).

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    Runs on the full-size frame.  Time is in seconds, from ogl_shader.c.
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
uniform vec2 OutputSize;
uniform float Time;
varying vec2 uv;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec3 c = texture2D(Texture, uv).rgb;
    float l = dot(c, vec3(0.299, 0.587, 0.114));

#if LOOK == 1
    // Greyscale.
    c = vec3(l);

#elif LOOK == 2
    // Sepia: the usual print-toning matrix, eased back a little so dark
    // corridors do not turn to mud.
    vec3 s = vec3(dot(c, vec3(0.393, 0.769, 0.189)),
                  dot(c, vec3(0.349, 0.686, 0.168)),
                  dot(c, vec3(0.272, 0.534, 0.131)));
    c = mix(vec3(l), s, 0.85);

#elif LOOK == 3
    // Night vision: amplified brightness in phosphor green, sensor noise,
    // a faint line structure and a round eyepiece.
    float tic = floor(Time * 30.0);
    float amp = pow(l, 0.6) * 1.35;
    amp += (hash(floor(uv * OutputSize * 0.5) + vec2(tic, tic * 1.7)) - 0.5) * 0.18;
    amp *= 0.9 + 0.1 * sin(uv.y * OutputSize.y * 1.2);
    vec2 d = (uv - 0.5) * vec2(OutputSize.x / OutputSize.y, 1.0);
    amp *= smoothstep(1.1, 0.7, length(d));  // wide enough to keep the HUD readable
    c = clamp(amp, 0.0, 1.2) * vec3(0.35, 1.0, 0.35);
#endif

    gl_FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
#endif
