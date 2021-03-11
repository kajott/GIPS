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
#include "string_util.h"

#include "gips_core.h"

namespace GIPS {

///////////////////////////////////////////////////////////////////////////////

bool Node::load(const char* filename, const GLutil::Shader& vs) {
    // initialize local variables to pessimistic defaults
    m_passCount = 0;
    m_filename = filename;
    m_name = filename;
    std::vector<Parameter> newParams;
    std::stringstream shader;
    std::stringstream err;
    GLutil::Program* prog = nullptr;
    Parameter* param = nullptr;
    int currentPass = 0;

    // create the fragment shader header
    shader.clear();
    shader << "#version 330 core"
         "\n" "#line 8000 42"
         "\n" "in vec2 gips_pos;"
         "\n" "out vec4 gips_frag;"
         "\n" "uniform sampler2D gips_tex;"
         "\n" "#line 1 1"
         "\n";

    shader << "uniform vec3 key;"
         "\n" "uniform float saturation;"
         "\n" "vec3 run(vec3 c) {"
         "\n" "  float gray = dot(c, key / (key.r + key.g + key.b));"
         "\n" "  return mix(vec3(gray), c, saturation);"
         "\n" "}"
         "\n";

    shader << "#line 9000 42"
         "\n" "void main() {"
         "\n" "  vec4 color = texture(gips_tex, gips_pos);"
         "\n" "  gips_frag = vec4(run(color.rgb), color.a);"
         "\n" "}"
         "\n";

    // create a parameter
    newParams.emplace_back();
    param = &newParams.back();
    param->m_name = "saturation";
    param->m_type = ParameterType::Value;
    param->m_minValue = 0.0f;
    param->m_maxValue = 5.0f;
    param->m_value[0] = 1.0f;

    // create another parameter
    newParams.emplace_back();
    param = &newParams.back();
    param->m_name = "key";
    param->m_desc = "key color";
    param->m_type = ParameterType::RGB;
    param->m_minValue = 0.0f;
    param->m_maxValue = 1.0f;
    param->m_value[0] = 0.299f;
    param->m_value[1] = 0.587f;
    param->m_value[2] = 0.114f;

    // compile shader and link program
    GLutil::Shader fs(GL_FRAGMENT_SHADER, shader.str().c_str());
    err << fs.getLog();
    if (!fs.good()) { goto load_finalize; }
    prog = &m_passes[currentPass].program;
    prog->link(vs, fs);
    err << prog->getLog();
    if (!prog->good()) { goto load_finalize; }

    // get uniform locations
    prog->use();
    GLutil::checkError("node setup");
    glUniform4f(prog->getUniformLocation("gips_area"), -1.0f, -1.0f, 2.0f, 2.0f);
    for (auto& p : newParams) {
        p.m_location[currentPass] = prog->getUniformLocation(p.m_name.c_str());
    }
    GLutil::checkError("node uniform lookup");

    // pass program setup done
    glUseProgram(0);
    ++currentPass;

    // setup done, proclaim success
    m_passCount = currentPass;

load_finalize:
    m_errors = err.str();
    m_params = newParams;
    return (m_passCount > 0);
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
