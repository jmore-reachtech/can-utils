#
# Binary programs
#

bin_PROGRAMS = candump canecho cansend
sbin_PROGRAMS = canconfig

SUBDIRS = include

AM_CPPFLAGS = \
	-I$(top_srcdir)/include
	-I$(top_builddir)/include

candump_SOURCES   = candump.c
canecho_SOURCES   = canecho.c
cansend_SOURCES   = cansend.c
canconfig_SOURCES = canconfig.c

EXTRA_DIST = autogen.sh can.h canconfig.8 candump.8 canecho.8 cansend.8

man_MANS = canconfig.8 candump.8 canecho.8 cansend.8

# info_TEXINFOS = can.texinfo

MAINTAINERCLEANFILES=		\
	include/can_config.h.in	\
	configure		\
	Makefile.in		\
	config.guess		\
	config.sub		\
	ltmain.sh		\
	aclocal.m4


