#five-or-more executable

five_or_more_sources = [
  'config.vapi',
  'main.vala',
  'window.vala',
  resources
]

five_or_more_deps = [
  gio_dep,
  gtk_dep,
]

five_or_more_vala_args = [
  '--gresources', resource_files,
]

executable('five-or-more-vala', five_or_more_sources,
  dependencies: five_or_more_deps,
  vala_args: five_or_more_vala_args,
  install: true,
)
