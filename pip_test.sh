#!/bin/sh

PIP_TESTS="\
	boulet.pip \
	brisebarre.pip \
	cg1.pip \
	esced.pip \
	ex2.pip \
	ex.pip \
	fimmel.pip \
	max.pip \
	negative.pip \
	small.pip \
	sor1d.pip \
	square.pip \
	sven.pip \
	tobi.pip"

for i in $PIP_TESTS; do
	echo $i;
	./isl_pip$EXEEXT -T < $srcdir/test_inputs/$i || exit
done
