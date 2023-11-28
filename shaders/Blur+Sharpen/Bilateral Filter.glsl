// SPDX-FileCopyrightText: 2023 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=pixel @filter=off

uniform float kernelSize   =  7.0;  // @min=1 @max=33 @digits=0 kernel size
uniform float spatialSigma =  1.0;  // @min=0 @max=10 spatial sigma
uniform float logIntSigma  = -5.0;  // @min=-5 @max=0 log intensity sigma

vec4 run(vec2 pos) {
    // precompute constants, fetch center pixel
    float kernelRadius = floor((kernelSize - 1.0) * 0.5);
    float spatialCoeff = (-0.5) / max(spatialSigma * spatialSigma, 1E-6);
    float intSigma = exp(logIntSigma * 2.302585092994046);
    float intCoeff = (-0.5) / max(intSigma * intSigma, 1E-6);
    vec4 c = pixel(pos);

    // main accumulation loop
    vec2 d;
    vec4 psum = vec4(0.0), wsum = vec4(0.0);
    for (d.y = -kernelRadius;  d.y <= kernelRadius;  d.y += 1.0) {
        for (d.x = -kernelRadius;  d.x <= kernelRadius;  d.x += 1.0) {
            vec4 p = pixel(pos + d);  // fetch pixel
            vec4 dCP = c - p;         // compute difference to center pixel
            vec4 w = vec4(exp(spatialCoeff * dot(d, d)))  // spatial weight
                   * exp(vec4(intCoeff) * (dCP * dCP));   // intensity weight
            psum += p * w;  wsum += w;  // accumulate
        }
    }
    return psum / wsum;
}
