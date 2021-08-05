// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform vec3 thresh = vec3(0.5, 0.5, 0.5);  // @color threshold color

vec3 run(vec3 color) {
    return mix(vec3(0.0), vec3(1.0), greaterThan(color, thresh));
}
