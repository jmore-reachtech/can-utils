#
# Binary programs
#

bin_PROGRAMS = candump canecho cansend canconfig

candump_SOURCES   = candump.c
canecho_SOURCES   = canecho.c
cansend_SOURCES   = cansend.c
canconfig_SOURCES = canconfig.c

EXTRA_DIST = autogen.sh can.h

man_MANS = canconfig.8 candump.8

# info_TEXINFOS = can.texinfo

MAINTAINERCLEANFILES=		\
	configure		\
	Makefile.in		\
	config.guess		\
	config.sub		\
	ltmain.sh		\
	.Makefile.am.swp	\
	aclocal.m4


