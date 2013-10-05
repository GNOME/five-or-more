SUBDIRS = icons

uidir = $(datadir)/five-or-more/
ui_DATA = \
	five-or-more.ui \
	five-or-more-preferences.ui \
	menu.ui

themedir = $(datadir)/five-or-more/themes
theme_DATA = \
	balls.svg	\
	classic.png	\
	dots.png	\
	gumball.png	\
	pulse.png	\
	shapes.svg

gsettings_SCHEMAS = org.gnome.five-or-more.gschema.xml
@GSETTINGS_RULES@

man_MANS = five-or-more.6

desktop_in_files = glines.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appdatadir = $(datadir)/appdata
appdata_DATA = five-or-more.appdata.xml

EXTRA_DIST = 			\
	$(theme_DATA)		\
        $(appdata_DATA)		\
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)	\
	$(man_MANS)	\
	$(ui_DATA)

CLEANFILES = $(desktop_DATA)

-include $(top_srcdir)/git.mk
