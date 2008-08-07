#!/bin/sh
libtoolize -c
aclocal
autoheader
automake -a -c --foreign
autoconf
if test -f piplib/autogen.sh; then
	(cd piplib; ./autogen.sh)
fi
