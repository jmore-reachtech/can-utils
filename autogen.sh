#!/bin/sh

echo "running libtoolize (tested with 1.5.6)..."
libtoolize --force -c 

echo "running aclocal (tested with 1.7.9)..."
aclocal

echo "running autoconf (tested with 2.59)..."
autoconf

echo "running automake (tested with 1.7.9)..."
automake

