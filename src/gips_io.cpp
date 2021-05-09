// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>

#include <algorithm>
#include <string>
#include <sstream>

#include "string_util.h"
#include "vfs.h"

#include "gips_core.h"

static void floatToHexColor(char* str, const float* values, int count) {
    *str++ = '#';
    while (count--) {
        int i = int((*values++) * 255.0f + 0.5f);
        if (i < 0) { i = 0; }
        if (i > 255) { i = 255; }
        const char *hex = "0123456789ABCDEF";
        *str++ = hex[i >> 4];
        *str++ = hex[i & 15];
    }
    *str = '\0';
}

namespace GIPS {

///////////////////////////////////////////////////////////////////////////////

std::string Pipeline::serialize(int showIndex) {
    std::ostringstream f;
    f << "[GIPS]\r\nversion = 1\r\n";
    if (showIndex == 0) {
        f << ".show = 1\r\n";
    }

    for (size_t nodeIndex = 0;  nodeIndex < m_nodes.size();  ++nodeIndex) {
        const Node& node = *m_nodes[nodeIndex];
        const char* relPath = VFS::getRelPath(node.m_filename.c_str());
        #ifdef _WIN32
            if (!StringUtil::isAbsPath(relPath)) {
                // there may still be backslashes in the (non-absolute) path ->
                // turn them into forward slashes
                char *rp2 = StringUtil::copy(relPath);
                for (char *p = rp2;  p && *p;  ++p) {
                    if (*p == '\\') { *p = '/'; }
                }
                f << "\r\n[" << (rp2 ? rp2 : relPath) << "]\r\n";
                ::free(rp2);
            } else
        #endif
            f << "\r\n[" << relPath << "]\r\n";
        if ((size_t(showIndex) == (nodeIndex + 1u)) && ((nodeIndex + 1u) < m_nodes.size())) {
            f << ".show = 1\r\n";
        }
        if (!node.m_enabled) {
            f << ".enabled = 0\r\n";
        }

        for (size_t paramIndex = 0;  paramIndex < node.m_params.size();  ++paramIndex) {
            const Parameter& param = node.m_params[paramIndex];
            f << param.m_name << " = ";
            switch (param.m_type) {
                case ParameterType::Value:
                case ParameterType::Toggle:
                case ParameterType::Angle:
                    f << param.m_value[0];
                    break;
                case ParameterType::Value2:
                    f << param.m_value[0] << ", " << param.m_value[1];
                    break;
                case ParameterType::Value3:
                    f << param.m_value[0] << ", " << param.m_value[1] << ", " << param.m_value[2];
                    break;
                case ParameterType::Value4:
                    f << param.m_value[0] << ", " << param.m_value[1] << ", " << param.m_value[2] << ", " << param.m_value[3];
                    break;
                case ParameterType::RGB:
                    { char c[8]; floatToHexColor(c, param.m_value, 3); f << c; }
                    break;
                case ParameterType::RGBA:
                    { char c[10]; floatToHexColor(c, param.m_value, 4); f << c; }
                    break;
            }
            f << "\r\n";
        }   // END of parameter loop
    }   // END of node loop

    return f.str();
}

///////////////////////////////////////////////////////////////////////////////

static inline bool isnumsep(char c) {
    return (c == ',') || isspace(c);
}

static int hex2int(char c) {
    if ((c >= '0') && (c <= '9')) { return c - '0'; }
    if ((c >= 'a') && (c <= 'f')) { return c - 'a' + 10; }
    if ((c >= 'A') && (c <= 'F')) { return c - 'A' + 10; }
    return -1;
}

int Pipeline::unserialize(char* data) {
    if (!data || !data[0]) { return -1; }
    bool waitingForSection = true;
    Node* node = nullptr;
    int showIndex = -1;
    float vnum[4];
    bool versionOK = false;
    while (*data) {  // line loop
        // skip initial whitespace
        while ((*data == ' ') || (*data == '\t')) { ++data; }
        char *line = data, *end = data;
        // mark last non-whitespace character as end
        for (;  *data && (*data != '\n');  ++data) {
            if (!isspace(*data)) { end = &data[1]; }
        }
        *end = '\0';
        if (*data) { ++data; }

        // ignore comment and empty lines
        if (!line[0] || (line[0] == ';') || (line[0] == '#')) {
            continue;
        }

        // check for section header
        if ((line[0] == '[') && (end[-1] == ']')) {
            end[-1] = '\0';
            ++line;

            // check for GIPS section
            if (waitingForSection) {
                if (!strcmp(line, "GIPS")) {
                    waitingForSection = false;
                    continue;
                }
                return -1;  // invalid first section
            }

            // special checks for first node
            if (!node) {
                if (!versionOK) { return -1; }
                clear();
            }

            // resolve filename
            #ifndef NDEBUG
                fprintf(stderr, "loading node for '%s'\n", line);
            #endif
            char *filename = VFS::getFullPath(line);
            #ifdef _WIN32
                // ensure that all slashes in the path are backslashes
                if (filename) {
                    for (char *c = filename;  *c;  ++c) {
                        if (*c == '/') { *c = '\\'; }
                    }
                }
            #endif

            // load node
            node = addNode(filename ? filename : line);
            ::free(filename);
            continue;
        }
        if (waitingForSection) { return -1; }  // file doesn't start with a section

        // split line into key/value pair
        char *key = line, *value = line;
        end = line;
        // mark last non-whitespace character as end
        for (;  *value && (*value != '=');  ++value) {
            if (!isspace(*value)) { end = &value[1]; }
        }
        *end = '\0';
        if (!*value || !key[0]) {
            // no valid separator or empty key -> syntax error
            if (!node) { return -1; }  // syntax errors are fatal in the GIPS section
            node->m_errors += "(GIPS) syntax error in pipeline file: '" + std::string(line) + "'\n";
            continue;
        }
        // skip separator and initial whitespace in value
        ++value;
        while (*value && isspace(*value)) { ++value; }
        if (!*value) {
            // empty value -> syntax error
            if (!node) { return -1; }  // syntax errors are fatal in the GIPS section
            node->m_errors += "(GIPS) syntax error in pipeline file: empty value for key '" + std::string(value) + "'\n";
            continue;
        }

        // parse values
        int numcount = 0;
        if (*value == '#') {
            // hexadecimal color
            char *vpos = &value[1];
            while (numcount < 4) {
                int n = hex2int(*vpos++);
                if (!*vpos) { numcount = -1; break; }
                n = (n << 4) | hex2int(*vpos++);
                if (n < 0) { numcount = -1; break; }
                vnum[numcount++] = float(n) / 255.0f;
                if (!*vpos) { break; }
            }
            if ((numcount < 3) || *vpos) {
                node->m_errors += "(GIPS) syntax error in pipeline file: invalid color '" + std::string(value) + "' for key '" + std::string(key) + "'\n";
                continue;
            }
        } else {
            // list of values
            while (numcount < 4) {
                end = nullptr;
                vnum[numcount] = std::strtof(value, &end);
                if (!end || (*end && !isnumsep(*end))) {
                    numcount = -1;
                    break;
                }
                while (*end && isnumsep(*end)) { ++end; }
                value = end;
                ++numcount;
                if (!*value) { break; }
            }
            if (numcount < 0) {
                node->m_errors += "(GIPS) syntax error in pipeline file: invalid value '" + std::string(value) + "' for key '" + std::string(key) + "'\n";
                continue;
            }
            if (*value) {
                node->m_errors += "(GIPS) syntax error in pipeline file: too many values for key '" + std::string(key) + "'\n";
            }
        }

        // look up the parameter (if valid) and set remaining dimensions of the value
        Parameter* param = node ? node->findParam(key) : nullptr;
        for (;  numcount < 4;  ++numcount) {
            vnum[numcount] = param ? param->m_defaultValue[numcount] : 0.0f;
        }

        // check for ".show" pseudo-key
        if (!strcmp(key, ".show") && (vnum[0] > 0.0f)) {
            showIndex = nodeCount();
            continue;
        }

        // check for special keys in GIPS section
        if (!node) {
            if (!strcmp(key, "version")) {
                versionOK = (std::abs(vnum[0] - 1.0f) < 1.0E-6f);
                if (!versionOK) { return -1; }  // invalid version
            }
            continue;
        }

        // check for ".enabled" pseudo-key
        if (!strcmp(key, ".enabled") || !strcmp(key, ".enable") || !strcmp(key, ".active")) {
            node->m_enabled = (vnum[0] > 0.5f);
            continue;
        }

        // assign standard parameter value
        if (!param) {
            node->m_errors += "(GIPS) unknown parameter '" + std::string(key) + "' in pipeline file\n";
        } else {
            for (int i = 0;  i < 4;  ++i) {
                param->m_value[i] = vnum[i];
            }
        }
    }
    return (showIndex >= 0) ? showIndex : nodeCount();
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
