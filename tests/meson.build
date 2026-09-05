check_dep = dependency('check')

test_game_exe = executable('test_game',
  'test_game.c',
  model_sources,
  dependencies : [check_dep],
)

test('game model', test_game_exe)

test_win_animation_exe = executable('test_win_animation',
  'test_win_animation.c',
  model_sources,
  anim_sources,
  dependencies : [check_dep],
)

test('win animation physics', test_win_animation_exe)
