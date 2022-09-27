// SPDX-FileCopyrightText: 2022 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform vec3  key = vec3(1.0, 0.8, 0.0);  // @color blindness axis
uniform float keepLuma = 1.0;             // @switch        preserve luminance
uniform float gamma = 2.2;                // @min=.2 @max=5 working gamma

vec3 run(vec3 rgb) {
    rgb = pow(rgb, vec3(gamma));
    float luma = dot(rgb, vec3(.25, .5, .25));
    rgb -= luma;
    float kluma = dot(pow(key, vec3(gamma)), vec3(.25, .5, .25));
    vec3 kaxis = normalize(key - kluma);
    rgb = kaxis * dot(rgb, kaxis);
    rgb += luma;
    if (keepLuma > 0.5) {
        rgb *= luma / dot(rgb, vec3(.25, .5, .25));
    }
    return pow(rgb, vec3(1.0 / gamma));
}
