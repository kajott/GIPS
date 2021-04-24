// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdio>
#include <cctype>

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "string_util.h"
#include "file_util.h"

#include "vfs.h"

namespace VFS {

///////////////////////////////////////////////////////////////////////////////

struct CachedDirList : public DirList {
    std::chrono::steady_clock::time_point nextUpdate;
};

static std::vector<std::string> roots;
static std::unordered_map<std::string, CachedDirList> dirCache;

void addRoot(const char* root) {
    roots.emplace_back(root);
}

int getRootCount() {
    return int(roots.size());
}

const char* getRoot(int index) {
    return ((index >= 0) && (index < int(roots.size()))) ? roots[size_t(index)].c_str() : nullptr;
}

///////////////////////////////////////////////////////////////////////////////

DirList getDirList(const char* relRoot) {
    DirList result;
    for (const auto& root : roots) {
        char* absRoot = StringUtil::pathJoin(root.c_str(), relRoot);
        #ifndef NDEBUG
            fprintf(stderr, "scanning directory '%s' ", absRoot);
        #endif

        DirList newList;
        FileUtil::Directory d(absRoot);
        while (d.nextNonDot()) {
            newList.items.emplace_back(root.c_str(), relRoot, d.currentItemName(), d.currentItemIsDir());
        }
        #ifndef NDEBUG
            fprintf(stderr, "(%d items)\n", int(newList.items.size()));
        #endif

        std::sort(newList.items.begin(), newList.items.end());
        result.merge(newList);

        ::free(absRoot);
    }
    return result;
}

const DirList& getCachedDirList(const char* relRoot) {
    CachedDirList &list = dirCache[relRoot];
    auto now = std::chrono::steady_clock::now();
    if (now > list.nextUpdate) {
        // re-scan the relative directory
        DirList dl = getDirList(relRoot);
        list.items = std::move(dl.items);

        // some time might have passed since we called now(), so update it
        now = std::chrono::steady_clock::now();
    }   // END list update
    list.nextUpdate = now + std::chrono::seconds(1);
    return list;
}

///////////////////////////////////////////////////////////////////////////////

DirList::Item::Item(const char* rootDir, const char* relDir, const char* name, bool isDir_) {
    char* rp = StringUtil::pathJoin(relDir, name);
    if (rp) { relPath = rp; }
    char* fp = StringUtil::pathJoin(rootDir, rp);
    if (fp) { fullPath = fp; }
    ::free(fp);
    ::free(rp);
    int dot = StringUtil::pathExtStartIndex(name);
    nameNoExt = std::string(name, size_t(dot));
    isDir = isDir_;
}

bool DirList::Item::operator< (const DirList::Item& other) const {
    if (isDir != other.isDir) { return isDir; }
    const char* a = nameNoExt.c_str();
    const char* b = other.nameNoExt.c_str();
    while (*a && *b && (tolower(*a) == tolower(*b))) { ++a; ++b; }
    return tolower(*a) < tolower(*b);
}

///////////////////////////////////////////////////////////////////////////////

void DirList::merge(DirList& src) {
    // short-circuit processing if one of the lists is empty
    if (src.items.empty()) {
        return;
    } 
    if (items.empty()) {
        items = std::move(src.items);
        src.items.clear();
        return;
    }

    // main merge loop
    auto posD = items.begin();
    auto posS = src.items.begin();
    for (;;) {
        // finish when source is done -> all remaining items in A can stay
        if (posS == src.items.end()) {
            src.items.clear();
            return;
        }

        // if destination is done, but there are still items in the source,
        // copy them verbatim
        if (posD == items.end()) {
            posD = items.insert(posD, std::move(*posS++)) + 1;
            continue;
        }

        // if the source's current item is smaller than the destination's
        // current item, insert it into the source list
        if (*posS < *posD) {
            posD = items.insert(posD, std::move(*posS++)) + 1;
            continue;
        }

        // if the destination's current item is smaller than the source's,
        // just skip it
        if (*posD < *posS) {
            ++posD;
            continue;
        }

        // if we arrived here, we have two equal items;
        // we need to decide which one "survives" and which one is discarded
        bool replace = false;

        // if this is a file, compare the timestamps
        // (don't do this for directories; those are always scanned for all roots anyway)
        // (also note that posD->isDir == posS->isDir, because we sorted the items that way)
        if (!posS->isDir) {
            FileUtil::FileFingerprint fpD(posD->fullPath.c_str());
            FileUtil::FileFingerprint fpS(posS->fullPath.c_str());
            replace = fpS.newerThan(fpD);
        }

        // decision done -> replace items as decided
        if (replace) {
            *posD = std::move(*posS);
        }
        // continue with the next items in both lists
        ++posD;  ++posS;
    }
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace VFS
