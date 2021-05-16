// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <algorithm>

#include "patterns.h"

///////////////////////////////////////////////////////////////////////////////

class PRNG {  // bread-and-butter xorshift128
    uint32_t x, y, z, w;
public:
    explicit inline PRNG(uint32_t seed=0) { setSeed(seed); }
    inline PRNG(int a, int b) { setSeed((uint32_t(a * b) ^ uint32_t(a - b)) + 'g'*'i'*'p'*'s'); }
    inline void setSeed(uint32_t seed) {
        x = 0x13375EED ^ seed;
        y = 0xAFFED00F - seed;
        z = 0xDEADBEEF + seed;
        w = (0x08154711 * (seed & 1)) ^ y;
    }
    inline uint32_t getU32() {
        uint32_t t = x ^ (x << 11);
        x = y;  y = z;  z = w;
        w ^= (w >> 19) ^ t ^ (t >> 8);
        return w;
    }
    inline int getRange(int from, int to) {
        if (from > to) { int t = from; from = to; to = t; }
        return from + int(getU32() % uint32_t(to - from + 1));
    }
    inline float getF() {
        return float(getU32()) * (1.0f / 4294967296.f);
    }
    inline float getRange(float from, float to) {
        return from + (to - from) * getF();
    }
};

const PatternDefinition Patterns[] = {

///////////////////////////////////////////////////////////////////////////////

{ "Gradient", true,
[](uint8_t* data, int width, int height, bool alpha) {
    PRNG r(width, height);

    struct Gradient {
        int fx, fy;
        inline explicit Gradient(PRNG &r) {
            float a = r.getF() * 6.28f;
            fx = int(std::sin(a) * 64.f + .5f);
            fy = int(std::cos(a) * 64.f + .5f);
        }
        inline int operator() (int x, int y) {
            return x * fx + y * fy;
        }
    };

    // get min/max from corners
    Gradient g(r);
    int m0 = g(1,     1);
    int m1 = g(width, 1);
    int m2 = g(1,     height);
    int m3 = g(width, height);
    int mapMin = std::min({m0,m1,m2,m3});
    int mapDelta = std::max({m0,m1,m2,m3}) - mapMin;

    // initialize colors
    const int nc = alpha ? 4 : 3;
    struct Component { uint8_t c0; int scale; } comp[4] = {{0,0},{0,0},{0,0},{0,0}};
    for (int c = 0;  c < nc;  ++c) {
        uint8_t c0 = uint8_t(r.getRange(c ? comp[c-1].c0 : 0,  200));
        uint8_t c1 = uint8_t(r.getRange(comp[c].c0 + 1, 255));
        comp[c].c0 = c0;
        comp[c].scale = (((c1 - c0) << 23) - 1) / mapDelta;
    }

    // randomize colors
    for (int a = 0;  a < (nc-1);  ++a) {
        int b = r.getRange(a, nc-1);
        if (a != b) {
            Component temp = comp[a];
            comp[a] = comp[b];
            comp[b] = temp;
        }
    }

    // dithering map
    static const uint8_t bayer3[8][8] = {
        {  0, 32,  8, 40,  2, 34, 10, 42 },
        { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 },
        { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 },
        { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 },
        { 63, 31, 55, 23, 61, 29, 53, 21 },
    };

    // produce bitmap
    for (int y = height;  y;  --y) {
        for (int x = width;  x;  --x) {
            int m = g(x, y) - mapMin;
            int radj = int(bayer3[y&7][x&7]) << (23 - 6);
            for (int c = 0;  c < nc;  ++c) {
                *data++ = comp[c].c0 + uint8_t((comp[c].scale * m + radj) >> 23);
            }
            if (!alpha) {
                *data++ = 0xFF;
            }
        }
    }
}},

///////////////////////////////////////////////////////////////////////////////

{ "Clouds", false,
[](uint8_t* data, int width, int height, bool alpha) {
    #define pixel(p,x,y) p[((x) + (y) * stride) << 2]

    struct Recursion {
        PRNG r;
        int stride;
        void recurse(uint8_t* p, int w, int h, int n) {
            // terminate recursion
            if ((w < 1) || (h < 1)) { return; }
            if ((w < 2) && (h < 2)) { return; }

            // get corner colors and center positions
            uint8_t c00 = pixel(p, 0, 0);
            uint8_t c20 = pixel(p, w, 0);
            uint8_t c02 = pixel(p, 0, h);
            uint8_t c22 = pixel(p, w, h);
            int x = w >> 1;
            int y = h >> 1;

            // generate interpolated pixels (if not already done so)
            const auto addNoise = [this,n](uint8_t c) -> uint8_t {
                return uint8_t(r.getRange(std::max(1, int(c) - n), std::min(255, int(c) + n)));
            };
            #define genpix(a,var,b,u,v) do { \
                var = pixel(p, u, v); \
                if (!var) { \
                    var = pixel(p, u, v) = addNoise(uint8_t((uint16_t(a) + uint16_t(b)) >> 1)); \
                } \
            } while (0)
            uint8_t c10 = 0, c12 = 0, c01 = 0, c21 = 0;
            if (x > 0) {
                genpix(c00, c10, c20, x, 0);
                genpix(c02, c12, c22, x, h);
            }
            if (y > 0) {
                genpix(c00, c01, c02, 0, y);
                genpix(c20, c21, c22, w, y);
            }
            #undef genpix
            if ((x > 0) && (y > 0)) {
                pixel(p, x, y) = addNoise(uint8_t((uint16_t(c10) + uint16_t(c12) + uint16_t(c01) + uint16_t(c21) + 2) >> 2));
            }

            // update noise and recurse
            n = (n >> 1) | 1;
            recurse(&pixel(p, 0, 0),     x,     y, n);
            recurse(&pixel(p, x, 0), w - x,     y, n);
            recurse(&pixel(p, 0, y),     x, h - y, n);
            recurse(&pixel(p, x, y), w - x, h - y, n);
        }
        inline explicit Recursion(int s) : r(42), stride(s) {}
    };

    memset(data, 0, size_t(width * height * 4));

    Recursion r(width);
    for (int c = (alpha ? 4 : 3);  c;  --c) {
        const int stride = width;
        
        // generate random corner colors
        pixel(data, 0,         0)          = uint8_t(r.r.getU32()) | 1;
        pixel(data, width - 1, 0)          = uint8_t(r.r.getU32()) | 1;
        pixel(data, 0,         height - 1) = uint8_t(r.r.getU32()) | 1;
        pixel(data, width - 1, height - 1) = uint8_t(r.r.getU32()) | 1;

        // start the interpolation
        r.recurse(data, width - 1, height - 1, 63);

        // continue with next component
        ++data;
    }
    #undef pixel
}},

///////////////////////////////////////////////////////////////////////////////

{ "Plasma", true,
[](uint8_t* data, int width, int height, bool alpha) {
    PRNG r(uint32_t(width * height) ^ uint32_t(width - height));
    int cx = r.getRange(-width,  2*width);
    int cy = r.getRange(-height, 2*height);
    float scale = 1.0f / float(std::min(width, height));

    struct Linear {
        float fx, fy, phase;
        inline Linear(PRNG &r, float scale) {
            fx = r.getRange(-10.0f * scale, 10.0f * scale);
            fy = r.getRange(-10.0f * scale, 10.0f * scale);
            phase = r.getF() * 6.28f;
        }
        inline float operator() (float x, float y) {
            return std::sin(x * fx + y * fy + phase);
        }
    };

    struct Circular {
        float fx, fy;
        inline Circular(PRNG &r, float scale) {
            scale *= scale;
            fx = r.getRange(10.0f * scale, 100.0f * scale);
            fy = r.getRange(10.0f * scale, 100.0f * scale);
        }
        inline float operator() (float x, float y) {
            return std::cos(std::sqrt(x*x*fx + y*y*fy));
        }
    };

    struct Octave {
        Linear l1, l2;
        Circular c;
        float selfmod;
        float oscale;
        inline Octave(PRNG &r, float scale) :
        l1(r, scale), l2(r, scale), c(r, scale) {
            selfmod = r.getRange(1.0f, 5.0f);
            oscale = 0.25f / std::pow(scale, 1.5f);
        }
        inline float operator() (float x, float y) {
            float f = l1(x,y) + l2(x,y) + c(x,y);
            return oscale * (f + std::cos(f * selfmod));
        }
    };

    Octave o1(r, 1.0f);
    Octave o2(r, 8.0f);
    Octave o3(r, 64.0f);
    float o2mod = r.getRange(1.0f, 10.0f);
    float o1mod = r.getRange(0.5f, 5.0f);
    float cAmp = r.getRange(3.0f, 5.0f);
    float cPhase = r.getF() * 6.28f;
    for (int iy = height;  iy;  --iy) {
        float y = scale * float(iy - cy);
        for (int ix = width;  ix;  --ix) {
            float x = scale * float(ix - cx);
            float f = o3(x,y);
            f += o2(x,y) + std::sin(f * o2mod);
            f += o1(x,y) + std::sin(f * o1mod);
            f *= cAmp;
            *data++ = uint8_t(128.0f + 127.9f * std::sin(f + cPhase));
            *data++ = uint8_t(128.0f + 127.9f * std::sin(f + cPhase + 2.1f));
            *data++ = uint8_t(128.0f + 127.9f * std::sin(f + cPhase + 4.2f));
            *data++ = !alpha ? 255
                    : uint8_t(128.0f + 127.9f * std::sin(f + cPhase + 3.1f));
        }
    }
}},

///////////////////////////////////////////////////////////////////////////////

{ "Voronoi", true,
[](uint8_t* data, int width, int height, bool alpha) {
    // allocate JFA map
    struct JFAPixel {
        uint16_t cx, cy;
        inline bool valid() const { return cx && cy; }
        inline int distTo(int x, int y) const {
            x -= cx;
            y -= cy;
            return x * x + y * y;
        }
    };
    JFAPixel *jfaMap = new(std::nothrow) JFAPixel[width * height];
    if (!jfaMap) {
        memset(data, 0, width * height * 4);
        return;
    }
    memset(jfaMap, 0, sizeof(JFAPixel) * size_t(width * height));

    // plant "seeds"
    PRNG r(width, height);
    int cellSize = std::max(width, height) >> 3;
    cellSize = r.getRange(cellSize - (cellSize >> 2), cellSize + (cellSize >> 2));
    for (int cby = -(cellSize >> 1);  cby < height;  cby += cellSize) {
        for (int cbx = -(cellSize >> 1);  cbx < width;  cbx += cellSize) {
            int x = cbx + r.getRange(cellSize >> 4, cellSize - (cellSize >> 4));
            int y = cby + r.getRange(cellSize >> 4, cellSize - (cellSize >> 4));
            if ((x >= 0) && (y >= 0) && (x < width) && (y < height)) {
                auto& m = jfaMap[y * width + x];
                m.cx = uint16_t(x);
                m.cy = uint16_t(y);
            }
        }
    }
    float distNorm = 1.0f / (std::sqrt(2.0f) * float(cellSize));

    // JFA propagation
    auto jfaSingleDir = [=](const int dx, const int dy) {
        for (int y = std::max(0, -dy);  y < std::min(height, height - dy);  ++y) {
            const auto *srcRow = &jfaMap[(y + dy) * width];
            auto *destRow = &jfaMap[y * width];
            for (int x = std::max(0, -dx);  x < std::min(width, width - dx);  ++x) {
                const auto& src = srcRow[x + dx];
                auto& dest = destRow[x];
                if (!dest.valid() || (src.valid() && (src.distTo(x, y) < dest.distTo(x, y)))) {
                    dest = src;
                }
            }
        }
    };
    int stepSize = cellSize;
    while (stepSize & (stepSize + 1)) { stepSize |= (stepSize >> 1); }
    for (++stepSize;  stepSize;  stepSize >>= 1) {
        jfaSingleDir( stepSize, 0);
        jfaSingleDir(-stepSize, 0);
        jfaSingleDir(0,  stepSize);
        jfaSingleDir(0, -stepSize);
    }

    // convert JFA map into result image
    const JFAPixel *pMap = jfaMap;
    JFAPixel cluster; cluster.cx = cluster.cy = 0;
    float baseCol[3] = {0.0f,};
    for (int y = 0;  y < height;  ++y) {
        for (int x = 0;  x < width;  ++x) {
            // get current cluster and its base color
            if ((pMap->cx != cluster.cx) || (pMap->cy != cluster.cy)) {
                cluster = *pMap;
                r.setSeed((uint32_t(cluster.cx) << 16) | uint32_t(cluster.cy));
                (void)r.getU32();
                baseCol[0] = r.getRange(0.0f, 0.5f);
                baseCol[1] = r.getRange(0.5f, 1.0f);
                baseCol[2] = r.getRange(baseCol[0], baseCol[1]);
                std::swap(baseCol[0], baseCol[r.getRange(0, 2)]);
                std::swap(baseCol[1], baseCol[r.getRange(1, 2)]);
            }
            ++pMap;

            // get (inverse) normalized distance from center
            float dist = 1.0f - std::min(1.0f, distNorm * std::sqrt(float(cluster.distTo(x, y))));

            // compute final color
            for (int i = 0;  i < 3;  ++i) {
                float c = baseCol[i];
                if (!alpha) { c *= dist; }
                *data++ = uint8_t(c * 255.984375f);
            }
            *data++ = alpha ? uint8_t(dist * 255.984375f) : 0xFF;
        }
    }

    delete[] jfaMap;
}},

///////////////////////////////////////////////////////////////////////////////

{ "XOR", true,
[](uint8_t* data, int width, int height, bool alpha) {
    for (int y = 0;  y < height;  ++y) {
        for (int x = 0;  x < width;  ++x) {
            *data++ = uint8_t(x) ^ 255;
            *data++ = uint8_t(x ^ y);
            *data++ = uint8_t(y);
            *data++ = alpha ? uint8_t(x - y) : 255;
        }
    }
}},

///////////////////////////////////////////////////////////////////////////////

};  // Patterns[]

const int NumPatterns = int(sizeof(Patterns) / sizeof(Patterns[0]));
