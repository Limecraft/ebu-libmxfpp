if ENABLE_ARCHIVE_WRITER
noinst_LTLIBRARIES = libarchivemxfwriter.la
endif

libarchivemxfwriter_la_SOURCES = \
	${top_srcdir}/examples/ArchiveMXFReader/ArchiveMXFContentPackage.cpp \
	ArchiveMXFWriter.cpp \
	ArchiveMXFWriter.h \
	ArchiveMXFExtensionsDataModel.h

libarchivemxfwriter_la_CXXFLAGS = $(LIBMXFPP_CFLAGS) \
	"-I${top_srcdir}/examples/Common" \
	"-I${top_srcdir}/examples/ArchiveMXFReader"
libarchivemxfwriter_la_LIBADD = \
	${top_builddir}/examples/Common/libexamplescommon.la \
	$(LIBMXFPP_LDADDLIBS)


if ENABLE_ARCHIVE_WRITER
noinst_PROGRAMS = test_archivemxfwriter
endif

test_archivemxfwriter_SOURCES = test_archivemxfwriter.cpp
test_archivemxfwriter_CXXFLAGS = $(LIBMXFPP_CFLAGS) \
	"-I${top_srcdir}/examples/Common" \
	"-I${top_srcdir}/examples/ArchiveMXFReader"
test_archivemxfwriter_LDADD = libarchivemxfwriter.la $(LIBMXFPP_LDADDLIBS)

