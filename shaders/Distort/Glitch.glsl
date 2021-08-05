// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel @filter=off

uniform float gscale = 1.0;  // @min=0 @max=5 global scale (logarithmic)
uniform float oscale = 1.5;  // @min=0 @max=5 octave scale (logarithmic)
uniform float angle;         // @angle @max=180
uniform vec3 strength;       // strength (low/mid/high octave)
uniform vec3 threshold;      // threshold (low/mid/high octave)
uniform float seed;          // random seed

float rand(float key, uint seed2) {
    const uint k = 1103515245u;
    uvec3 x = uvec3(floatBitsToUint(floor(key)), seed2, floatBitsToUint(seed));
    x = ((x >> 8u) ^ x.yzx) * k;
    x = ((x >> 8u) ^ x.yzx) * k;
    return 2.0 * (uintBitsToFloat((x.r & 0x007FFFFFu) | 0x3F800000u) - 1.5);
}

vec4 run(vec2 pos) {
    float c = cos(angle), s = sin(angle);

    // get "octave" position
    vec3 oct = floor(dot(pos, vec2(-s,c)) * vec3(exp(gscale), exp(gscale + oscale), exp(gscale + 2.0 * oscale)));

    // get displacement noise
    vec3 noise = vec3(rand(oct.x, 0x13375EEDu), rand(oct.y, 0xDEADC0DEu), rand(oct.z, 0xCAFEF00Du));

    // extract sign
    bvec3 sign = lessThan(noise, vec3(0.0));
    noise = abs(noise);

    // apply threshold and re-apply sign
    noise = max(vec3(0.0), noise - threshold);
    noise = mix(noise, -noise, sign);

    // apply displacement
    return pixel(pos + vec2(c,s) * dot(noise, (exp2(strength) - 1.0) / (1.01 - threshold)));
}
