// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
    #define _CRT_SECURE_NO_WARNINGS  // prevent MSVC warnings
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cassert>

#include "string_util.h"
namespace StringUtil {

///////////////////////////////////////////////////////////////////////////////

void Tokenizer::init(const char* str, int len) {
    m_str = str;
    m_len = (len < 0) ? int(strlen(str)) : len;
    m_pos = m_start = 0;
    m_token[0] = '\0';
}

bool Tokenizer::next() {
    if (!m_str) { return false; }
    while ((m_pos < m_len) && isspace(m_str[m_pos])) { ++m_pos; }
    if (m_pos >= m_len) {
        // EOS reached
        m_start = m_pos;
        m_token[0] = '\0';
        return false;
    }
    m_start = m_pos;
    bool tokenIsIdent = isident(m_str[m_pos]);
    int copyPos = 0;
    do {
        if (copyPos < MaxTokenLength) {
            m_token[copyPos++] = m_str[m_pos];
        }
        ++m_pos;
    } while ((m_pos < m_len) && !isspace(m_str[m_pos]) && (isident(m_str[m_pos]) == tokenIsIdent));
    m_token[copyPos] = '\0';
    return true;
}

bool Tokenizer::extendUntil(const char* pattern, bool untilEndIfNotFound) {
    if (!m_str) { return false; }
    int patLen = int(strlen(pattern));
    int checkEnd = m_len - patLen;
    for (int checkPos = m_pos;  checkPos <= checkEnd;  ++checkPos) {
        const char *s = &m_str[checkPos];
        while (*pattern && (*s == *pattern)) { ++s; ++pattern; }
        if (!*pattern) {
            // pattern found
            m_pos = checkPos + patLen;
            return true;
        }
    }
    // pattern not found
    if (untilEndIfNotFound) { extendUntilEnd(); }
    return true;
}

char* Tokenizer::extractToken() const {
    int len = m_pos - m_start;
    if (!m_str || (len < 0)) { return nullptr; }
    char* t = static_cast<char*>(malloc(size_t(len + 1)));
    if (!t) { return nullptr; }
    memcpy(t, &m_str[m_start], size_t(len));
    t[len] = '\0';
    return t;
}

///////////////////////////////////////////////////////////////////////////////

char* loadTextFile(const char* filename, int *p_size) {
    if (!filename || !filename[0]) { return nullptr; }
    FILE* f = fopen(filename, "rb");
    if (!f) { return nullptr; }
    fseek(f, 0, SEEK_END);
    size_t size = size_t(ftell(f));
    fseek(f, 0, SEEK_SET);
    char* data = static_cast<char*>(malloc(size + 1));
    if (!data) { fclose(f); return nullptr; }
    size_t res = fread(data, 1, size, f);
    fclose(f);
    if (p_size) { *p_size = int(res); }
    if (res != size) {
        ::free(data);
        return nullptr;
    } else {
        data[size] = '\0';
        return data;
    }
}

///////////////////////////////////////////////////////////////////////////////

int countLines(const char* s) {
    int lines = 1, realLines = 0;
    for (;  *s;  ++s) {
        if (*s == '\n') {
            lines++;
        } else if (!isspace(*s)) {
            realLines = lines;
        }
    }
    return realLines;
}

int stringLengthWithoutTrailingWhitespace(const char *s) {
    int lastNonWS = 0;
    for (int i = 0;  *s;  ++i, ++s) {
        if (!isspace(*s)) { lastNonWS = i + 1; }
    }
    return lastNonWS;
}

char* copy(const char* str, int extraChars) {
    if (!str) { return nullptr; }
    char *res = static_cast<char*>(malloc(strlen(str) + size_t(extraChars) + 1u));
    if (!res) { return nullptr; }
    strcpy(res, str);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

int pathBaseNameIndex(const char* path) {
    if (!path) { return 0; }
    int res = 0;
    for (int i = 0;  path[i];  ++i) {
        if (ispathsep(path[i])) { res = i + 1; }
    }
    return res;
}

int pathExtStartIndex(const char* path) {
    if (!path) { return 0; }
    int i, dot = -1;
    for (i = 0;  path[i];  ++i) {
        if (path[i] == '.') { dot = i; }
        else if (ispathsep(path[i])) { dot = -1; }
    }
    return (dot < 0) ? i : dot;
}

uint32_t extractExtCode(const char* path) {
    if (!path) { return 0; }
    path = pathExt(path);
    if (*path == '.') { ++path; }
    uint32_t code = 0;
    for (int shift = 0;  (shift < 32) && *path;  ++path, shift += 8) {
        code |= uint32_t(tolower(*path)) << shift;
    }
    return code;
}

char* pathJoin(const char* a, const char* b) {
    // handle trivial cases
    if (isempty(a) || isAbsPath(b)) { return copy(b); }
    if (isempty(b)) { return copy(a); }

    // allocate sufficient space for the result
    char* path = copy(a, int(strlen(b)) + 1);
    if (!path) { return nullptr; }

    // Win32 special case: B might be a drive-relative path
    #ifdef _WIN32
        if (ispathsep(b[0]) && !ispathsep(b[1])) {
            if (isalpha(a[0]) && (a[1] == ':')) {
                // A starts with a drive letter -> fine, join the paths
                strcpy(&path[2], b);
                return path;
            } else {
                // A is something else -> overwrite it with B completely
                strcpy(path, b);
                return path;
            }
        }
    #endif

    // remove any remaining "stray" path separators off path A
    int idx = int(strlen(path));
    while ((idx > 0) && ispathsep(path[idx-1])) { --idx; }

    // glue the strings together with a path separator
    path[idx++] = defaultPathSep;
    strcpy(&path[idx], b);
    return path;
}

bool pathContains(const char* path, const char* comp) {
    if (!path || !comp || !comp[0]) { return false; }
    const char *pC = comp;
    bool match = true;
    for (const char *pP = path;  *pP;  ++pP) {
        if (ispathsep(*pP)) {
            if (!*pC && match) { 
                                 return true; }
            pC = comp;
            match = true;
        } else if (match) {
            #ifdef _WIN32
                match = (tolower(*pP) == tolower(*pC));
                ++pC;
            #else
                match = (*pP == *pC++);
            #endif
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace StringUtil
