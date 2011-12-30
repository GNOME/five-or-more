SUBDIRS = icons

uidir = $(pkgdatadir)/glines/
ui_DATA = \
	glines.ui \
	glines-preferences.ui

pixmapdir = $(pkgdatadir)/glines/pixmaps
pixmap_DATA = \
	balls.svg	\
	shapes.svg

gsettings_in_file = org.gnome.glines.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

man_MANS = glines.6

desktop_in_files = glines.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 			\
	$(pixmap_DATA)		\
	$(gsettings_in_file)    \
	$(man_MANS)	\
	$(ui_DATA)

CLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
