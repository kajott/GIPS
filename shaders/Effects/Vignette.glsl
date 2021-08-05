// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel

uniform float strength;
uniform float size = 1.0;   // @min=0.01 @max=2
uniform float power = 2.0;  // falloff power @min=1 @max=10
uniform float sign = -1.0;  // inverse (correct vignetting) @toggle @on=1 @off=-1

vec4 run(vec2 pos) {
    vec4 color = pixel(pos);
    float d = length(pos);
    float f = max(0.0, 1.0 + sign * strength * pow(d / size, power));
    return vec4(f * color.rgb, color.a);
}
