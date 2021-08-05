// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel @filter=on

uniform vec2 center;         // @min=-2 @max=2
uniform float angle;         // @angle rotation
uniform float radius = 1.0;  // @min=0 @max=3
uniform float distortion;    // @min=-1 @max=1
uniform float overlap;       // @max=0.5

vec4 run(vec2 pos) {
    // main coordinate transform
    pos -= center;
    float d = 1.0 - pow(length(pos) / radius, exp(distortion));
    float a = fract(0.5 + (((abs(pos.x) > abs(pos.y)) ? (1.5707963267949 - atan(pos.x, pos.y)) : atan(pos.y, pos.x)) + 4.71238898038469 - angle) / 6.28318530717959);

    // overlap handling
    a = a * (1.0 - overlap) + 0.5 * overlap;
    vec4 color = textureLod(gips_tex, vec2(a, d), 0.0);
    if (a < overlap) {
        color = mix(color, textureLod(gips_tex, vec2(a + 1.0 - overlap, d), 0.0), smoothstep(0.0, 1.0, 1.0 - a / overlap));
    } else if (a > (1.0 - overlap)) {
        color = mix(color, textureLod(gips_tex, vec2(a - 1.0 + overlap, d), 0.0), smoothstep(0.0, 1.0, 1.0 - (1.0 - a) / overlap));
    }

    return color;
}
