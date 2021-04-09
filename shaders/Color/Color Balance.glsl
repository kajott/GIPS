// @gips_version=1

uniform float balR;         // @min=-1 @max=1 cyan<->red
uniform float balG;         // @min=-1 @max=1 magenta<->green
uniform float balB;         // @min=-1 @max=1 yellow<->blue
uniform float keepLuma;     // @switch        preserve luminance
uniform float gamma = 2.2;  // @min=.2 @max=5 working gamma

vec3 run(vec3 rgb) {
    rgb = pow(rgb, vec3(gamma));
    float luma = dot(rgb, vec3(.25, .5, .25));
    rgb.r *= 1.0 + balR;
    rgb.g *= 1.0 + balG;
    rgb.b *= 1.0 + balB;
    if (keepLuma > 0.5) {
        rgb *= luma / dot(rgb, vec3(.25, .5, .25));
    }
    return pow(rgb, vec3(1.0 / gamma));
}
