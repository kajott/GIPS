// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=none @filter=off

vec3 med3rgb(vec3 a, vec3 b, vec3 c) {
    return max(min(a, b), min(max(a, b), c));
}

vec4 run(vec2 pos) {
    vec4 center = textureLod(gips_tex, pos, 0.0);
    return vec4(med3rgb(med3rgb(textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,-1)).rgb,
                                textureLodOffset(gips_tex, pos, 0.0, ivec2( 0,-1)).rgb,
                                textureLodOffset(gips_tex, pos, 0.0, ivec2( 1,-1)).rgb),
                        med3rgb(textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 0)).rgb,
                                center.rgb,
                                textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 0)).rgb),
                        med3rgb(textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 1)).rgb,
                                textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, 1)).rgb,
                                textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 1)).rgb)), center.a);
    
}
