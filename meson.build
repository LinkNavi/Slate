project('slate', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++20']
)

qt6 = import('qt6')

qt6_dep = dependency('qt6', modules: [
  'Core',
  'Gui',
  'Quick',
  'Qml',
])

resources = qt6.compile_resources(
  name: 'slate_resources',
  sources: 'resources.qrc'
)

executable('slate',
  'src/main.cpp',
  resources,
  dependencies: qt6_dep,
  install: true
)
