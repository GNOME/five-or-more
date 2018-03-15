project('five-or-more', ['c'],
	version: '3.28.0',
	meson_version: '>= 0.43.0',
	license: 'GPLv2'
)

gnome = import('gnome')
i18n = import('i18n')

locale_dir = join_paths(get_option('prefix'), get_option('localedir'))
po_dir = join_paths(meson.source_root(), 'po')
data_dir = join_paths (get_option ('prefix'), get_option ('datadir'))
pkgdata_dir = join_paths (data_dir, 'five-or-more')

# Dependencies
glib_min_version = '2.32'

librsvg = dependency('librsvg-2.0', version: '>= 2.32.0')
gio = dependency('gio-2.0', version: '>= ' + glib_min_version)
glib = dependency('glib-2.0', version: '>= ' + glib_min_version)
gmodule_export = dependency('gmodule-export-2.0')
gtk = dependency('gtk+-3.0', version: '>= 3.20.0')

# Libraries
cc = meson.get_compiler('c')
libmath = cc.find_library('m')

# Configuration
conf = configuration_data()

conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('LOCALEDIR', locale_dir)
conf.set_quoted('DATA_DIRECTORY', pkgdata_dir)
conf.set_quoted('VERSION', meson.project_version())

configure_file(output: 'config.h', configuration: conf)
config_h_dir = include_directories('.')

# Extra scripts
meson.add_install_script('meson_post_install.py')

# Subdirs
subdir('data')
subdir('help')
subdir('po')
subdir('src')
