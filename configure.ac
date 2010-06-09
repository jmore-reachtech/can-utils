# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)

AC_INIT([canutils], [4.0.6], [bugs@pengutronix.de])
AC_CONFIG_HEADERS([include/can_config.h])
AC_CONFIG_SRCDIR([src/canconfig.c])
AC_CONFIG_MACRO_DIR([config/m4])
AC_CONFIG_AUX_DIR([config/autoconf])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

CFLAGS="${CFLAGS} -Wall -g"

#
# Checks for programs.
#
AC_PROG_CC
AM_MISSING_PROG(PERL, perl, $missing_dir)
# libtool, old:
AC_LIBTOOL_WIN32_DLL
#AC_LIBTOOL_TAGS([])
AC_PROG_LIBTOOL
# libtool, new:
# LT_INIT(win32-dll)

AM_INIT_AUTOMAKE([foreign no-exeext dist-bzip2])


#
# Checks for libraries.
#


#
# Checks for header files.
#
AC_HEADER_STDC
AC_CHECK_HEADERS([ \
	fcntl.h \
	limits.h \
	stdlib.h \
	string.h \
	unistd.h \
	\
	sys/ioctl.h \
	sys/socket.h \
])

#
# Checks for typedefs, structures, and compiler characteristics.
#
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T


#
# Checks for library functions.
#
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([ \
	socket \
	strchr \
	strtoul \
])

PKG_CHECK_MODULES([libsocketcan],
		  [libsocketcan >= 0.0.8],
		  [],
		  [AC_MSG_ERROR([*** libsocketcan version above 0.0.8 not found on your system])])
AC_SUBST(libsocketcan_CFLAGS)
AC_SUBST(libsocketcan_LIBS)

#
# Debugging
#
AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug], [enable debugging @<:@default=no@:>@]),
	[case "$enableval" in
	y | yes) CONFIG_DEBUG=yes ;;
        *) CONFIG_DEBUG=no ;;
    esac],
    [CONFIG_DEBUG=no])
AC_MSG_RESULT([${CONFIG_DEBUG}])
if test "${CONFIG_DEBUG}" = "yes"; then
    CFLAGS="${CFLAGS} -Werror -g -O1"
    AC_DEFINE(DEBUG, 1, [debugging])
else
    CFLAGS="${CFLAGS} -O2"
fi


AC_CONFIG_FILES([ \
	GNUmakefile \
	config/canutils.pc \
	config/GNUmakefile \
	include/GNUmakefile \
	src/GNUmakefile \
	man/GNUmakefile
])
AC_OUTPUT
