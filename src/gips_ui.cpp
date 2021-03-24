#include <algorithm>

#include "imgui.h"

#define NOMINMAX  // pfd includes <windows.h>, which trashes std::min and std::max
#define PFD_SKIP_IMPLEMENTATION 1
#include "portable-file-dialogs.h"

#include "string_util.h"
#include "dirlist.h"

#include "patterns.h"

#include "gips_app.h"

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
            ImGui::PushStyleColor(ImGuiCol_TitleBg, color);  ++nPopColor;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, color);  ++nPopColor;
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
    ButtonColorOverride(int rgb24) : ButtonColorOverride((rgb24 >> 16) / 255.0f, ((rgb24 >> 8) & 0xFF) / 255.0f, (rgb24 & 0xFF) / 255.0f) {}
    ~ButtonColorOverride() {
        ImGui::PopStyleColor(3);
    }
};

///////////////////////////////////////////////////////////////////////////////

static void ShaderBrowserMenu(GIPS::App& app, int nodeIndex, const char* dir) {
    const DirList& list = getCachedDirList(dir);
    for (const auto& item : list.items) {
        if (item.isDir) {
            if (ImGui::BeginMenu(item.nameNoExt.c_str())) {
                ShaderBrowserMenu(app, nodeIndex, item.fullPath.c_str());
                ImGui::EndMenu();
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
            ShaderBrowserMenu(app, nodeIndex, app.getShaderDir());
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
            (m_statusType == StatusType::Error)   ? 0xC00000FF :
            (m_statusType == StatusType::Success) ? 0x00A000FF :
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
                if (ImGui::MenuItem("Save Result ...", "Ctrl+S")) { showSaveUI(); }
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
                    if (ImGui::MenuItem((dc + GIPS::pixelFormatName(m_pipeline.format()) + ")").c_str(), nullptr, &sel)) {
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
                #ifndef NDEBUG
                    ImGui::Separator();
                    ImGui::MenuItem("Show ImGui Demo", nullptr, &m_showDemo);
                #endif
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
                if (ImGui::Button("Open ...")) { showLoadUI(false); }
                ImGui::SameLine();
                ImGui::Text("%s", m_imgFilename.c_str());
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
            ShaderBrowserMenu(*this, 0, getShaderDir());
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
        ImGui::TextUnformatted("(C) 2021 Martin J. Fiedler");
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

void GIPS::App::showLoadUI(bool withShaders) {
    std::vector<std::string> filters;
    static const std::string extI("*.jpg *.jpeg *.png *.bmp *.tga *.pgm *.ppm *.gif *.psd");
    static const std::string extS("*.glsl *.frag *.gips");
    if (withShaders) {
        filters.push_back("Image or Shader Files");
        filters.push_back(extI + " " + extS);
    }
    filters.push_back("Image Files");
    filters.push_back(extI);
    if (withShaders) {
        filters.push_back("Shader Files");
        filters.push_back(extS);
    }
    filters.push_back("All Files");
    filters.push_back("*");

    auto path = pfd::open_file("Open Source Image or Shader", m_imgFilename.c_str(), filters).result();
    if (!path.empty()) {
        requestHandleFile(path[0].c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////

void GIPS::App::showSaveUI() {
    auto path = pfd::save_file(
        "Save Result Image", m_lastSaveFilename,
        { "Image Files", "*.jpg *.png *.bmp *.tga",
          "All Files", "*" }
    ).result();
    if (!path.empty()) {
        requestSaveResult(path.c_str());
    }
}

///////////////////////////////////////////////////////////////////////////////
