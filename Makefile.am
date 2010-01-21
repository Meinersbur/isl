SUBDIRS = . doc
DIST_SUBDIRS = doc

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = nostdinc

lib_LTLIBRARIES = libisl.la
noinst_PROGRAMS = isl_test isl_polyhedron_sample isl_pip \
	isl_polyhedron_minimize isl_polytope_scan \
	isl_polyhedron_detect_equalities isl_cat \
	isl_closure isl_bound
TESTS = isl_test pip_test.sh bound_test.sh

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

INCLUDES = -I. -I$(srcdir) -I$(srcdir)/include -Iinclude/ 

libisl_la_SOURCES = \
	$(ISL_PIPLIB) \
	$(GET_MEMORY_FUNCTIONS) \
	isl_affine_hull.c \
	isl_arg.c \
	isl_basis_reduction.h \
	basis_reduction_tab.c \
	isl_bernstein.c \
	isl_bernstein.h \
	isl_blk.c \
	isl_bound.c \
	isl_bound.h \
	isl_coalesce.c \
	isl_constraint.c \
	isl_convex_hull.c \
	isl_ctx.c \
	isl_dim.c \
	isl_dim_private.h \
	isl_div.c \
	isl_equalities.c \
	isl_equalities.h \
	isl_factorization.c \
	isl_factorization.h \
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
	isl_mat_private.h \
	isl_morph.c \
	isl_morph.h \
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
	isl_range.c \
	isl_range.h \
	isl_reordering.c \
	isl_reordering.h \
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
	isl_union_map.c \
	isl_union_map_private.h \
	isl_vec.c \
	isl_version.c \
	isl_vertices_private.h \
	isl_vertices.c
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
libisl_la_CPPFLAGS = $(INCLUDES) @PIPLIB_CPPFLAGS@ @GMP_CPPFLAGS@

isl_test_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_test_LDADD = libisl.la

isl_polyhedron_sample_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_polyhedron_sample_LDADD = libisl.la
isl_polyhedron_sample_SOURCES = \
	polyhedron_sample.c

isl_pip_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_pip_LDADD = libisl.la
isl_pip_SOURCES = \
	pip.c

isl_bound_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_bound_LDADD = libisl.la
isl_bound_SOURCES = \
	bound.c

isl_polyhedron_minimize_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_polyhedron_minimize_LDADD = libisl.la
isl_polyhedron_minimize_SOURCES = \
	polyhedron_minimize.c

isl_polytope_scan_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_polytope_scan_LDADD = libisl.la
isl_polytope_scan_SOURCES = \
	polytope_scan.c

isl_polyhedron_detect_equalities_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_polyhedron_detect_equalities_LDADD = libisl.la
isl_polyhedron_detect_equalities_SOURCES = \
	polyhedron_detect_equalities.c

isl_cat_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_cat_LDADD = libisl.la
isl_cat_SOURCES = \
	cat.c

isl_closure_CPPFLAGS = $(INCLUDES) @GMP_CPPFLAGS@
isl_closure_LDADD = libisl.la
isl_closure_SOURCES = \
	closure.c

nodist_pkginclude_HEADERS = \
	include/isl/config.h \
	include/isl/stdint.h
pkginclude_HEADERS = \
	include/isl/arg.h \
	include/isl/blk.h \
	include/isl/constraint.h \
	include/isl/ctx.h \
	include/isl/dim.h \
	include/isl/div.h \
	include/isl/flow.h \
	include/isl/ilp.h \
	include/isl/int.h \
	include/isl/hash.h \
	include/isl/list.h \
	include/isl/lp.h \
	include/isl/mat.h \
	include/isl/map.h \
	include/isl/obj.h \
	include/isl/options.h \
	include/isl/point.h \
	include/isl/polynomial.h \
	include/isl/printer.h \
	include/isl/seq.h \
	include/isl/set.h \
	include/isl/stream.h \
	include/isl/union_map.h \
	include/isl/union_set.h \
	include/isl/vec.h \
	include/isl/version.h \
	include/isl/vertices.h

EXTRA_DIST = \
	basis_reduction_templ.c \
	isl_pw_templ.c \
	isl_union_templ.c \
	doc/chicago.bst \
	doc/chicago.sty \
	doc/implementation.tex \
	doc/isl.bib \
	doc/mypod2latex \
	doc/manual.tex \
	doc/user.pod \
	bound_test.sh \
	pip_test.sh \
	test_inputs

dist-hook:
	echo @GIT_HEAD_VERSION@ > $(distdir)/GIT_HEAD_ID
	(cd doc; make manual.pdf)
	cp doc/manual.pdf $(distdir)/doc/

pkgconfigdir=$(pkgconfig_libdir)
pkgconfig_DATA = $(pkgconfig_libfile)

gitversion.h: @GIT_HEAD@
	echo '#define GIT_HEAD_ID "'@GIT_HEAD_VERSION@'"' > $@
