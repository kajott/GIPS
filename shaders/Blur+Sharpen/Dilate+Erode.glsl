// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=pixel @filter=off

uniform float size = 3.0;  // @min=1 @max=10 @int
uniform float mixval;      // dilate<->erode

vec4 run_main(vec2 pos, vec2 dir) {
    vec4 center = pixel(pos);
    vec3 vmin = center.rgb;
    vec3 vmax = center.rgb;
    float rend = ceil(size * 0.5);
    for (float dist = -floor(size * 0.5);  dist <= rend;  dist += 1.0) {
        vec3 sample = pixel(pos + dist * dir).rgb;
        vmin = min(vmin, sample);
        vmax = max(vmax, sample);
    }
    return vec4(mix(vmin, vmax, mixval), center.a);
}

vec4 run_pass1(vec2 pos) {
    return run_main(pos, vec2(1.0, 0.0));
}

vec4 run_pass2(vec2 pos) {
    return run_main(pos, vec2(0.0, 1.0));
}
