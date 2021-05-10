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

int addRoot(const char* root);
void removeRoot(int idx);
inline int addRoot(const std::string& root) { return addRoot(root.c_str()); }
int getRootCount();
const char* getRoot(int index);

//! helper to temporarily add a VFS root for a file's directory when loading
//! and saving pipeline files
class TemporaryRoot {
    int m_idx = -1;
public:
    void begin(const char* filename);
    inline void end() { removeRoot(m_idx); m_idx = -1; }
    inline TemporaryRoot() {}
    inline TemporaryRoot(const char* filename) { begin(filename); }
    inline ~TemporaryRoot() { end(); }
};

DirList getDirList(const char* relRoot="");
const DirList& getCachedDirList(const char* relRoot="");

//! get the full absolute path from a relative one
//! \returns a newly-allocated string containing a full path,
//!          or a copy of the input path if it was already an absolute path
//!          or if it could not be found inside the VFS
char* getFullPath(const char* relPath);

//! get the relative path from an absolute one
//! \returns a pointer to the point inside fullPath where the relative path begins,
//!          or fullPath itself if it's not relative to one of the roots
const char* getRelPath(const char* fullPath);

}  // namespace VFA
