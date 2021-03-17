#include <cstdio>
#include <cctype>

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "string_util.h"
#include "file_util.h"

#include "dirlist.h"

struct CachedDirList : public DirList {
    std::chrono::steady_clock::time_point nextUpdate;
};

static std::unordered_map<std::string, CachedDirList> dirCache;

const DirList& getCachedDirList(const char* dir) {
    CachedDirList &list = dirCache[dir];
    auto now = std::chrono::steady_clock::now();
    if (now > list.nextUpdate) {
        #ifndef NDEBUG
            fprintf(stderr, "re-scanning directory '%s'\n", dir);
        #endif
        std::vector<DirList::Item> newList;

        FileUtil::Directory d(dir);
        while (d.nextNonDot()) {
            newList.emplace_back(dir, d.currentItemName(), d.currentItemIsDir());
        }

        std::sort(newList.begin(), newList.end());
        list.items = std::move(newList);

        // some time might have passed since we called now(), so update it
        now = std::chrono::steady_clock::now();
    }   // END list update
    list.nextUpdate = now + std::chrono::seconds(1);
    return list;
}

DirList::Item::Item(const char* baseDir, const char* name, bool isDir_) {
    char* fp = StringUtil::pathJoin(baseDir, name);
    if (fp) { fullPath = fp; }
    ::free(fp);
    int dot = StringUtil::pathExtStartIndex(name);
    nameNoExt = std::string(name, dot);
    isDir = isDir_;
}

bool DirList::Item::operator< (const DirList::Item& other) const {
    if (isDir != other.isDir) { return isDir; }
    const char* a = nameNoExt.c_str();
    const char* b = other.nameNoExt.c_str();
    while (*a && *b && (tolower(*a) == tolower(*b))) { ++a; ++b; }
    return tolower(*a) < tolower(*b);
}
