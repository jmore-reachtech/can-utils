#!/bin/sh

echo "running aclocal (tested with 1.7.9)..."
aclocal -I config/m4

echo "running libtoolize (tested with 1.5.6)..."
libtoolize --force

echo "running autoheader"
autoheader 

echo "running automake (tested with 1.7.9)..."
automake --add-missing --gnu -Wall

echo "running autoconf (tested with 2.59)..."
autoconf