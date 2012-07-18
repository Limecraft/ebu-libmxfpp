if ENABLE_D10_WRITER
lib_LTLIBRARIES = libd10mxfop1awriter-@LIBMXFPP_MAJORMINOR@.la
endif

libd10mxfop1awriter_@LIBMXFPP_MAJORMINOR@_la_SOURCES = \
	D10ContentPackage.cpp \
	D10MXFOP1AWriter.cpp

libd10mxfop1awriter_@LIBMXFPP_MAJORMINOR@_la_CXXFLAGS = $(LIBMXFPP_CFLAGS)
libd10mxfop1awriter_@LIBMXFPP_MAJORMINOR@_la_LDFLAGS = -version-info $(LIBMXFPP_LIBVERSION)
libd10mxfop1awriter_@LIBMXFPP_MAJORMINOR@_la_LIBADD = \
	${top_builddir}/examples/Common/libexamplescommon.la \
	$(LIBMXFPP_LDADDLIBS)


if ENABLE_D10_WRITER
noinst_PROGRAMS = test_d10mxfop1awriter
endif

test_d10mxfop1awriter_SOURCES = test_d10mxfop1awriter.cpp
test_d10mxfop1awriter_CXXFLAGS = $(LIBMXFPP_CFLAGS)
test_d10mxfop1awriter_LDADD = libd10mxfop1awriter-@LIBMXFPP_MAJORMINOR@.la $(LIBMXFPP_LDADDLIBS)



if ENABLE_D10_WRITER
library_includedir = ${includedir}/libMXF++-@LIBMXFPP_MAJORMINOR@/libMXF++/examples/D10MXFOP1AWriter
library_include_HEADERS = \
	D10ContentPackage.h \
	D10MXFOP1AWriter.h
endif

