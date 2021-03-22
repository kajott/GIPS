#define _CRT_SECURE_NO_WARNINGS  // prevent MSVC warnings

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cmath>
#include <cassert>

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

#include "gl_header.h"
#include "gl_util.h"
#include "string_util.h"

#include "gips_core.h"

namespace GIPS {

///////////////////////////////////////////////////////////////////////////////

constexpr float MaxSupportedVersionCode = 1.0;

enum class GLSLToken : int {
    Other       = 0,
    Ignored     = -1,
    Uniform     = 10,
    Float       = 1,
    Vec2        = 2,
    Vec3        = 3,
    Vec4        = 4,
    RunSingle   = 99,
    RunPass1    = 100,
    RunPass2    = 200,
    RunPass3    = 300,
    RunPass4    = 400,
    OpenParens  = 90,
    CloseParens = 91,
};

enum class PassInput { Coord, RGB, RGBA };
enum class PassOutput { RGB, RGBA };

static const StringUtil::LookupEntry<GLSLToken> tokenMap[] = {
    { "in",        GLSLToken::Ignored },
    { "uniform",   GLSLToken::Uniform },
    { "float",     GLSLToken::Float },
    { "vec2",      GLSLToken::Vec2 },
    { "vec3",      GLSLToken::Vec3 },
    { "vec4",      GLSLToken::Vec4 },
    { "run",       GLSLToken::RunSingle },
    { "run_pass1", GLSLToken::RunPass1 },
    { "run_pass2", GLSLToken::RunPass2 },
    { "run_pass3", GLSLToken::RunPass3 },
    { "run_pass4", GLSLToken::RunPass4 },
    { "(",         GLSLToken::OpenParens },
    { ")",         GLSLToken::CloseParens },
    { "){",        GLSLToken::CloseParens },
    { nullptr,     GLSLToken::Other },
};

///////////////////////////////////////////////////////////////////////////////

bool Node::load(const char* filename, const GLutil::Shader& vs) {
    // Declare all variables right here, C89-style.
    // This is required because we're using "goto end"-style error handling
    // here, and we can't jump over class initializations.
    FILE *f = nullptr;
    size_t fsize = 0;
    char *code = nullptr;
    std::vector<Parameter> newParams;
    std::ostringstream shader;
    std::ostringstream err;
    StringUtil::Tokenizer tok;
    GLutil::Shader fs;
    GLutil::Program* prog = nullptr;
    Parameter* param = nullptr;
    GLSLToken paramDataType = GLSLToken::Other;
    int paramValueIndex = -1;
    bool inParamStatement = false;
    int currentPass = 0;
    int passMask = 0;
    bool singlePass = false;
    PassInput inputs[MaxPasses];
    PassOutput outputs[MaxPasses];
    bool texFilter = true;
    CoordMapMode coordMode = CoordMapMode::Pixel;
    static constexpr int GLSLTokenHistorySize = 4;
    GLSLToken tt[GLSLTokenHistorySize] = { GLSLToken::Other, };

    // a quick sanity check
    if (!filename || !filename[0]) { return false; }
    #ifndef NDEBUG
        printf("loading shader '%s'\n", filename);
    #endif

    // initialize member variables to pessimistic defaults
    m_programChanged = true;
    m_passCount = 0;
    m_filename = filename;
    {
        const char *basename = StringUtil::pathBaseName(filename);
        m_name = std::string(basename, StringUtil::pathExtStartIndex(basename));
    }

    // load the file
    f = fopen(filename, "rb");
    if (!f) {
        err << "(GIPS) failed to open input file '" << filename << "'\n";
        goto load_finalize;
    }
    fseek(f, 0, SEEK_END);
    fsize = size_t(ftell(f));
    fseek(f, 0, SEEK_SET);
    code = (char*) malloc(fsize + 1);
    if (!code) {
        err << "(GIPS) out of memory while loading the input file\n";
        goto load_finalize;
    }
    if (fread(code, 1, fsize, f) != fsize) {
        err << "(GIPS) failed to read input file '" << filename << "'\n";
        goto load_finalize;
    }
    fclose(f);  f = nullptr;
    code[fsize] = '\0';

    // analyze the GLSL code
    tok.init(code);
    while (tok.next()) {
        // check for comment
        bool singleLineComment = tok.isToken("//");
        bool multiLineComment  = tok.isToken("/*");
        if (singleLineComment || multiLineComment) {
            // extract comment
            if (singleLineComment) { tok.extendUntil("\n"); }
            if (multiLineComment)  { tok.extendUntil("*/"); }
            char* comment = tok.extractToken();
            if (multiLineComment) { comment[strlen(comment) - 2] = '\0'; }  // strip '*/' at end
            char* content = &comment[2];  // skip '//' or '/*'
            if (content[0] == '!') { ++content; }  // handle '//!' Doxygen-style comment

            // search for @key=value tokens
            char *pos = content;
            for (;;) {
                pos = strchr(pos, '@');
                if (!pos) { break; }  // no token found
                if (isalnum(pos[-1])) { ++pos; continue; }  // ignore token in the middle of a word
                // extract key
                char *key = &pos[1];
                char *value = nullptr;
                for (pos = key;  StringUtil::isident(*pos);  ++pos) { *pos = tolower(*pos); }
                if (*pos == '=') {
                    // extract value
                    *pos = '\0';
                    value = &pos[1];
                    for (pos = value;  StringUtil::isident(*pos);  ++pos) { *pos = tolower(*pos); }
                }
                // terminate key/value and move pos to character after token
                if (*pos) { *pos++ = '\0'; }

                // at this point, key and value have been extracted; now parse them
                bool isNum = false;
                float fval = 0.0f;
                if (value) {
                    char *end = nullptr;
                    fval = strtof(value, &end);
                    isNum = (end && !*end);
                }

                // define a few parsing helper functions
                bool keyMatched = false;
                const auto isKey = [&] (const char *t) -> bool {
                    bool match = !strcmp(key, t);
                    keyMatched = keyMatched || match;
                    return match;
                };
                const auto isValue = [&] (const char *t) -> bool {
                    return !strcmp(value, t);
                };
                const auto needParam = [&] () -> bool {
                    if (!param) {
                        err << "(GIPS) '@" << key << "' token is only valid inside a parameter comment\n";
                    }
                    return !!param;
                };
                const auto needGlobal = [&] () -> bool {
                    if (param) {
                        err << "(GIPS) '@" << key << "' token is only valid inside a global comment\n";
                    }
                    return !param;
                };
                const auto needValue = [&] () -> bool {
                    if (!value) {
                        err << "(GIPS) '@" << key << "' token requires a value\n";
                    }
                    return !!value;
                };
                const auto needNum = [&] () -> bool {
                    if (!isNum) {
                        err << "(GIPS) '@" << key << "' token requires a numeric value\n";
                    }
                    return isNum;
                };
                const auto setParamType = [&] (GLSLToken dt, ParameterType pt, bool fail=true) -> bool {
                    if (paramDataType != dt) {
                        if (fail) { err << "(GIPS) '@" << key << "' format is incompatible with uniform data type of parameter '" << param->m_name << "'\n"; }
                        return false;
                    } else {
                        param->m_type = pt;
                        return true;
                    }
                };

                // evaluate the tokens
                     if ((isKey("min") || isKey("off")) && needParam() && needNum()) { param->m_minValue = fval; }
                else if ((isKey("max") || isKey("on"))  && needParam() && needNum()) { param->m_maxValue = fval; }
                else if (isKey("digits") && needParam() && needNum()) { param->m_digits = int(fval + 0.5f); }
                else if (isKey("int") && needParam()) { param->m_digits = 0; }
                else if (isKey("unit") && needParam() && needValue()) { param->m_format = value; }
                else if ((isKey("toggle") || isKey("switch")) && needParam()) {
                    setParamType(GLSLToken::Float, ParameterType::Toggle);
                } else if ((isKey("angle")) && needParam()) {
                    setParamType(GLSLToken::Float, ParameterType::Angle);
                } else if (isKey("color")  && needParam()) {
                    setParamType(GLSLToken::Vec3, ParameterType::RGB, false) ||
                    setParamType(GLSLToken::Vec4, ParameterType::RGBA);
                } else if ((isKey("coord") || isKey("coords") || isKey("map")) && needGlobal() && needValue()) {
                         if (isValue("pixel")) { coordMode = CoordMapMode::Pixel; }
                    else if (isValue("none"))  { coordMode = CoordMapMode::None; }
                    else if (isValue("relative") || isValue("rel")) { coordMode = CoordMapMode::Relative; }
                    else { err << "(GIPS) unrecognized coordinate mapping mode '" << value << "'\n"; }
                } else if ((isKey("filter") || isKey("filt")) && needGlobal() && needValue()) {
                         if (isValue("1") || isValue("on")  || isValue("linear")  || isValue("bilinear")) { texFilter = true; }
                    else if (isValue("0") || isValue("off") || isValue("nearest") || isValue("point"))    { texFilter = false; }
                    else { err << "(GIPS) unrecognized texture filtering mode '" << value << "'\n"; }
                } else if ((isKey("version") || isKey("gips_version")) && needGlobal() && needNum()) {
                    if (fval > MaxSupportedVersionCode) {
                        err << "(GIPS) shader requires GIPS version " << fval << ", but only " << MaxSupportedVersionCode << " is supported\n";
                        ::free(comment);
                        goto load_finalize;
                    }
                } else if (!keyMatched) { err << "(GIPS) unrecognized token '@" << key << "'\n"; }

                // delete the token and continue searching for the next token
                memmove(&key[-1], pos, strlen(pos) + 1);
                pos = &key[-1];
            }   // END of comment tokenizer loop

            // if this is a parameter comment, trim and store it
            if (param) {
                content = StringUtil::skipWhitespace(content);
                StringUtil::trimTrailingWhitespace(content);
                if (content[0]) { param->m_desc = content; }
            }

            // done with comment processing
            ::free(comment);
            param = nullptr;  // parameter comment handled, forget about the parameter
            continue;
        }   // END of comment handling

        // add token type to history
        GLSLToken newTT = StringUtil::lookup(tokenMap,tok.token());
        if (newTT == GLSLToken::Ignored) {
            continue;
        }
        for (int i = GLSLTokenHistorySize - 1;  i;  --i) { tt[i] = tt[i-1]; }
        tt[0] = newTT;

        // check for new uniform
        // pattern: [2]="uniform", [1]="float"|"vec3"|"vec4", [0]=name
        if (tt[2] == GLSLToken::Uniform) {
            if ((tt[1] == GLSLToken::Float) || (tt[1] == GLSLToken::Vec2) || (tt[1] == GLSLToken::Vec3) || (tt[1] == GLSLToken::Vec4)) {
                newParams.emplace_back();
                param = &newParams.back();
                param->m_name = std::string(tok.stringFromStart(), tok.length());
                // set a default parameter type based on the data type
                paramDataType = tt[1];
                switch (paramDataType) {
                    case GLSLToken::Float: param->m_type = ParameterType::Value;  break;
                    case GLSLToken::Vec2:  param->m_type = ParameterType::Value2; break;
                    case GLSLToken::Vec3:  param->m_type = ParameterType::Value3; break;
                    case GLSLToken::Vec4:  param->m_type = ParameterType::Value4; break;
                    default: break;
                }
                paramValueIndex = -1;
                inParamStatement = true;
            } else {  // uniform detected, but unsupported type
                err << "(GIPS) uniform variable '" << tok.token() << "' has unsupported data type\n";
                inParamStatement = false;
            }
            continue;
        }

        // check for begin of uniform default value assignment
        if (param && inParamStatement && (paramValueIndex < 0) && tok.contains('=')) {
            paramValueIndex = 0;
            continue;
        }

        // if we're inside a uniform default assignment,
        // check if we can parse the token as a number
        if (param && inParamStatement && (paramValueIndex >= 0) && (paramValueIndex < 4)) {
            char* end = nullptr;
            float f = strtof(tok.token(), &end);
            if (end && !*end) {
                param->m_value[paramValueIndex++] = f;
            }
        }

        // check for end of uniform statement (and, hence, assignment)
        if (tok.contains(';')) {
            inParamStatement = false;
            continue;
        }

        // check pass definition
        // pattern: [3]="vec3/4", [2]="run[_passX]", [1]="(", [0]="vec2/3/4"
        if (((tt[3] == GLSLToken::Vec3) || (tt[3] == GLSLToken::Vec4))
        &&  ((tt[2] == GLSLToken::RunSingle) || (tt[2] == GLSLToken::RunPass1) || (tt[2] == GLSLToken::RunPass2) || (tt[2] == GLSLToken::RunPass3) || (tt[2] == GLSLToken::RunPass4))
        &&   (tt[1] == GLSLToken::OpenParens)
        &&  ((tt[0] == GLSLToken::Vec2) || (tt[0] == GLSLToken::Vec3) || (tt[0] == GLSLToken::Vec4)))
        {
            switch (tt[2]) {
                case GLSLToken::RunSingle: currentPass = 0; singlePass = true; break;
                case GLSLToken::RunPass1:  currentPass = 0; singlePass = false; break;
                case GLSLToken::RunPass2:  currentPass = 1; break;
                case GLSLToken::RunPass3:  currentPass = 2; break;
                case GLSLToken::RunPass4:  currentPass = 3; break;
                default: assert(0);
            }
            passMask |= (1 << currentPass);
            if (currentPass >= MaxPasses) { continue; }
            switch (tt[0]) {
                case GLSLToken::Vec2: inputs[currentPass] = PassInput::Coord; break;
                case GLSLToken::Vec3: inputs[currentPass] = PassInput::RGB;   break;
                case GLSLToken::Vec4: inputs[currentPass] = PassInput::RGBA;  break;
                default: assert(0);
            }
            switch (tt[3]) {
                case GLSLToken::Vec3: outputs[currentPass] = PassOutput::RGB;  break;
                case GLSLToken::Vec4: outputs[currentPass] = PassOutput::RGBA; break;
                default: assert(0);
            }
            // apply pass settings
            m_passes[currentPass].texFilter = texFilter;
            m_passes[currentPass].coordMode = coordMode;
            continue;
        }
    }   // END of GLSL tokenizer loop

    // finalize parameters
    for (auto& p : newParams) {
        // auto-detect number of digits if not set explicitly
        if (p.m_digits < 0) {
            float absMax = std::max(std::abs(p.m_minValue), std::abs(p.m_maxValue));
            p.m_digits = std::max(0, 2 - int(std::floor(std::log10(std::max(absMax, 1e-6f)))));
        }

        // set format string
        std::string fmt = std::string("%.") + std::to_string(p.m_digits) + std::string("f");
        if (p.m_format.empty()) {
            p.m_format = fmt;
        } else {
            p.m_format = fmt + std::string(" ") + p.m_format;
        }

        // copy old parameter values
        Parameter* oldParam = findParam(p.name());
        if (oldParam) {
            for (int i = 0;  i < 4;  ++i) {
                p.m_value[i] = oldParam->m_value[i];
            }
        }
    }

    // first pass defined?
    if (!(passMask & 1)) {
        err << "(GIPS) no valid 'run' or 'run_pass1' function found\n";
        goto load_finalize;
    }

    // generate code for the passes
    for (currentPass = 0;  (currentPass < MaxPasses) && ((passMask >> currentPass) & 1);  ++currentPass) {
        auto& pass = m_passes[currentPass];
        passMask &= ~(1 << currentPass);
        PassInput input = inputs[currentPass];
        PassOutput output = outputs[currentPass];
        if (input != PassInput::Coord) {
            // coordinate remapping not needed (nor wanted) for RGB(A)->RGB(A) filters
            pass.coordMode = CoordMapMode::None;
        }

        // fragment shader assembly: boilerplate
        shader.str("");
        shader.clear();
        shader << "#version 330 core\n"
                  "#line 8000 0\n"
                  "in vec2 gips_pos;\n"
                  "out vec4 gips_frag;\n"
                  "uniform sampler2D gips_tex;\n"
                  "uniform vec2 gips_image_size;\n";
        if (input == PassInput::Coord) {
            shader << "uniform vec4 gips_map2tex;\n"
                      "vec4 pixel(in vec2 pos) {\n"
                      "  return texture(gips_tex, gips_map2tex.xy + pos * gips_map2tex.zw);\n"
                      "}\n";
        }

        // fragment shader assembly: add user code
        shader << "#line 1 " << (currentPass + 1) << "\n" << code;

        // fragment shader assembly: main() function prologue
        shader << "\n#line 9000 0\n"
                  "void main() {\n";
        if (input != PassInput::Coord) {
            // Coord->RGB(A) case: implicit texture lookup
            shader << "  vec4 color = texture(gips_tex, gips_pos);\n";
        }

        // fragment shader assembly: output statement generation
        shader << "  gips_frag = ";
        if (output == PassOutput::RGB) {
            shader << "vec4(";  // if output isn't already RGBA, convert it
        }
        shader << "run";
        if (currentPass || !singlePass) { shader << "_pass" << (currentPass + 1); }
        switch (input) {
            case PassInput::Coord: shader << "(gips_pos)"; break;
            case PassInput::RGB:   shader << "(color.rgb)"; break;
            case PassInput::RGBA:  shader << "(color)"; break;
            default: assert(0);
        }
        if (output == PassOutput::RGB) {
            if (input == PassInput::Coord) {
                shader << ", 1.0)";  // Coord->RGB case: set alpha to 1
            } else {
                shader << ", color.a)";  // RGB(A)->RGB case: keep source alpha
            }
        }
        shader << ";\n}\n";

        // compile shader and link program
        fs.compile(GL_FRAGMENT_SHADER, shader.str().c_str());
        if (fs.haveLog()) { err << fs.getLog() << "\n"; }
        if (!fs.good()) {
            #ifndef NDEBUG
                fprintf(stderr, "----- failed shader source code -----\n%s\n----- end of failed shader code -----\n", shader.str().c_str());
            #endif
            goto load_finalize;
        }
        prog = &pass.program;
        prog->link(vs, fs);
        if (prog->haveLog()) { err << prog->getLog() << "\n"; }
        fs.free();
        if (!prog->good()) { goto load_finalize; }

        // get uniform locations
        prog->use();
        GLutil::checkError("node setup");
        glUniform4f(prog->getUniformLocation("gips_pos2ndc"), -1.0f, -1.0f, 2.0f, 2.0f);
        pass.locImageSize = prog->getUniformLocation("gips_image_size");
        pass.locRel2Map = prog->getUniformLocation("gips_rel2map");
        pass.locMap2Tex = (input == PassInput::Coord) ? prog->getUniformLocation("gips_map2tex") : (-1);
        for (auto& p : newParams) {
            p.m_location[currentPass] = prog->getUniformLocation(p.m_name.c_str());
        }
        GLutil::checkError("node uniform lookup");

        // pass program setup done
        glUseProgram(0);
    }   // END of pass instantiation loop

    // all passes processed?
    if (passMask) {
        err << "(GIPS) intermediate passes are missing, truncating pipeline\n";
    }

    // setup done, proclaim success
    m_passCount = currentPass;

load_finalize:
    if (f) { fclose(f); }
    ::free(code);
    m_errors = err.str();
    m_params = newParams;
    return (m_passCount > 0);
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
