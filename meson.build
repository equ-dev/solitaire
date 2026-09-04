project('solitaire', 'c',
  version : '0.1.0',
  default_options : ['warning_level=2', 'c_std=gnu11'])

gtk_dep = dependency('gtk4')

executable('solitaire',
  'src/main.c',
  dependencies : [gtk_dep],
  install : false,
)
