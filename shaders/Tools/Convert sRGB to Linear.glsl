// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @format=16

float srgb2linear(float v) {
    return (v <= 0.04045) ? (v / 12.92)
                          : pow((v + 0.055) / 1.055, 2.4);
}

vec3 run(vec3 color) {
    return vec3(srgb2linear(color.r), srgb2linear(color.g), srgb2linear(color.b));
}
