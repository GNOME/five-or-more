scoredir=$(localstatedir)/games
pixmapdir = $(datadir)/pixmaps/glines
icondir = $(datadir)/pixmaps
Gamesdir = $(datadir)/gnome/apps/Games

INCLUDES = -I.. -I$(srcdir)/..		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(includedir)			\
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = glines

glines_SOURCES = 	\
	glines.c glines.h

glines_LDADD =		\
	$(GNOME_LIBDIR)         \
	$(GNOMEUI_LIBS)		\
	$(INTLLIBS)

pixmap_DATA = ball.png classic.png dots.png pulse.png \
		black.xpm gray.xpm woody.xpm

EXTRA_DIST = glines.desktop $(pixmap_DATA) $(icon_DATA)

icon_DATA = glines.png

Games_DATA = glines.desktop

install-data-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	touch $(DESTDIR)$(scoredir)/glines.scores
	-chown games.games $(DESTDIR)$(scoredir)/glines.scores
	-chmod 664 $(DESTDIR)$(scoredir)/glines.scores
	-chgrp games $(DESTDIR)$(bindir)/glines && chmod 2111 $(DESTDIR)$(bindir)/glines
