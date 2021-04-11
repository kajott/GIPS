// @gips_version=1 @coord=rel @filter=off

uniform float frequency = 150.0;  // @min=0.1 @max=1000 @digits=1
uniform float contrast  = 1.0;    // @max=10 @digits=2
uniform float toAlpha;            // @switch apply to alpha channel

vec4 run(vec2 pos) {
    float v = 0.5 + 0.5 * contrast * cos(dot(pos, pos) * frequency);
    if (toAlpha > 0.5) {
        return vec4(pixel(pos).rgb, v);
    } else {
        return vec4(vec3(v), 1.0);
    }
}
