bin_PROGRAMS = glines

glines_SOURCES =	\
	games-file-list.c	\
	games-file-list.h	\
	games-gridframe.c	\
	games-gridframe.h	\
	games-preimage.c	\
	games-preimage.h	\
	games-settings.c	\
	games-settings.h	\
	games-stock.c	\
	games-stock.h	\
	glines.c	\
	glines.h

glines_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

glines_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/glines\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(RSVG_CFLAGS) \
	$(AM_CFLAGS)

glines_LDADD =	\
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GMODULE_LIBS) \
	$(GTK_LIBS)	\
	$(RSVG_LIBS) \
	$(INTLLIBS)	\
	-lm

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/glines && chmod 2555 $(DESTDIR)$(bindir)/glines ;\
	fi

-include $(top_srcdir)/git.mk
