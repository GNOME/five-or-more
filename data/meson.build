desktop = 'org.gnome.five-or-more.desktop'
# Desktop file
i18n.merge_file (
  input: desktop + '.in',
  output: desktop,
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths (data_dir, 'applications')
)

# Themes
foreach file: ['balls.svg', 'shapes.svg', 'tango.svg']
  install_data (file,
                install_dir: join_paths(pkgdata_dir, 'themes')
  )
endforeach

# UI files
foreach file: ['five-or-more.ui', 'five-or-more-preferences.ui']
  install_data (file,
    install_dir: join_paths (pkgdata_dir)
  )
endforeach

# Icons
foreach size: [ '16x16', '22x22', '24x24', '32x32', '48x48', '256x256', '512x512' ]
    install_data(join_paths('icons', 'hicolor', size, 'org.gnome.five-or-more.png'),
                 install_dir: join_paths (data_dir, 'icons', 'hicolor', size, 'apps')
    )
endforeach

install_data (join_paths ('icons', 'hicolor', 'symbolic', 'org.gnome.five-or-more-symbolic.svg'),
             install_dir: join_paths (data_dir, 'icons', 'hicolor', 'symbolic', 'apps')
             )

appdata = 'org.gnome.five-or-more.appdata.xml'
# Appdata
i18n.merge_file (
  input: appdata + '.in',
  output: appdata,
  type: 'xml',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths (data_dir, 'metainfo'),
)

# Gschema
install_data ('org.gnome.five-or-more.gschema.xml',
  install_dir: join_paths (data_dir, 'glib-2.0', 'schemas')
)

# Gresource
resource_files = files ('org.gnome.five-or-more.gresource.xml')
resources = gnome.compile_resources ('five-or-more', resource_files)

# Manpage
install_man (
  'five-or-more.6'
)
