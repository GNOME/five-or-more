SUBDIRS = data help po src

EXTRA_DIST = \
	**/meson.build \
	meson_options.txt \
	meson_post_install.py \
	$(NULL)

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN)

# We currently have no custom macros
GITIGNOREFILES = m4

-include $(top_srcdir)/git.mk
