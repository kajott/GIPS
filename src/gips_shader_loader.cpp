#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>

#include <string>
#include <sstream>
#include <vector>

#include "gl_header.h"
#include "gl_util.h"

#include "gips_core.h"

namespace GIPS {

///////////////////////////////////////////////////////////////////////////////

bool Node::load(const char* filename, const GLutil::Shader& vs) {
    // initialize local variables to pessimistic defaults
    m_passCount = 0;
    m_filename = filename;
    m_name = filename;
    std::vector<Parameter> newParams;
    std::stringstream err;
    GLutil::Program* prog = nullptr;

    GLutil::Shader fs(GL_FRAGMENT_SHADER,
         "#version 330 core"
    "\n" "uniform sampler2D gips_tex;"
    "\n" "in vec2 gips_pos;"
    "\n" "out vec4 gips_frag;"
    "\n" "void main() {"
    "\n" "  vec4 t = texture(gips_tex, gips_pos);"
    "\n" "  gips_frag = vec4(vec3(dot(t.rgb, vec3(0.299, 0.587, 0.114))), t.a);"
    "\n" "}"
    "\n");
    err << fs.getLog();
    if (!fs.good()) { goto load_finalize; }

    prog = &m_passes[0].program;
    prog->link(vs, fs);
    err << prog->getLog();
    if (!prog->good()) { goto load_finalize; }

    prog->use();
    glUniform4f(prog->getUniformLocation("gips_area"), -1.0f, -1.0f, 2.0f, 2.0f);

    glUseProgram(0);

    m_passCount = 1;

load_finalize:
    m_errors = err.str();
    m_params = newParams;
    return (m_passCount > 0);
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
