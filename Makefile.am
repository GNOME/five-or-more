NULL =

SUBDIRS = help

# FIXMEchpe: this should really use pixmapdir = $(pkgdatadir)/glines/pixmaps !!
pixmapdir = $(datadir)/pixmaps/glines
pixmap_DATA = \
	balls.svg	\
	shapes.svg	\
	$(NULL)

bin_PROGRAMS = glines

glines_SOURCES =	\
	glines.c	\
	glines.h	\
	$(NULL)

glines_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/libgames-support	\
	-DDATADIR="\"$(datadir)\""		\
	-DSHAREDIR="\"$(pkgdatadir)\""		\
	-DPIXMAPDIR="\"$(pixmapdir)\""		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"\
	$(AM_CPPFLAGS)

glines_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)

glines_LDADD =	\
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS)	\
	$(INTLLIBS)	\
	$(NULL)

if HAVE_GNOME
glines_CFLAGS += $(GNOME_CFLAGS)
glines_LDADD += $(GNOME_LIBS)
endif

if HAVE_RSVG
glines_CFLAGS += $(RSVG_CFLAGS)
glines_LDADD += $(RSVG_LIBS)
endif

if WITH_GTHREAD
glines_CFLAGS += $(GHTREAD_CFLAGS)
glines_LDADD += $(GTHREAD_LIBS)
endif

schema_in_files = glines.schemas.in
schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

desktop_in_files = glines.desktop.in.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 			\
	$(pixmap_DATA)		\
	$(schema_in_files)	\
	$(NULL)

CLEANFILES = $(desktop_DATA) $(schema_DATA)
DISTCLEANFILES = $(desktop_DATA) $(schema_DATA)

SCOREFILES = \
	Small	\
	Medium	\
	Large	\
	$(NULL)

install-schemas-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
		for p in $^ ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
		done \
	fi
endif

install-scorefiles-local:
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

install-data-local: install-schemas-local install-scorefiles-local
