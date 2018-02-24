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
$(desktop_DATA): $(desktop_in_files)
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

appstream_in_files = five-or-more.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)
@APPSTREAM_XML_RULES@
$(appstream_XML): $(appstream_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = 			\
	$(appstream_in_files)	\
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)	\
	$(man_MANS)		\
	$(ui_DATA)		\
	$(theme_DATA)		\
	translatable_themes.h

CLEANFILES = \
	$(appstream_XML)	\
	$(desktop_DATA)

check-local: $(desktop_DATA)
	$(DESKTOP_FILE_VALIDATE) $(desktop_DATA)

-include $(top_srcdir)/git.mk
