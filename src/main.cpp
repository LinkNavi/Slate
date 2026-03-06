#include "ui/ui.h"
#include "theme/theme.h"
#include "ui/slate_ui.h"

#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_glue.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_imgui.h"

static UI* g_ui = nullptr;

static void init() {
    sg_desc gfx = {};
    gfx.environment = sglue_environment();
    sg_setup(&gfx);
    stm_setup();

    simgui_desc_t imgui_desc = {};
    simgui_setup(&imgui_desc);

    Theme theme = Theme::fallback_dark();
    SlateUI::apply_theme(theme);  // push rounded style + colors to ImGui
    g_ui = new UI(theme);
}

static void frame() {
    simgui_new_frame({
        sapp_width(),
        sapp_height(),
        sapp_frame_duration(),
        sapp_dpi_scale()
    });

    g_ui->update();
    g_ui->render();

    sg_pass pass = {};
    pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass.action.colors[0].clear_value = {0.08f, 0.08f, 0.10f, 1.0f};
    pass.swapchain = sglue_swapchain();
    sg_begin_pass(&pass);
    simgui_render();
    sg_end_pass();
    sg_commit();
}

static void event(const sapp_event* e) {
    simgui_handle_event(e);
    if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse)
        g_ui->handle_event(e);
}

static void cleanup() {
    delete g_ui;
    simgui_shutdown();
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    sapp_desc desc = {};
    desc.init_cb      = init;
    desc.frame_cb     = frame;
    desc.event_cb     = event;
    desc.cleanup_cb   = cleanup;
    desc.width        = 1280;
    desc.height       = 720;
    desc.window_title = "Slate";
    desc.high_dpi     = true;
    return desc;
}