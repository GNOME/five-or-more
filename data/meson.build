desktop = 'org.gnome.five-or-more.desktop'
# Desktop file
desktop_file = i18n.merge_file (
  input: desktop + '.in',
  output: desktop,
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths (data_dir, 'applications')
)
desktop_file_validate = find_program('desktop-file-validate', required: false)
if desktop_file_validate.found()
  test(
    'validate-desktop',
    desktop_file_validate,
    args: [
      desktop_file.full_path()
    ]
  )
endif

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
appdata_file = i18n.merge_file (
  input: appdata + '.in',
  output: appdata,
  type: 'xml',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths (data_dir, 'metainfo'),
)
appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test(
    'validate-appdata', appstream_util,
    args: [
      'validate-relax', '--nonet', appdata_file.full_path()
    ]
  )
endif

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
