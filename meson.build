project('five-or-more', ['c', 'vala'],
  version: '48.1',
  meson_version: '>= 0.57.0',
  license: 'GPLv2'
)

gnome = import('gnome')
i18n = import('i18n')

locale_dir  = join_paths (get_option ('prefix'), get_option ('localedir'))
po_dir      = join_paths (meson.project_source_root (), 'po')
data_dir    = join_paths (get_option ('prefix'), get_option ('datadir'))
pkgdata_dir = join_paths (data_dir, 'five-or-more')
bindir      = join_paths (get_option ('prefix'), get_option ('bindir'))

# Dependencies
glib_min_version = '2.32'

gee_dep = dependency('gee-0.8')
gio_dep = dependency('gio-2.0', version: '>= ' + glib_min_version)
glib_dep = dependency('glib-2.0', version: '>= ' + glib_min_version)
gmodule_export_dep = dependency('gmodule-export-2.0')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.24.0')
libgnome_games_support_dep = dependency('libgnome-games-support-1', version: '>= 1.7.1')
librsvg_dep = dependency('librsvg-2.0', version: '>= 2.32.0')

# Libraries
cc = meson.get_compiler('c')
libmath_dep = cc.find_library('m')

# Configuration
conf = configuration_data()

conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALE_DIRECTORY', locale_dir)
conf.set_quoted('DATA_DIRECTORY', pkgdata_dir)
conf.set_quoted('VERSION', meson.project_version())

configure_file(output: 'config.h', configuration: conf)
config_h_dir = include_directories('.')

add_project_arguments([
    '-include', 'config.h'
  ],
  language: 'c'
)

# Extra scripts
meson.add_install_script('meson_post_install.py')

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
)

# Subdirs
subdir('data')
subdir('help')
subdir('po')
subdir('src')
