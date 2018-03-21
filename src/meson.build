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
	'games-gridframe.c',
	'games-gridframe.h',
	'games-preimage.c',
	'games-preimage.h',
	'games-score.c',
	'games-score.h',
	'games-scores-backend.c',
	'games-scores-backend.h',
	'games-scores-dialog.c',
	'games-scores-dialog.h',
	'games-scores.c',
	'games-scores.h'
]

five_or_more_sources = [
	five_or_more_c_sources
]

five_or_more_deps = [
	libmath,
	librsvg,
	glib,
	gio,
	gtk
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
