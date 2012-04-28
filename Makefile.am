SUBDIRS = libMXF++ examples test tools msvc_build


ACLOCAL_AMFLAGS = -I m4


pkgconfig_DATA = libMXF++-$(LIBMXFPP_MAJORMINOR).pc

libMXF++-$(LIBMXFPP_MAJORMINOR).pc: libMXF++.pc
	cp libMXF++.pc libMXF++-$(LIBMXFPP_MAJORMINOR).pc

CLEANFILES = libMXF++-$(LIBMXFPP_MAJORMINOR).pc


BUILT_SOURCES = scm_version.h

scm_version.h:
	sh $(top_srcdir)/gen_scm_version.sh

.PHONY: scm_version.h

DISTCLEANFILES = scm_version.h


EXTRA_DIST = \
	autogen.sh \
	gen_scm_version.sh \
	scm_version.h

