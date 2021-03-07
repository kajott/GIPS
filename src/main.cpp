#include "imgui.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_imgui.h"

sg_pass_action pass_action;

void OnInit() {
    static sg_desc gfx_desc;
    gfx_desc.context = sapp_sgcontext();
    sg_setup(&gfx_desc);

    pass_action.colors[0].action = SG_ACTION_CLEAR;
    pass_action.colors[0].value = {1.0f, 0.0f, 0.0f, 1.0f};

    static simgui_desc_t imgui_desc;
    simgui_setup(&imgui_desc);
}

void OnFrame() {
    float g = pass_action.colors[0].value.g + 0.01f;
    pass_action.colors[0].value.g = (g > 1.0f) ? 0.0f : g;
    sg_begin_default_pass(&pass_action, sapp_width(), sapp_height());
    simgui_new_frame(sapp_width(), sapp_height(), 1.0/60);
    ImGui::Text("Hello World!");
    simgui_render();
    sg_end_pass();
    sg_commit();
}

void OnEvent(const sapp_event* ev) {
    simgui_handle_event(ev);
}

void OnQuit() {
    simgui_shutdown();
    sg_shutdown();
}

extern "C"
sapp_desc sokol_main(int argc, char* argv[]) {
    static sapp_desc app_desc;
    app_desc.init_cb = OnInit;
    app_desc.frame_cb = OnFrame;
    app_desc.event_cb = OnEvent;
    app_desc.cleanup_cb = OnQuit;
    app_desc.width = 800;
    app_desc.height = 600;
    app_desc.window_title = "GIPS";
    return app_desc;
}
