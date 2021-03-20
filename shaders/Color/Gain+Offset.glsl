// @gips_version=1

uniform float gain = 1.0;  // @min=0 @max=2
uniform float offset;      // @min=-1 @max=1

vec3 run(vec3 rgb) {
    return (rgb + vec3(offset)) * gain;
}
