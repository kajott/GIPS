// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel

uniform float logScale = 4.5;     // @max=10 scale (exponential)
uniform float smoothness = 1.0;   // @min=0 @max=10
uniform vec4  angles = vec4(15., 75., 0., 45.);   // @min=0 @max=90 @int screen angles
uniform float cmykEnabled = 1.0;  // @switch CMYK mode

float halftone(vec2 pos, float value, float angle, float scale) {
    // pre-process value to match the halftoning coverage function
    // see https://twitter.com/KeyJ_trbl/status/1375964040195870720
    float x = value * 4.6598 - 3.6598;
    float r = 0.5642 * sqrt(value) + ((x <= 0.0) ? 0.0 : 0.1429 * (1.0 - sqrt(1 - x*x)));

    // actual halftoning
    float c = cos(angle), s = sin(angle);
    pos = mat2(c, -s, s, c) * pos * scale;
    float d = length(fract(pos) - 0.5);
    float border = smoothness * length(dFdx(pos));
    return smoothstep(0, border, r * (1.0 + border) - d);
}

vec4 run(vec2 pos) {
    vec4 color = pixel(pos);
    vec3 rgb = pow(color.rgb, vec3(2.2));  // assuming sRGB-ish gamma here!
    float scale = exp(logScale);
    vec4 rad = radians(angles);
    if (cmykEnabled > 0.5) {
        float k = 1.0 - max(max(rgb.r, rgb.g), rgb.b);
        vec4 cmyk = clamp(vec4(1.0 - rgb / (1.0 - k), k), 0.0, 1.0);
        cmyk = vec4(
            halftone(pos, cmyk.r, rad.r, scale),
            halftone(pos, cmyk.g, rad.g, scale),
            halftone(pos, cmyk.b, rad.b, scale),
            halftone(pos, cmyk.a, rad.a, scale)
        );
        rgb = (1.0 - cmyk.rgb) * (1.0 - cmyk.a);
    } else {
        rgb = vec3(
            halftone(pos, rgb.r, rad.r, scale),
            halftone(pos, rgb.g, rad.g, scale),
            halftone(pos, rgb.b, rad.b, scale)
        );
    }
    return vec4(pow(rgb, vec3(1.0/2.2)), color.a);
}
