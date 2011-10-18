if ENABLE_EXAMPLES_COMMON
noinst_LTLIBRARIES = libexamplescommon.la
endif

libexamplescommon_la_SOURCES = DynamicByteArray.cpp
libexamplescommon_la_CXXFLAGS = $(LIBMXFPP_CFLAGS)



if ENABLE_EXAMPLES_COMMON
library_includedir = ${includedir}/libMXF++-@LIBMXFPP_MAJORMINOR@/libMXF++/examples/Common
library_include_HEADERS = \
	CommonTypes.h \
	DynamicByteArray.h
endif
