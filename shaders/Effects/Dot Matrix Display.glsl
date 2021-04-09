// @gips_version=1 @coords=rel @filter=on

uniform float logScale = 4.0;   // @min=0 @max=10  scale (logarithmic)
uniform float gap      = 0.01;  // @min=0 @max=0.2 gap size
uniform float blur     = 0.5;   // @min=0 @max=5   gap blur
uniform float ledMode;          // @switch         RGB subpixel mode
uniform float transparent;      // @switch

float genMask(vec2 pos, vec2 dscale) {
    float r = fwidth(pos.x * dscale.x) * blur;
    pos = fract(pos) * dscale;
    pos = min(pos, vec2(1.0) - pos) / dscale;
    return smoothstep(-r, r, min(pos.x, pos.y) - gap);
}

vec4 run(vec2 pos) {
    float scale = exp(logScale);
    pos *= scale;

    vec4 color = pixel((floor(pos) + vec2(0.5)) / scale);

    vec4 mask;
    if (ledMode < 0.5) {
        mask.rgb = vec3(genMask(pos, vec2(1.0)));
    } else {
        mask.rgb = vec3(genMask(pos + vec2(0.0/3.0, 0.0), vec2(3.0, 1.0)),
                        genMask(pos + vec2(1.0/3.0, 0.0), vec2(3.0, 1.0)),
                        genMask(pos + vec2(2.0/3.0, 0.0), vec2(3.0, 1.0)));
    }
    mask.a = max(max(mask.r, mask.g), mask.b);

    if (transparent < 0.5) {
        return vec4(color.rgb * mask.rgb, mix(1.0, color.a, mask.a));
    } else {
        return vec4(color.rgb * (mask.rgb / mask.a), color.a * mask.a);
    }
}
