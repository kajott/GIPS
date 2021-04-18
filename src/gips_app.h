// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

#include <string>
#include <list>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
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

    // GLFW and ImGui stuff
    GLFWwindow* m_window = nullptr;
    ImGuiIO* m_io = nullptr;

    // UI state
    bool m_active = true;
    int m_renderFrames = 2;
    bool m_showWidgets = true;
    bool m_showDemo = false;
    bool m_showInfo = false;
    bool m_showAlpha = true;
    bool m_showDebug =
        #ifdef NDEBUG
            false;
        #else
            true;
        #endif

    // source image
    GLuint m_imgTex = 0;
    ImageSource m_imgSource = ImageSource::Color;
    GLfloat m_imgColor[4] = { 0.1f, 0.4f, 0.7f, 1.0f };
    int m_imgPatternID = 0;
    bool m_imgPatternNoAlpha = true;
    bool m_imgResize = false;
    void* m_clipboardImage = nullptr;
    int m_clipboardWidth = 0;
    int m_clipboardHeight = 0;
    std::string m_imgFilename;
    std::string m_lastSaveFilename;
    int m_targetImgWidth   = 1080;
    int m_targetImgHeight  =  720;
    int m_editTargetWidth  = 1080;
    int m_editTargetHeight =  720;
    int m_imgWidth = 0;
    int m_imgHeight = 0;
    int m_imgMaxSize = 1024;

    // rendering resources
    struct RenderProgram {
        GLutil::Program prog;
        GLint areaLoc = -1;
        bool init(GLuint vs, const char* desc, const char *fsSource);
    };
    RenderProgram m_renderDirect;
    RenderProgram m_renderWithAlpha;
    GLutil::FBO m_helperFBO;

    // GL information
    std::string m_glVendor;
    std::string m_glRenderer;
    std::string m_glVersion;

    // the main event of the show
    Pipeline m_pipeline;
    int m_showIndex = 0;
    PixelFormat m_requestedFormat = PixelFormat::DontCare;

    // image geometry, zoom&pan
    int m_imgX0 = 0;
    int m_imgY0 = 0;
    float m_imgZoom = 1.0f;
    bool m_imgAutofit = true;
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
        Message,
        Success,
        Error,
    };
    std::string m_statusText;
    StatusType m_statusType = StatusType::Message;
    bool m_statusVisible = false;

    // "pipeline change requests", basically APCs for modifying the popeline
    struct PipelineChangeRequest {
        enum class Type {
            None,
            InsertNode,
            ReloadNode,
            RemoveNode,
            MoveNode,
            ClearPipeline,
            UpdateSource,
            HandleFile,
            SaveResult,
            LoadClipboard,
            SaveClipboard,
        } type = Type::None;
        int nodeIndex = 0;    //!< node index (1-based) for all operations
        int targetIndex = 0;  //!< target index (for MoveNode only)
        std::string path;     //!< path to load (for LoadNode and SaveResult only)
    } m_pcr;

    // auto-test mode
    std::list<std::string> m_autoTestList;
    int m_autoTestTotal = 0;
    int m_autoTestDone = 0;
    int m_autoTestOK = 0;
    int m_autoTestWarn = 0;
    int m_autoTestFail = 0;

    // event and PCR handling
    void handleKeyEvent(int key, int scancode, int action, int mods);
    void handleMouseButtonEvent(int button, int action, int mods);
    void handleCursorPosEvent(double xpos, double ypos);
    void handleScrollEvent(double xoffset, double yoffset);
    void handleDropEvent(int path_count, const char* paths[]);
    bool handlePCR();

    // UI functions (implemented in gips_ui.cpp)
    void drawUI();
    void showLoadUI(bool withShaders=true);
    void showSaveUI();

    // image source modification functions
    bool uploadImageTexture(uint8_t* data, int width, int height, ImageSource src, bool mustFreeData=true);
    bool loadColor();
    bool loadImage(const char* filename, bool useClipboard=false, bool updateClipboard=false);
    bool loadPattern();
    bool updateImage();

    // image result saving
    bool saveResult(const char* filename, bool toClipboard=false);

    // auto-test mode implementation
    void startAutoTest(const char* scanDir=nullptr);
    inline bool autoTestInProgress() const { return (m_autoTestTotal > 0); }
    void handleAutoTest();

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

    inline int getNodeCount() const { return m_pipeline.nodeCount() + 1; }
    inline Node* getNode(int idx) { return ((idx > 0) && (idx <= m_pipeline.nodeCount())) ? &m_pipeline.node(idx - 1) : nullptr; }

    inline void requestInsertNode(const char* filename, int nodeIndex=0)
        { m_pcr.type = PipelineChangeRequest::Type::InsertNode; m_pcr.nodeIndex = nodeIndex; m_pcr.path = filename; }
    inline void requestReloadNode(int nodeIndex)
        { m_pcr.type = PipelineChangeRequest::Type::ReloadNode; m_pcr.nodeIndex = nodeIndex; }
    inline void requestRemoveNode(int nodeIndex)
        { m_pcr.type = PipelineChangeRequest::Type::RemoveNode; m_pcr.nodeIndex = nodeIndex; }
    inline void requestMoveNode(int fromIndex, int toIndex)
        { m_pcr.type = PipelineChangeRequest::Type::MoveNode; m_pcr.nodeIndex = fromIndex; m_pcr.targetIndex = toIndex; }
    inline void requestClearPipeline()
        { m_pcr.type = PipelineChangeRequest::Type::ClearPipeline; }
    inline void requestUpdateSource()
        { m_pcr.type = PipelineChangeRequest::Type::UpdateSource; }
    inline void requestHandleFile(const char* filename)
        { m_pcr.type = PipelineChangeRequest::Type::HandleFile; m_pcr.path = filename; }
    inline void requestSaveResult(const char* filename)
        { m_pcr.type = PipelineChangeRequest::Type::SaveResult; m_pcr.path = filename; }
    inline void requestLoadClipboard()
        { m_pcr.type = PipelineChangeRequest::Type::LoadClipboard; }
    inline void requestSaveClipboard()
        { m_pcr.type = PipelineChangeRequest::Type::SaveClipboard; }

    inline void requestFrames(int n)
        { if (n > m_renderFrames) { m_renderFrames = n; } }

    inline void setStatus(StatusType type, const char* msg)
        { m_statusType = type; requestFrames(2);
          if (msg && msg[0]) { m_statusText = msg; m_statusVisible = true; }
          else { m_statusText.clear(); m_statusVisible = false; } }

    inline bool setMessage(const std::string& msg)
        { setStatus(StatusType::Message, msg.c_str()); return false; }
    inline bool setMessage(const char* msg)
        { setStatus(StatusType::Message, msg); return false; }
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
