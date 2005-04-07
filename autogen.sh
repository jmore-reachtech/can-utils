#!/bin/sh
ACLOCAL=${ACLOCAL:=aclocal}
AUTOHEADER=${AUTOHEADER:=autoheader}
AUTOMAKE=${AUTOMAKE:=automake}
AUTOCONF=${AUTOCONF:=autoconf}

echo use aclocal: $ACLOCAL
echo use autoheader: $AUTOHEADER
echo use automake: $AUTOMAKE
echo use autoconf: $AUTOCONF

$ACLOCAL --version | \
   awk -vPROG="aclocal" -vVERS=1.7\
   '{if ($1 == PROG) {gsub ("-.*","",$4); if ($4 < VERS) print PROG" < version "VERS"\nThis may result in errors\n"}}'

$AUTOMAKE --version | \
   awk -vPROG="automake" -vVERS=1.7\
   '{if ($1 == PROG) {gsub ("-.*","",$4); if ($4 < VERS) print PROG" < version "VERS"\nThis may result in errors\n"}}'

$ACLOCAL -I config/m4 && \
libtoolize --force && \
$AUTOHEADER && \
$AUTOMAKE --gnu --add-missing -Wall && \
$AUTOCONF -Wall

