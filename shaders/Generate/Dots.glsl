// @gips_version=1 @coords=rel @filter=off

uniform float scale      = 1.5;  // @min=-1 @max=10 scale (logarithmic)
uniform float angle;             // @angle
uniform vec2  phase;             // @min=-1 @max=1
uniform float size       = 0.4;  // dot size (relative)
uniform float smoothness = 0.0;  // @min=-2 @max=7 smoothness (logarithmic)
uniform vec4  c1 = vec4(0.0, 0.0, 0.0, 1.0);  //@color color 1
uniform vec4  c2 = vec4(1.0, 1.0, 1.0, 1.0);  //@color color 2
uniform float alphaOnly;         // @switch set alpha channel only

float dotDist(vec2 pos) {
    return length(mod(pos, vec2(1.0, 1.732)) - vec2(0.5, 0.866));
}

vec4 run(vec2 pos) {
    vec4 bg = pixel(pos);
    float dotScale = exp(scale);
    float borderScale = exp(smoothness) * dotScale / min(gips_image_size.x, gips_image_size.y);
    float c = cos(angle), s = sin(angle);
    pos = pos * dotScale - phase;
    pos = mat2(c, -s, s, c) * pos;
    float d = min(dotDist(pos), dotDist(pos + vec2(0.5, 0.866)));
    d = smoothstep(-borderScale, borderScale, size - d);
    if (alphaOnly > 0.5) {
        return vec4(bg.rgb, mix(c1.a, c2.a, d));
    } else {
        vec3 m1 = mix(bg.rgb, c1.rgb, c1.a);
        vec3 m2 = mix(bg.rgb, c2.rgb, c2.a);
        return vec4(mix(m1, m2, d), bg.a);
    }
}
