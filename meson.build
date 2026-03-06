project('mesonide', 'cpp',
        version : '0.1.0',
        default_options : [
            'cpp_std=c++17',
            'warning_level=2',
            'optimization=2',
        ]
)

cc = meson.get_compiler('cpp')

thread_dep = dependency('threads')
curl_dep   = dependency('libcurl')

if host_machine.system() == 'windows'
    sokol_defines  = ['-DSOKOL_D3D11']
    platform_deps  = [cc.find_library('d3d11'), cc.find_library('dxgi')]
    platform_links = []
    win_sub        = 'windows'
elif host_machine.system() == 'darwin'
    sokol_defines  = ['-DSOKOL_METAL']
    platform_deps  = []
    platform_links = ['-framework', 'Metal', '-framework', 'MetalKit',
                      '-framework', 'Cocoa', '-framework', 'QuartzCore']
    win_sub        = 'gui'
else
    sokol_defines  = ['-DSOKOL_GLCORE33']
    platform_deps  = [
        dependency('gl'),
        dependency('x11'),
        dependency('xi'),
        dependency('xcursor'),
    ]
    platform_links = []
    win_sub        = 'gui'
endif

imgui_sources = files(
    'src/imgui/imgui.cpp',
    'src/imgui/imgui_draw.cpp',
    'src/imgui/imgui_tables.cpp',
    'src/imgui/imgui_widgets.cpp',
)

sources = files(
    'src/main.cpp',
    'src/sokol/sokol_impl.cpp',
    'src/editor/editor.cpp',
    'src/lsp/lsp_client.cpp',
    'src/debug/gdb_client.cpp',
    'src/build/meson_runner.cpp',
    'src/theme/theme.cpp',
    'src/ui/homepage.cpp',
    'src/ui/slate_ui.cpp',
    'src/ui/settings.cpp',
    'src/ai/claude_client.cpp',
    'src/ui/ui.cpp',
) + imgui_sources

executable('mesonide', sources,
           cpp_args         : sokol_defines,
           link_args        : platform_links,
           dependencies     : [thread_dep, curl_dep, platform_deps],
           include_directories : include_directories('src', 'src/imgui'),
           win_subsystem    : win_sub,
)
