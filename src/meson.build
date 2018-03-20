#five-or-more executable

five_or_more_c_sources = [
	'five-or-more.c',
	'game-area.h',
	'game-area.c',
	'five-or-more-app.c',
	'five-or-more-app.h',
	'balls-preview.c',
	'balls-preview.h',
	'games-file-list.c',
	'games-file-list.h',
	'games-preimage.c',
	'games-preimage.h'
]

five_or_more_sources = [
	five_or_more_c_sources
]

five_or_more_deps = [
	glib,
	gio,
	gtk,
	libgnome_games_support,
	libmath,
	librsvg
]

five_or_more_includes = [
	config_h_dir
]

five_or_more_c_flags = [
	'-include', 'config.h'
]

executable('five-or-more', five_or_more_sources,
	dependencies: five_or_more_deps,
	include_directories: five_or_more_includes,
	c_args: five_or_more_c_flags,
	install: true
)
