#include "imgui.h"

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

        // input image status
        ImGui::AlignTextToFramePadding();
        bool open = ImGui::TreeNodeEx("Input Image", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap);
        /*
        ImGui::SameLine(ImGui::GetWindowWidth() - 70.0f);
        ImGui::Button("Off");
        ImGui::SameLine(ImGui::GetWindowWidth() - 40.0f);
        ImGui::Button("Show");
        */
        if (open) {
            ImGui::Text("Working Resolution: %dx%d", m_imgWidth, m_imgHeight);
            ImGui::TreePop();
        }

    // main window end
    }
    ImGui::End();
}

///////////////////////////////////////////////////////////////////////////////
