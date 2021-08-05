// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

vec4 run(vec4 c) {
    return vec4(c.rgb, 1.0 - c.a);
}
