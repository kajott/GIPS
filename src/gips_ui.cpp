#include "imgui.h"

#include "string_util.h"

#include "gips_app.h"

///////////////////////////////////////////////////////////////////////////////

struct StatusWindow {
    StatusWindow(const char* name, float ax, float ay) {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(
            vp->WorkPos.x + ax * vp->WorkSize.x,
            vp->WorkPos.y + ay * vp->WorkSize.y
        ), ImGuiCond_Always, ImVec2(ax, ay));
        ImGui::SetNextWindowBgAlpha(0.375f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(16.0f, 16.0f));
        ImGui::Begin(name, nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing |
                     ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoMove);
    }
    ~StatusWindow() {
        ImGui::End();
        ImGui::PopStyleVar(2);
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

static bool TreeNodeForGIPSNode(const char* name, int &showIndex, int nodeIndex=-1, GIPS::Node* node=nullptr) {
    ImGui::AlignTextToFramePadding();
    bool open = ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
    if (node) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 70.0f);
        ButtonColorOverride _(node->enabled() ? 0x208020 : 0x802020);
        if (ImGui::Button(node->enabled() ? "On" : "Off")) { node->toggle(); }
    }
    {
        ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);
        ButtonColorOverride _((showIndex == nodeIndex + 1) ? 0xC0C040 : 0x405060);
        if (ImGui::Button("Show")) { showIndex = nodeIndex + 1; }
    }
    return open;
}

///////////////////////////////////////////////////////////////////////////////

void GIPS::App::drawUI() {
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

    // main window begin
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos, ImGuiCond_Once, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Filters")) {
        int oldShowIndex = m_showIndex;

        // input image status
        if (TreeNodeForGIPSNode("Input Image", m_showIndex)) {
            ImGui::Text("Working Resolution: %dx%d", m_imgWidth, m_imgHeight);
            ImGui::TreePop();
        }

        // processing nodes
        for (int nodeIndex = 0;  nodeIndex < m_pipeline.nodeCount();  ++nodeIndex) {
            auto& node = m_pipeline.node(nodeIndex);
            ImGui::PushID(nodeIndex);
            if (TreeNodeForGIPSNode(node.name().c_str(), m_showIndex, nodeIndex, &node)) {
                // parameters
                for (int paramIndex = 0 ;  paramIndex < node.paramCount();  ++paramIndex) {
                    auto& param = node.param(paramIndex);
                    switch (param.type()) {
                        case ParameterType::Value:
                            ImGui::SliderFloat(param.desc().c_str(), param.value(), param.minValue(), param.maxValue(), "%.2f");
                            break;
                        case ParameterType::RGB:
                            ImGui::ColorEdit3(param.desc().c_str(), param.value());
                            break;
                        case ParameterType::RGBA:
                            ImGui::ColorEdit4(param.desc().c_str(), param.value());
                            break;
                        default:
                            break;
                    }
                }

                // error messages (if present)
                if (!node.errors().empty()) {
                    if (node.passCount()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xFF202020);
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF4080FF);
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0x800000FF);
                    }
                    ImGui::InputTextMultiline("errors",
                        const_cast<char*>(node.errors().c_str()),
                        StringUtil::stringLengthWithoutTrailingWhitespace(node.errors().c_str()),
                        ImVec2(-FLT_MIN, ImGui::GetFrameHeight() + ImGui::GetTextLineHeight() * (StringUtil::countLines(node.errors().c_str()) - 1)),
                        ImGuiInputTextFlags_ReadOnly);
                    ImGui::PopStyleColor(2);
                }

                // end of processing node
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        // force re-rendering when the show index changed
        if (m_showIndex != oldShowIndex) {
            m_pipeline.markAsChanged();
        }
    // main window end
    }
    ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
