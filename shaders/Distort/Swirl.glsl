// @gips_version=1 @coord=rel

uniform float strength;          // @min=-5 @max=5 @digits=2 linear strength
uniform float frequency = 20.0;  // @min=1 @max=100 @digits=1
uniform float amplitude;         // @max=1.5 @digits=3
uniform float phase;             // @angle
uniform vec2  center;            // @min=-2 @max=2

vec4 run(vec2 pos) {
    pos -= center;
    float d = length(pos);
    float a = d * strength + amplitude * sin(d * frequency + phase);
    float c = cos(a), s = sin(a);
    return pixel(mat2(c, -s, s, c) * pos + center);
}
