#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include "string_util.h"

namespace StringUtil {

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

}  // namespace StringUtil
