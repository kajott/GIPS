#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include "string_util.h"

#include "gl_header.h"
#include "gl_util.h"

namespace GLutil {

///////////////////////////////////////////////////////////////////////////////

bool initialized = false;

static GLuint theVAO = 0;

bool init() {
    if (initialized) { return true; }
    glGenVertexArrays(1, &theVAO);
    glBindVertexArray(theVAO);
    initialized = true;
    return true;
}

void done() {
    if (initialized) {
        glBindVertexArray(0);
        if (theVAO) { glDeleteVertexArrays(1, &theVAO); }
    }
    initialized = false;
    theVAO = 0;
}

///////////////////////////////////////////////////////////////////////////////

const char* errorString(GLuint code) {
    switch (code) {
        case GL_NO_ERROR:                      return nullptr;
        case GL_INVALID_ENUM:                  return "invalid enum";
        case GL_INVALID_VALUE:                 return "invalid value";
        case GL_INVALID_OPERATION:             return "invalid operation";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "invalid framebuffer operation";
        case GL_OUT_OF_MEMORY:                 return "out of memory";
        default:                               return "unknown error";
    }
}

bool checkError(const char* prefix) {
    bool err = false;
    for (;;) {
        GLuint code = glGetError();
        if (!code) { break; }
        #ifndef NDEBUG
        if (prefix) {
            fprintf(stderr, "%s: OpenGL Error: [0x%04X] %s\n", prefix, code, errorString(code));
        } else {
            fprintf(stderr, "OpenGL Error: [0x%04X] %s\n", code, errorString(code));
        }
        #endif
    }
    return err;
}

static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    static const char *s_source, *s_type, *s_severity;
    switch (source) {
        case 0x8246: s_source = "API "; break;  // DEBUG_SOURCE_API
        case 0x8247: s_source = "WSys"; break;  // DEBUG_SOURCE_WINDOW_SYSTEM
        case 0x8248: s_source = "GLSL"; break;  // DEBUG_SOURCE_SHADER_COMPILER
        case 0x8249: s_source = "3rdP"; break;  // DEBUG_SOURCE_THIRD_PARTY
        case 0x824A: s_source = "App "; break;  // DEBUG_SOURCE_APPLICATION
        case 0x824B: s_source = "misc"; break;  // DEBUG_SOURCE_OTHER
        default:     s_source = "??? "; break;
    }
    switch (type) {
        case 0x824C: s_type = "error"; break;  // DEBUG_TYPE_ERROR
        case 0x824D: s_type = "depr "; break;  // DEBUG_TYPE_DEPRECATED_BEHAVIOR
        case 0x824E: s_type = "undef"; break;  // DEBUG_TYPE_UNDEFINED_BEHAVIOR
        case 0x824F: s_type = "port "; break;  // DEBUG_TYPE_PORTABILITY
        case 0x8250: s_type = "perf "; break;  // DEBUG_TYPE_PERFORMANCE
        case 0x8251: s_type = "other"; break;  // DEBUG_TYPE_OTHER
        case 0x8268: s_type = "mark "; break;  // DEBUG_TYPE_MARKER
        case 0x8269: s_type = "push "; break;  // DEBUG_TYPE_PUSH_GROUP
        case 0x826A: s_type = "pop  "; break;  // DEBUG_TYPE_POP_GROUP
        default:     s_type = "???  "; break;
    }
    switch (severity) {
        case 0x9146: s_severity = "high"; break;  // DEBUG_SEVERITY_HIGH
        case 0x9147: s_severity = "med "; break;  // DEBUG_SEVERITY_MEDIUM
        case 0x9148: s_severity = "low "; break;  // DEBUG_SEVERITY_LOW
        case 0x826B: s_severity = "note"; break;  // DEBUG_SEVERITY_NOTIFICATION
        default:     s_severity = "??? "; break;
    }
    fprintf(stderr, "GL Debug [src:%s type:%s sev:%s] %s\n", s_source, s_type, s_severity, message);
}

void enableDebugMessages() {
#ifndef NDEBUG
    #ifdef GL_KHR_debug
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageCallback(GLdebugCallback, nullptr);
        glEnable(GL_DEBUG_OUTPUT);
    #elif GL_ARB_debug_output
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, 0x826B, 0, nullptr, GL_FALSE);
        glDebugMessageCallbackARB(debugCallback, nullptr);
    #endif
#else
    (void)GLdebugCallback;
#endif
}

///////////////////////////////////////////////////////////////////////////////

bool Shader::init(GLuint type_) {
    if (!initialized) { return false; }
    if (id && (type == type_)) { return true; }
    if (id) { glDeleteShader(id); id = type = 0; }
    id = glCreateShader(type_);
    if (!id) { return false; }
    type = type_;
    return true;
}

void Shader::free() {
    if (initialized && id) {
        glDeleteShader(id);
        id = type = 0;
    }
    ::free(log);
    log = nullptr;
    logAlloc = 0;
}

bool Shader::compile(const char* src) {
    if (!initialized || !id) {
        ok = false;
        if (log) { log[0] = '\0'; }
        return false;
    }
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint logLen = 0;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > logAlloc) {
        ::free(log);
        logAlloc = logLen + 128;
        log = (char*) malloc(logAlloc);
        if (!log) { logAlloc = 0; }
    }
    if (logLen && log) {
        glGetShaderInfoLog(id, logAlloc, nullptr, log);
        StringUtil::trimTrailingWhitespace(log);
        #ifndef NDEBUG
            if (log[0]) { fprintf(stderr, "----- %s shader compilation log -----\n%s\n", (type == GL_VERTEX_SHADER) ? "vertex" : (type == GL_FRAGMENT_SHADER) ? "fragment" : "other", log); }
        #endif
    } else if (log) {
        log[0] = '\0';
    }
    GLint status = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    ok = (status == GL_TRUE);
    return ok;
}

///////////////////////////////////////////////////////////////////////////////

bool Program::init() {
    if (!initialized) { return false; }
    if (id) { return true; }
    id = glCreateProgram();
    return (id != 0);
}

void Program::free() {
    if (initialized && id) {
        glDeleteProgram(id);
        id = 0;
    }
    ::free(log);
    log = nullptr;
    logAlloc = 0;
}

bool Program::link(GLuint vs, GLuint fs) {
    if (!id && initialized) {
        id = glCreateProgram();
    }
    if (!id) {
        ok = false;
        if (log) { log[0] = '\0'; }
        return false;
    }
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);
    GLint logLen = 0;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > logAlloc) {
        ::free(log);
        logAlloc = logLen + 128;
        log = (char*) malloc(logAlloc);
        if (!log) { logAlloc = 0; }
    }
    if (logLen && log) {
        glGetProgramInfoLog(id, logAlloc, nullptr, log);
        StringUtil::trimTrailingWhitespace(log);
        #ifndef NDEBUG
            if (log[0]) { fprintf(stderr, "----- program link log -----\n%s\n", log); }
        #endif
    } else if (log) {
        log[0] = '\0';
    }
    GLint status = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    glDetachShader(id, vs);
    glDetachShader(id, fs);
    ok = (status == GL_TRUE);
    return ok;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace GLutil
