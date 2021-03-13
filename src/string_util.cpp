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
    char* t = (char*) malloc(len + 1);
    if (!t) { return nullptr; }
    memcpy(t, &m_str[m_start], len);
    t[len] = '\0';
    return t;
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

///////////////////////////////////////////////////////////////////////////////

}  // namespace StringUtil
