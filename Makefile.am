SUBDIRS = help

schemadir   = @GCONF_SCHEMA_FILE_DIR@
schema_in_files = glines.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

pixmapdir = $(datadir)/pixmaps/glines
icondir = $(datadir)/pixmaps
Gamesdir = $(datadir)/applications

INCLUDES = -I.. -I$(srcdir)/..		\
	$(GNOME_GAMES_CFLAGS)		\
	-I$(top_srcdir)/libgames-support           \
	-DDATADIR=\""$(datadir)"\"		   \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = glines

glines_SOURCES = 	\
	glines.c glines.h

glines_LDADD =		\
	$(GNOME_GAMES_LIBS) \
	../libgames-support/libgames-support.la \
	$(INTLLIBS)

pixmap_DATA = balls.svg shapes.svg

EXTRA_DIST = 			\
	glines.desktop.in	\
	$(pixmap_DATA)		\
	$(icon_DATA)		\
	$(schema_in_files)


icon_DATA = glines.png

Games_in_files = glines.desktop.in
Games_DATA = $(Games_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES = $(Games_DATA) $(schema_DATA)

SCOREFILES = Small Medium Large

install-data-local:
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/glines/$(schema_DATA)
endif
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	## Migrate to the new scoring system (with a link for dual-version
	## installs).
	-if [ -f $(DESTDIR)$(scoredir)/glines.scores -a ! -f $(DESTDIR)$(scoredir)/glines.Small.scores ] ; then \
	  mv -f $(DESTDIR)$(scoredir)/glines.scores $(DESTDIR)$(scoredir)/glines.Small.scores ; \
	  ln -s $(DESTDIR)$(scoredir)/glines.Medium.scores $(DESTDIR)$(scoredir)/glines.scores ; \
	fi
	-for i in ${SCOREFILES} ; do \
	  touch $(DESTDIR)$(scoredir)/glines.$$i.scores; \
	  chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/glines.$$i.scores; \
	  chmod 664 $(DESTDIR)$(scoredir)/glines.$$i.scores; \
	done
	-if test "x$(setgid)" = "xtrue"; then chgrp $(scores_group) $(DESTDIR)$(bindir)/glines && chmod 2555 $(DESTDIR)$(bindir)/glines ; fi
