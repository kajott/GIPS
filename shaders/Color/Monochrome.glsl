// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform vec3 downmix = vec3(.299, .587, .114);  // @min=-1 @max=2 RGB weights
uniform float normWeights = 1.0;                // @switch normalize weights
uniform vec3 tint = vec3(1.0, 1.0, 1.0);        // @color

vec3 run(vec3 rgb) {
    vec3 w = downmix;
    if (normWeights > 0.5) { w /= w.x + w.y + w.z; }
    return pow(vec3(clamp(dot(rgb, w), 0.0, 1.0)), 2.0 - tint);
}
