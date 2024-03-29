# Process this file with autoconf to produce a configure script
AC_INIT([wirebot],[2.0],[dev@read-write.fr])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Include shared macros
m4_include([libwired/config.m4])

# Get host
AC_CANONICAL_HOST

# Get Subversion revision
WI_CHECK_SVN_REVISION

# These are used in the source code
AC_DEFINE([WR_VERSION], ["AC_PACKAGE_VERSION"], [Application version])
AC_DEFINE([WR_PROTOCOL_VERSION], ["1.1"], [Protocol version])
AC_DEFINE([WR_BUGREPORT], ["AC_PACKAGE_BUGREPORT"], [Maintainer's email address])

# These are used in the Makefile
AC_SUBST([WR_VERSION], [AC_PACKAGE_VERSION])


#######################################################################
# Checks for compilers

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET


#######################################################################
# Checks for flags

# Check for warnings
AC_MSG_CHECKING([for warning flags])
AC_ARG_ENABLE([warnings], AS_HELP_STRING([--enable-warnings],[enable warnings]))

if test -n "$enable_warnings"; then
	WI_INCLUDE_WARNING_FLAG([-W])
	WI_INCLUDE_WARNING_FLAG([-Wall])
	WI_INCLUDE_WARNING_FLAG([-Wno-unknown-pragmas])
	WI_INCLUDE_WARNING_FLAG([-Wno-unused-parameter])
	WI_INCLUDE_WARNING_FLAG([-Wsign-compare])
	WI_INCLUDE_WARNING_FLAG([-Wstrict-prototypes])
	WI_INCLUDE_WARNING_FLAG([-Wmissing-prototypes])
	WI_INCLUDE_WARNING_FLAG([-Wmissing-declarations])
	WI_INCLUDE_WARNING_FLAG([-Wredundant-decls])

	AC_MSG_RESULT([$WARNFLAGS])
else
	AC_MSG_RESULT([none])
fi


# Check for object directory name
AC_ARG_WITH([objdir])

if test -n "$with_objdir"; then
	AC_SUBST([objdir], ["$with_objdir"])
	wr_libwired_objdir="$with_objdir"
else
	AC_SUBST([objdir], ["obj"])
	wr_libwired_objdir="../obj"
fi


# Check for run directory name
AC_ARG_WITH([rundir])

if test -n "$with_rundir"; then
	AC_SUBST([rundir], ["$with_rundir"])
	wr_libwired_rundir="$with_rundir/libwired"
else
	AC_SUBST([rundir], ["run"])
	wr_libwired_rundir="../run/libwired"
fi


# Check for maintainer mode
AC_ARG_ENABLE([maintainer_mode])

if test -n "$enable_maintainer_mode"; then
	AC_SUBST([WR_MAINTAINER], [1])
else
	AC_SUBST([WR_MAINTAINER], [0])
fi


#######################################################################
# Checks for additional paths

WI_INCLUDE_EXTRA_INCLUDE_PATHS
WI_INCLUDE_EXTRA_LIBRARY_PATHS


#######################################################################
# Checks for libraries

WI_CHECK_LIBWIRED
WI_INCLUDE_PTHREADS
WI_INCLUDE_LIBWIRED_LIBRARIES
WI_INCLUDE_P7_LIBRARIES
WI_INCLUDE_ICONV_LIBRARY
WI_INCLUDE_TERMCAP_LIBRARY
WI_INCLUDE_READLINE_LIBRARY
WI_INCLUDE_NCURSES_LIBRARY

#######################################################################
# Checks for typedefs, structures, and compiler characteristics

AC_SYS_LARGEFILE


#######################################################################
# Output

AC_CONFIG_FILES([Makefile])

# Configure libwired
AC_ARG_WITH([libwired], , [
	if test "$with_libwired" = "yes"; then
		wr_libwired=1
	fi
], [
	wr_libwired=1
])

if test "$wr_libwired"; then
	ac_configure_args="$ac_configure_args --enable-p7 --enable-pthreads --enable-termcap --enable-iconv --enable-libxml2 --with-rundir=$wr_libwired_rundir --with-objdir=$wr_libwired_objdir"
	AC_CONFIG_SUBDIRS([libwired])
fi

AC_OUTPUT


#######################################################################
# Print summary

A=$(eval echo ${bindir});	A=$(eval echo ${A})
B=$(eval echo ${mandir});	B=$(eval echo ${B})

echo ""
echo "wirebot has been configured with the following options:"

echo ""
echo "              Binary: ${A}/wirebot"
echo "         Manual page: ${B}/man1/wirebot.1"

echo ""
echo "                Host: ${host}"
echo "            Compiler: ${CC}"
echo "      Compiler flags: ${CFLAGS}"
echo "  Preprocessor flags: ${CPPFLAGS}"
echo "        Linker flags: ${LDFLAGS}"
echo "           Libraries: ${LIBS}"

echo ""
