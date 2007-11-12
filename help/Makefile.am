# The languages below have not converted to gnome-doc-utils yet;
# remove your language code from SUBDIRS here and from AC_CONFIG_FILES
# in configure.in once you've migrated.

# SUBDIRS = it
# 'it' is even older, it still uses sgmldocs.make!

# GNOME doc utils

include $(top_srcdir)/gnome-doc-utils.make

DOC_LINGUAS = ca en_GB es fr oc ru sv

DOC_MODULE = glines

DOC_ENTITIES = \
	legal.xml

DOC_FIGURES = \
	figures/glines-gameover.png \
	figures/glines-startgame.png \
	figures/preferences.png

dist-hook: doc-dist-hook
