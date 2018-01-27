AUTOMAKE_OPTIONS = nostdinc

noinst_PROGRAMS = extract_interface
TESTS =
if HAVE_CXX11
  noinst_PROGRAMS += isl_test_cpp-noexceptions
  TESTS += isl_test_cpp-noexceptions
endif
if HAVE_PYTHON
  TESTS += isl_test_python.py
  isl_test_python.py: isl.py
endif
TEST_EXTENSIONS = .py
AM_TESTS_ENVIRONMENT = \
	export PYTHONPATH=.; \
	export LD_LIBRARY_PATH=../.libs;
PY_LOG_COMPILER = $(PYTHON)

includes = -I$(top_builddir) -I$(top_srcdir) \
	-I$(top_builddir)/include -I$(top_srcdir)/include

extract_interface_CPPFLAGS = $(includes)
extract_interface_CXXFLAGS = $(CLANG_CXXFLAGS)
extract_interface_SOURCES = \
	generator.h \
	generator.cc \
	python.h \
	python.cc \
	cpp.h \
	cpp.cc \
	extract_interface.h \
	extract_interface.cc
extract_interface_LDFLAGS = $(CLANG_LDFLAGS)
extract_interface_LDADD = \
	-lclangFrontend -lclangSerialization -lclangParse -lclangSema \
	$(LIB_CLANG_EDIT) \
	-lclangAnalysis -lclangAST -lclangLex -lclangBasic -lclangDriver \
	$(CLANG_LIBS) $(CLANG_LDFLAGS)

isl_test_cpp_noexceptions_CPPFLAGS = $(includes) -I.
isl_test_cpp_noexceptions_CXXFLAGS = @CXX11FLAGS@
isl_test_cpp_noexceptions_SOURCES = \
	isl_test_cpp-noexceptions.cc \
	isl-noexceptions.h
isl_test_cpp_noexceptions_LDFLAGS = @MP_LDFLAGS@
isl_test_cpp_noexceptions_LDADD = ../libisl.la @MP_LIBS@

BUILT_SOURCES = isl-noexceptions.h
CLEANFILES = isl.py isl-noexceptions.h

# dummy library that captures the dependencies on all headers
# that are relevant for the bindings
noinst_LIBRARIES = libdep.a
libdep_a_CPPFLAGS = $(includes)
libdep_a_SOURCES = all.c

isl.py: extract_interface libdep.a isl.py.top
	(cat $(srcdir)/isl.py.top; \
		./extract_interface$(EXEEXT) --language=python $(includes) \
			$(srcdir)/all.h) \
			> $@

isl-noexceptions.h: extract_interface libdep.a isl-noexceptions.h.top \
		isl-noexceptions.h.pre isl-noexceptions.h.bot
	(cat $(srcdir)/isl-noexceptions.h.top $(srcdir)/all.h \
	    $(srcdir)/isl-noexceptions.h.pre; \
		./extract_interface$(EXEEXT) --language=cpp $(includes) \
			$(srcdir)/all.h; \
		cat $(srcdir)/isl-noexceptions.h.bot) \
			> $@

dist-hook: isl.py
	cp isl.py $(distdir)/
