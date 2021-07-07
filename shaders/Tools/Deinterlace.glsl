// @gips_version=1 @coord=none @filter=off

uniform float field;      // @min=-1 @max=1 top <-> bottom field
uniform float tolerance;
uniform float yuvCorr;    // @switch YUV 4:2:0 interlacing correction (BT.709 only)

const vec3 lumaWeight = vec3(0.299, 0.587, 0.114);

const mat3 rgb2yuv = mat3(0.2126, -0.1146,  0.5,
                          0.7152, -0.3854, -0.4542,
                          0.0722,  0.5,    -0.0458);
const mat3 yuv2rgb = mat3(1.0, 1.0, 1.0,
                          0.0, -0.1873, 1.8556,
                          1.5748, -0.4681, 0.0);

vec4 run_pass1(vec2 pos) {
    vec4 lumaPixel = textureLod(gips_tex, pos, 0.0);
    if (yuvCorr < 0.5) { return lumaPixel; }
    uint y = uint(gl_FragCoord.y) & 3u;
    if ((y == 0u) || (y == 3u)) { return lumaPixel; }
    vec4 chromaPixel = (y == 1u) ? textureLodOffset(gips_tex, pos, 0.0, ivec2(0,-2))
                                 : textureLodOffset(gips_tex, pos, 0.0, ivec2(0,+2));
    return vec4(yuv2rgb * vec3((rgb2yuv * lumaPixel.rgb).r, (rgb2yuv * chromaPixel.rgb).gb), lumaPixel.a);
}

vec4 run_pass2(vec2 pos) {
    vec4 c = textureLod(gips_tex, pos, 0.0);
    uint y = uint(gl_FragCoord.y) & 1u;
    if (((y == 0u) && (field <= 0.0)) || (y != 0u) && (field >= 0.0)) { return c; }

    vec4 a = textureLodOffset(gips_tex, pos, 0.0, ivec2(0,-1));
    vec4 b = textureLodOffset(gips_tex, pos, 0.0, ivec2(0,+1));
    float La = dot(lumaWeight, a.rgb);
    float Lb = dot(lumaWeight, b.rgb);
    float Lc = dot(lumaWeight, c.rgb);
    float badness = max(0.0, max(min(La, Lb) - Lc, Lc - max(La, Lb)));
    return mix(c, 0.5 * (a + b), smoothstep(0.0, tolerance, badness) * abs(field));
}
