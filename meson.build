project('solitaire', 'c',
  version : '0.1.0',
  default_options : ['warning_level=2', 'c_std=gnu11'])

gtk_dep = dependency('gtk4')

model_sources = files(
  'src/model/card.c',
  'src/model/deck.c',
  'src/model/game.c',
)

executable('solitaire',
  'src/main.c',
  dependencies : [gtk_dep],
  install : false,
)

subdir('tests')
