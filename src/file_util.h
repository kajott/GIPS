#pragma once

namespace FileUtil {

///////////////////////////////////////////////////////////////////////////////

//! get the current working directory
//! \returns a newly-malloc'd string containing the working directory name
//!          (must be free()d by the caller)
char* getCurrentDirectory();

///////////////////////////////////////////////////////////////////////////////

class Directory {
    struct DirectoryPrivate *priv = nullptr;
public:
    bool open(const char* dir);
    inline bool good() const { return (priv != nullptr); }
    void close();

    bool next();
    const char* currentItemName() const;
    bool currentItemIsDir();

    inline bool nextNonDot() {
        bool res;
        const char* n;
        do {
            res = next();
            n = currentItemName();
        } while (res && n && (n[0] == '.'));
        return res;
    }

    inline Directory() {}
    inline Directory(const char* dir) { open(dir); }
    inline ~Directory() { close(); }
};

///////////////////////////////////////////////////////////////////////////////

}  // namespace FileUtil
