// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform vec3 key;          // @color color
uniform float tolerance  = 0.5;
uniform float smoothness = 0.0;
uniform float matchMode;   // @switch match hue only
uniform float invert;      // @switch
uniform float desaturate;  // @switch desaturate all other colors
uniform float mapAlpha;    // @switch map to alpha channel

vec4 run(vec4 inColor) {
    float res;
    if (matchMode < 0.5) {
        res = length(key - inColor.rgb);
    } else {
        res = dot(normalize(key), normalize(inColor.rgb));
        res = 1.0 - res * res * res;
    }
    res = 1.0 - smoothstep(tolerance-smoothness, tolerance+smoothness, res);
    res = abs(invert - res);

    vec4 outColor = inColor;
    if (desaturate > 0.5) {
        outColor.rgb = mix(vec3(dot(outColor.rgb, vec3(.299,.587,.114))), outColor.rgb, res);
    } else if (mapAlpha < 0.5) {
        outColor.rgb = vec3(res);
    }
    if (mapAlpha > 0.5) {
        outColor.a = res;
    }
    return outColor;
}
