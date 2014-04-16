AC_INIT([libMXF++], [1.0.3], [], [libmxf++])
AC_CONFIG_SRCDIR([libMXF++/File.cpp])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall parallel-tests])
AM_SILENT_RULES([yes])

dnl define for automake 1.12 to avoid warnings
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])


dnl-----------------------------------------------------------------------------
dnl -- Version
dnl-----------------------------------------------------------------------------

dnl CURRENT, REVISION, AGE
dnl - library source changed -> increment REVISION
dnl - interfaces added/removed/changed -> increment CURRENT, REVISION = 0
dnl - interfaces added -> increment AGE
dnl - interfaces removed -> AGE = 0
LIBMXFPP_LIBVERSION="4:0:0"
AC_SUBST(LIBMXFPP_LIBVERSION)


LIBMXF_MAJORMINOR=1.0
LIBMXFPP_MAJORMINOR=${LIBMXF_MAJORMINOR}
AC_SUBST(LIBMXF_MAJORMINOR)
AC_SUBST(LIBMXFPP_MAJORMINOR)


dnl-----------------------------------------------------------------------------
dnl -- Checks for programs.
dnl-----------------------------------------------------------------------------

AC_PROG_LIBTOOL

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL


dnl-----------------------------------------------------------------------------
dnl -- Enable options
dnl-----------------------------------------------------------------------------

AC_ARG_ENABLE(examples,
	AC_HELP_STRING([--disable-examples],
		[disable building of all examples [[default=no]]]),
	[enable_examples=$enableval],
	[enable_examples="yes"])


AC_ARG_ENABLE(archive-reader,
	AC_HELP_STRING([--disable-archive-reader],
		[disable building of archive reader example [[default=no]]]),
	[enable_archive_reader=$enableval],
	[enable_archive_reader="default"])
if test x"$enable_archive_reader" = xdefault && test x"$enable_examples" = xyes; then
	enable_archive_reader="yes"
fi
AM_CONDITIONAL(ENABLE_ARCHIVE_READER, test x"$enable_archive_reader" = xyes)


AC_ARG_ENABLE(archive-writer,
	AC_HELP_STRING([--disable-archive-writer],
		[disable building of archive writer example [[default=no]]]),
	[enable_archive_writer=$enableval],
	[enable_archive_writer="default"])
if test x"$enable_archive_writer" = xdefault && test x"$enable_examples" = xyes; then
	enable_archive_writer="yes"
fi
AM_CONDITIONAL(ENABLE_ARCHIVE_WRITER, [test x"$enable_archive_writer" = xyes])


AC_ARG_ENABLE(avid-writer,
	AC_HELP_STRING([--disable-avid-writer],
		[disable building of Avid clip writer example [[default=no]]]),
	[enable_avid_writer=$enableval],
	[enable_avid_writer="default"])
if test x"$enable_avid_writer" = xdefault && test x"$enable_examples" = xyes; then
	enable_avid_writer="yes"
fi
AM_CONDITIONAL(ENABLE_AVID_WRITER, [test x"$enable_avid_writer" = xyes])


AC_ARG_ENABLE(d10-writer,
	AC_HELP_STRING([--disable-d10-writer],
		[disable building of D10 writer example [[default=no]]]),
	[enable_d10_writer=$enableval],
	[enable_d10_writer="default"])
if test x"$enable_d10_writer" = xdefault && test x"$enable_examples" = xyes; then
	enable_d10_writer="yes"
fi
AM_CONDITIONAL(ENABLE_D10_WRITER, [test x"$enable_d10_writer" = xyes])


AC_ARG_ENABLE(opatom-reader,
	AC_HELP_STRING([--disable-opatom-reader],
		[disable building of OP Atom reader example [[default=no]]]),
	[enable_opatom_reader=$enableval],
	[enable_opatom_reader="default"])
if test x"$enable_opatom_reader" = xdefault && test x"$enable_examples" = xyes; then
	enable_opatom_reader="yes"
fi
AM_CONDITIONAL(ENABLE_OPATOM_READER, [test x"$enable_opatom_reader" = xyes])


AM_CONDITIONAL(ENABLE_EXAMPLES_COMMON,
	[test x"$enable_archive_reader" = xyes ||
	 test x"$enable_archive_writer" = xyes ||
	 test x"$enable_avid_writer" = xyes ||
	 test x"$enable_d10_writer" = xyes ||
	 test x"$enable_opatom_reader" = xyes])


dnl-----------------------------------------------------------------------------
dnl -- Checks for libraries.
dnl-----------------------------------------------------------------------------


dnl-----------------------------------------------------------------------------
dnl -- Check for modules
dnl-----------------------------------------------------------------------------

PKG_CHECK_MODULES(LIBMXF, libMXF-1.0 >= 1.0.3)
AC_SUBST(LIBMXF_CFLAGS)
AC_SUBST(LIBMXF_LIBS)


dnl-----------------------------------------------------------------------------
dnl -- Checks for header files.
dnl-----------------------------------------------------------------------------


dnl-----------------------------------------------------------------------------
dnl -- Checks for typedefs, structures, and compiler characteristics.
dnl-----------------------------------------------------------------------------

AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T


dnl mingw doesn't support the 'z' printf format length modifier for size_t
case "$host" in
	*-*-*mingw32) AC_DEFINE([PRIszt], ["u"], [Define printf size_t format specifier]) ;;
			   *) AC_DEFINE([PRIszt], ["zu"], [Define printf size_t format specifier])
esac


dnl-----------------------------------------------------------------------------
dnl -- Checks for library functions
dnl-----------------------------------------------------------------------------

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO


AC_CHECK_FUNCS([memset strerror])


dnl-----------------------------------------------------------------------------
dnl -- Compiler flags etc.
dnl-----------------------------------------------------------------------------

WARN_CFLAGS=
AC_APPEND_SUPPORTED_CFLAGS(WARN_CFLAGS, [-W -Wall])
AC_SUBST(WARN_CFLAGS)

LIBMXFPP_CFLAGS="$WARN_CFLAGS $LIBMXF_CFLAGS -I\$(top_srcdir)"
AC_SUBST(LIBMXFPP_CFLAGS)

LIBMXFPP_LIBADDLIBS="$LIBMXF_LIBS"
AC_SUBST(LIBMXFPP_LIBADDLIBS)

LIBMXFPP_LDADDLIBS="$LIBMXFPP_LIBADDLIBS \
	\$(top_builddir)/libMXF++/libMXF++-$LIBMXFPP_MAJORMINOR.la"
AC_SUBST(LIBMXFPP_LDADDLIBS)

LIBMXFPP_LDFLAGS=
AC_APPEND_SUPPORTED_LDFLAGS(LIBMXFPP_LDFLAGS, [-Wl,--no-undefined])
AC_SUBST(LIBMXFPP_LDFLAGS)


dnl-----------------------------------------------------------------------------
dnl -- Package Config
dnl-----------------------------------------------------------------------------

pkgconfigdir="\$(libdir)/pkgconfig"
AC_SUBST(pkgconfigdir)


dnl-----------------------------------------------------------------------------
dnl -- Config files
dnl-----------------------------------------------------------------------------

AC_CONFIG_FILES([
	Makefile
    libMXF++/Makefile
    libMXF++/metadata/Makefile
    libMXF++/metadata/base/Makefile
    libMXF++/extensions/Makefile
    examples/Makefile
    examples/Common/Makefile
    examples/D10MXFOP1AWriter/Makefile
    examples/OPAtomReader/Makefile
    test/Makefile
    tools/Makefile
    tools/gen_classes/Makefile
    msvc_build/Makefile
    msvc_build/vs10/Makefile
    libMXF++.pc])

AC_OUTPUT

