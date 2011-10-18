if ENABLE_ARCHIVE_READER
noinst_LTLIBRARIES = libarchivemxfreader.la
endif

libarchivemxfreader_la_SOURCES = \
	ArchiveMXFContentPackage.cpp \
	ArchiveMXFReader.cpp \
	ArchiveMXFContentPackage.h \
	ArchiveMXFReader.h

libarchivemxfreader_la_CXXFLAGS = $(LIBMXFPP_CFLAGS) "-I${top_srcdir}/examples/Common"
libarchivemxfreader_la_LIBADD = ${top_builddir}/examples/Common/libexamplescommon.la


if ENABLE_ARCHIVE_READER
noinst_PROGRAMS = test_archivemxfreader
endif

test_archivemxfreader_SOURCES = test_archivemxfreader.cpp
test_archivemxfreader_CXXFLAGS = $(LIBMXFPP_CFLAGS) "-I${top_srcdir}/examples/Common"
test_archivemxfreader_LDADD = libarchivemxfreader.la $(LIBMXFPP_LDADDLIBS)

