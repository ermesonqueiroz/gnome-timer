learning_sources = [
  'main.c',
  'learning-application.c',
  'learning-window.c',
  resources
]

learning_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
]

learning_sources += gnome.compile_resources('learning-resources',
  'learning.gresource.xml',
  c_name: 'learning'
)

executable('learning', learning_sources,
  dependencies: learning_deps,
       install: true,
)
