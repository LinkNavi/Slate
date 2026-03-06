#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE33
#endif

#define SOKOL_IMPL
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#include "sokol_fetch.h"

// imgui.h MUST come before sokol_imgui.h
#include "../imgui/imgui.h"

#define SOKOL_IMGUI_IMPL
#include "sokol_imgui.h"