scoredir=$(localstatedir)/games
pixmapdir = $(datadir)/pixmaps/glines
icondir = $(datadir)/pixmaps
Gamesdir = $(datadir)/applications

INCLUDES = -I.. -I$(srcdir)/..		\
	$(GNOME_GAMES_CFLAGS)		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED \
	-DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED \
	-I$(includedir)
	
bin_PROGRAMS = glines

glines_SOURCES = 	\
	glines.c glines.h

glines_LDADD =		\
	$(GNOME_GAMES_LIBS) \
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
