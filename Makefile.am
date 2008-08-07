if BUNDLED_PIPLIB
MAYBE_PIPLIB = piplib
endif

SUBDIRS = $(MAYBE_PIPLIB) .

lib_LTLIBRARIES = libisl.la

if HAVE_POLYLIB
ISL_POLYLIB = \
	isl_map_polylib.c \
	isl_map_polylib.h \
	isl_polylib.h
endif

if HAVE_PIPLIB
ISL_PIPLIB = \
	isl_lp_piplib.h \
	isl_lp_piplib.c \
	isl_map_piplib.c \
	isl_map_piplib.h \
	isl_piplib.h \
	isl_piplib.c
endif
if BUNDLED_PIPLIB
PIPLIB_LA = $(top_builddir)/piplib/libpiplibMP.la
endif

libisl_la_SOURCES = \
	$(ISL_PIPLIB) \
	$(ISL_POLYLIB) \
	isl_blk.c \
	isl_blk.h \
	isl_ctx.c \
	isl_ctx.h \
	isl_gmp.c \
	isl_int.h \
	isl_lp.h \
	isl_lp.c \
	isl_map.c \
	isl_map.h \
	isl_map_affine_hull.c \
	isl_map_private.h \
	isl_set.h \
	isl_seq.c \
	isl_seq.h
EXTRA_libisl_la_SOURCES = \
	isl_lp_piplib.h \
	isl_lp_piplib.c \
	isl_map_piplib.c \
	isl_map_piplib.h \
	isl_map_polylib.c \
	isl_map_polylib.h \
	isl_piplib.h \
	isl_piplib.c \
	isl_polylib.h
libisl_la_LIBADD = $(PIPLIB_LA) @PIPLIB_LIBS@ @POLYLIB_LIBS@ -lgmp
libisl_la_LDFLAGS = -release @VERSION@ @PIPLIB_LDFLAGS@ @POLYLIB_LDFLAGS@ \
	@GMP_LDFLAGS@
libisl_la_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@PIPLIB_CPPFLAGS@ @POLYLIB_CPPFLAGS@ \
	@GMP_CPPFLAGS@

pkginclude_HEADERS = \
	include/isl_blk.h \
	include/isl_ctx.h \
	include/isl_int.h \
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
