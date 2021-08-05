// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=pixel @filter=off

uniform float angle;        // @angle
uniform float scale = 1.0;  // @max=5 amplification
uniform float absolute;     // @switch
uniform float alpha;        // @switch process alpha

vec4 run(vec2 pos) {
    vec2 dir = vec2(sin(angle), cos(angle));
    vec4 acc = vec4(0.0);
    vec2 delta;
    for (delta.y = -1.0;  delta.y <= 1.0;  delta.y += 1.0) {
        for (delta.x = -1.0;  delta.x <= 1.0;  delta.x += 1.0) {
            acc += pixel(pos + delta) * (dot(dir, delta) / max(length(delta), 1.0));
        }
    }
    acc *= scale;
    if (absolute < 0.5) { acc += vec4(0.5); } else { acc = abs(acc); }
    if (alpha < 0.5) { acc.a = pixel(pos).a; }
    return acc;
}
