// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#define _POSIX_C_SOURCE 200809L  // for precise timestamps in stat()

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <new>
#include <string>

#include "string_util.h"
#include "file_util.h"

namespace FileUtil {

///////////////////////////////////////////////////////////////////////////////

char* getCurrentDirectory() {
    int len = 128;
    char *cwd = nullptr, *res;
    do {
        ::free(cwd);
        len *= 2;
        cwd = (char*) malloc(len);
        if (!cwd) { return nullptr; }
        res = getcwd(cwd, len);
    } while (!res && (errno == ERANGE));
    if (!res) { ::free(cwd); return nullptr; }
    return cwd;
}

///////////////////////////////////////////////////////////////////////////////

struct DirectoryPrivate {
    std::string dirName;
    DIR* dirHandle;
    struct dirent* currentItem;
};

bool Directory::open(const char* dir) {
    close();
    priv = new(std::nothrow) DirectoryPrivate;
    if (!priv) { return false; }
    priv->dirHandle = opendir(dir);
    if (!priv->dirHandle) {
        delete priv;
        priv = nullptr;
        return false;
    }
    priv->dirName = dir;
    priv->currentItem = nullptr;
    return true;
}

void Directory::close() {
    if (priv) {
        closedir(priv->dirHandle);
        delete priv;
        priv = nullptr;
    }
}

bool Directory::next() {
    if (!priv) { return false; }
    priv->currentItem = readdir(priv->dirHandle);
    return (priv->currentItem != nullptr);
}

const char* Directory::currentItemName() const {
    return (priv && priv->currentItem) ? priv->currentItem->d_name : nullptr;
}

bool Directory::currentItemIsDir() {
    if (!priv || !priv->currentItem) { return false; }
    char *fullPath = StringUtil::pathJoin(priv->dirName.c_str(), priv->currentItem->d_name);
    if (!fullPath) { return false; }
    struct stat st;
    int res = stat(fullPath, &st);
    ::free(fullPath);
    return !res && S_ISDIR(st.st_mode);
}

///////////////////////////////////////////////////////////////////////////////

bool FileFingerprint::update(const char* path) {
    m_size = m_mtime = 0;
    if (!path || !path[0]) { return false; }
    struct stat st;
    if (stat(path, &st)) { return false; }
    m_size = st.st_size;
    m_mtime = uint64_t(st.st_mtim.tv_sec) * 1000000000ULL + uint64_t(st.st_mtim.tv_nsec);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace FileUtil
