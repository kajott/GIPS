// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform vec3 color0 = vec3(0.0, 0.0, 0.0);  // @color lower color
uniform vec3 color1 = vec3(1.0, 1.0, 1.0);  // @color upper color
uniform vec3 gamma  = vec3(1.0, 1.0, 1.0);  // @min=.3 @max=3

vec3 run(vec3 c) {
    return mix(color0, color1, pow(c, gamma));
}
