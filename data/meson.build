# Desktop file 
i18n.merge_file(
	input: 'five-or-more.desktop.in',
	output: 'five-or-more.desktop',
	type: 'desktop',
	po_dir: po_dir,
	install: true,
	install_dir: join_paths(pkgdata_dir, 'applications')
)

# Themes
foreach file: ['balls.svg', 'dots.png', 'gumball.png', 'shapes.svg']
	install_data (file,
		install_dir: join_paths(pkgdata_dir, 'themes')
	)
endforeach

# UI files
foreach file: ['five-or-more.ui', 'five-or-more-preferences.ui', 'menu.ui']
	install_data (file,
		install_dir: join_paths(pkgdata_dir)
	)
endforeach

# Icons
install_subdir('icons',
  install_dir: join_paths(pkgdata_dir),
  exclude_files: [ 'Makefile', 'Makefile.in', 'Makefile.am', 'five-or-more-src.svg' ],
)

# Appdata
i18n.merge_file(
	input: 'five-or-more.appdata.xml.in',
	output: 'five-or-more.appdata.xml',
	type: 'xml',
	po_dir: po_dir,
	install: true,
	install_dir: join_paths(pkgdata_dir, 'meta-info')
)

# Gschema
install_data('org.gnome.five-or-more.gschema.xml',
	install_dir: join_paths(pkgdata_dir, 'appdata')
)

# Manpage
install_man(
	'five-or-more.6'
)
