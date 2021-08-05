// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=pixel @filter=on

uniform float radius = 0.0;   // @max=100
uniform float N = 7.0;        // @min=3 @max=23 @int sample count
uniform float passes = 3.0;   // @min=1 @max=4 @int pass count
uniform float decay = 0.707;  // pass decay
uniform float amin = 1.0;     // @switch control radius by alpha channel @on=0 @off=1

vec4 run_main(vec2 xy, float r, float phase) {
    float alpha = pixel(xy).a;
    r *= max(amin, alpha);
    vec3 accum = vec3(0.0);
    for (float i = phase;  i < N;  i += 1.0) {
        float a = i * (6.28318530717959 / N);
        accum += pixel(xy + r * vec2(cos(a), sin(a))).rgb;
    }
    return vec4(accum / N, alpha);
}

vec4 run_pass1(vec2 pos) {
    return run_main(pos, radius, 0.0);
}

vec4 run_pass2(vec2 pos) {
    return (passes > 1.5) ? run_main(pos, radius * decay, 0.618) : pixel(pos);
}

vec4 run_pass3(vec2 pos) {
    return (passes > 2.5) ? run_main(pos, radius * decay * decay, 0.236) : pixel(pos);
}

vec4 run_pass4(vec2 pos) {
    return (passes > 3.5) ? run_main(pos, radius * decay * decay * decay, 0.854) : pixel(pos);
}
