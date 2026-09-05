project('solitaire', 'c',
  version : '0.1.0',
  default_options : ['warning_level=2', 'c_std=gnu11'])

gtk_dep = dependency('gtk4')

model_sources = files(
  'src/model/card.c',
  'src/model/deck.c',
  'src/model/game.c',
)

view_sources = files(
  'src/view/board_view.c',
  'src/view/app_window.c',
)

# Pure physics/logic for the view layer, deliberately kept GTK-free so it
# can be linked into the Check test suite the same way model_sources is.
anim_sources = files(
  'src/view/win_animation.c',
)

controller_sources = files(
  'src/controller/game_controller.c',
)

executable('solitaire',
  'src/main.c',
  model_sources,
  view_sources,
  anim_sources,
  controller_sources,
  dependencies : [gtk_dep],
  install : false,
)

subdir('tests')
