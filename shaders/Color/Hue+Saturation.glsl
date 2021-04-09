// @gips_version=1

uniform float hue;               // @angle
uniform float saturation = 1.0;  // @min=0 @max=5
uniform float invert;      // invert luminance   @toggle
uniform float sign = 1.0;  // invert chrominance @toggle @off=1 @on=-1

vec3 run(vec3 rgb) {
    float luma = dot(rgb, vec3(.299, .587, .114));
    vec3 chroma = rgb - vec3(luma);

    // rotate chrominance vector around (1,1,1) axis
    float s = sqrt(1.0/3.0) * sin(hue), c = cos(hue), b = (1.0/3.0) * (1.0 - c);
    chroma = mat3(b+c, b-s, b+s,
                  b+s, b+c, b-s,
                  b-s, b+s, b+c) * chroma;

    if (invert > 0.5) { luma = 1.0 - luma; }
    return vec3(luma) + chroma * saturation * sign;
}
