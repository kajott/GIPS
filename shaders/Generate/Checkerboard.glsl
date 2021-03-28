// @gips_version=1 @coord=rel

uniform float size   = 0.1;  // @min=0.01 @digits=2
uniform float aspect = 0.0;  // @min=-1 @max=1
uniform vec2  phase;         // @min=-1 @max=1
uniform vec4  c1 = vec4(0.0, 0.0, 0.0, 1.0);  //@color color 1
uniform vec4  c2 = vec4(1.0, 1.0, 1.0, 1.0);  //@color color 2
uniform float alphaOnly;     // @switch set alpha channel only

vec4 run(vec2 pos) {
    float m = (mod(pos.x / size * exp(-aspect) - phase.x, 2.0) - 1.0)
            * (mod(pos.y / size * exp( aspect) - phase.y, 2.0) - 1.0);
    vec4 fg = (m < 0.0) ? c1 : c2;
    vec4 bg = pixel(pos);
    if (alphaOnly > 0.5) {
        return vec4(bg.rgb, fg.a);
    } else {
        return vec4(mix(bg.rgb, fg.rgb, fg.a), bg.a);
    }
}
