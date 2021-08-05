// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=none @filter=off

vec3 med3rgb(vec3 a, vec3 b, vec3 c) {
    return max(min(a, b), min(max(a, b), c));
}

vec4 run(vec2 pos) {
    vec3 p00 = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,-1)).rgb;
    vec3 p10 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 0,-1)).rgb;
    vec3 p20 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 1,-1)).rgb;
    vec3 p01 = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 0)).rgb;
    vec4 p11 = textureLod      (gips_tex, pos, 0.0);
    vec3 p21 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 0)).rgb;
    vec3 p02 = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 1)).rgb;
    vec3 p12 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, 1)).rgb;
    vec3 p22 = textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 1)).rgb;
    return vec4(med3rgb(p11.rgb, med3rgb(p11.rgb, med3rgb(p11.rgb, p01, p21),
                                                  med3rgb(p11.rgb, p10, p12)),
                                 med3rgb(p11.rgb, med3rgb(p11.rgb, p00, p22),
                                                  med3rgb(p11.rgb, p20, p02))), p11.a);
}
