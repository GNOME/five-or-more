bin_PROGRAMS = glines

glines_SOURCES =	\
	glines.c	\
	glines.h

glines_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

glines_CFLAGS = \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS)	\
	$(AM_CFLAGS)

glines_LDADD =	\
	$(top_builddir)/libgames-support/libgames-support.la \
	$(GMODULE_LIBS) \
	$(GTK_LIBS)	\
	$(INTLLIBS)	\
	-lm

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

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/glines && chmod 2555 $(DESTDIR)$(bindir)/glines ;\
	fi

-include $(top_srcdir)/git.mk
