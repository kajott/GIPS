#pragma once

#include "gl_header.h"

namespace GLutil {

extern bool initialized;

bool init();
void done();

inline void clearError() {
    while (glGetError());
}

const char* errorString(GLuint code);
inline const char* errorString() { return errorString(glGetError()); }

GLenum checkError(const char* prefix=nullptr);

void enableDebugMessages();

class Shader {
private:
    int logAlloc = 0;
public:
    GLuint id = 0;
    GLuint type = 0;
    char* log = nullptr;
    inline const char* getLog() const { return log ? log : ""; }
    inline bool haveLog() const { return log && log[0]; }
    bool ok = false;
    inline bool good() const { return initialized && ok; }
    bool init(GLuint type_);
    bool compile(const char* src);
    inline bool compile(GLuint type_, const char* src) { return init(type_) && compile(src); }
    void free();
    inline Shader() {}
    inline explicit Shader(GLuint type_) { init(type_); }
    inline Shader(GLuint type_, const char* src) { compile(type_, src); }
    inline ~Shader() { free(); }
    inline operator GLuint() const { return id; }
};

class Program {
private:
    int logAlloc = 0;
public:
    GLuint id = 0;
    char* log = nullptr;
    inline const char* getLog() const { return log ? log : ""; }
    inline bool haveLog() const { return log && log[0]; }
    bool ok = false;
    inline bool good() const { return initialized && ok; }
    bool init();
    bool link(GLuint vs, GLuint fs);
    void free();
    inline bool use() const { if (initialized && ok) { glUseProgram(id); return true; } else { return false; } }
    inline GLint getUniformLocation(const char* name) const { return initialized ? glGetUniformLocation(id, name) : -1; }
    inline Program() {}
    inline Program(GLuint vs, GLuint fs) { link(vs, fs); }
    inline ~Program() { free(); }
    inline operator GLuint() const { return id; }
};

class FBO {
public:
    GLuint id = 0;
    GLenum status = 0;
    bool init();
    void free();
    bool begin(GLuint tex);
    void end();
    inline FBO() {}
    inline ~FBO() { free(); }
    inline operator GLuint() const { return id; }
};

}  // namespace GLutil
