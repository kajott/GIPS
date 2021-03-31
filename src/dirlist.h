// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>

struct DirList {
    struct Item {
        std::string nameNoExt;
        std::string fullPath;
        bool isDir;
        Item(const char* baseDir, const char* name, bool isDir_);
        bool operator< (const Item& other) const;
    };
    std::vector<Item> items;
};

const DirList& getCachedDirList(const char* dir);
