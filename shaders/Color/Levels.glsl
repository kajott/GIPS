// @gips_version=1

uniform vec2 inRange = vec2(0.0, 1.0);   // input min/max
uniform float gamma = 1.0;               // @min=0.2 @max=5.0
uniform vec2 outRange = vec2(0.0, 1.0);  // output min/max
uniform float lumaOnly;                  // @switch luminance only

vec3 run(vec3 rgb) {
    if (lumaOnly < 0.5) {
        // RGB mode
        rgb = (rgb - inRange.x) / (inRange.y - inRange.x);
        rgb = pow(clamp(rgb, 0.0, 1.0), vec3(1.0 / gamma));
        return rgb * (outRange.y - outRange.x) + outRange.x;
    } else {
        // luma mode
        float luma = dot(rgb, vec3(.25, .5, .25));
        vec3 chroma = rgb - vec3(luma);
        luma = (luma - inRange.x) / (inRange.y - inRange.x);
        luma = pow(clamp(luma, 0.0, 1.0), 1.0 / gamma);
        luma = luma * (outRange.y - outRange.x) + outRange.x;
        return vec3(luma) + chroma;
    }
}
