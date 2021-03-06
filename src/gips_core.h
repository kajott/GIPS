// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>
#include <type_traits>

#include "gl_header.h"
#include "gl_util.h"
#include "file_util.h"

namespace GIPS {

constexpr int MaxPasses = 4;


enum class ParameterType {
    Value,
    Value2,
    Value3,
    Value4,
    Toggle,
    Angle,
    RGB,
    RGBA,
};


enum class CoordMapMode {
    None,
    Pixel,
    Relative,
};


enum class PixelFormat {
    DontCare =   0,
    Int8     =   8,
    Int16    =  16,
    Float16  = 116,
    Float32  = 132,
};
inline bool operator< (const PixelFormat a, const PixelFormat b) {
    using ut = std::underlying_type<PixelFormat>::type;
    return static_cast<ut>(a) < static_cast<ut>(b);
}
int getBytesPerPixel(PixelFormat fmt);
const char* pixelFormatName(PixelFormat fmt);


class Parameter {
    friend class Node;
    friend class Pipeline;
    std::string m_name;
    std::string m_desc;
    std::string m_format;
    ParameterType m_type        = ParameterType::Value;
    int m_digits                = -1;
    float m_minValue            = 0.0f;
    float m_maxValue            = 1.0f;
    float m_value[4]            = { 0.0f, };
    float m_oldValue[4]         = { 0.0f, };
    float m_defaultValue[4]     = { 0.0f, };
    GLint m_location[MaxPasses] = { 0, };
public:
    inline Parameter() {}
    bool changed();
    void reset();
    inline const char*   name()     const { return m_name.c_str(); }
    inline const char*   desc()     const { return m_desc.empty() ? m_name.c_str() : m_desc.c_str(); }
    inline const char*   format()   const { return m_format.empty() ? "%.2f" : m_format.c_str(); }
    inline ParameterType type()     const { return m_type; }
    inline const float*  value()    const { return m_value; }
    inline       float*  value()          { return m_value; }
    inline       float   minValue() const { return m_minValue; }
    inline       float   maxValue() const { return m_maxValue; }
};


class Node {
    friend class Pipeline;
    std::string m_name;
    std::string m_filename;
    std::string m_errors;
    int m_passCount = 0;
    struct PassData {
        bool texFilter = true;
        CoordMapMode coordMode = CoordMapMode::None;
        GLutil::Program program;
        GLint locImageSize = -1;
        GLint locRel2Map = -1;
        GLint locMap2Tex = -1;
        inline PassData() {}
    } m_passes[MaxPasses];
    std::vector<Parameter> m_params;
    bool m_programChanged = true;
    bool m_enabled = true;
    bool m_wasEnabled = false;
    FileUtil::FileFingerprint m_fp;
    PixelFormat m_preferredFormat = PixelFormat::DontCare;

public:
    bool load(const char* filename, const GLutil::Shader& vs, const FileUtil::FileFingerprint* fp=nullptr);
    bool reload(const GLutil::Shader& vs, bool force=false);

    bool changed();
    void reset();

    inline const char*      name()       const { return m_name.c_str(); }
    inline const char*      filename()   const { return m_filename.c_str(); }
    inline       bool       hasErrors()  const { return !m_errors.empty(); }
    inline const char*      errors()     const { return m_errors.c_str(); }
    inline       bool       good()       const { return (m_passCount > 0); }
    inline       int        passCount()  const { return m_passCount; }
    inline       bool       enabled()    const { return m_enabled; }
    inline       int        paramCount() const { return int(m_params.size()); }
    inline const Parameter& param(int i) const { return m_params[size_t(i)]; }
    inline       Parameter& param(int i)       { return m_params[size_t(i)]; }

    inline void setEnabled(bool e) { m_enabled = e; }
    inline void enable()           { m_enabled = true; }
    inline void disable()          { m_enabled = false; }
    inline bool toggle()           { m_enabled = !m_enabled; return m_enabled; }

    Parameter* findParam(const char* name);

    inline Node() {}
    inline Node(const char* filename, const GLutil::Shader& vs) { load(filename, vs); }
    Node(const Node&) = delete;
};


class Pipeline {
    std::vector<Node*> m_nodes;
    int m_width = 0;
    int m_height = 0;
    PixelFormat m_format = PixelFormat::DontCare;
    GLuint m_tex[2] = {0,0};
    GLutil::FBO m_fbo;
    bool m_pipelineChanged = true;
    GLutil::Shader m_vs;
    GLuint m_resultTex = 0;
    bool m_initialized = false;
    bool m_initOK = false;
    float m_lastRenderTime_ms = 0.0f;

public:
    bool init();
    inline const GLutil::Shader& vs()        const { return m_vs; }
    inline       bool            good()      const { return m_initOK; }
    inline       GLuint          resultTex() const { return m_resultTex; }
    inline       PixelFormat     format()    const { return m_format; }
    inline       float lastRenderTime_ms()   const { return m_lastRenderTime_ms; }
    inline       int             nodeCount() const { return int(m_nodes.size()); }
    inline const Node&           node(int i) const { return *m_nodes[size_t(i)]; }
    inline       Node&           node(int i)       { return *m_nodes[size_t(i)]; }
    Node* addNode(int index=-1);
    inline Node* addNode(const char* filename, int index=-1) {
        Node* n = addNode(index);
        if (n) { n->load(filename, m_vs); }
        return n;
    }
    void removeNode(int index);
    void moveNode(int fromIndex, int toIndex);

    bool changed();
    inline void  markAsChanged() { m_pipelineChanged = true; }

    void reload(bool force=false);
    void clear();

    void render(GLuint srcTex, int width, int height, PixelFormat format=PixelFormat::DontCare, int maxNodes=-1);

    PixelFormat detectFormat() const;

    std::string serialize(int showIndex);
    int unserialize(char* data);

    inline Pipeline() {}
    Pipeline(const Pipeline&) = delete;
    void free();
    inline ~Pipeline() { free(); }
};


}  // namespace GIPS
