if ENABLE_OPATOM_READER
lib_LTLIBRARIES = libopatomreader-@LIBMXFPP_MAJORMINOR@.la
endif

libopatomreader_@LIBMXFPP_MAJORMINOR@_la_SOURCES = \
	FixedSizeEssenceParser.cpp \
	FrameOffsetIndexTable.cpp \
	OPAtomClipReader.cpp \
	OPAtomContentPackage.cpp \
	OPAtomTrackReader.cpp \
	PCMEssenceParser.cpp \
	RawEssenceParser.cpp \
	test_opatomreader.cpp \
	VariableSizeEssenceParser.cpp

libopatomreader_@LIBMXFPP_MAJORMINOR@_la_CXXFLAGS = $(LIBMXFPP_CFLAGS)
libopatomreader_@LIBMXFPP_MAJORMINOR@_la_LDFLAGS = -version-info $(LIBMXFPP_LIBVERSION)
libopatomreader_@LIBMXFPP_MAJORMINOR@_la_LIBADD = \
	${top_builddir}/examples/Common/libexamplescommon.la \
	$(LIBMXFPP_LDADDLIBS)


if ENABLE_OPATOM_READER
noinst_PROGRAMS = test_opatomreader
endif

test_opatomreader_SOURCES = test_opatomreader.cpp
test_opatomreader_CXXFLAGS = $(LIBMXFPP_CFLAGS)
test_opatomreader_LDADD = libopatomreader-@LIBMXFPP_MAJORMINOR@.la $(LIBMXFPP_LDADDLIBS)



if ENABLE_OPATOM_READER
library_includedir = ${includedir}/libMXF++-@LIBMXFPP_MAJORMINOR@/libMXF++/examples/OPAtomReader
library_include_HEADERS = \
	FixedSizeEssenceParser.h \
	FrameOffsetIndexTable.h \
	OPAtomClipReader.h \
	OPAtomContentPackage.h \
	OPAtomTrackReader.h \
	PCMEssenceParser.h \
	RawEssenceParser.h \
	VariableSizeEssenceParser.h
endif

