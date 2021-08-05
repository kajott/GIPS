// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform vec3 mixR = vec3(1., 0., 0.);  // @min=-2 @max=2 red mix
uniform vec3 mixG = vec3(0., 1., 0.);  // @min=-2 @max=2 green mix
uniform vec3 mixB = vec3(0., 0., 1.);  // @min=-2 @max=2 blue mix
uniform vec3 offset;                   // @min=-2 @max=2 RGB offset

vec3 run(vec3 rgb) {
    rgb = vec3(dot(rgb, mixR), dot(rgb, mixG), dot(rgb, mixB));
    return rgb + offset;
}
