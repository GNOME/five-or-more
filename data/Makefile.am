SUBDIRS = icons

uidir = $(datadir)/five-or-more/
ui_DATA = \
	five-or-more.ui \
	five-or-more-preferences.ui

themedir = $(datadir)/five-or-more/themes
theme_DATA = \
	balls.svg	\
	classic.png	\
	dots.png	\
	gumball.png	\
	pulse.png	\
	shapes.svg

gsettings_in_file = org.gnome.five-or-more.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_file:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

man_MANS = five-or-more.6

desktop_in_files = five-or-more.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 			\
	$(theme_DATA)		\
	$(gsettings_in_file)    \
	$(man_MANS)	\
	$(ui_DATA)

CLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
