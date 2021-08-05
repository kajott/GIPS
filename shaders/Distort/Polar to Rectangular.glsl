// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel @filter=on

uniform vec2 center;         // @min=-2 @max=2
uniform float angle;         // @angle rotation
uniform float radius = 1.0;  // @min=0 @max=3
uniform float distortion;    // @min=-1 @max=1

vec4 run(vec2 pos) {
    pos.x *= min(1.0, gips_image_size.y / gips_image_size.x);
    pos.y *= min(1.0, gips_image_size.x / gips_image_size.y);
    pos = (pos * 0.5) + 0.5;
    float a = (pos.x - 0.25) * 6.28318530717959 + angle;
    float d = radius * pow(1.0 - pos.y, exp(-distortion));
    return pixel(center + d * vec2(cos(a), sin(a)));
}
