#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>

#include <string>
#include <vector>

#include "gl_header.h"
#include "gl_util.h"

#include "gips_core.h"

namespace GIPS {

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

Node* Pipeline::addNode(int index) {
    if (!init()) { return nullptr; }
    Node* n = new(std::nothrow) Node;
    if (!n) { return nullptr; }
    m_nodes.push_back(nullptr);
    int lastIndex = int(m_nodes.size() - 1);
    if ((index < 0) || (index > lastIndex)) { index = lastIndex; }
    for (int i = lastIndex;  i > index;  --i) {
        m_nodes[i] = m_nodes[i-1];
    }
    m_nodes[index] = n;
    m_pipelineChanged = true;
    return n;
}

void Pipeline::removeNode(int index) {
    int lastIndex = int(m_nodes.size() - 1);
    if ((index < 0) || (index > lastIndex)) { return; }
    delete m_nodes[index];
    for (;  index < lastIndex;  ++index) {
        m_nodes[index] = m_nodes[index+1];
    }
    m_nodes.pop_back();
    m_pipelineChanged = true;
}

Pipeline::~Pipeline() {
    for (size_t i = 0;  i < m_nodes.size();  ++i) {
        delete m_nodes[i];
    }
    m_nodes.clear();
}

///////////////////////////////////////////////////////////////////////////////

bool Pipeline::init() {
    if (m_initialized) {
        return m_initOK;
    }
    m_vs.compile(GL_VERTEX_SHADER,
         "#version 330 core"
    "\n" "uniform vec4 gips_area;"
    "\n" "out vec2 gips_pos;"
    "\n" "void main() {"
    "\n" "  vec2 pos = vec2(float(gl_VertexID & 1), float((gl_VertexID & 2) >> 1));"
    "\n" "  gips_pos = pos;"
    "\n" "  gl_Position = vec4(gips_area.xy + pos * gips_area.zw, 0., 1.);"
    "\n" "}"
    "\n");

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(2, m_tex);
    for (int i = 0;  i < 2;  ++i) {
        glBindTexture(GL_TEXTURE_2D, m_tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    m_initOK = m_vs.good();
    m_initialized = true;
    return m_initOK;
}

void Pipeline::render(GLuint srcTex, int width, int height, int maxNodes) {
    GLutil::clearError();
    if ((maxNodes < 0) || (maxNodes > nodeCount())) { maxNodes = nodeCount(); }
    #ifndef NDEBUG
        fprintf(stderr, "render: %dx%d, %d nodes\n", width, height, maxNodes);
    #endif

    // format change?
    if ((width != m_width) || (height != m_height)) {
        #ifndef NDEBUG
            fprintf(stderr, "render format changed (was %dx%d)\n", m_width, m_height);
        #endif
        for (int i = 0;  i < 2;  ++i) {
            glBindTexture(GL_TEXTURE_2D, m_tex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        GLutil::checkError("intermediate buffer allocation");
        m_width = width;
        m_height = height;
    }

    // set viewport
    glViewport(0, 0, width, height);
    GLutil::checkError("processing viewport setup");

    // iterate over the nodes and passes
    m_resultTex = srcTex;
    for (int nodeIndex = 0;  nodeIndex < maxNodes;  ++nodeIndex) {
        const auto& node = *m_nodes[nodeIndex];
        if (!node.enabled()) { continue; }
        for (int passIndex = 0;  passIndex < node.passCount();  ++passIndex) {
            const auto& pass = node.m_passes[passIndex];

            // select output buffer to use
            GLuint outTex = (m_resultTex == m_tex[0]) ? m_tex[1] : m_tex[0];

            // prepare FBO for rendering
            GLutil::clearError();
            glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex, 0);
            #ifndef NDEBUG
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    fprintf(stderr, "Error: framebuffer isn't complete (status 0x%04X)\n", status);
                }
            #endif
            glBindTexture(GL_TEXTURE_2D, m_resultTex);
            pass.program.use();
            GLutil::checkError("FBO/tex/shader setup");

            // set up input texture
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // set up parameters
            for (int paramIndex = 0;  paramIndex < node.paramCount();  ++paramIndex) {
                const auto& param = node.m_params[paramIndex];
                GLint loc = param.m_location[passIndex];
                switch (param.m_type) {
                    case ParameterType::Value:
                        glUniform1f(loc, param.m_value[0]);
                        break;
                    case ParameterType::RGB:
                        glUniform3fv(loc, 1, param.m_value);
                        break;
                    case ParameterType::RGBA:
                        glUniform4fv(loc, 1, param.m_value);
                        break;
                    default:
                        break;
                }
            }
            GLutil::checkError("uniform setup");

            // now render!
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            GLutil::checkError("filter rendering");

            // "unprepare" everything
            glUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            GLutil::checkError("FBO/tex/shader teardown");

            // set result to output buffer
            m_resultTex = outTex;

        }   // END pass loop
    }   // END node loop
}   // END render()

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
