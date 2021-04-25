// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>
#include <string>

namespace VFS {

struct DirList {
    struct Item {
        std::string nameNoExt;
        std::string relPath;
        std::string fullPath;
        bool isDir;
        Item(const char* rootDir, const char* relDir, const char* name, bool isDir_);
        bool operator< (const Item& other) const;
    };
    std::vector<Item> items;

    //! merge two sorted DirLists; resolve ties by comparing modification times;
    //! after this, 'src' will be cleared
    void merge(DirList& src);
};

void addRoot(const char* root);
inline void addRoot(const std::string& root) { addRoot(root.c_str()); }

int getRootCount();
const char* getRoot(int index);

DirList getDirList(const char* relRoot="");
const DirList& getCachedDirList(const char* relRoot="");

//! get the relative path from an absolute one
//! \returns a pointer to the point inside fullPath where the relative path begins,
//!          or fullPath itself if it's not relative to one of the roots
const char* getRelPath(const char* fullPath);

}  // namespace VFA
