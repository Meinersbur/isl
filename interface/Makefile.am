AUTOMAKE_OPTIONS = nostdinc

noinst_PROGRAMS = extract_interface

AM_CXXFLAGS = $(CLANG_CXXFLAGS)
AM_LDFLAGS = $(CLANG_LDFLAGS)

includes = -I$(top_builddir) -I$(top_srcdir) \
	-I$(top_builddir)/include -I$(top_srcdir)/include

extract_interface_CPPFLAGS = $(includes)
extract_interface_SOURCES = \
	generator.h \
	generator.cc \
	python.h \
	python.cc \
	cpp.h \
	cpp.cc \
	extract_interface.h \
	extract_interface.cc
extract_interface_LDADD = \
	-lclangFrontend -lclangSerialization -lclangParse -lclangSema \
	$(LIB_CLANG_EDIT) \
	-lclangAnalysis -lclangAST -lclangLex -lclangBasic -lclangDriver \
	$(CLANG_LIBS) $(CLANG_LDFLAGS)

CLEANFILES = isl.py isl-noexceptions.h

test: extract_interface
	./extract_interface$(EXEEXT) $(includes) $(srcdir)/all.h

isl.py: extract_interface isl.py.top
	(cat $(srcdir)/isl.py.top; \
		./extract_interface$(EXEEXT) --language=python $(includes) \
			$(srcdir)/all.h) \
			> isl.py

isl-noexceptions.h: extract_interface all.h isl.h.top
	(cat $(srcdir)/isl.h.top; \
		./extract_interface$(EXEEXT) --language=cpp $(includes) \
			$(srcdir)/all.h) \
			> isl-noexceptions.h

dist-hook: isl.py isl-noexceptions.h
	cp isl.py isl-noexceptions.h $(distdir)/
