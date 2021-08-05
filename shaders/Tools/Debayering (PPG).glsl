// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

// @gips_version=1 @coord=pixel @filter=off

// https://web.archive.org/web/20160923211135/https://sites.google.com/site/chklin/demosaic/

uniform float pattern;  // @int @max=3 RGGB / BGGR / GBRG / GRBG

uint getPattern(vec2 pos) {  // 0=R 1=B 2=Gr 3=Gb
    uvec2 ipos = uvec2(pos) & uvec2(1u);
    uint p = ipos.x | (ipos.y << 1);
    p = (p ^ (p << 1)) & 3u;
    return p ^ uint(pattern);
}

float getColor(const float dx, const float dy, const uint p) {
    vec3 c = pixel(gips_pos + vec2(dx, dy)).rgb;
    return ((p & 1u) != 0u) ? c.b : c.r;
}

float getGreen(const float dx, const float dy) {
    return pixel(gips_pos + vec2(dx, dy)).g;
}

vec2 getColorAndGreen(const float dx, const float dy, const uint p) {
    vec3 c = pixel(gips_pos + vec2(dx, dy)).rgb;
    return ((p & 1u) != 0u) ? c.bg : c.rg;
}

float hueTransit(const float p2_g, const vec2 p1, const vec2 p3) {
    return (((p1.g < p2_g) && (p2_g < p3.g)) || ((p1.g > p2_g) && (p2_g > p3.g)))
         ? (p1.x + (p3.x - p1.x) * (p2_g - p1.g) / (p3.g - p1.g))
         : (0.5 * (p1.x + p3.x) + 0.25 * (p2_g * 2. - p1.g - p3.g));
}

vec4 run_pass1(vec2 pos) {  // green interpolation
    vec4 color = pixel(pos);
    uint p = getPattern(pos);
    if ((p & 2u) != 0u) { return color; /* NOP at green pixel locations */ }
    float PC = ((p & 1u) != 0u) ? color.b : color.r;
    float PN = getColor( 0.,-2., p);
    float PS = getColor( 0.,+2., p);
    float PW = getColor(-2., 0., p);
    float PE = getColor(+2., 0., p);
    float GN = getGreen( 0.,-1.);
    float GS = getGreen( 0.,+1.);
    float GW = getGreen(-1., 0.);
    float GE = getGreen(+1., 0.);
    float dS = abs(GN - GS);
    float dN = abs(PC - PN) * 2. + dS;
          dS = abs(PC - PS) * 2. + dS;
    float dE = abs(GW - GE);
    float dW = abs(PC - PW) * 2. + dE;
          dE = abs(PC - PE) * 2. + dE;
         if ((dN <= dS) && (dN <= dW) && (dN <= dE)) { color.g = 0.25 * (GN * 3. + GS + PC - PN); }
    else if ((dS <= dN) && (dS <= dW) && (dS <= dE)) { color.g = 0.25 * (GS * 3. + GN + PC - PS); }
    else if ((dW <= dN) && (dW <= dS) && (dW <= dE)) { color.g = 0.25 * (GW * 3. + GE + PC - PW); }
    else                                             { color.g = 0.25 * (GE * 3. + GW + PC - PE); }
    return color;
}

vec4 run_pass2(vec2 pos) {  // R/B interpolation
    vec4 color = pixel(pos);
    uint p = getPattern(pos);
    if ((p & 2u) != 0u) {
        // at green location -> compute R and B
        vec2 hv = vec2(
            hueTransit(color.g,
                       getColorAndGreen(-1., 0., p^3u),
                       getColorAndGreen(+1., 0., p^3u)),
            hueTransit(color.g,
                       getColorAndGreen( 0.,-1., p^2u),
                       getColorAndGreen( 0.,+1., p^2u))
        );
        if ((p & 1u) != 0u) { color.rb = hv; }
        else                { color.br = hv; }
    } else {
        // at R/B location -> compute opposite R/B
        float qP = ((p & 1u) != 0u) ? color.b : color.r;
        vec2 pNW = getColorAndGreen(-1.,-1., p^1u);
        vec2 pNE = getColorAndGreen(+1.,-1., p^1u);
        vec2 pSW = getColorAndGreen(-1.,+1., p^1u);
        vec2 pSE = getColorAndGreen(+1.,+1., p^1u);
        float qNW = getColor(-2.,-2., p);
        float qNE = getColor(+2.,-2., p);
        float qSW = getColor(-2.,+2., p);
        float qSE = getColor(+2.,+2., p);
        float dD = abs(pNW.x - pSE.x) + abs(qP - qNW) + abs(qP - qSE) + abs(color.g - pNW.g) + abs(color.g - pSE.g);
        float dA = abs(pNE.x - pSW.x) + abs(qP - qNE) + abs(qP - qSW) + abs(color.g - pNE.g) + abs(color.g - pSW.g);
        float pP = (dD < dA) ? hueTransit(color.g, pNW, pSE)
                             : hueTransit(color.g, pNE, pSW);
        if ((p & 1u) != 0u) { color.r = pP; }
        else                { color.b = pP; }
    }
    return color;
}
