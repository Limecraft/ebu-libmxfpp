SUBDIRS = libMXF++ examples test tools msvc_build


ACLOCAL_AMFLAGS = -I m4


pkgconfig_DATA = libMXF++-$(LIBMXFPP_MAJORMINOR).pc

libMXF++-$(LIBMXFPP_MAJORMINOR).pc: libMXF++.pc
	cp libMXF++.pc libMXF++-$(LIBMXFPP_MAJORMINOR).pc

CLEANFILES = libMXF++-$(LIBMXFPP_MAJORMINOR).pc


BUILT_SOURCES = mxfpp_scm_version.h

mxfpp_scm_version.h:
	sh $(top_srcdir)/gen_scm_version.sh mxfpp_scm_version.h $(top_srcdir)/mxfpp_scm_version.h LIBMXFPP_SCM_VERSION

.PHONY: mxfpp_scm_version.h

DISTCLEANFILES = mxfpp_scm_version.h


EXTRA_DIST = \
	autogen.sh \
	gen_scm_version.sh \
	mxfpp_scm_version.h

