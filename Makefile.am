SUBDIRS = . doc
DIST_SUBDIRS = doc

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libisl.la
noinst_PROGRAMS = isl_test isl_polyhedron_sample isl_pip \
	isl_polyhedron_minimize isl_polytope_scan \
	isl_polyhedron_detect_equalities isl_cat \
	isl_closure
TESTS = isl_test pip_test.sh

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

if NEED_GET_MEMORY_FUNCTIONS
GET_MEMORY_FUNCTIONS=mp_get_memory_functions.c
endif

libisl_la_SOURCES = \
	$(ISL_PIPLIB) \
	$(GET_MEMORY_FUNCTIONS) \
	isl_affine_hull.c \
	isl_arg.c \
	isl_basis_reduction.h \
	basis_reduction_tab.c \
	isl_blk.c \
	isl_coalesce.c \
	isl_constraint.c \
	isl_convex_hull.c \
	isl_ctx.c \
	isl_dim.c \
	isl_div.c \
	isl_equalities.c \
	isl_equalities.h \
	isl_flow.c \
	isl_fold.c \
	isl_gmp.c \
	isl_hash.c \
	isl_ilp.c \
	isl_input.c \
	isl_list.c \
	isl_lp.c \
	isl_lp_piplib.h \
	isl_map.c \
	isl_map_simplify.c \
	isl_map_subtract.c \
	isl_map_private.h \
	isl_map_piplib.h \
	isl_mat.c \
	isl_name.c \
	isl_name.h \
	isl_obj.c \
	isl_options.c \
	isl_output.c \
	isl_piplib.h \
	isl_point_private.h \
	isl_point.c \
	isl_polynomial_private.h \
	isl_polynomial.c \
	isl_printer_private.h \
	isl_printer.c \
	isl_sample.h \
	isl_sample.c \
	isl_scan.c \
	isl_scan.h \
	isl_stream.c \
	isl_seq.c \
	isl_tab.c \
	isl_tab.h \
	isl_tab_pip.c \
	isl_transitive_closure.c \
	isl_vec.c
EXTRA_libisl_la_SOURCES = \
	isl_lp_piplib.c \
	isl_lp_no_piplib.c \
	isl_map_piplib.c \
	isl_map_no_piplib.c \
	isl_sample_no_piplib.c \
	isl_sample_piplib.c \
	isl_sample_piplib.h \
	isl_piplib.c
libisl_la_LIBADD = @PIPLIB_LIBS@ -lgmp
libisl_la_LDFLAGS = -version-info @versioninfo@ \
	@PIPLIB_LDFLAGS@ @GMP_LDFLAGS@
libisl_la_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@PIPLIB_CPPFLAGS@ @GMP_CPPFLAGS@

isl_test_CPPFLAGS = -I$(srcdir)/include -Iinclude/ @GMP_CPPFLAGS@
isl_test_LDADD = libisl.la

isl_polyhedron_sample_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_polyhedron_sample_LDADD = libisl.la
isl_polyhedron_sample_SOURCES = \
	polyhedron_sample.c

isl_pip_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_pip_LDADD = libisl.la
isl_pip_SOURCES = \
	pip.c

isl_polyhedron_minimize_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_polyhedron_minimize_LDADD = libisl.la
isl_polyhedron_minimize_SOURCES = \
	polyhedron_minimize.c

isl_polytope_scan_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_polytope_scan_LDADD = libisl.la
isl_polytope_scan_SOURCES = \
	polytope_scan.c

isl_polyhedron_detect_equalities_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_polyhedron_detect_equalities_LDADD = libisl.la
isl_polyhedron_detect_equalities_SOURCES = \
	polyhedron_detect_equalities.c

isl_cat_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_cat_LDADD = libisl.la
isl_cat_SOURCES = \
	cat.c

isl_closure_CPPFLAGS = -I$(srcdir)/include -Iinclude/ \
	@GMP_CPPFLAGS@
isl_closure_LDADD = libisl.la
isl_closure_SOURCES = \
	closure.c

nodist_pkginclude_HEADERS = \
	include/isl_config.h \
	include/isl_stdint.h
pkginclude_HEADERS = \
	include/isl_arg.h \
	include/isl_blk.h \
	include/isl_constraint.h \
	include/isl_ctx.h \
	include/isl_dim.h \
	include/isl_div.h \
	include/isl_flow.h \
	include/isl_ilp.h \
	include/isl_int.h \
	include/isl_hash.h \
	include/isl_list.h \
	include/isl_lp.h \
	include/isl_mat.h \
	include/isl_map.h \
	include/isl_obj.h \
	include/isl_options.h \
	include/isl_point.h \
	include/isl_polynomial.h \
	include/isl_printer.h \
	include/isl_seq.h \
	include/isl_set.h \
	include/isl_stream.h \
	include/isl_vec.h

EXTRA_DIST = \
	basis_reduction_templ.c \
	isl_pw_templ.c \
	doc/chicago.bst \
	doc/chicago.sty \
	doc/implementation.tex \
	doc/isl.bib \
	doc/mypod2latex \
	doc/manual.tex \
	doc/user.pod \
	pip_test.sh \
	test_inputs

dist-hook:
	(cd doc; make manual.pdf)
	cp doc/manual.pdf $(distdir)/doc/

pkgconfigdir=$(pkgconfig_libdir)
pkgconfig_DATA = $(pkgconfig_libfile)
