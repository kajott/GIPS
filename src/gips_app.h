#pragma once

#include <string>

#include <SDL.h>
#include "gl_header.h"
#include "imgui.h"
#include "gl_util.h"

#include "gips_core.h"

namespace GIPS {

class App {
private:
    // SDL and ImGui stuff
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glctx = nullptr;
    ImGuiIO* m_io = nullptr;
    bool m_active = true;
    bool m_showDemo = false;

    // source image
    GLuint m_imgTex = 0;
    std::string m_imgFilename;
    bool m_imgLoaded = false;
    int m_targetImgWidth = 768;
    int m_targetImgHeight = 576;
    int m_imgWidth = 0;
    int m_imgHeight = 0;

    // rendering resources
    GLutil::Shader m_vertexShader;
    GLutil::Program m_imgProgram;
    GLint m_imgProgramAreaLoc = -1;

    // the main event of the show
    Pipeline m_pipeline;
    int m_showIndex = 0;

    // image geometry, zoom&pan
    int m_imgX0 = 0;
    int m_imgY0 = 0;
    float m_imgZoom = 1.0f;
    float m_imgAutofit = true;
    int m_panRefX = 0;
    int m_panRefY = 0;
    bool m_panning = false;
    float getFitZoom();
    void updateImageGeometry();
    void panStart(int x, int y);
    void panUpdate(int x, int y);
    void zoomAt(int x, int y, int step);

    // "pipeline change requests", basically APCs for modifying the popeline
    struct PipelineChangeRequest {
        enum class Type {
            None,
            LoadNode,
            ReloadNode,
            RemoveNode,
            MoveNode,
        } type = Type::None;
        int nodeIndex = 0;    //!< node index (1-based) for all operations
        int targetIndex = 0;  //!< target index (for MoveNode only)
        std::string path;     //!< path to load (for LoadNode only)
    } m_pcr;

    bool handleEvents(bool wait);
    bool handlePCR();
    void drawUI();

    bool loadImage(const std::string& filename);


public:
    inline App() {}
    int run(int argc, char* argv[]);

    void handleInputFile(const char* filename);

    //! return the number of the currently shown filter (1-based; 0=input)
    inline int getShowIndex() const { return m_showIndex; }

    //! set the number of the currently shown filter (1-based; 0=input)
    inline int setShowIndex(int i) {
        m_showIndex = (i < 0) ? 0 : (i > m_pipeline.nodeCount()) ? m_pipeline.nodeCount() : i;
        return m_showIndex;
    }

    inline int getNodeCount() const { return m_pipeline.nodeCount(); }

    inline void requestLoadNode(const char* filename, int nodeIndex=0)
        { m_pcr.type = PipelineChangeRequest::Type::LoadNode; m_pcr.nodeIndex = nodeIndex; m_pcr.path = filename; }
    inline void requestReloadNode(int nodeIndex)
        { m_pcr.type = PipelineChangeRequest::Type::ReloadNode; m_pcr.nodeIndex = nodeIndex; }
    inline void requestRemoveNode(int nodeIndex)
        { m_pcr.type = PipelineChangeRequest::Type::RemoveNode; m_pcr.nodeIndex = nodeIndex; }
    inline void requestMoveNode(int fromIndex, int toIndex)
        { m_pcr.type = PipelineChangeRequest::Type::MoveNode; m_pcr.nodeIndex = fromIndex; m_pcr.targetIndex = toIndex; }
};

}  // namespace GIPS
