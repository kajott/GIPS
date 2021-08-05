// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1

uniform float levels = 64;   // @min=2 @max=64 @int
uniform float dither;        // @switch enable dithering
uniform float randomness;    // dither randomness
uniform float gammaCorrect;  // @switch gamma-correct dithering
uniform float gamma = 2.2;   // @min=.5 @max=5

const float bayer3[64] = float[64](
0.000000, 0.500000, 0.125000, 0.625000, 0.031250, 0.531250, 0.156250, 0.656250,
0.750000, 0.250000, 0.875000, 0.375000, 0.781250, 0.281250, 0.906250, 0.406250,
0.187500, 0.687500, 0.062500, 0.562500, 0.218750, 0.718750, 0.093750, 0.593750,
0.937500, 0.437500, 0.812500, 0.312500, 0.968750, 0.468750, 0.843750, 0.343750,
0.046875, 0.546875, 0.171875, 0.671875, 0.015625, 0.515625, 0.140625, 0.640625,
0.796875, 0.296875, 0.921875, 0.421875, 0.765625, 0.265625, 0.890625, 0.390625,
0.234375, 0.734375, 0.109375, 0.609375, 0.203125, 0.703125, 0.078125, 0.578125,
0.984375, 0.484375, 0.859375, 0.359375, 0.953125, 0.453125, 0.828125, 0.328125);

vec3 run(vec3 rgb) {
    // compute rounding adjustment (i.e., dither)
    float radj = 0.5;
    if (dither > 0.5) {
        uvec2 pos = uvec2(gl_FragCoord.xy);
        radj = bayer3[((pos.y & 7u) << 3u) | (pos.x & 7u)];
        if (randomness > 0.0) {
            // PRNG function adapted from https://www.shadertoy.com/view/XlXcW4
            const uint k = 1103515245u;
            uvec3 x = uvec3(floatBitsToUint(gl_FragCoord.xy), 0x13375EEDu);
            x = ((x >> 8u) ^ x.yzx) * k;
            x = ((x >> 8u) ^ x.yzx) * k;
            radj = mod(radj + (uintBitsToFloat((x.r & 0x007FFFFFu) | 0x3F800000u) - 1.0) * randomness, 1.0);
        }
    }

    float limit = levels - 1.0;
    if (gammaCorrect < 0.5) {
        return floor(min(rgb * limit, limit) + radj) / limit;
    } else {
        vec3 g3 = vec3(gamma);
        vec3 loG = floor(min(rgb * limit, limit)) / limit;
        vec3 hiG = loG + 1.0 / limit;
        vec3 loL = pow(loG, g3);
        vec3 hiL = pow(hiG, g3);
        rgb = (pow(rgb, g3) - loL) / (hiL - loL);
        return mix(loG, hiG, greaterThan(rgb, vec3(radj)));
    }
}
