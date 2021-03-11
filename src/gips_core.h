#pragma once

#include <string>
#include <vector>

#include "gl_header.h"
#include "gl_util.h"

namespace GIPS {

constexpr int MaxPasses = 4;


enum class ParameterType {
    Value,
    RGB,
    RGBA,
};


class Parameter {
    friend class Node;
    friend class Pipeline;
    std::string m_name;
    std::string m_desc;
    ParameterType m_type = ParameterType::Value;
    float m_minValue     = 0.0f;
    float m_maxValue     = 1.0f;
    float m_value[4]     = { 0.0f, };
    float m_oldValue[4]  = { 0.0f, };
    GLint m_location[MaxPasses] = { 0, };
public:
    inline Parameter() {}
    bool changed();
    inline const std::string&  name()     const { return m_name; }
    inline const std::string&  desc()     const { return m_desc.empty() ? m_name : m_desc; }
    inline const ParameterType type()     const { return m_type; }
    inline const float*        value()    const { return m_value; }
    inline       float*        value()          { return m_value; }
    inline const float         minValue() const { return m_minValue; }
    inline const float         maxValue() const { return m_maxValue; }
};


class Node {
    friend class Pipeline;
    std::string m_name;
    std::string m_filename;
    std::string m_errors;
    int m_passCount = 0;
    struct PassData {
        GLutil::Program program;
        inline PassData() {}
    } m_passes[MaxPasses];
    std::vector<Parameter> m_params;
    bool m_programChanged = true;
    bool m_enabled = true;
    bool m_wasEnabled = false;

public:
    bool load(const char* filename, const GLutil::Shader& vs);

    bool changed();

    inline const std::string& name()       const { return m_name; }
    inline const std::string& filename()   const { return m_filename; }
    inline const std::string& errors()     const { return m_errors; }
    inline const int          passCount()  const { return m_passCount; }
    inline const bool         enabled()    const { return m_enabled; }
    inline const int          paramCount() const { return int(m_params.size()); }
    inline const Parameter&   param(int i) const { return m_params[i]; }
    inline       Parameter&   param(int i)       { return m_params[i]; }

    inline void setEnabled(bool e) { m_enabled = e; }
    inline void enable()           { m_enabled = true; }
    inline void disable()          { m_enabled = false; }
    inline bool toggle()           { m_enabled = !m_enabled; return m_enabled; }

    inline Node() {}
    inline Node(const char* filename, const GLutil::Shader& vs) { load(filename, vs); }
    Node(const Node&) = delete;
};


class Pipeline {
    std::vector<Node*> m_nodes;
    int m_width = 0;
    int m_height = 0;
    GLuint m_tex[2] = {0,0};
    GLuint m_fbo = 0;
    bool m_pipelineChanged = true;
    GLutil::Shader m_vs;
    GLuint m_resultTex = 0;
    bool m_initialized = false;
    bool m_initOK = false;

public:
    bool init();
    inline const GLutil::Shader& vs() const { return m_vs; }
    inline const bool   good()        const { return m_initOK; }
    inline const GLuint resultTex()   const { return m_resultTex; }
    inline const int    nodeCount()   const { return int(m_nodes.size()); }
    inline const Node&  node(int i)   const { return *m_nodes[i]; }
    inline       Node&  node(int i)         { return *m_nodes[i]; }
    Node* addNode(int index=-1);
    inline Node* addNode(const char* filename, int index=-1) {
        Node* n = addNode(index);
        if (n) { n->load(filename, m_vs); }
        return n;
    }
    void removeNode(int index);
    bool changed();
    inline void  markAsChanged() { m_pipelineChanged = true; }
    void render(GLuint srcTex, int width, int height, int maxNodes=-1);

    inline Pipeline() {}
    Pipeline(const Pipeline&) = delete;
    ~Pipeline();
};


}  // namespace GIPS