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

}  // namespace FileUtil
