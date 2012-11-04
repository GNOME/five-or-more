bin_PROGRAMS = five-or-more

BUILT_SOURCES = five-or-more-resources.c

five_or_more_SOURCES =	\
	games-file-list.c	\
	games-file-list.h	\
	games-gridframe.c	\
	games-gridframe.h	\
	games-preimage.c	\
	games-preimage.h	\
	games-stock.c	\
	games-stock.h	\
	config.vapi \
	main.vala \
	application.vala \
	board.vala \
	field.vala \
	history.vala \
	piece.vala \
	preview-queue.vala \
	score-dialog.vala \
	view-2d.vala \
	view-cli.vala \
	$(BUILT_SOURCES)

five_or_more_CPPFLAGS = \
	-I$(top_srcdir) \
	$(AM_CPPFLAGS)

five_or_more_CFLAGS = \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DDATA_DIRECTORY=\"$(datadir)/five-or-more\" \
	-DICON_THEME_DIRECTORY="\"$(datadir)/icons\"" \
	$(FIVE_OR_MORE_CFLAGS)

five_or_more_LDADD =	\
	$(FIVE_OR_MORE_LIBS) \
	-lm

five_or_more_VALAFLAGS = \
	--pkg posix \
	--pkg gtk+-3.0 \
	--pkg gee-1.0

five-or-more-resources.c: five-or-more.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies five-or-more.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source five-or-more.gresource.xml

EXTRA_DIST = \
	five-or-more.gresource.xml

-include $(top_srcdir)/git.mk
