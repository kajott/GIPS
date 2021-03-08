#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <SDL.h>
#include "gl_header.h"
#include "gl_util.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include "stb_image.h"

#include "gips_app.h"

namespace GIPS {

int App::run(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    #ifndef NDEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,        SDL_GL_CONTEXT_DEBUG_FLAG);
    #endif

    SDL_Window *window = SDL_CreateWindow("GIPS",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1080, 720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_MakeCurrent(window, context);
    SDL_GL_SetSwapInterval(1);

    #ifdef GL_HEADER_IS_GLAD
        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            fprintf(stderr, "failed to load OpenGL 3.3 functions\n");
            return 1;
        }
    #else
        #error no valid GL header / loader
    #endif
    if (!GLutil::init()) {
        fprintf(stderr, "OpenGL initialization failed\n");
        return 1;
    }
    GLutil::enableDebugMessages();

    glClearColor(0.125f, 0.125f, 0.125f, 1.0f);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init(nullptr);

    uint8_t* image = nullptr;
    int imageWidth = 0, imageHeight = 0;
    if (argc > 1) {
        image = stbi_load(argv[1], &imageWidth, &imageHeight, nullptr, 4);
    }
    if (!image) {
        imageWidth = 640;
        imageHeight = 480;
        image = (uint8_t*)malloc(imageWidth * imageHeight * 4);
        assert(image != nullptr);
        auto p = image;
        for (int y = 0;  y < imageHeight;  ++y) {
            for (int x = 0;  x < imageWidth;  ++x) {
                *p++ = uint8_t(x);
                *p++ = uint8_t(y);
                *p++ = uint8_t(x ^ y);
                *p++ = 255;
            }
        }
    }

    GLuint imgTex = 0;
    glGenTextures(1, &imgTex);
    glBindTexture(GL_TEXTURE_2D, imgTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    GLutil::checkError("texture setup");

    GLutil::Shader vs(GL_VERTEX_SHADER,
         "#version 330 core"
    "\n" "uniform vec4 gips_screen_area;"
    "\n" "uniform vec4 gips_tex_area;"
    "\n" "out vec2 gips_tc;"
    "\n" "void main() {"
    "\n" "  vec2 pos = vec2(float(gl_VertexID & 1), float((gl_VertexID & 2) >> 1));"
    "\n" "  gips_tc = gips_tex_area.xy + pos * gips_tex_area.zw;"
    "\n" "  gl_Position = vec4(gips_screen_area.xy + pos * gips_screen_area.zw, 0., 1.);"
    "\n" "}"
    "\n");
    GLutil::checkError("VS compilation");
    puts(vs.getLog());
    GLutil::Shader fs(GL_FRAGMENT_SHADER,
         "#version 330 core"
    "\n" "uniform sampler2D gips_tex;"
    "\n" "in vec2 gips_tc;"
    "\n" "out vec4 gips_frag;"
    "\n" "void main() {"
    "\n" "  //gips_frag = vec4(1., 1., 1., 1.);"
    "\n" "  gips_frag = texture(gips_tex, gips_tc);"
    "\n" "}"
    "\n");
    GLutil::checkError("FS compilation");
    puts(fs.getLog());
    GLutil::Program prog(vs, fs);
    GLutil::checkError("linking");
    puts(prog.getLog());

    prog.use();
    GLutil::checkError("program activation");
    GLint locScreenArea = prog.getUniformLocation("gips_screen_area");
    GLint locTexArea = prog.getUniformLocation("gips_tex_area");
    GLutil::checkError("uniform lookup");

    glUniform4f(locScreenArea, -1.f, 1.f, 2.f, -2.f);
    glUniform4f(locTexArea, 0.f, 0.f, 1.f, 1.f);
    GLutil::checkError("uniform setup");

    for (bool active = true;  active;) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            switch (ev.type) {
                case SDL_QUIT:
                    active = false;
                    break;
                default:
                    break;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        ImGui::Text("Hi from ImGui!"); 

        ImGui::Render();

        glViewport(0, 0, int(io.DisplaySize.x), int(io.DisplaySize.y));
        glClear(GL_COLOR_BUFFER_BIT);
        GLutil::checkError("viewport&clear");

        prog.use();
        glDisable(GL_CULL_FACE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        GLutil::checkError("image draw");

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        GLutil::checkError("GUI draw");
        SDL_GL_SwapWindow(window);
        GLutil::checkError("swap");
    }

    ::free(image);

    GLutil::done();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

}  // namespace GIPS
