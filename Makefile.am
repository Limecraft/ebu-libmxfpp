SUBDIRS = libMXF++ examples test tools msvc_build


ACLOCAL_AMFLAGS = -I m4


pkgconfig_DATA = libMXF++-$(LIBMXFPP_MAJORMINOR).pc

libMXF++-$(LIBMXFPP_MAJORMINOR).pc: libMXF++.pc
	cp libMXF++.pc libMXF++-$(LIBMXFPP_MAJORMINOR).pc

CLEANFILES = libMXF++-$(LIBMXFPP_MAJORMINOR).pc



EXTRA_DIST = autogen.sh
