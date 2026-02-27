project('slate', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++20']
)

qt6 = import('qt6')

qt6_dep = dependency('qt6', modules: [
  'Core',
  'Gui',
  'Widgets',
  'Quick',
  'Qml',
  'QuickControls2',
])

moc_headers = qt6.compile_moc(
  headers: [
    'src/projectmanager.h',
    'src/filesystemmodel.h',
    'src/editorbuffer.h',
    'src/lspclient.h',
    'src/dialoghelper.h',
    'src/thememanager.h',
    'src/settingsmanager.h',
    'src/syntaxhighlighter.h',
  ],
  dependencies: qt6_dep,
  include_directories: include_directories('src'),
)

resources = qt6.compile_resources(
  name: 'slate_resources',
  sources: 'resources.qrc'
)

executable('slate',
  'src/main.cpp',
  'src/syntaxhighlighter.cpp',
  moc_headers,
  resources,
  dependencies: qt6_dep,
  install: true
)
