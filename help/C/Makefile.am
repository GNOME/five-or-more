
glines_helpdir = $(datadir)/gnome/help/glines/C

glines_help_DATA =         \
        topic.dat

# Files that aren't in a binary/data/library target have to be listed here
# to be included in the tarball when you 'make dist'.
# We include the timestamp files so it won't even try to rebuild the docs
# unless you edit the SGML files.
EXTRA_DIST = \
	user-guide.sgml		\
	user-guide.stamp	\
        topic.dat


## The '-' before the command means to ignore it if it fails.  That way
## people can still build the software without the docbook tools.

all: user-guide.stamp

user-guide.stamp: user-guide.sgml
	-db2html user-guide.sgml
	@echo timestamp > user-guide.stamp 2> /dev/null

# I haven't tested these yet.
user-guide.ps: user-guide.sgml
	-db2ps $<

user-guide.rtf: user-guide.sgml
	-db2rtf $<

## When we make dist, we include the generated HTML so people don't
## have to have the docbook tools.
dist-hook:
	mkdir $(distdir)/user-guide
	mkdir $(distdir)/user-guide-images
	mkdir $(distdir)/user-guide/stylesheet-images
	-cp user-guide/*.html user-guide/*.css $(distdir)/user-guide
	-cp user-guide-images/*.png $(distdir)/user-guide-images
	-cp user-guide/stylesheet-images/*.gif $(distdir)/user-guide/stylesheet-images
	-cp user-guide/stylesheet-images/*.png $(distdir)/user-guide/stylesheet-images

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(glines_helpdir)/user-guide/user-guide-images
	$(mkinstalldirs) $(DESTDIR)$(glines_helpdir)/user-guide/stylesheet-images
	-for file in $(srcdir)/user-guide/*.html $(srcdir)/user-guide/*.css; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(glines_helpdir)/user-guide/$$basefile; \
	done
	-for file in $(srcdir)/user-guide-images/*.png; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(glines_helpdir)/user-guide/user-guide-images/$$basefile; \
	done
	-for file in $(srcdir)/user-guide/stylesheet-images/*.png $(srcdir)/user-guide/stylesheet-images/*.gif; do \
	  basefile=`basename $$file`; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(glines_helpdir)/user-guide/stylesheet-images/$$basefile; \
	done

clean:
	-rm -rf user-guide 
	-rm -f user-guide.stamp
	-rm -rf user-guide.junk
