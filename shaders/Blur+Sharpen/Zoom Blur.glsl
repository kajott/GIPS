// @gips_version=1 @coord=rel @filter=on
// not a "true" Gaussian blur -- using a cheap (finite) approximation

uniform vec2 center;           // @min=-2 @max=2
uniform float strength;        // strength (exponential)
uniform float samples = 10.0;  // @min=1 @max=100 @int sample count (quality)
uniform float box;             // @switch box blur (instead of Gaussian)

vec4 run(vec2 pos) {
    vec4 color = vec4(0.0);
    float wsum = 0.0;
    pos -= center;
    for (float i = -samples;  i <= samples;  i += 1.0) {
        float dist = i / samples;
        float w = 1.0 - dist;
        w = 1.0 - w * w;
        w = 1.0 - w * w;
        w = max(w, box);

        color += w * pixel(pos * exp(strength * dist) + center);
        wsum += w;
    }
    return color / wsum;
}
