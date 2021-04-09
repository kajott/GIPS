// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <algorithm>

#include <SDL.h>
#include <SDL_syswm.h>

#include "stb_image.h"

#include "clipboard.h"

namespace Clipboard {

///////////////////////////////////////////////////////////////////////////////

HWND hWnd = 0;

void init(SDL_Window* window) {
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        hWnd = info.info.win.window;
    }
}

bool isAvailable() {
    return (hWnd != 0);
}

///////////////////////////////////////////////////////////////////////////////

void* getRGBA8Image(int &width, int &height) {
    // open the clipboard
    width = height = 0;
    if (!OpenClipboard(hWnd)) { return nullptr; }

    // check if there's a PNG format available; if so, use it
    for (UINT fmt = EnumClipboardFormats(0);  fmt;  fmt = EnumClipboardFormats(fmt)) {
        constexpr int fmtNameLength = 16;
        char fmtName[fmtNameLength];
        if (!GetClipboardFormatNameA(fmt, fmtName, fmtNameLength - 1)) { fmtName[0] = '\0'; }
        fmtName[fmtNameLength - 1] = '\0';
        if (!strcmp(fmtName, "PNG") || !strcmp(fmtName, "image/png") || !strcmp(fmtName, "image/x-png")) {
            HANDLE hPNG = GetClipboardData(fmt);
            if (hPNG) {
                const uint8_t* png = (uint8_t*) GlobalLock(hPNG);
                if (png) {
                    void *decoded = stbi_load_from_memory(png, int(GlobalSize(hPNG)), &width, &height, nullptr, 4);
                    GlobalUnlock(hPNG);
                    if (decoded) {
                        CloseClipboard();
                        #ifndef NDEBUG
                            fprintf(stderr, "decoded clipboard from PNG\n");
                        #endif
                        return decoded;
                    }
                }
            }
        }   // END PNG handler
    }   // END format loop

    // load the image as DIB data
    HANDLE hDIB = GetClipboardData(CF_DIB);
    if (!hDIB) { CloseClipboard(); return false; }
    const uint8_t* dibData = (const uint8_t*) GlobalLock(hDIB);
    if (!dibData) { CloseClipboard(); return false; }
    int dibSize = int(GlobalSize(hDIB));

    // parse the bitmap header
    BITMAPINFOHEADER* bmih = (BITMAPINFOHEADER*) dibData;
    bool headerOK = (dibSize >= sizeof(BITMAPINFOHEADER))
                 && (int(bmih->biSize) < dibSize)
                 && (bmih->biPlanes == 1)
                 && ( ( // format 1: classic BI_RGB, 24 or 32 bits BGR(A)
                            (bmih->biCompression == BI_RGB)
                        && ((bmih->biBitCount == 24) || (bmih->biBitCount == 32))
                      ) || (
                      // format 2: BI_BITFIELDS with BI_RGB-compatible layout
                            (bmih->biCompression == BI_BITFIELDS)
                        &&  (bmih->biBitCount == 32)
                        &&  (dibSize > int(bmih->biSize + 12))
                        &&  (((const uint32_t*)(&dibData[bmih->biSize]))[0] == 0x00FF0000u)
                        &&  (((const uint32_t*)(&dibData[bmih->biSize]))[1] == 0x0000FF00u)
                        &&  (((const uint32_t*)(&dibData[bmih->biSize]))[2] == 0x000000FFu)
                      )
                    );
    int bpp = 0;
    if (headerOK) {
        width = int(bmih->biWidth);
        height = std::abs(int(bmih->biHeight));
        bpp = (bmih->biBitCount == 32) ? 4 : 3;
        headerOK = (dibSize >= (int(bmih->biSize) + width * height * bpp));
    }

    // is this image in a format we can handle directly?
    if (headerOK) {
        dibData += bmih->biSize;
        void *decoded = malloc(width * height * 4);
        if (decoded) {
            for (int y = height - 1;  y >= 0;  --y) {
                uint8_t* pDest = &((uint8_t*)decoded)[y * width * 4];
                for (int x = width;  x;  --x) {
                    *pDest++ = dibData[2];
                    *pDest++ = dibData[1];
                    *pDest++ = dibData[0];
                    *pDest++ = 255;
                    dibData += bpp;
                }
            }
            #ifndef NDEBUG
                fprintf(stderr, "decoded clipboard from raw 24/32-bit DIB\n");
            #endif
            GlobalUnlock(hDIB);
            CloseClipboard();
            return decoded;
        }
    }

    // last-ditch effort: add a BITMAPFILEHEADER and let stb_image have a stab at it
    int fullSize = dibSize + sizeof(BITMAPFILEHEADER);
    uint8_t* fullDIB = (uint8_t*) malloc(fullSize);
    if (!fullDIB) { GlobalUnlock(hDIB); CloseClipboard(); return false; }
    fullDIB[0] = 'B';
    fullDIB[1] = 'M';
    BITMAPFILEHEADER* bmfh = (BITMAPFILEHEADER*) fullDIB;
    bmfh->bfSize = DWORD(fullSize);
    bmfh->bfReserved1 = bmfh->bfReserved2 = 0;
    bmfh->bfOffBits = DWORD(sizeof(BITMAPFILEHEADER)) + bmih->biSize;
    memcpy(&fullDIB[sizeof(BITMAPFILEHEADER)], dibData, dibSize);

    // we're done with the clipboard API at this point
    GlobalUnlock(hDIB);
    CloseClipboard();

    // decode the image and return it
    #ifndef NDEBUG
        fprintf(stderr, "trying to decode clipboard DIB via stb_image\n");
    #endif
    void* result = stbi_load_from_memory(fullDIB, fullSize, &width, &height, nullptr, 4);
    ::free(fullDIB);
    return result;
}

///////////////////////////////////////////////////////////////////////////////

bool setRGBA8Image(const void* image, int width, int height) {
    // sanity check
    if (!image || (width < 1) || (height < 1)) { return false; }

    // create global memory handle and lock it
    HGLOBAL hImage = GlobalAlloc(GMEM_MOVEABLE, SIZE_T(width * height * 4 + sizeof(BITMAPINFOHEADER)));
    if (!hImage) { return false; }
    uint8_t *dibData = (uint8_t*) GlobalLock(hImage);
    if (!dibData) { GlobalFree(hImage); return false; }

    // set bitmap header
    BITMAPINFOHEADER* bmih = (BITMAPINFOHEADER*) dibData;
    bmih->biSize = DWORD(sizeof(BITMAPINFOHEADER));
    bmih->biWidth = LONG(width);
    bmih->biHeight = LONG(height);
    bmih->biPlanes = 1;
    bmih->biBitCount = 32;
    bmih->biCompression = BI_RGB;
    bmih->biSizeImage = DWORD(width * height * 4);
    bmih->biXPelsPerMeter = bmih->biYPelsPerMeter = 3780;  // 96 dpi
    bmih->biClrUsed = bmih->biClrImportant = 0;
    dibData += sizeof(BITMAPINFOHEADER);

    // copy bitmap data
    for (int y = height - 1;  y >= 0;  --y) {
        const uint8_t* pSrc = &((const uint8_t*)image)[y * width * 4];
        for (int x = width;  x;  --x) {
            uint8_t r = *pSrc++;
            uint8_t g = *pSrc++;
            uint8_t b = *pSrc++;
            *dibData++ = b;
            *dibData++ = g;
            *dibData++ = r;
            *dibData++ = *pSrc++;
        }
    }
    GlobalUnlock(hImage);

    // send the data to the clipboard
    if (!OpenClipboard(hWnd)) { GlobalFree(hImage); return false; }
    if (!EmptyClipboard()) { GlobalFree(hImage); CloseClipboard(); return false; }
    bool ok = SetClipboardData(CF_DIB, hImage) != NULL;
    if (!ok) { GlobalFree(hImage); }
    CloseClipboard();
    return ok;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace Clipboard
