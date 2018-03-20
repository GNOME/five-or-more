bin_PROGRAMS = five-or-more

five_or_more_SOURCES =	\
	five-or-more.c	\
	games-file-list.c	\
	games-file-list.h	\
	games-gridframe.c	\
	games-gridframe.h	\
	games-preimage.c	\
	games-preimage.h	\
	five-or-more-app.h \
	five-or-more-app.c \
	balls-preview.c \
	balls-preview.h \
	game-area.h \
	game-area.c

five_or_more_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

five_or_more_CFLAGS = \
	-DDATA_DIRECTORY=\"$(datadir)/five-or-more\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	$(FIVE_OR_MORE_CFLAGS)

five_or_more_LDADD =	\
	$(FIVE_OR_MORE_LIBS) \
	-lm

-include $(top_srcdir)/git.mk
