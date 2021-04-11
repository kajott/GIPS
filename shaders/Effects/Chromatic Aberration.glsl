// @gips_version=1 @coord=rel @filter=on

uniform float red;     // @min=-0.01 @max=0.01 red/cyan strength
uniform float blue;    // @min=-0.01 @max=0.01 blue/yellow strength
uniform float aspect;  // @min=-2 @max=2 R/B aspect ratio
uniform vec2 center;   // @min=-1 @max=1

vec4 sampleCA(vec2 pos, float strength) {
    vec2 dir = (pos - center) * vec2(exp(aspect), exp(-aspect));
    return pixel(pos + dir * strength);
}

vec4 run(vec2 pos) {
    vec4 g = pixel(pos);
    return vec4(sampleCA(pos, red).r, g.g, sampleCA(pos, blue).b, g.a);
}
