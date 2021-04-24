// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS  // prevent MSVC warnings
#endif

#include <cstdio>

#define STR2(x) #x
#define STR(x) STR2(x)

namespace SysInfo {

///////////////////////////////////////////////////////////////////////////////

#if defined(__amd64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
    const char* getPlatformID() {
        return "x86_64";
    }

#elif defined(__i386__) || defined(_M_IX86)
    const char* getPlatformID() {
        return "x86";
    }

#elif defined(__aarch64__)
    const char* getPlatformID() {
        return "AArch64";
    }

#elif defined(__ARM_ARCH_6__) || (defined(_M_ARM) && (_M_ARM >= 6))
    const char* getPlatformID() {
        return "ARMv6";
    }

#elif defined(__ARM_ARCH_7__) || (defined(_M_ARM) && (_M_ARM >= 7))
    const char* getPlatformID() {
        return "ARMv7";
    }

#else
    const char* getPlatformID() {
        return "unknown";
    }

#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
    const char* getSystemID() {
        return "Win32";
    }

#elif defined(__MACH__)
    const char* getSystemID() {
        return "macOS";
    }

#elif defined(__ANDROID__)
    const char* getSystemID() {
        return "Android";
    }

#elif defined(__linux__)
    const char* getSystemID() {
        return "Linux";
    }

#elif defined(__unix__)
    const char* getSystemID() {
        return "POSIX";
    }

#else
    const char* getSystemID() {
        return "unknown";
    }

#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(__clang__)
    const char* getCompilerID() {
        return "Clang " __clang_version__;
    }

#elif defined(_MSC_VER)
    const char* getCompilerID() {
        static char res[40] = "";
        if (!res[0]) {
            sprintf(res, "MSVC %d.%02d", _MSC_VER/100, _MSC_VER%100);
        }
        return res;
    }

#elif defined(__GNUC__)
    const char* getCompilerID() {
        return "GCC " STR(__GNUC__) "." STR(__GNUC_MINOR__) "." STR(__GNUC_PATCHLEVEL__);
    }

#else
    const char* getCompilerID() {
        return "unknown";
    }

#endif

///////////////////////////////////////////////////////////////////////////////

int getBitness() {
    return int(sizeof(void*) * 8);
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace SysInfo
