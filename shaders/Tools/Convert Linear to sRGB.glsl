// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

float linear2srgb(float v) {
    return (v <= 0.0031308) ? (v * 12.92)
                            : (1.055 * pow(v, 1.0 / 2.4) - 0.055);
}

vec3 run(vec3 color) {
    return vec3(linear2srgb(color.r), linear2srgb(color.g), linear2srgb(color.b));
}
