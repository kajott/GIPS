// @gips_version=1 @coord=rel

uniform float amplitude;         // @min=0 @max=0.2
uniform float frequency = 50.0;  // @min=0 @max=200
uniform float phase;             // @angle
uniform vec2  center;            // @min=-2 @max=2

vec4 run(vec2 pos) {
    vec2 tp = pos - center;
    float d = length(tp);
    vec2 n = tp / d;
    d += amplitude * sin(frequency * d + phase);
    return pixel(n * d + center);
}
