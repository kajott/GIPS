#pragma once

#include <cstring>
#include <cctype>

namespace StringUtil {

///////////////////////////////////////////////////////////////////////////////

inline bool isident(char c) {
    return c && (isalnum(c) || (c == '_') || (c == '.') || (c == '-') || (c == '+') || (c == '/'));
}

class Tokenizer {
public:
    static constexpr int MaxTokenLength = 31;
private:
    const char* m_str = nullptr;
    int m_pos = 0;
    int m_len = 0;
    char m_token[MaxTokenLength+1] = {0,};
    int m_start = 0;
public:
    void init(const char* str, int len=-1);
    bool next();
    bool extendUntil(const char* pattern, bool untilEndIfNotFound=true);
    inline void extendUntilEnd() { m_pos = m_len; }
    inline const char* token()  const { return m_token; }
    inline const int   start()  const { return m_start; }
    inline const int   end()    const { return m_pos; }
    inline const int   length() const { return m_pos - m_start; }
    inline const char* stringFromStart() const { return m_str ? &m_str[m_start] : nullptr; }
    inline const char* stringFromEnd()   const { return m_str ? &m_str[m_pos]   : nullptr; }
    inline bool isToken(const char* checkToken) const { return !strcmp(m_token, checkToken); }
    inline bool contains(char c) const { return !!strchr(m_token, c); }
    char* extractToken() const;  // result must be free()'d by caller!

    inline Tokenizer() {}
    inline Tokenizer(const char* str, int len=-1) { init(str, len); }
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct LookupEntry {
    const char* pattern;
    T value;
};
template <typename T>
inline T lookup(const LookupEntry<T>* table, const char* str) {
    if (!table || !str) { return T(0); }
    for(;  table->pattern && strcmp(table->pattern, str);  ++table);
    return table->value;
}

///////////////////////////////////////////////////////////////////////////////

int countLines(const char* s);

int stringLengthWithoutTrailingWhitespace(const char *s);

inline void trimTrailingWhitespace(char* s) {
    s[stringLengthWithoutTrailingWhitespace(s)] = '\0';
}

inline const char* skipWhitespace(const char* s) {
    while (s && *s && isspace(*s)) { ++s; }
    return s;
}

inline char* skipWhitespace(char* s) {
    while (s && *s && isspace(*s)) { ++s; }
    return s;
}

///////////////////////////////////////////////////////////////////////////////

inline bool ispathsep(char c) {
    return (c == '/')
      #ifdef _WIN32
        || (c == '\\')
      #endif
    ;
}

//! find the index of the first character of the last path component
int pathBaseNameIndex(const char* path);

//! return the last component of a path
inline const char* pathBaseName(const char* path) {
    return &path[pathBaseNameIndex(path)];
}

//! find the index of the dot separating the filename from the extension,
//! or the index of the terminating null byte if there
int pathExtStartIndex(const char* path);

//! remove the extension from a file name
inline void pathRemoveExt(char* path) {
    if (path) { path[pathExtStartIndex(path)] = '\0'; }
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace StringUtil
