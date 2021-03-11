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

    void handleEvents();
    void drawUI();

public:
    inline App() {}

    bool loadImage(const std::string& filename);

    int run(int argc, char* argv[]);
};

}  // namespace GIPS
