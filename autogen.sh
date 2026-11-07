#!/bin/sh
# Regenerate the autoconf/automake build system after editing
# configure.ac or any Makefile.am.
set -e
aclocal
automake --add-missing
autoconf
