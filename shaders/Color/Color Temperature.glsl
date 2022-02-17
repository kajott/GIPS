// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// computes a Standard Bradford Transform
// (see: https://onlinelibrary.wiley.com/doi/pdf/10.1002/9781119021780.app3)
//
// This shader performs quite a bit of arithmetic; let's hope that the
// compiler is smart enough to pre-multiply some of the constant matrices,
// and pre-compute the results of the cct2xyz() function that only depends
// on uniform inputs.
//
// NOTE: sRGB color primaries are assumed.

// @gips_version=1

uniform float ct_s  = 6500.0;  // @min=2000 @max=10000 source color temperature @unit=K
uniform float ct_t  = 6500.0;  // @min=2000 @max=10000 target color temperature @unit=K
uniform float gamma = 2.2;     // @min=.5 @max=10 working gamma

const mat3 rgb2xyz = mat3(+0.4124, +0.2126, +0.0193,
                          +0.3576, +0.7152, +0.1192,
                          +0.1805, +0.0722, +0.9505);
const mat3 xyz2rgb = mat3(+3.2406, -0.9689, +0.0557,
                          -1.5372, +1.8758, -0.2040,
                          -0.4986, +0.0415, +1.0570);
const mat3 xyz2lms = mat3(+0.8951, -0.7502, +0.0389,
                          +0.2664, +1.7135, -0.0685,
                          -0.1614, +0.0367, +1.0296);
const mat3 lms2xyz = mat3(+0.9867, +0.4323, -0.0085,
                          -0.1471, +0.5184, +0.0400,
                          +0.1600, +0.0493, +0.9685);

vec3 cct2xyz(float k) {
    // polynomial approximation of the Planckian Locus
    // (see: https://en.wikipedia.org/wiki/Planckian_locus#Approximation)
    float t = 1.0 / k;
    float x = (k <= 4000.0) ? (0.179910 + t * (0.8776956e3 + t * (-0.2343589e6 + t * (-0.2661239e9))))
                            : (0.240390 + t * (0.2226347e3 + t * (+2.1070379e6 + t * (-3.0258469e9))));
    float y = (k <= 2222.0) ? (-0.20219683 + x * (2.18555832 + x * (-1.34811020 + x * (-1.1063814))))
            : (k <= 4000.0) ? (-0.16748867 + x * (2.09137015 + x * (-1.37418593 + x * (-0.9549476))))
                            : (-0.37001483 + x * (3.75112997 + x * (-5.87338670 + x * (+3.0817580))));
    return vec3(x, y, 1.0 - x - y);
}

vec3 run(vec3 rgb_in) {
    vec3 wr = xyz2lms * cct2xyz(ct_s);
    vec3 wt = xyz2lms * cct2xyz(ct_t);

    vec3 lms = xyz2lms * rgb2xyz * pow(rgb_in, vec3(gamma));

    lms.rg *= wt.rg / wr.rg;
    lms.b = wt.b * pow(lms.b / wr.b, pow(wr.b / wt.b, 0.0834));

    return pow(xyz2rgb * lms2xyz * lms, vec3(1.0 / gamma));
}
