// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel @filter=off

uniform float radius = 1.0;      // @max=5
uniform float aspect;            // @min=-2 @max=2 aspect ratio
uniform float smoothness;
uniform vec2 center;             // @min=-1 @max=1
uniform float angle;             // @angle @max=180
uniform vec4  c1 = vec4(0.0, 0.0, 0.0, 1.0);  //@color color 1
uniform vec4  c2 = vec4(1.0, 1.0, 1.0, 1.0);  //@color color 2
uniform float alphaOnly;         // @switch set alpha channel only

vec4 run(vec2 pos) {
    vec4 bg = pixel(pos);
    float c = cos(angle), s = sin(angle);
    float d = length(mat2(c,-s,s, c) * (pos - center) * vec2(exp(-aspect), exp(aspect))) - radius;
    float t = smoothness * radius + 0.5 * fwidth(d);
    d = smoothstep(-t, t, d);
    if (alphaOnly > 0.5) {
        return vec4(bg.rgb, mix(c1.a, c2.a, d));
    } else {
        vec3 m1 = mix(bg.rgb, c1.rgb, c1.a);
        vec3 m2 = mix(bg.rgb, c2.rgb, c2.a);
        return vec4(mix(m1, m2, d), bg.a);
    }
}
