// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <new>

#include "string_util.h"
#include "file_util.h"

namespace FileUtil {

///////////////////////////////////////////////////////////////////////////////

char* getCurrentDirectory() {
    DWORD len = ::GetCurrentDirectoryA(0, nullptr);
    if (!len) { return nullptr; }
    char* cwd = (char*) malloc(len);
    if (!cwd) { return nullptr; }
    if (!::GetCurrentDirectoryA(len, cwd)) { ::free(cwd); return nullptr; }
    return cwd;
}

///////////////////////////////////////////////////////////////////////////////

struct DirectoryPrivate {
    HANDLE hFind;
    WIN32_FIND_DATAA findData;
    bool first;
};

bool Directory::open(const char* dir) {
    close();
    char *search = StringUtil::pathJoin(dir, "*");
    if (!search) { return false; }
    priv = new(std::nothrow) DirectoryPrivate;
    if (!priv) { return false; }
    priv->hFind = FindFirstFileA(search, &priv->findData);
    ::free(search);
    if (priv->hFind == INVALID_HANDLE_VALUE) {
        delete priv;
        priv = nullptr;
        return false;
    }
    priv->first = true;
    return true;
}

void Directory::close() {
    if (priv) {
        FindClose(priv->hFind);
        delete priv;
        priv = nullptr;
    }
}

bool Directory::next() {
    if (!priv) { return false; }
    if (priv->first) { priv->first = false; return true; }
    if (!FindNextFileA(priv->hFind, &priv->findData)) {
        close();
        return false;
    }
    return true;
}

const char* Directory::currentItemName() const {
    return priv ? priv->findData.cFileName : nullptr;
}

bool Directory::currentItemIsDir() {
    return priv ? !!(priv->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) : false;
}

///////////////////////////////////////////////////////////////////////////////

inline uint64_t makeU64(DWORD hi, DWORD lo) {
    return (uint64_t(hi) << 32) | uint64_t(lo);
}

bool FileFingerprint::update(const char* path) {
    m_size = m_mtime = 0;
    if (!path || !path[0]) { return false; }
    HANDLE hFile = CreateFileA(path, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, 0, nullptr);
    if (!hFile) { return false; }
    DWORD sizeH, sizeL = GetFileSize(hFile, &sizeH);
    if ((sizeL != INVALID_FILE_SIZE) || (GetLastError() == NO_ERROR)) {
        m_size = makeU64(sizeH, sizeL);
    }
    FILETIME ft;
    if (GetFileTime(hFile, nullptr, nullptr, &ft)) {
        m_mtime = makeU64(ft.dwHighDateTime, ft.dwLowDateTime);
    }
    CloseHandle(hFile);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace FileUtil
