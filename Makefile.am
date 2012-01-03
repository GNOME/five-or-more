SUBDIRS = src data help

SCOREFILES = \
	Small	\
	Medium	\
	Large

install-scorefiles-local:
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	## Migrate to the new scoring system (with a link for dual-version
	## installs).
	-if [ -f $(DESTDIR)$(scoredir)/glines.scores -a ! -f $(DESTDIR)$(scoredir)/glines.Small.scores ] ; then \
	  mv -f $(DESTDIR)$(scoredir)/glines.scores $(DESTDIR)$(scoredir)/glines.Small.scores ; \
	  ln -s $(DESTDIR)$(scoredir)/glines.Medium.scores $(DESTDIR)$(scoredir)/glines.scores ; \
	fi
	-for i in ${SCOREFILES} ; do \
	  touch $(DESTDIR)$(scoredir)/glines.$$i.scores; \
	  chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/glines.$$i.scores; \
	  chmod 664 $(DESTDIR)$(scoredir)/glines.$$i.scores; \
	done

install-data-local: install-scorefiles-local

-include $(top_srcdir)/git.mk
