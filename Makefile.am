AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src data doc

MAINTAINERCLEANFILES = aclocal.m4 compile \
		       config.guess config.h.in config.sub configure \
		       depcomp install-sh ltmain.sh missing Makefile.in \
		       imlib2_docs.tar.gz

bin_SCRIPTS = imlib2-config

EXTRA_DIST = AUTHORS COPYING COPYING-PLAIN gendoc \
             Doxyfile \
	     README.in README \
	     imlib2.pc.in \
	     imlib2.spec.in imlib2.spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = imlib2.pc
