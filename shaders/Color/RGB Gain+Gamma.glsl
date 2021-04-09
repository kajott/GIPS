// @gips_version=1

uniform vec3 midpoint = vec3(0.0, 0.0, 0.0);
uniform vec3 gain     = vec3(1.0, 1.0, 1.0);  // @min=0 @max=5
uniform vec3 gamma    = vec3(1.0, 1.0, 1.0);  // @min=0.2 @max=5.0

vec3 run(vec3 rgb) {
    rgb = (rgb - midpoint) * gain;
    bvec3 sign = lessThan(rgb, vec3(0.0));
    rgb = pow(abs(rgb), 1.0 / gamma);
    return mix(rgb, -rgb, sign) + midpoint;
}
