SUBDIRS = help




pixmapdir = $(datadir)/pixmaps/glines
icondir = $(datadir)/pixmaps
Gamesdir = $(datadir)/applications

INCLUDES = -I.. -I$(srcdir)/..		\
	$(GNOME_GAMES_CFLAGS)		\
	-DDATADIR=\""$(datadir)"\"		   \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED

bin_PROGRAMS = glines

glines_SOURCES = 	\
	glines.c glines.h

glines_LDADD =		\
	$(GNOME_GAMES_LIBS) \
	$(INTLLIBS)

pixmap_DATA = ball.png classic.png dots.png pulse.png \
		black.xpm gray.xpm woody.xpm

EXTRA_DIST = glines.desktop.in $(pixmap_DATA) $(icon_DATA)

icon_DATA = glines.png

Games_in_files = glines.desktop.in
Games_DATA = $(Games_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

CLEANFILES = $(Games_DATA)

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	touch $(DESTDIR)$(scoredir)/glines.scores
	-chown $(scores_user).$(scores_group) $(DESTDIR)$(scoredir)/glines.scores
	-chmod 664 $(DESTDIR)$(scoredir)/glines.scores
	-if test "x$(setgid)" = "xtrue"; then chgrp $(scores_group) $(DESTDIR)$(bindir)/glines && chmod 2555 $(DESTDIR)$(bindir)/glines ; fi
