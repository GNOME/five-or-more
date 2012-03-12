bin_PROGRAMS = glines

glines_SOURCES =	\
	glines.c	\
	glines.h

glines_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

glines_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/glines\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
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
