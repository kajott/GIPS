// @gips_version=1 @coord=pixel @filter=on
// not a "true" Gaussian blur -- using a cheap (finite) approximation

uniform float sigma = 0.3;   // @min=0.3 @max=50 @digits=2
uniform float angle;         // @angle @max=180
uniform float box;           // @switch box blur (instead of Gaussian)
uniform float amin = 1.0;    // @switch control sigma by alpha channel @on=0 @off=1

vec4 run(vec2 pos) {
    vec4 color = pixel(pos);
    float wsum = 0.5;
    float radius = max(amin, color.a) * sigma * 2.6412;
    vec2 dir = vec2(cos(angle), sin(angle));
    for (float dist = 1.0;  dist < radius;  dist += 1.0) {
        float w = 1.0 - dist / radius;
        w = 1.0 - w * w;
        w = 1.0 - w * w;
        w = max(w, box);
        color.rgb += w * (pixel(pos - dist * dir).rgb + pixel(pos + dist * dir).rgb);
        wsum += w;
    }
    return vec4(color.rgb / (wsum * 2.0), color.a);
}
