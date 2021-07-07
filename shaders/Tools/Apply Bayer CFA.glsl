// @gips_version=1 @coord=pixel @filter=off

uniform float pattern;  // @int @max=3 RGGB / BGGR / GBRG / GRBG
uniform float mono;     // @switch monochrome output

uint getPattern(vec2 pos) {  // 0=R 1=B 2=Gr 3=Gb
    uvec2 ipos = uvec2(pos) & uvec2(1u);
    uint p = ipos.x | (ipos.y << 1);
    p = (p ^ (p << 1)) & 3u;
    return p ^ uint(pattern);
}

const vec3 weights[4] = vec3[4](
    vec3(1.,0.,0.),  // R
    vec3(0.,0.,1.),  // B
    vec3(0.,1.,0.),  // G(r)
    vec3(0.,1.,0.)   // G(b)
);

vec4 run(vec2 pos) {
    vec4 color = pixel(pos);
    color.rgb *= weights[getPattern(pos)];
    if (mono > 0.5) {
        color.rgb = vec3(dot(color.rgb, vec3(1.,1.,1.)));
    }
    return color;
}
