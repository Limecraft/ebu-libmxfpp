SUBDIRS = metadata extensions .


lib_LTLIBRARIES = libMXF++-@LIBMXFPP_MAJORMINOR@.la

libMXF___@LIBMXFPP_MAJORMINOR@_la_SOURCES = \
	AvidHeaderMetadata.cpp \
	DataModel.cpp \
	File.cpp \
	HeaderMetadata.cpp \
	IndexTable.cpp \
	MetadataSet.cpp \
	MXFException.cpp \
	MXFTypes.cpp \
	MXFVersion.cpp \
	Partition.cpp

libMXF___@LIBMXFPP_MAJORMINOR@_la_CXXFLAGS = $(LIBMXFPP_CFLAGS)
libMXF___@LIBMXFPP_MAJORMINOR@_la_LDFLAGS = $(LIBMXFPP_LDFLAGS) -version-info $(LIBMXFPP_LIBVERSION)
libMXF___@LIBMXFPP_MAJORMINOR@_la_LIBADD = \
	metadata/libmetadata.la \
	metadata/base/libbase.la \
	extensions/libextensions.la \
	$(LIBMXFPP_LIBADDLIBS)



library_includedir = ${includedir}/libMXF++-@LIBMXFPP_MAJORMINOR@/libMXF++
library_include_HEADERS = \
	AvidHeaderMetadata.h \
	DataModel.h \
	File.h \
	HeaderMetadata.h \
	IndexTable.h \
	MetadataSet.h \
	MXFException.h \
	MXFTypes.h \
	MXF.h \
	MXFVersion.h \
	Partition.h
