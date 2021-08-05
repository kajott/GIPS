// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=rel @filter=on

uniform float distance = 3.0;         // @min=1.5 @max=10
uniform float curvature = 0.01;       // @min=0.01 @max=0.7
uniform float bevelSize = 0.02;       // bevel width
uniform float vignette;               // @max=10 depth vignetting
uniform vec4 bg = vec4(0.,0.,0.,1.);  // @color background color

uniform float gridEn;                 // @switch enable shadow mask
uniform float gridScale = 4.5;        // @min=0 @max=10 grid size
uniform float dotSize = 0.4;          // @min=0 @max=0.5 dot size
uniform float dotSmooth = 0.1;        // @min=0.001 @max=0.5 dot smoothness
uniform float gamma = 2.8;            // @min=0.1 @max=10 working gamma
uniform float gain = 1.0;             // @max=10 exposure gain

float shadowMask(vec2 rpos) {
    return smoothstep(dotSize + dotSmooth, dotSize - dotSmooth,
                      length(mod(rpos, vec2(3.0, 1.732)) - vec2(1.5, 0.866)));
}

vec4 run(vec2 rel) {
    // set target point (scaled such that the overall size keeps approximately
    // constant) and ray direction
    vec2 target = rel * distance * sin(curvature) / (distance - 1.0);
    vec3 dir = normalize(vec3(target, distance));

    // compute ray-sphere intersection point
    float t = dir.z * distance;
    float u = t * t - distance * distance + 1.0;
    if (u < 0.0) { return bg; }
    t -= sqrt(u);

    // compute screen coordinate, sample pixel
    vec2 hit = asin(dir.xy * t) / curvature;
    vec4 fg = pixel(hit);

    // apply bevel
    vec2 bevelDist = smoothstep(0.0, bevelSize, vec2(
        max(1.0, gips_image_size.x / gips_image_size.y),
        max(1.0, gips_image_size.y / gips_image_size.x)
    ) - abs(hit));
    fg.a *= bevelDist.x * bevelDist.y;

    // apply depth vignetting
    if (vignette > 0.0) {
        fg.rgb *= 1.0 - (1.0 - distance + dir.z * t) * vignette;
    }

    // generate and apply shadow mask
    if (gridEn > 0.5) {
        vec2 maskPos = hit * exp(gridScale);
        vec3 maskCol = vec3(
            shadowMask(maskPos)                  + shadowMask(maskPos + vec2(1.5, 0.866)),
            shadowMask(maskPos + vec2(1.0, 0.0)) + shadowMask(maskPos + vec2(2.5, 0.866)),
            shadowMask(maskPos + vec2(2.0, 0.0)) + shadowMask(maskPos + vec2(3.5, 0.866))
        );
        fg.rgb = pow(pow(fg.rgb, vec3(gamma)) * gain * maskCol, vec3(1.0 / gamma));
    }

    // compose final color
    float bga = bg.a * (1.0 - fg.a);
    fg.rgb = fg.rgb * fg.a + bg.rgb * bga;
    fg.a += bga;
    return vec4(fg.rgb / fg.a, fg.a);
}
