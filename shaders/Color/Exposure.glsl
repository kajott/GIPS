// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform float ev;           // @min=-5 @max=5 EV
uniform float gamma = 2.2;  // @min=.5 @max=10 working gamma
uniform float reinhard;     // @switch brighten with Reinhard tone compression
uniform float gamutMap;     // @switch preserve hue in clipped regions
uniform float clipMark;     // @switch mark clipped regions

vec3 run(vec3 rgb) {
    // forward gamma
    rgb = pow(rgb, vec3(gamma));

    // apply gain
    float gain = exp2(ev);
    rgb *= gain;

    // tone mapping
    if (reinhard > 0.5) {
        rgb = rgb / (rgb + 1.0);
        // post-scale so white stays white
        rgb *= (gain + 1.0) / gain;
    }

    // gamut mapping
    if (gamutMap > 0.5) {
        float luma = dot(rgb, vec3(0.299, 0.587, 0.114));
        rgb -= vec3(luma);
        float limit = max(1.0 - luma, 1.0e-6);
        float maxv = max(max(rgb.r, rgb.g), rgb.b);
        if (maxv > limit) {
            rgb *= limit / maxv;
        }
        rgb += vec3(luma);
    }

    // reverse gamma
    rgb = pow(rgb, vec3(1.0 / gamma));

    // mark clipped pixels
    if (clipMark > 0.5) {
        if (max(max(rgb.r, rgb.g), rgb.b) >= (254.0/255.0)) { return vec3(1.0, 0.0, 0.0); }
        if (min(min(rgb.r, rgb.g), rgb.b) <=   (1.0/255.0)) { return vec3(0.0, 0.0, 1.0); }
    }

    return rgb;
}
