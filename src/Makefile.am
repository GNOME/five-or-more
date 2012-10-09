bin_PROGRAMS = five-or-more

five_or_more_SOURCES =	\
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
five_or_more_SOURCES +=		\
	games-setgid-io.c		\
	games-setgid-io.h
endif # ENABLE_SETGID

five_or_more_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

five_or_more_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/five-or-more\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	-DSCORESDIR="\"$(scoredir)\""				\
	$(FIVE_OR_MORE_CFLAGS)

five_or_more_LDADD =	\
	$(FIVE_OR_MORE_LIBS) \
	-lm

install-exec-hook:
	-if test "$(setgid)" = "true"; then \
	  chgrp $(scores_group) $(DESTDIR)$(bindir)/five-or-more && chmod 2555 $(DESTDIR)$(bindir)/five-or-more ;\
	fi

-include $(top_srcdir)/git.mk
