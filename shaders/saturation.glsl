// @gips_version=1

uniform float saturation = 1.0;  // @min=0 @max=5
uniform vec3 key = vec3(.299, .587, .114);  // grayscale downmix
uniform float invert;      // invert luminance   @toggle
uniform float sign = 1.0;  // invert chrominance @toggle @off=1 @on=-1

vec3 run(vec3 rgb) {
    float luma = dot(rgb, key / (key.r + key.g + key.b));
    vec3 chroma = rgb - vec3(luma);
    if (invert > 0.5) { luma = 1.0 - luma; }
    return vec3(luma) + chroma * saturation * sign;
}
