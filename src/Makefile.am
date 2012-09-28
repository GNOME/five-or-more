bin_PROGRAMS = glines

glines_SOURCES =	\
	games-file-list.c	\
	games-file-list.h	\
	games-gridframe.c	\
	games-gridframe.h	\
	games-preimage.c	\
	games-preimage.h	\
	games-score.h			\
	games-score.c			\
	games-scores.c			\
	games-scores.h			\
	games-scores-dialog.c	\
	games-scores-dialog.h	\
	games-scores-backend.c	\
	games-scores-backend.h \
	games-stock.c	\
	games-stock.h	\
	glines.c	\
	glines.h

if ENABLE_SETGID
glines_SOURCES +=		\
	games-setgid-io.c		\
	games-setgid-io.h
endif # ENABLE_SETGID

glines_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

glines_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/glines\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	-DSCORESDIR="\"$(scoredir)\""				\
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(RSVG_CFLAGS) \
	$(AM_CFLAGS)

glines_LDADD =	\
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
