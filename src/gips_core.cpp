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
#include <vector>
#include <chrono>

#include "gl_header.h"
#include "gl_util.h"

#include "file_util.h"

#include "gips_core.h"

namespace GIPS {

///////////////////////////////////////////////////////////////////////////////

int getBytesPerPixel(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::Int16:
        case PixelFormat::Float16:
            return 8;
        case PixelFormat::Float32:
            return 16;
        default:
            return 4;
    }
}

const char* pixelFormatName(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::DontCare: return "don't care";
        case PixelFormat::Int16:    return "16-bit integer";
        case PixelFormat::Float16:  return "16-bit floating point";
        case PixelFormat::Float32:  return "32-bit floating point";
        default:                    return "8-bit integer";
    }
}

///////////////////////////////////////////////////////////////////////////////

bool Parameter::changed() {
    bool res = false;
    for (int i = 0;  i < 4;  ++i) {
        if (m_value[i] != m_oldValue[i]) { res = true; }
        m_oldValue[i] = m_value[i];
    }
    return res;
}

bool Node::changed() {
    bool res = m_programChanged || (m_wasEnabled != m_enabled);
    m_programChanged = false;
    m_wasEnabled = m_enabled;
    for (size_t i = 0;  i < m_params.size();  ++i) {
        if (m_params[i].changed()) { res = true; }
    }
    return res;
}

bool Pipeline::changed() {
    bool res = m_pipelineChanged;
    m_pipelineChanged = false;
    for (size_t i = 0;  i < m_nodes.size();  ++i) {
        if (m_nodes[i]->changed()) { res = true; }
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void Parameter::reset() {
    for (int i = 0;  i < 4;  ++i) {
        m_value[i] = m_defaultValue[i];
    }
}

void Node::reset() {
    for (size_t i = 0;  i < m_params.size();  ++i) {
        m_params[i].reset();
    }
}

bool Node::reload(const GLutil::Shader& vs, bool force) {
    FileUtil::FileFingerprint fp(m_filename.c_str());
    if (!force && (fp == m_fp)) {
        #ifndef NDEBUG
            fprintf(stderr, "not reloading '%s' (not changed)\n", m_filename.c_str());
        #endif
        return true;
    }
    return load(m_filename.c_str(), vs, &fp);
}

void Pipeline::reload(bool force) {
    m_pipelineChanged = true;
    for (size_t i = 0;  i < m_nodes.size();  ++i) {
        m_nodes[i]->reload(m_vs, force);
    }
}

Parameter* Node::findParam(const char* name) {
    for (size_t i = 0;  i < m_params.size();  ++i) {
        if (!strcmp(name, m_params[i].m_name.c_str())) { return &m_params[i]; }
    }
    return nullptr;
}

PixelFormat Pipeline::detectFormat() const {
    PixelFormat fmt = PixelFormat::Int8;
    for (size_t i = 0;  i < m_nodes.size();  ++i) {
        if (m_nodes[i]->m_preferredFormat > fmt) {
            fmt = m_nodes[i]->m_preferredFormat;
        }
    }
    return fmt;
}

///////////////////////////////////////////////////////////////////////////////

Node* Pipeline::addNode(int index) {
    if (!init()) { return nullptr; }
    Node* n = new(std::nothrow) Node;
    if (!n) { return nullptr; }
    m_nodes.push_back(nullptr);
    int lastIndex = int(m_nodes.size() - 1);
    if ((index < 0) || (index > lastIndex)) { index = lastIndex; }
    for (int i = lastIndex;  i > index;  --i) {
        m_nodes[size_t(i)] = m_nodes[size_t(i-1)];
    }
    m_nodes[size_t(index)] = n;
    m_pipelineChanged = true;
    return n;
}

void Pipeline::removeNode(int index) {
    int lastIndex = int(m_nodes.size() - 1);
    if ((index < 0) || (index > lastIndex)) { return; }
    delete m_nodes[size_t(index)];
    for (;  index < lastIndex;  ++index) {
        m_nodes[size_t(index)] = m_nodes[size_t(index+1)];
    }
    m_nodes.pop_back();
    m_pipelineChanged = true;
}

void Pipeline::moveNode(int fromIndex, int toIndex) {
    int lastIndex = int(m_nodes.size() - 1);
    if ((fromIndex < 0) || (fromIndex > lastIndex)
    ||  (toIndex < 0)   ||   (toIndex > lastIndex)
    ||  (fromIndex == toIndex)) { return; }
    Node *n = m_nodes[size_t(fromIndex)];
    while (fromIndex < toIndex) { m_nodes[size_t(fromIndex)] = m_nodes[size_t(fromIndex + 1)]; ++fromIndex; }
    while (fromIndex > toIndex) { m_nodes[size_t(fromIndex)] = m_nodes[size_t(fromIndex - 1)]; --fromIndex; }
    m_nodes[size_t(toIndex)] = n;
    m_pipelineChanged = true;
}

void Pipeline::clear() {
    for (size_t i = 0;  i < m_nodes.size();  ++i) {
        delete m_nodes[i];
    }
    m_nodes.clear();
    m_pipelineChanged = true;
}

void Pipeline::free() {
    clear();
    m_fbo.free();
    m_vs.free();
    if ((m_tex[0] || m_tex[1]) && GLutil::initialized) {
        glDeleteTextures(2, m_tex);
        m_tex[0] = m_tex[1] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool Pipeline::init() {
    if (m_initialized) {
        return m_initOK;
    }
    m_vs.compile(GL_VERTEX_SHADER,
         "#version 330 core"
    "\n" "uniform vec4 gips_pos2ndc;"
    "\n" "uniform vec4 gips_rel2map;"
    "\n" "out vec2 gips_pos;"
    "\n" "void main() {"
    "\n" "  vec2 pos = vec2(float(gl_VertexID & 1), float((gl_VertexID & 2) >> 1));"
    "\n" "  gips_pos = gips_rel2map.xy + pos * gips_rel2map.zw;"
    "\n" "  gl_Position = vec4(gips_pos2ndc.xy + pos * gips_pos2ndc.zw, 0., 1.);"
    "\n" "}"
    "\n");

    m_fbo.init();
    glGenTextures(2, m_tex);
    for (int i = 0;  i < 2;  ++i) {
        glBindTexture(GL_TEXTURE_2D, m_tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    m_initOK = m_vs.good();
    m_initialized = true;
    return m_initOK;
}

void Pipeline::render(GLuint srcTex, int width, int height, PixelFormat format, int maxNodes) {
    GLutil::clearError();
    if ((maxNodes < 0) || (maxNodes > nodeCount())) { maxNodes = nodeCount(); }
    if (format == PixelFormat::DontCare) { format = detectFormat(); }
    #ifndef NDEBUG
        fprintf(stderr, "render: %dx%d, fmt #%d, %d nodes\n", width, height, static_cast<int>(format), maxNodes);
    #endif

    // format change?
    if ((width != m_width) || (height != m_height) || (format != m_format)) {
        #ifndef NDEBUG
            fprintf(stderr, "render format changed (was %dx%d, #%d)\n", m_width, m_height, static_cast<int>(m_format));
        #endif
        for (int i = 0;  i < 2;  ++i) {
            glBindTexture(GL_TEXTURE_2D, m_tex[i]);
            GLint glfmt; GLenum dtype;
            switch (format) {
                case PixelFormat::Int16:   glfmt = GL_RGBA16;  dtype = GL_UNSIGNED_SHORT; break;
                case PixelFormat::Float16: glfmt = GL_RGBA16F; dtype = GL_FLOAT;          break;
                case PixelFormat::Float32: glfmt = GL_RGBA32F; dtype = GL_FLOAT;          break;
                default:                   glfmt = GL_RGBA8;   dtype = GL_UNSIGNED_BYTE;  break;
            }
            glTexImage2D(GL_TEXTURE_2D, 0, glfmt, width, height, 0, GL_RGBA, dtype, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        GLutil::checkError("intermediate buffer allocation");
        m_width = width;
        m_height = height;
        m_format = format;
    }

    // set viewport
    glViewport(0, 0, width, height);
    GLutil::checkError("processing viewport setup");
    auto t0 = std::chrono::high_resolution_clock::now();

    // iterate over the nodes and passes
    m_resultTex = srcTex;
    for (int nodeIndex = 0;  nodeIndex < maxNodes;  ++nodeIndex) {
        const auto& node = *m_nodes[size_t(nodeIndex)];
        if (!node.enabled()) { continue; }
        for (int passIndex = 0;  passIndex < node.passCount();  ++passIndex) {
            const auto& pass = node.m_passes[passIndex];

            // select output buffer to use
            GLuint outTex = (m_resultTex == m_tex[0]) ? m_tex[1] : m_tex[0];

            // prepare FBO, texture and program for rendering
            GLutil::clearError();
            if (!m_fbo.begin(outTex)) {
                #ifndef NDEBUG
                    fprintf(stderr, "Error: framebuffer isn't complete (status 0x%04X)\n", m_fbo.status);
                #endif
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, m_resultTex);
            pass.program.use();
            GLutil::checkError("FBO/tex/shader setup");

            // set up input texture
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, pass.texFilter ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, pass.texFilter ? GL_LINEAR : GL_NEAREST);

            // set up geometry
            glUniform2f(pass.locImageSize, GLfloat(m_width), GLfloat(m_height));
            double ox = 0.0, oy = 0.0, sx = 1.0, sy = 1.0;
            switch (pass.coordMode) {
                case CoordMapMode::Pixel:
                    sx = m_width;
                    sy = m_height;
                    break;
                case CoordMapMode::Relative:
                    ox = -std::max(1.0, double(m_width) / double(m_height));
                    oy = -std::max(1.0, double(m_height) / double(m_width));
                    sx = -2.0 * ox;
                    sy = -2.0 * oy;
                    break;
                default:  // None
                    break;
            }
            glUniform4f(pass.locRel2Map, GLfloat(ox), GLfloat(oy), GLfloat(sx), GLfloat(sy));
            if (pass.locMap2Tex >= 0) {
                glUniform4f(pass.locMap2Tex, GLfloat(-ox / sx), GLfloat(-oy / sy), GLfloat(1.0 / sx), GLfloat(1.0 / sy));
            }

            // set up parameters
            for (int paramIndex = 0;  paramIndex < node.paramCount();  ++paramIndex) {
                const auto& param = node.m_params[size_t(paramIndex)];
                GLint loc = param.m_location[passIndex];
                switch (param.m_type) {
                    case ParameterType::Value:
                    case ParameterType::Toggle:
                    case ParameterType::Angle:
                        glUniform1f(loc, param.m_value[0]);
                        break;
                    case ParameterType::Value2:
                        glUniform2fv(loc, 1, param.m_value);
                        break;
                    case ParameterType::Value3:
                    case ParameterType::RGB:
                        glUniform3fv(loc, 1, param.m_value);
                        break;
                    case ParameterType::Value4:
                    case ParameterType::RGBA:
                        glUniform4fv(loc, 1, param.m_value);
                        break;
                    // no default here; all enumerants are supposed to be handled
                }
            }
            GLutil::checkError("uniform setup");

            // now render!
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            GLutil::checkError("filter rendering");

            // "unprepare" everything
            glUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, 0);
            m_fbo.end();
            GLutil::checkError("FBO/tex/shader teardown");

            // set result to output buffer
            m_resultTex = outTex;

        }   // END pass loop
    }   // END node loop

    // force full pipeline flush to measure timing
    glBindTexture(GL_TEXTURE_2D, m_resultTex);
    glFlush();
    glFinish();
    glBindTexture(GL_TEXTURE_2D, 0);
    auto t1 = std::chrono::high_resolution_clock::now();
    m_lastRenderTime_ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
}   // END render()

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
