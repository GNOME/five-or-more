#five-or-more executable

five_or_more_sources = [
  'config.vapi',
  'board.vala',
  'game.vala',
  'main.vala',
  'next-pieces-widget.vala',
  'piece.vala',
  'piece-generator.vala',
  'theme-renderer.vala',
  'view.vala',
  'window.vala',
  resources,
]

five_or_more_deps = [
  gee_dep,
  gio_dep,
  gtk_dep,
  libgnome_games_support_dep,
  librsvg_dep,
  libmath_dep,
]

five_or_more_vala_args = [
  '--gresources', resource_files,
]

executable('five-or-more', five_or_more_sources,
  dependencies: five_or_more_deps,
  vala_args: five_or_more_vala_args,
  install: true,
)
