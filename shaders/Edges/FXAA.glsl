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
    float maxSM = max(lumaS, rgbyM.w);
    float minSM = min(lumaS, rgbyM.w);
    float maxESM = max(lumaE, maxSM);
    float minESM = min(lumaE, minSM);
    float maxWN = max(lumaN, lumaW);
    float minWN = min(lumaN, lumaW);
    float rangeMax = max(maxWN, maxESM);
    float rangeMin = min(minWN, minESM);
    float rangeMaxScaled = rangeMax * pEdgeThresh;
    float range = rangeMax - rangeMin;
    float rangeMaxClamped = max(pEdgeMin, rangeMaxScaled);
    bool earlyExit = range < rangeMaxClamped;
    if(earlyExit) { return vec4(rgbyM.rgb, 1.0); }
    float lumaNW = textureLodOffset(gips_tex, posM, 0.0, ivec2(-1,-1)).w;
    float lumaSE = textureLodOffset(gips_tex, posM, 0.0, ivec2( 1, 1)).w;
    float lumaNE = textureLodOffset(gips_tex, posM, 0.0, ivec2( 1,-1)).w;
    float lumaSW = textureLodOffset(gips_tex, posM, 0.0, ivec2(-1, 1)).w;
    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float subpixRcpRange = 1.0/range;
    float subpixNSWE = lumaNS + lumaWE;
    float edgeHorz1 = (-2.0 * rgbyM.w) + lumaNS;
    float edgeVert1 = (-2.0 * rgbyM.w) + lumaWE;
    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float edgeHorz2 = (-2.0 * lumaE) + lumaNESE;
    float edgeVert2 = (-2.0 * lumaN) + lumaNWNE;
    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;
    float edgeHorz4 = (abs(edgeHorz1) * 2.0) + abs(edgeHorz2);
    float edgeVert4 = (abs(edgeVert1) * 2.0) + abs(edgeVert2);
    float edgeHorz3 = (-2.0 * lumaW) + lumaNWSW;
    float edgeVert3 = (-2.0 * lumaS) + lumaSWSE;
    float edgeHorz = abs(edgeHorz3) + edgeHorz4;
    float edgeVert = abs(edgeVert3) + edgeVert4;
    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float lengthSign = rcpFrame.x;
    bool horzSpan = edgeHorz >= edgeVert;
    float subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;
    if(!horzSpan) lumaN = lumaW;
    if(!horzSpan) lumaS = lumaE;
    if(horzSpan) lengthSign = rcpFrame.y;
    float subpixB = (subpixA * (1.0/12.0)) - rgbyM.w;
    float gradientN = lumaN - rgbyM.w;
    float gradientS = lumaS - rgbyM.w;
    float lumaNN = lumaN + rgbyM.w;
    float lumaSS = lumaS + rgbyM.w;
    bool pairN = abs(gradientN) >= abs(gradientS);
    float gradient = max(abs(gradientN), abs(gradientS));
    if(pairN) lengthSign = -lengthSign;
    float subpixC = clamp(abs(subpixB) * subpixRcpRange, 0.0, 1.0);
    vec2 posB = posM;
    vec2 offNP;
    offNP.x = (!horzSpan) ? 0.0 : rcpFrame.x;
    offNP.y = ( horzSpan) ? 0.0 : rcpFrame.y;
    if(!horzSpan) posB.x += lengthSign * 0.5;
    if( horzSpan) posB.y += lengthSign * 0.5;
    vec2 posN;
    posN.x = posB.x - offNP.x * 1.0;
    posN.y = posB.y - offNP.y * 1.0;
    vec2 posP;
    posP.x = posB.x + offNP.x * 1.0;
    posP.y = posB.y + offNP.y * 1.0;
    float subpixD = ((-2.0)*subpixC) + 3.0;
    float lumaEndN = textureLod(gips_tex, posN, 0.0).w;
    float subpixE = subpixC * subpixC;
    float lumaEndP = textureLod(gips_tex, posP, 0.0).w;
    if(!pairN) lumaNN = lumaSS;
    float gradientScaled = gradient * 1.0/4.0;
    float lumaMM = rgbyM.w - lumaNN * 0.5;
    float subpixF = subpixD * subpixE;
    bool lumaMLTZero = lumaMM < 0.0;
    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    if(!doneN) posN.x -= offNP.x * 1.0;
    if(!doneN) posN.y -= offNP.y * 1.0;
    bool doneNP = (!doneN) || (!doneP);
    if(!doneP) posP.x += offNP.x * 1.0;
    if(!doneP) posP.y += offNP.y * 1.0;
    if(doneNP) {
        if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
        if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if(!doneN) posN.x -= offNP.x * 1.0;
        if(!doneN) posN.y -= offNP.y * 1.0;
        doneNP = (!doneN) || (!doneP);
        if(!doneP) posP.x += offNP.x * 1.0;
        if(!doneP) posP.y += offNP.y * 1.0;
        if(doneNP) {
            if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
            if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;
            if(!doneN) posN.x -= offNP.x * 1.0;
            if(!doneN) posN.y -= offNP.y * 1.0;
            doneNP = (!doneN) || (!doneP);
            if(!doneP) posP.x += offNP.x * 1.0;
            if(!doneP) posP.y += offNP.y * 1.0;
            if(doneNP) {
                if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                doneN = abs(lumaEndN) >= gradientScaled;
                doneP = abs(lumaEndP) >= gradientScaled;
                if(!doneN) posN.x -= offNP.x * 1.0;
                if(!doneN) posN.y -= offNP.y * 1.0;
                doneNP = (!doneN) || (!doneP);
                if(!doneP) posP.x += offNP.x * 1.0;
                if(!doneP) posP.y += offNP.y * 1.0;
                if(doneNP) {
                    if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                    if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                    if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                    if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                    doneN = abs(lumaEndN) >= gradientScaled;
                    doneP = abs(lumaEndP) >= gradientScaled;
                    if(!doneN) posN.x -= offNP.x * 1.5;
                    if(!doneN) posN.y -= offNP.y * 1.5;
                    doneNP = (!doneN) || (!doneP);
                    if(!doneP) posP.x += offNP.x * 1.5;
                    if(!doneP) posP.y += offNP.y * 1.5;
                    if(doneNP) {
                        if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                        if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                        doneN = abs(lumaEndN) >= gradientScaled;
                        doneP = abs(lumaEndP) >= gradientScaled;
                        if(!doneN) posN.x -= offNP.x * 2.0;
                        if(!doneN) posN.y -= offNP.y * 2.0;
                        doneNP = (!doneN) || (!doneP);
                        if(!doneP) posP.x += offNP.x * 2.0;
                        if(!doneP) posP.y += offNP.y * 2.0;
                        if(doneNP) {
                            if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                            if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                            doneN = abs(lumaEndN) >= gradientScaled;
                            doneP = abs(lumaEndP) >= gradientScaled;
                            if(!doneN) posN.x -= offNP.x * 2.0;
                            if(!doneN) posN.y -= offNP.y * 2.0;
                            doneNP = (!doneN) || (!doneP);
                            if(!doneP) posP.x += offNP.x * 2.0;
                            if(!doneP) posP.y += offNP.y * 2.0;
                            if(doneNP) {
                                if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                                if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                                if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                                if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                                doneN = abs(lumaEndN) >= gradientScaled;
                                doneP = abs(lumaEndP) >= gradientScaled;
                                if(!doneN) posN.x -= offNP.x * 2.0;
                                if(!doneN) posN.y -= offNP.y * 2.0;
                                doneNP = (!doneN) || (!doneP);
                                if(!doneP) posP.x += offNP.x * 2.0;
                                if(!doneP) posP.y += offNP.y * 2.0;
                                if(doneNP) {
                                    if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                                    if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                                    if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                                    if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                                    doneN = abs(lumaEndN) >= gradientScaled;
                                    doneP = abs(lumaEndP) >= gradientScaled;
                                    if(!doneN) posN.x -= offNP.x * 2.0;
                                    if(!doneN) posN.y -= offNP.y * 2.0;
                                    doneNP = (!doneN) || (!doneP);
                                    if(!doneP) posP.x += offNP.x * 2.0;
                                    if(!doneP) posP.y += offNP.y * 2.0;
                                    if(doneNP) {
                                        if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                                        if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                                        if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                                        if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                                        doneN = abs(lumaEndN) >= gradientScaled;
                                        doneP = abs(lumaEndP) >= gradientScaled;
                                        if(!doneN) posN.x -= offNP.x * 4.0;
                                        if(!doneN) posN.y -= offNP.y * 4.0;
                                        doneNP = (!doneN) || (!doneP);
                                        if(!doneP) posP.x += offNP.x * 4.0;
                                        if(!doneP) posP.y += offNP.y * 4.0;
                                        if(doneNP) {
                                            if(!doneN) lumaEndN = textureLod(gips_tex, posN.xy, 0.0).w;
                                            if(!doneP) lumaEndP = textureLod(gips_tex, posP.xy, 0.0).w;
                                            if(!doneN) lumaEndN = lumaEndN - lumaNN * 0.5;
                                            if(!doneP) lumaEndP = lumaEndP - lumaNN * 0.5;
                                            doneN = abs(lumaEndN) >= gradientScaled;
                                            doneP = abs(lumaEndP) >= gradientScaled;
                                            if(!doneN) posN.x -= offNP.x * 8.0;
                                            if(!doneN) posN.y -= offNP.y * 8.0;
                                            doneNP = (!doneN) || (!doneP);
                                            if(!doneP) posP.x += offNP.x * 8.0;
                                            if(!doneP) posP.y += offNP.y * 8.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    float dstN = posM.x - posN.x;
    float dstP = posP.x - posM.x;
    if(!horzSpan) dstN = posM.y - posN.y;
    if(!horzSpan) dstP = posP.y - posM.y;
    bool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    float spanLength = (dstP + dstN);
    bool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    float spanLengthRcp = 1.0/spanLength;
    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float subpixG = subpixF * subpixF;
    float pixelOffset = (dst * (-spanLengthRcp)) + 0.5;
    float subpixH = subpixG * pSubpix;
    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    if(!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if( horzSpan) posM.y += pixelOffsetSubpix * lengthSign;
    return vec4(textureLod(gips_tex, posM, 0.0).xyz, 1.0);
}
