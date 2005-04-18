# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)

AC_INIT([canutils], [1.0.3], [socket-can@pengutronix.de])
AC_CONFIG_HEADERS([include/can_config.h])
AC_CONFIG_SRCDIR([cansend.c])
AC_CONFIG_MACRO_DIR([config/m4])
AC_CONFIG_AUX_DIR([config/autoconf])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AM_INIT_AUTOMAKE([foreign no-exeext dist-bzip2])
AM_MAINTAINER_MODE

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([	\
	fcntl.h		\
	limits.h	\
	stdlib.h	\
	string.h	\
	termios.h	\
	unistd.h	\
			\
	netinet/in.h	\
			\
	sys/ioctl.h	\
	sys/socket.h	\
	])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CONFIG_FILES([ \
	Makefile \
	config/canutils.pc \
	config/Makefile \
	include/Makefile \
	])

AC_OUTPUT
