## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
  autogen.sh \
  NEC2-bug.txt 

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = nec2c

nec2c_SOURCES = \
  calculations.c \
  geometry.c \
  input.c \
  matrix.c \
  network.c \
  shared.c \
  fields.c \
  ground.c \
  main.c \
  misc.c \
  radiation.c \
  somnec.c \
  nec2c.h \
  shared.h

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

