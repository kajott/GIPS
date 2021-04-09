// @gips_version=1 @coord=rel @filter=on

uniform float scale = 2.0;  // @min=0 @max=8 scale (logarithmic)
uniform float aspect;       // @min=-1 @max=1
uniform float strength;     // @max=5
uniform float hard;         // @switch hard pixels
uniform float angle;        // @angle @max=90
uniform vec2 phase;         // @min=-1 @max=1

vec4 run(vec2 pos) {
    // pre-scale position, split into integer + fractional part
    float s = sin(angle), c = cos(angle);
    pos *= mat2(c,s,-s,c);
    vec2 scxy = vec2(exp(scale + aspect), exp(scale - aspect));
    pos = (pos * scxy) + phase;
    vec2 base = floor(pos);
    pos -= base;

    // apply cubification transform (repeated smoothstep or step)
    if (hard > 0.5) {
        pos = step(0.5, pos);
    } else {
        float m = strength;
        while (m > 1.0) {
            pos = smoothstep(0.0, 1.0, pos);
            m -= 1.0;
        }
        pos = mix(pos, smoothstep(0.0, 1.0, pos), m);
    }

    // re-assemble position
    pos = (pos + base - phase) / scxy;
    pos *= mat2(c,-s,s,c);
    return pixel(pos);
}
