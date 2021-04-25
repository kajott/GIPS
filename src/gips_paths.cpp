// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS  // prevent MSVC warnings
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "string_util.h"
#include "vfs.h"

#include "gips_app.h"

// whether to use the system-wide shader directories even for portable installations
// -- I'm really torn on this. One one hand, portable means "doesn't interfere
// with system config", but on the other hand, there's no real harm in having
// the system-wide registered shaders available in a portable install either.
static const bool allowSystemWideShaderDirsForPortableInstalls = true;

///////////////////////////////////////////////////////////////////////////////

static char* getExecutablePath(const char* argv0) {
    #ifndef _WIN32
        // not Win32 -> assume we're on Linux and can use /proc/self/exe
        for (size_t size = 256;  size <= (1u << 24u);  size *= 2) {
            char* lstr = static_cast<char*>(malloc(size));
            if (!lstr) { break; }
            ssize_t res = readlink("/proc/self/exe", lstr, size);
            if ((res > 0) && (res < ssize_t(size))) {
                lstr[res] = '\0'; return lstr;
            }
            ::free(lstr);
            if (res < 0) { break; }
        } 
    #endif

    // last resort: splice the path together from directory and argv[0]
    // (this is usually sufficient on Win32 -- sure, there is
    // GetModuleHandleA(), but it's more trouble than it's worth)
    char* cwd = FileUtil::getCurrentDirectory();
    char* me = StringUtil::pathJoin(cwd, argv0);
    ::free(cwd);
    return me;
}

///////////////////////////////////////////////////////////////////////////////

void GIPS::App::setPaths(const char* argv0) {
    // get app's base directory
    char* me = getExecutablePath(argv0);
    StringUtil::pathRemoveBaseName(me);
    m_appDir = me;
    ::free(me);
    #ifndef NDEBUG
        fprintf(stderr, "application directory: '%s'\n", m_appDir.c_str());
    #endif

    // detect whether we have a "portable" installation
    #ifdef _WIN32
        // Win32: it's portable if the executable isn't located in "Program Files"
        // or "ProgramData" (the latter can happen if installed via a package manger)
        bool portable = !StringUtil::pathContains(m_appDir.c_str(), "Program Files")
                     && !StringUtil::pathContains(m_appDir.c_str(), "Program Files (x86)")
                     && !StringUtil::pathContains(m_appDir.c_str(), "ProgramData")
                     && !StringUtil::pathContains(m_appDir.c_str(), "PROGRA~1")
                     && !StringUtil::pathContains(m_appDir.c_str(), "PROGRA~2")
                     && !StringUtil::pathContains(m_appDir.c_str(), "PROGRA~3");
    #else
        // POSIX: it's portable if the executable isn't located in /usr or /opt
        bool portable = strncmp(m_appDir.c_str(), "/usr/", 5)
                     && strncmp(m_appDir.c_str(), "/opt/", 5);
    #endif
    #ifndef NDEBUG
        fprintf(stderr, "installation type: %s\n", portable ? "portable" : "system");
    #endif

    // get user configuration directory
    std::string userCfgDir;
    #ifdef _WIN32
        const char* appDataDir = getenv("APPDATA");
        if (appDataDir && appDataDir[0]) {
            char* ucd = StringUtil::pathJoin(appDataDir, "GIPS");
            userCfgDir = ucd;
            ::free(ucd);
        }
    #else
        const char* homeDir = getenv("HOME");
        if (homeDir && homeDir[0]) {
            char* ucd = StringUtil::pathJoin(homeDir, ".config/gips");
            userCfgDir = ucd;
            ::free(ucd);
        }
    #endif

    // set configuration file location
    if (portable || userCfgDir.empty()) {
        m_appUIConfigFile = m_appDir + StringUtil::defaultPathSep + "gips_ui.ini";
    } else {
        // system-wide installation: ensure that the config directory exists
        #ifdef _WIN32
            CreateDirectoryA(userCfgDir.c_str(), NULL);
        #else
            mkdir(userCfgDir.c_str(), 0755);
        #endif
        m_appUIConfigFile = userCfgDir + StringUtil::defaultPathSep + "gips_ui.ini";
    }
    #ifndef NDEBUG
        fprintf(stderr, "UI config file: '%s'\n", m_appUIConfigFile.c_str());
    #endif

    // set shader directories
    // - program directory (and that's the only one in true portable mode)
    VFS::addRoot(m_appDir + StringUtil::defaultPathSep + "shaders");
    if (!portable || allowSystemWideShaderDirsForPortableInstalls) {
        // - "shaders" subdirectory of user config directory
        if (!userCfgDir.empty()) {
            VFS::addRoot(userCfgDir + StringUtil::defaultPathSep + "shaders");
        }
        // - Linux: {/usr,/usr/local,~/.local}/share/gips/shaders
        #ifndef _WIN32
            if (homeDir) {
                char* d = StringUtil::pathJoin(homeDir, ".local/share/gips/shaders");
                VFS::addRoot(d);
                ::free(d);
            }
            VFS::addRoot("/usr/share/gips/shaders");
            VFS::addRoot("/usr/local/share/gips/shaders");
        #endif
    }
}
