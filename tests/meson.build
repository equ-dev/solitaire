check_dep = dependency('check')

test_game_exe = executable('test_game',
  'test_game.c',
  model_sources,
  dependencies : [check_dep],
)

test('game model', test_game_exe)
