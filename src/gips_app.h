#pragma once

#include <cstdint>

#include <string>

#include <SDL.h>
#include "gl_header.h"
#include "gl_util.h"
#include "imgui.h"

#include "string_util.h"

#include "gips_core.h"

namespace GIPS {

///////////////////////////////////////////////////////////////////////////////

enum class ImageSource {
    Color,
    Image,
    Pattern,
};

class App {
private:
    // paths
    std::string m_appDir;
    std::string m_appUIConfigFile;
    std::string m_shaderDir;

    // SDL and ImGui stuff
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glctx = nullptr;
    ImGuiIO* m_io = nullptr;
    bool m_active = true;
    bool m_showDemo = false;

    // source image
    GLuint m_imgTex = 0;
    ImageSource m_imgSource = ImageSource::Color;
    GLfloat m_imgColor[4] = { 0.1f, 0.4f, 0.7f, 1.0f };
    int m_imgPatternID = 0;
    bool m_imgPatternNoAlpha = true;
    bool m_imgResize = false;
    std::string m_imgFilename;
    std::string m_lastSaveFilename;
    int m_targetImgWidth   = 960;
    int m_targetImgHeight  = 640;
    int m_editTargetWidth  = 960;
    int m_editTargetHeight = 640;
    int m_imgWidth = 0;
    int m_imgHeight = 0;

    // rendering resources
    GLutil::Shader m_vertexShader;
    GLutil::Program m_imgProgram;
    GLint m_imgProgramAreaLoc = -1;
    GLutil::FBO m_helperFBO;

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

    // status message
    enum class StatusType {
        Neutral,
        Success,
        Error,
    };
    std::string m_statusText;
    StatusType m_statusType = StatusType::Neutral;
    bool m_statusVisible = false;

    // "pipeline change requests", basically APCs for modifying the popeline
    struct PipelineChangeRequest {
        enum class Type {
            None,
            InsertNode,
            ReloadNode,
            RemoveNode,
            MoveNode,
            UpdateSource,
            LoadImage,
            SaveResult,
        } type = Type::None;
        int nodeIndex = 0;    //!< node index (1-based) for all operations
        int targetIndex = 0;  //!< target index (for MoveNode only)
        std::string path;     //!< path to load (for LoadNode and SaveResult only)
    } m_pcr;

    bool handleEvents(bool wait);
    bool handlePCR();
    void drawUI();

    bool uploadImageTexture(uint8_t* data, int width, int height, ImageSource src);
    bool loadColor();
    bool loadImage(const char* filename);
    bool loadPattern();
    bool updateImage();

    bool saveResult(const char* filename);

    //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //

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

    inline const char* getShaderDir() const { return m_shaderDir.c_str(); }

    inline int getNodeCount() const { return m_pipeline.nodeCount(); }

    inline void requestInsertNode(const char* filename, int nodeIndex=0)
        { m_pcr.type = PipelineChangeRequest::Type::InsertNode; m_pcr.nodeIndex = nodeIndex; m_pcr.path = filename; }
    inline void requestReloadNode(int nodeIndex)
        { m_pcr.type = PipelineChangeRequest::Type::ReloadNode; m_pcr.nodeIndex = nodeIndex; }
    inline void requestRemoveNode(int nodeIndex)
        { m_pcr.type = PipelineChangeRequest::Type::RemoveNode; m_pcr.nodeIndex = nodeIndex; }
    inline void requestMoveNode(int fromIndex, int toIndex)
        { m_pcr.type = PipelineChangeRequest::Type::MoveNode; m_pcr.nodeIndex = fromIndex; m_pcr.targetIndex = toIndex; }
    inline void requestUpdateSource()
        { m_pcr.type = PipelineChangeRequest::Type::UpdateSource; }
    inline void requestLoadImage(const char* filename)
        { m_pcr.type = PipelineChangeRequest::Type::LoadImage; m_pcr.path = filename; }
    inline void requestSaveResult(const char* filename)
        { m_pcr.type = PipelineChangeRequest::Type::SaveResult; m_pcr.path = filename; }

    inline void setStatus(StatusType type, const char* msg)
        { m_statusType = type; 
          if (msg && msg[0]) { m_statusText = msg; m_statusVisible = true; }
          else { m_statusText.clear(); m_statusVisible = false; } }

    inline bool setError(const std::string& msg)
        { setStatus(StatusType::Error, msg.c_str()); return false; }
    inline bool setError(const char* msg)
        { setStatus(StatusType::Error, msg); return false; }
    inline bool setSuccess(const std::string& msg)
        { setStatus(StatusType::Success, msg.c_str()); return true; }
    inline bool setSuccess(const char* msg=nullptr)
        { setStatus(StatusType::Success, msg); return true; }

    static bool isShaderFile(uint32_t extCode);
    static inline bool isShaderFile(const char* filename)
        { return isShaderFile(StringUtil::extractExtCode(filename)); }

    static bool isImageFile(uint32_t extCode);
    static inline bool isImageFile(const char* filename)
        { return isImageFile(StringUtil::extractExtCode(filename)); }

    static bool isSaveImageFile(uint32_t extCode);
    static inline bool isSaveImageFile(const char* filename)
        { return isSaveImageFile(StringUtil::extractExtCode(filename)); }
};

///////////////////////////////////////////////////////////////////////////////

}  // namespace GIPS
