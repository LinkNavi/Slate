project('slate', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'warning_level=2', 'optimization=2']
)

cmake = import('cmake')

ftxui_opts = cmake.subproject_options()
ftxui_opts.add_cmake_defines({
  'FTXUI_BUILD_EXAMPLES': 'OFF',
  'FTXUI_BUILD_TESTS': 'OFF',
  'FTXUI_BUILD_DOCS': 'OFF',
})

ftxui_proj = cmake.subproject('ftxui', options: ftxui_opts)

ftxui_screen    = ftxui_proj.dependency('screen')
ftxui_dom       = ftxui_proj.dependency('dom')
ftxui_component = ftxui_proj.dependency('component')

wren_proj = subproject('wren')
wren_dep  = wren_proj.get_variable('wren_dep')

sources = files(
  'src/main.cpp',
  'src/app.cpp',
  'src/buffer.cpp',
  'src/screen_manager.cpp',
  'src/scripting.cpp',
)

executable('slate',
  sources,
  include_directories: include_directories('include'),
  dependencies: [ftxui_screen, ftxui_dom, ftxui_component, wren_dep],
)
