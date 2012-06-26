if ENABLE_AVID_WRITER
noinst_LTLIBRARIES = libavidclipwriter.la
endif

libavidclipwriter_la_SOURCES = \
	AvidClipWriter.cpp \
	AvidClipWriter.h

libavidclipwriter_la_CXXFLAGS = $(LIBMXFPP_CFLAGS)
libavidclipwriter_la_LIBADD = \
	${top_builddir}/examples/Common/libexamplescommon.la \
	$(LIBMXFPP_LDADDLIBS)


if ENABLE_AVID_WRITER
noinst_PROGRAMS = avidclipwriter
endif

avidclipwriter_SOURCES = main.cpp
avidclipwriter_CXXFLAGS = $(LIBMXFPP_CFLAGS)
avidclipwriter_LDADD = libavidclipwriter.la $(LIBMXFPP_LDADDLIBS)

