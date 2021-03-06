// SPDX-FileCopyrightText: 2021 Martin J. Fiedler <keyj@emphy.de>
// SPDX-License-Identifier: MIT

#include <algorithm>

#include "imgui.h"

#include "libs.h"  // includes wrappers for pfd functions

#include "sysinfo.h"
#include "string_util.h"
#include "vfs.h"
#include "clipboard.h"
#include "patterns.h"

#include "gips_app.h"

#include "gips_version.h"
extern "C" const char* git_rev;
extern "C" const char* git_branch;

///////////////////////////////////////////////////////////////////////////////

struct StatusWindow {
    bool shallShow;
    int nPopColor;
    StatusWindow(const char* name, float ax, float ay, bool *p_open=nullptr, uint32_t color=0) {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(
            vp->WorkPos.x + ax * vp->WorkSize.x,
            vp->WorkPos.y + ay * vp->WorkSize.y
        ), ImGuiCond_Always, ImVec2(ax, ay));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(20.0f, 20.0f));
        nPopColor = 0;
        if (color) {
            ImGui::PushStyleColor(ImGuiCol_TitleBg,       color);  ++nPopColor;
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, color);  ++nPopColor;
            ImGui::PushStyleColor(ImGuiCol_WindowBg,      color);  ++nPopColor;
        }
        ImGui::SetNextWindowBgAlpha(0.375f);
        shallShow = ImGui::Begin(name, p_open,
                    (p_open ? 0 : ImGuiWindowFlags_NoTitleBar) |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoNav |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize | 
                    ImGuiWindowFlags_NoCollapse);
    }
    ~StatusWindow() {
        ImGui::End();
        ImGui::PopStyleVar(2);
        if (nPopColor) { ImGui::PopStyleColor(nPopColor); }
    }
};

///////////////////////////////////////////////////////////////////////////////

struct ButtonColorOverride {
    ButtonColorOverride(float r, float g, float b) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(r, g, b, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(r * 1.125f, g * 1.125f, b * 1.125f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.25f + 0.75f * r, 0.25f + 0.75f * g, 0.25f + 0.75f * b, 1.0f));
    }
    explicit ButtonColorOverride(int rgb24) : ButtonColorOverride((rgb24 >> 16) / 255.0f, ((rgb24 >> 8) & 0xFF) / 255.0f, (rgb24 & 0xFF) / 255.0f) {}
    ~ButtonColorOverride() {
        ImGui::PopStyleColor(3);
    }
};

///////////////////////////////////////////////////////////////////////////////

static void ShaderBrowserMenu(GIPS::App& app, int nodeIndex, const char* dir) {
    const auto& list = VFS::getCachedDirList(dir);
    for (const auto& item : list.items) {
        if (item.isDir) {
            if (ImGui::BeginMenu(item.nameNoExt.c_str())) {
                ShaderBrowserMenu(app, nodeIndex, item.relPath.c_str());
                ImGui::EndMenu();
                app.requestFrames(1);
            }
        } else if (app.isShaderFile(item.fullPath.c_str())) {
            if (ImGui::Selectable(item.nameNoExt.c_str())) {
                app.requestInsertNode(item.fullPath.c_str(), nodeIndex);
            }
        }
    }
}

static bool TreeNodeForGIPSNode(GIPS::App& app, int nodeIndex=0, GIPS::Node* node=nullptr) {
    ImGui::AlignTextToFramePadding();
    bool open = ImGui::TreeNodeEx(node ? node->name() : "Input Image",
        ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);

    // add context menu
    if (node && ImGui::BeginPopupContextItem("node context menu popup")) {
        if (ImGui::BeginMenu("insert")) {
            ShaderBrowserMenu(app, nodeIndex, "");
            ImGui::EndMenu();
        }

        // move menus
        bool mayMoveUp = (nodeIndex > 1);
        bool mayMoveDown = (nodeIndex < (app.getNodeCount() - 1));
        bool mayMoveTo = (app.getNodeCount() > 2);
        if (mayMoveUp || mayMoveDown || mayMoveTo) {
            ImGui::Separator();
        }
        if (mayMoveUp && ImGui::Selectable("move up")) {
            app.requestMoveNode(nodeIndex, nodeIndex - 1);
        }
        if (mayMoveDown && ImGui::Selectable("move down")) {
            app.requestMoveNode(nodeIndex, nodeIndex + 1);
        }
        if (mayMoveTo && ImGui::BeginMenu("move to")) {
            for (int subIndex = 0;  subIndex < app.getNodeCount();  ++subIndex) {
                ImGui::PushID(subIndex);
                if (subIndex == nodeIndex) {
                    ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
                }
                ImGui::TextUnformatted(subIndex ? app.getNode(subIndex)->name() : "Input Image");
                if (subIndex == nodeIndex) {
                    ImGui::PopStyleColor(1);
                }
                if ((subIndex != nodeIndex) && (subIndex != (nodeIndex - 1))) {
                    if (ImGui::Selectable("##move")) {
                        app.requestMoveNode(nodeIndex, (subIndex < nodeIndex) ? (subIndex + 1) : subIndex);
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();

        if (ImGui::BeginMenu("filename")) {
            ImGui::TextUnformatted(node->filename());
            ImGui::Text("relative path: %s", VFS::getRelPath(node->filename()));
            ImGui::EndMenu();
        }
        if (ImGui::Selectable("restore defaults")) {
            node->reset();
        }
        if (ImGui::Selectable("reload")) {
            app.requestReloadNode(nodeIndex);
        }
        if (ImGui::Selectable("remove")) {
            app.requestRemoveNode(nodeIndex);
        }
        ImGui::EndPopup();
    }   // END node header context menu

    // add node toggle and show index buttons
    if (node) {
        ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 55.0f);
        ButtonColorOverride _(node->enabled() ? 0x208020 : 0x802020);
        if (ImGui::Button(node->enabled() ? "On" : "Off")) { node->toggle(); }
    }
    {
        ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 24.0f);
        ButtonColorOverride _((app.getShowIndex() == nodeIndex) ? 0xC0C040 : 0x405060);
        if (ImGui::Button("Show")) { app.setShowIndex(nodeIndex); }
    }
    return open;
}

///////////////////////////////////////////////////////////////////////////////

void GIPS::App::drawUI() {
    // status windows ("widgets")
    if (m_showWidgets) {
        // mouse position status
        bool mouseValid = ImGui::IsMousePosValid();
        float mpx = 0.0f, mpy = 0.0f;
        if (mouseValid) {
            mpx = (m_io->MousePos.x - float(m_imgX0)) / m_imgZoom;
            mpy = (m_io->MousePos.y - float(m_imgY0)) / m_imgZoom;
            if ((mpx < 0.0f) || (mpy < 0.0f) || (mpx >= float(m_imgWidth)) || (mpy >= float(m_imgHeight))) {
                mouseValid = false;
            }
        }
        if (mouseValid) {
            StatusWindow _("Mouse Position", 0.0f, 1.0f);
            ImGui::Text("%d,%d", int(mpx), int(mpy));
        }

        // zoom status
        if (!m_imgAutofit || (m_imgZoom >= 0.99f)) {
            StatusWindow _("Zoom", 1.0f, 1.0f);
            if (m_imgZoom >= 0.99f) {
                ImGui::Text("%.0fx", m_imgZoom);
            } else {
                ImGui::Text("1/%.0fx", 1.0f / m_imgZoom);
            }
        }
    }

    // status message
    if (m_statusVisible) {
        StatusWindow _(
            (m_statusType == StatusType::Error)   ?   "Error##statusMsg" :
            (m_statusType == StatusType::Success) ? "Success##statusMsg" :
                                                    "Message##statusMsg",
            0.5f, 1.0f, &m_statusVisible,
            (m_statusType == StatusType::Error)   ? 0xFF0000C0 :
            (m_statusType == StatusType::Success) ? 0xFF00A000 :
                                                    0);
        if (_.shallShow) {
            ImGui::TextUnformatted(m_statusText.c_str());
        }
    }

    // main window begin
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 480.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Filters", nullptr, ImGuiWindowFlags_MenuBar)) {
        int oldShowIndex = m_showIndex;

        // main menu
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open ...", "Ctrl+O")) { showLoadUI(); }
                if (ImGui::MenuItem("Save ...", "Ctrl+S")) { showSaveUI(true); }
                if (ImGui::MenuItem("Save As ...", "Ctrl+Shift+S")) { showSaveUI(); }
                ImGui::Separator();
                if (Clipboard::isAvailable()) {
                    if (ImGui::MenuItem("Paste from Clipboard", "Ctrl+V")) { requestLoadClipboard(); }
                    if (ImGui::MenuItem("Copy to Clipboard", "Ctrl+C")) { requestSaveClipboard(); }
                    ImGui::Separator();
                }
                if (ImGui::BeginMenu("Clear Pipeline", (m_pipeline.nodeCount() > 0))) {
                    if (ImGui::MenuItem("Yes, really clear.")) { requestClearPipeline(); }
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::BeginMenu("Quit")) {
                    if (ImGui::MenuItem("Yes, really quit.", "Ctrl+Q")) { m_active = false; }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Options")) {
                if (ImGui::BeginMenu("Pipeline Pixel Format")) {
                    std::string dc("autodetect (");
                    bool sel = (m_requestedFormat == GIPS::PixelFormat::DontCare);
                    if (ImGui::MenuItem((dc + GIPS::pixelFormatName(m_pipeline.detectFormat()) + ")").c_str(), nullptr, &sel)) {
                        m_requestedFormat = PixelFormat::DontCare;
                        m_pipeline.markAsChanged();
                    }
                    auto handlePixelFormat = [this] (GIPS::PixelFormat fmt) {
                        bool sel = (m_requestedFormat == fmt);
                        if (ImGui::MenuItem(GIPS::pixelFormatName(fmt), nullptr, &sel)) {
                            m_requestedFormat = fmt;
                            m_pipeline.markAsChanged();
                        }
                    };
                    handlePixelFormat(GIPS::PixelFormat::Int8);
                    handlePixelFormat(GIPS::PixelFormat::Int16);
                    handlePixelFormat(GIPS::PixelFormat::Float16);
                    handlePixelFormat(GIPS::PixelFormat::Float32);
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                ImGui::MenuItem("Show Coordinates", nullptr, &m_showWidgets);
                ImGui::MenuItem("Show Alpha Checkerboard", nullptr, &m_showAlpha);
                if (m_showDebug) {
                    ImGui::Separator();
                    if (ImGui::BeginMenu("Debug")) {
                        if (ImGui::MenuItem("Clear Pipeline + Auto-Test All Shaders")) {
                            startAutoTest();
                        }
                        #ifndef NDEBUG
                            ImGui::MenuItem("Show ImGui Demo", nullptr, &m_showDemo);
                        #endif
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Info")) { m_showInfo = true; }

            ImGui::EndMenuBar();
        }   // END main menu

        // input image
        if (TreeNodeForGIPSNode(*this)) {
            // source
            int src = static_cast<int>(m_imgSource);
            ImGui::RadioButton("Image",   &src, static_cast<int>(ImageSource::Image));
            ImGui::SameLine();
            ImGui::RadioButton("Color",   &src, static_cast<int>(ImageSource::Color));
            ImGui::SameLine();
            ImGui::RadioButton("Pattern", &src, static_cast<int>(ImageSource::Pattern));
            if (src != static_cast<int>(m_imgSource)) {
                m_imgSource = static_cast<ImageSource>(src);
                requestUpdateSource();
            }

            // image source
            if (m_imgSource == ImageSource::Image) {
                if (ImGui::Button("Open ...")) { showLoadUI(true); }
                ImGui::SameLine();
                ImGui::TextUnformatted(m_clipboardImage ? "(Clipboard)" : m_imgFilename.c_str());
                if (ImGui::Checkbox("resize to target size if larger", &m_imgResize)) {
                    requestUpdateSource();
                }
            }

            // color source
            if (m_imgSource == ImageSource::Color) {
                if (ImGui::ColorEdit4("", m_imgColor)) {
                    requestUpdateSource();
                }
            }

            // pattern source
            if (m_imgSource == ImageSource::Pattern) {
                struct FuncWrapper {
                    static bool getPatternName(void*, int idx, const char **p_str) {
                        *p_str = Patterns[idx].name;
                        return true;
                    }
                };
                if (ImGui::Combo("##pat", &m_imgPatternID, &FuncWrapper::getPatternName, nullptr, NumPatterns)) {
                    requestUpdateSource();
                }
                if (ImGui::Checkbox("opaque alpha channel", &m_imgPatternNoAlpha)) {
                    requestUpdateSource();
                }
            }

            // target size
            if ((m_imgSource == ImageSource::Image) && !m_imgResize) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Target Size:");
            ImGui::SameLine();
            ImGui::PushItemWidth(40.0f);
            ImGui::InputInt("##tw", &m_editTargetWidth, 0);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::TextUnformatted("x");
            ImGui::SameLine();
            ImGui::PushItemWidth(40.0f);
            ImGui::InputInt("##th", &m_editTargetHeight, 0);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Apply")) {
                m_targetImgWidth  = m_editTargetWidth  = std::min(m_editTargetWidth,  m_imgMaxSize);
                m_targetImgHeight = m_editTargetHeight = std::min(m_editTargetHeight, m_imgMaxSize);
                requestUpdateSource();
            }
            if ((m_imgSource == ImageSource::Image) && !m_imgResize) {
                ImGui::PopStyleVar(1);
            }

            ImGui::Text("Current Size: %dx%d", m_imgWidth, m_imgHeight);
            ImGui::TreePop();
        }

        // processing nodes
        for (int nodeIndex = 1;  nodeIndex <= m_pipeline.nodeCount();  ++nodeIndex) {
            auto& node = m_pipeline.node(nodeIndex - 1);
            ImGui::PushID(nodeIndex);
            if (TreeNodeForGIPSNode(*this, nodeIndex, &node)) {
                // parameters
                for (int paramIndex = 0 ;  paramIndex < node.paramCount();  ++paramIndex) {
                    auto& param = node.param(paramIndex);
                    bool ctlOK = true;
                    ImGui::PushID(paramIndex);
                    switch (param.type()) {
                        case ParameterType::Toggle: {
                            bool checked = std::abs(param.value()[0] - param.maxValue())
                                         < std::abs(param.value()[0] - param.minValue());
                            if (ImGui::Checkbox(param.desc(), &checked)) {
                                param.value()[0] = checked ? param.maxValue() : param.minValue();
                            }
                            break; }
                        case ParameterType::Angle:
                            ImGui::SliderAngle(param.desc(), param.value(),
                                (std::abs(param.minValue()) < 5.0f) ? (param.minValue() * 360.0f) : param.minValue(),
                                (std::abs(param.maxValue()) < 5.0f) ? (param.maxValue() * 360.0f) : param.maxValue());
                            break;
                        case ParameterType::Value:
                            ImGui::SliderFloat(param.desc(), param.value(), param.minValue(), param.maxValue(), param.format());
                            break;
                        case ParameterType::Value2:
                            ImGui::SliderFloat2(param.desc(), param.value(), param.minValue(), param.maxValue(), param.format());
                            break;
                        case ParameterType::Value3:
                            ImGui::SliderFloat3(param.desc(), param.value(), param.minValue(), param.maxValue(), param.format());
                            break;
                        case ParameterType::Value4:
                            ImGui::SliderFloat4(param.desc(), param.value(), param.minValue(), param.maxValue(), param.format());
                            break;
                        case ParameterType::RGB:
                            ImGui::ColorEdit3(param.desc(), param.value());
                            break;
                        case ParameterType::RGBA:
                            ImGui::ColorEdit4(param.desc(), param.value());
                            break;
                        default:
                            ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
                            ImGui::Text("parameter '%s' has unsupported type", param.name());
                            ImGui::PopStyleColor(1);
                            ctlOK = false;
                            break;
                    }
                    // reset menu
                    if (ctlOK && ImGui::BeginPopupContextItem("parameter popup")) {
                        if (ImGui::Selectable("restore default")) {
                            param.reset();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }   // END parameter iteration

                // error messages (if present)
                if (node.errors()[0]) {
                    if (node.passCount()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFF202020);
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF80C0FF);
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x800000FF);
                    }
                    ImGui::InputTextMultiline("errors",
                        const_cast<char*>(node.errors()),
                        StringUtil::stringLengthWithoutTrailingWhitespace(node.errors()),
                        ImVec2(-FLT_MIN, ImGui::GetFrameHeight() + ImGui::GetTextLineHeight() * (StringUtil::countLines(node.errors()) - 1)),
                        ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(2);
                }

                // end of processing node
                ImGui::TreePop();
            }   // END node UI block
            ImGui::PopID();
        }   // END node iteration

        // force re-rendering when the show index changed
        if (m_showIndex != oldShowIndex) {
            m_pipeline.markAsChanged();
        }

        // "Add Filter" button
        if (ImGui::Button("Add Filter ...")) {
            ImGui::OpenPopup("add_filter");
        }
        if (ImGui::BeginPopup("add_filter")) {
            ShaderBrowserMenu(*this, 0, "");
            ImGui::EndPopup();
        }
    }   // END main window
    ImGui::End();

    // info window
    if (m_showInfo) {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(
            vp->WorkPos.x + 0.5f * vp->WorkSize.x,
            vp->WorkPos.y + 0.5f * vp->WorkSize.y
        ), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Information", &m_showInfo,
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoResize);
        ImGui::TextUnformatted("GIPS - The GLSL Imaging Processing System");
        if (!git_rev) {
            ImGui::Text("Version %s", GIPS_VERSION);
        } else if (!git_branch) {
            ImGui::Text("Version %s (Git %s)", GIPS_VERSION, git_rev);
        } else {
            ImGui::Text("Version %s (Git %s %s)", GIPS_VERSION, git_branch, git_rev);
        }
        ImGui::TextUnformatted("(C) 2021 Martin J. Fiedler");
        ImGui::Separator();
        if (m_showDebug) {
            ImGui::Text("Platform: %s on %s", SysInfo::getSystemID(), SysInfo::getPlatformID());
            ImGui::Text("Compiler: %s (%d-bit)", SysInfo::getCompilerID(), SysInfo::getBitness());
            ImGui::Separator();
            ImGui::TextUnformatted("Library Versions:");
            ImGui::Text("- GLFW %s", glfwGetVersionString());
            ImGui::Text("- Dear ImGui %s", ImGui::GetVersion());
            ImGui::Separator();
        }
        ImGui::Text("OpenGL %s", m_glVersion.c_str());
        ImGui::TextUnformatted(m_glVendor.c_str());
        ImGui::TextUnformatted(m_glRenderer.c_str());
        ImGui::Separator();
        ImGui::Text("pipeline format: %dx%d, %s",
            m_imgWidth, m_imgHeight, GIPS::pixelFormatName(m_pipeline.format()));
        // video memory estimator:
        // - 1x 8-bit RGBA input image buffer
        // - 1x 8-bit RGBA export buffer (if not running in 8-bit mode)
        // - 2x variable-format processing buffers
        // - 2x 8-bit RGBA buffers for the display screen
        uint64_t area = uint64_t(m_imgWidth * m_imgHeight);
        uint64_t mem = area * 4ull  // input
                     + 2ull * area * getBytesPerPixel(m_pipeline.format())  // processing
                     + 2ull * uint64_t(m_io->DisplaySize.x * m_io->DisplaySize.y) * 4ull;  // display
        if (m_pipeline.format() != GIPS::PixelFormat::Int8) {
            mem += area * 4ull;  // export
        }
        ImGui::Text("estimated video memory usage: %.1f MiB", double(mem) / 1048576.0);
        ImGui::Text("processing time: %.1f ms", m_pipeline.lastRenderTime_ms());
        ImGui::End();
    }   // END info window
}

///////////////////////////////////////////////////////////////////////////////

void GIPS::App::showLoadUI(bool imagesOnly) {
    std::vector<std::string> filters;
    static const std::string extP("*gips");
    static const std::string extI("*.jpg *.jpeg *.png *.bmp *.tga *.pgm *.ppm *.gif *.psd");
    static const std::string extS("*.glsl *.frag *.fs");
    if (!imagesOnly) {
        filters.push_back("All Supported Files");
        filters.push_back(extP + " " + extI + " " + extS);
        filters.push_back("GIPS Pipelines (" + extP + ")");
        filters.push_back(extP);
    }
    filters.push_back("Image Files (" + extI + ")");
    filters.push_back(extI);
    if (!imagesOnly) {
        filters.push_back("Shader Files (" + extS + ")");
        filters.push_back(extS);
    }
    filters.push_back("All Files");
    filters.push_back("*");

    auto path = pfd_open_file_wrapper(imagesOnly ? "Open Source Image" : "Open Pipeline, Shader or Source Image", m_imgFilename, filters);
    if (!path.empty()) {
        requestLoadFile(path[0].c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////

void GIPS::App::showSaveUI(bool useLastPath) {
    std::string path((useLastPath && !m_lastSaveFilename.empty()) ? m_lastSaveFilename :
        pfd_save_file_wrapper(
            "Save Pipeline or Result Image", m_lastSaveFilename,
            { "GIPS Pipelines (*.gips)", "*.gips",
            "Image Files (*.jpg *.png *.bmp *.tga)", "*.jpg *.png *.bmp *.tga",
            "All Files", "*" }
        ));
    if (!path.empty()) {
        if (!StringUtil::extractExtCode(path.c_str())) {
            path += ".gips";  // add default extension
        }
        requestSaveFile(path.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
