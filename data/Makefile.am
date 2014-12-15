SUBDIRS = icons

uidir = $(datadir)/five-or-more/
ui_DATA = \
	five-or-more.ui \
	five-or-more-preferences.ui \
	menu.ui

themedir = $(datadir)/five-or-more/themes
theme_DATA = \
	balls.svg	\
	dots.png	\
	gumball.png	\
	shapes.svg

gsettings_SCHEMAS = org.gnome.five-or-more.gschema.xml
@GSETTINGS_RULES@

man_MANS = five-or-more.6

desktop_in_files = five-or-more.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appstream_in_files = five-or-more.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)
@APPSTREAM_XML_RULES@
@INTLTOOL_XML_RULE@

EXTRA_DIST = 			\
	$(theme_DATA)		\
	$(appstream_in_files)	\
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)	\
	$(man_MANS)	\
	$(ui_DATA)

CLEANFILES = \
	$(appstream_XML)	\
	$(desktop_DATA)

check-local: $(desktop_DATA)
	$(DESKTOP_FILE_VALIDATE) $(desktop_DATA)

-include $(top_srcdir)/git.mk
