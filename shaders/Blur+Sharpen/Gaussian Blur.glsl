// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=pixel @filter=off

// not a "true" Gaussian blur -- using a cheap (finite) approximation

uniform float sigma = 0.3;   // @min=0.3 @max=50 @digits=2
uniform float aspect;        // @min=-1.5 @max=1.5
uniform float amin = 1.0;    // @switch control sigma by alpha channel @on=0 @off=1

vec4 run_main(vec2 pos, vec2 dir, float radius) {
    vec4 color = pixel(pos);
    float wsum = 0.5;
    radius *= max(amin, color.a);
    for (float dist = 1.0;  dist < radius;  dist += 1.0) {
        float w = 1.0 - dist / radius;
        w = 1.0 - w * w;
        w = 1.0 - w * w;
        color.rgb += w * (pixel(pos - dist * dir).rgb + pixel(pos + dist * dir).rgb);
        wsum += w;
    }
    return vec4(color.rgb / (wsum * 2.0), color.a);
}

vec4 run_pass1(vec2 pos) {
    return run_main(pos, vec2(1.0, 0.0), sigma * 2.6412 * exp(aspect));
}

vec4 run_pass2(vec2 pos) {
    return run_main(pos, vec2(0.0, 1.0), sigma * 2.6412 * exp(-aspect));
}
