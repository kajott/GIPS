// @gips_version=1

uniform float strength;        // @max=5 strength (logarithmic)
uniform float gaussian = 1.0;  // @switch Gaussian luma noise instead of uniform RGB noise

vec4 run(vec4 color) {
    // PRNG function adapted from https://www.shadertoy.com/view/XlXcW4
    const uint k = 1103515245u;
    uvec3 x = uvec3(floatBitsToUint(gl_FragCoord.xy), 0x13375EEDu);
    x = ((x >> 8u) ^ x.yzx) * k;
    x = ((x >> 8u) ^ x.yzx) * k;
    x = ((x >> 8u) ^ x.yzx) * k;
    vec3 noise = uintBitsToFloat((x & 0x007FFFFFu) | 0x3F800000u) - 1.5;
    if (gaussian > 0.5) {
        noise = vec3(dot(noise, vec3(1.0)) / 3.0);
    }
    return vec4(color.rgb + noise * (exp2(strength) - 1.0), color.a);
}
