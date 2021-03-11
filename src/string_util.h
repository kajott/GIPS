#pragma once

namespace StringUtil {

int countLines(const char* s);

int stringLengthWithoutTrailingWhitespace(const char *s);

inline void trimTrailingWhitespace(char* s) {
    s[stringLengthWithoutTrailingWhitespace(s)] = '\0';
}

}  // namespace StringUtil
