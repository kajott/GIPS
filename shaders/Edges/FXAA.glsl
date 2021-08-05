// SPDX-FileCopyrightText: 2009-2011 Timothy Lottes
// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=none @filter=on

// This is Timothy Lottes' public-domain FXAA algorithm,
// maximally stripped down to the following configuration:
// - FXAA_PC = 1
// - FXAA_GLSL_130 = 1
// - FXAA_GATHER4_ALPHA = 0
// - FXAA_QUALITY__PRESET = 39 ("no dither, very expensive")

// Note that this filter destroys the alpha channel!

uniform float pSubpix = 0.75;       // subpixel quality
uniform float pEdgeThresh = 0.166;  // edge threshold @min=0 @max=0.5
uniform float pEdgeMin = 0.0833;    // minimum brightness @max=0.1 @digits=4

vec4 run_pass1(vec3 rgb) {
    // pre-processing: create luma information in alpha channel
    return vec4(rgb, dot(rgb, vec3(0.25, 0.5, 0.25)));
}

vec4 run_pass2(vec2 posM) {
    vec2 rcpFrame = vec2(1.0) / gips_image_size;
    vec4 rgbyM = textureLod(gips_tex, posM, 0.0);
    float lumaS = textureLodOffset(gips_tex, posM, 0.0, ivec2( 0, 1)).w;
    float lumaE = textureLodOffset(gips_tex, posM, 0.0, ivec2( 1, 0)).w;
    float lumaN = textureLodOffset(gips_tex, posM, 0.0, ivec2( 0,-1)).w;
    float lumaW = textureLodOffset(gips_tex, posM, 0.0, ivec2(-1, 0)).w;
    float rangeMax = max(max(lumaN, lumaW), max(lumaE, max(lumaS, rgbyM.w)));
    float rangeMin = min(min(lumaN, lumaW), min(lumaE, min(lumaS, rgbyM.w)));
    float range = rangeMax - rangeMin;
    if (range < max(pEdgeMin, rangeMax * pEdgeThresh)) { return vec4(rgbyM.rgb, 1.0); }
    float lumaNW = textureLodOffset(gips_tex, posM, 0.0, ivec2(-1,-1)).w;
    float lumaSE = textureLodOffset(gips_tex, posM, 0.0, ivec2( 1, 1)).w;
    float lumaNE = textureLodOffset(gips_tex, posM, 0.0, ivec2( 1,-1)).w;
    float lumaSW = textureLodOffset(gips_tex, posM, 0.0, ivec2(-1, 1)).w;
    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;
    float subpixA = (lumaNS + lumaWE) * 2.0 + (lumaNWSW + lumaNESE);
    subpixA = clamp(abs((subpixA * (1.0/12.0)) - rgbyM.w) / range, 0.0, 1.0);
    subpixA = (3.0 - 2.0 * subpixA) * (subpixA * subpixA);
    float edgeHorz = abs(lumaNWSW - 2.0 * lumaW) + 2.0 * abs(lumaNS - 2.0 * rgbyM.w) + abs(lumaNESE - 2.0 * lumaE);
    float edgeVert = abs(lumaSWSE - 2.0 * lumaS) + 2.0 * abs(lumaWE - 2.0 * rgbyM.w) + abs(lumaNWNE - 2.0 * lumaN);
    bool horzSpan = edgeHorz >= edgeVert;
    float lengthSign = horzSpan ? rcpFrame.y : rcpFrame.x;
    if (!horzSpan) { lumaN = lumaW; lumaS = lumaE; }
    float gradientN = abs(lumaN - rgbyM.w);
    float gradientS = abs(lumaS - rgbyM.w);
    float gradient = max(gradientN, gradientS) * 0.25;
    bool pairN = (gradientN >= gradientS);
    if (pairN) { lengthSign = -lengthSign; }
    float lumaNN = 0.5 * ((pairN ? lumaN : lumaS) + rgbyM.w);
    vec2 offNP = horzSpan ? vec2(rcpFrame.x, 0.0) : vec2(0.0, rcpFrame.y);
    vec2 posB = posM;
    if (horzSpan) { posB.y += lengthSign * 0.5; }
             else { posB.x += lengthSign * 0.5; }
    vec2 posN = posB - offNP;
    vec2 posP = posB + offNP;
    bool doneN = false, doneP = false;
    float lumaEndN, lumaEndP;
    const float stepSizes[12] = float[](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);
    for (int step = 0;  step < 12;  ++step) {
        if (!doneN) { lumaEndN = textureLod(gips_tex, posN, 0.0).w - lumaNN; }
        if (!doneP) { lumaEndP = textureLod(gips_tex, posP, 0.0).w - lumaNN; }
        doneN = abs(lumaEndN) >= gradient;
        doneP = abs(lumaEndP) >= gradient;
        if (!doneN) { posN -= offNP * stepSizes[step]; }
        if (!doneP) { posP += offNP * stepSizes[step]; }
        if (doneN && doneP) { break; }
    }
    float dstN = horzSpan ? (posM.x - posN.x) : (posM.y - posN.y);
    float dstP = horzSpan ? (posP.x - posM.x) : (posP.y - posN.y);
    bool goodSpan = (((dstN < dstP) ? lumaEndN : lumaEndP) < 0.0) != ((rgbyM.w - lumaNN) < 0.0);
    float pixelOffset = 0.5 - min(dstN, dstP) / (dstN + dstP);
    float pixelOffsetSubpix = max(goodSpan ? pixelOffset : 0.0, (subpixA * subpixA) * pSubpix);
    if (horzSpan) { posM.y += pixelOffsetSubpix * lengthSign; }
             else { posM.x += pixelOffsetSubpix * lengthSign; }
    return vec4(textureLod(gips_tex, posM, 0.0).xyz, 1.0);
}
