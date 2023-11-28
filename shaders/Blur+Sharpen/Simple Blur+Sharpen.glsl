// SPDX-FileCopyrightText: 2021-2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=none @filter=off

uniform float blurriness;       // @min=-5 @max=1
uniform float threshold = 1.0;  // @digits=3
uniform float ringing   = 1.0;  // ringing allowed

vec4 run(vec2 pos) {
    vec3 p00 = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,-1)).rgb;
    vec3 p10 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 0,-1)).rgb;
    vec3 p20 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 1,-1)).rgb;
    vec3 p01 = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 0)).rgb;
    vec4 p11 = pixel(pos);
    vec3 p21 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 0)).rgb;
    vec3 p02 = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 1)).rgb;
    vec3 p12 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, 1)).rgb;
    vec3 p22 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 1)).rgb;
    vec3 res = 0.25 * p11.rgb + 0.125 * (p01 + p10 + p12 + p21) + 0.0625 * (p00 + p02 + p20 + p22);
    res = mix(p11.rgb, res, blurriness);
    res = mix(p11.rgb, res, lessThan(abs(res - p11.rgb), vec3(threshold)));
    if ((ringing < 1.0) && (blurriness < 0.0)) {
        vec3 vmin = min(min(min(min(p00, p22), min(p20, p02)), min(min(p01, p10), min(p21, p12))), p11.rgb);
        vec3 vmax = max(max(max(max(p00, p22), max(p20, p02)), max(max(p01, p10), max(p21, p12))), p11.rgb);
        res = mix(min(max(res, vmin), vmax), res, ringing);
    }
    return vec4(res, p11.a);
}
