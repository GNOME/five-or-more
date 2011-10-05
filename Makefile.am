SUBDIRS = data

if BUILD_HELP
SUBDIRS += help
endif

NULL =

pixmapdir = $(pkgdatadir)/glines/pixmaps
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
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

glines_CFLAGS = \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)

glines_LDADD =	\
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GTK_LIBS)	\
	$(INTLLIBS)	\
	-lm \
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
	$(man_MANS)		\
	$(NULL)

CLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)
DISTCLEANFILES = $(desktop_DATA) $(gsettings_SCHEMAS)

SCOREFILES = \
	Small	\
	Medium	\
	Large	\
	$(NULL)

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

install-data-local: install-scorefiles-local

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/glines && chmod 2555 $(DESTDIR)$(bindir)/glines ;\
	fi


-include $(top_srcdir)/git.mk
