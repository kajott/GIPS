// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel

uniform float strength;      // @min=-1 @max=1
uniform float radius = 1.0;  // @min=0.01 @max=2
uniform vec2  center;        // @min=-2 @max=2

vec4 run(vec2 pos) {
    pos -= center;
    float oldDist = length(pos);
    float d = oldDist / radius;
    float u = 1.0 - strength;
    if (d < 1.0) {
        d = d * (u + d * (2.0 - 2.0 * u + d * (u - 1.0)));
    }
    d *= radius;
    pos *= d / oldDist;
    pos += center;
    return pixel(pos);
}
