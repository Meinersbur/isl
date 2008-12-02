if BUNDLED_PIPLIB
MAYBE_PIPLIB = piplib
endif

SUBDIRS = $(MAYBE_PIPLIB) .
DIST_SUBDIRS = $(MAYBE_PIPLIB)

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libisl.la
noinst_PROGRAMS = isl_test
TESTS = isl_test

if HAVE_POLYLIB
ISL_POLYLIB = \
	isl_map_polylib.c
endif

if HAVE_PIPLIB
ISL_PIPLIB = \
	isl_lp_piplib.c \
	isl_map_piplib.c \
	isl_sample_piplib.c \
	isl_sample_piplib.h \
	isl_piplib.c
else
ISL_PIPLIB = \
	isl_lp_no_piplib.c \
	isl_map_no_piplib.c \
	isl_sample_no_piplib.c
endif
if BUNDLED_PIPLIB
PIPLIB_LA = $(top_builddir)/piplib/libpiplibMP.la
endif

if NEED_GET_MEMORY_FUNCTIONS
GET_MEMORY_FUNCTIONS=mp_get_memory_functions.c
endif

libisl_la_SOURCES = \
	$(ISL_PIPLIB) \
	$(ISL_POLYLIB) \
	$(GET_MEMORY_FUNCTIONS) \
	isl_affine_hull.c \
	isl_blk.c \
	isl_constraint.c \
	isl_convex_hull.c \
	isl_ctx.c \
	isl_equalities.c \
	isl_equalities.h \
	isl_gmp.c \
	isl_input.c \
	isl_input_omega.c \
	isl_input_omega.h \
	isl_list.c \
	isl_lp.c \
	isl_map.c \
	isl_map_private.h \
	isl_mat.c \
	isl_mat.h \
	isl_output.c \
	isl_sample.h \
	isl_sample.c \
	isl_seq.c \
	isl_vec.c \
	isl_vec.h
EXTRA_libisl_la_SOURCES = \
	isl_lp_piplib.c \
	isl_lp_no_piplib.c \
	isl_map_piplib.c \
	isl_map_no_piplib.c \
	isl_map_polylib.c \
	isl_sample_no_piplib.c \
	isl_sample_piplib.c \
	isl_sample_piplib.h \
	isl_piplib.c
libisl_la_LIBADD = $(PIPLIB_LA) @PIPLIB_LIBS@ @POLYLIB_LIBS@ -lgmp
libisl_la_LDFLAGS = -release @VERSION@ @PIPLIB_LDFLAGS@ @POLYLIB_LDFLAGS@ \
	@GMP_LDFLAGS@
libisl_la_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@PIPLIB_CPPFLAGS@ @POLYLIB_CPPFLAGS@ \
	@GMP_CPPFLAGS@

isl_test_CPPFLAGS = -I$(srcdir)/include -Iinclude/ @GMP_CPPFLAGS@
isl_test_LDADD = libisl.la

nodist_pkginclude_HEADERS = \
	include/isl_ctx.h \
	include/isl_stdint.h
pkginclude_HEADERS = \
	include/isl_blk.h \
	include/isl_constraint.h \
	include/isl_int.h \
	include/isl_list.h \
	include/isl_lp.h \
	include/isl_lp_piplib.h \
	include/isl_map.h \
	include/isl_map_piplib.h \
	include/isl_map_polylib.h \
	include/isl_piplib.h \
	include/isl_polylib.h \
	include/isl_seq.h \
	include/isl_set.h \
	include/isl_set_polylib.h

EXTRA_DIST = test_inputs
