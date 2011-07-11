/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_OPTIONS_H
#define ISL_OPTIONS_H

#include <isl/arg.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_options {
	#define			ISL_LP_TAB	0
	#define			ISL_LP_PIP	1
	unsigned		lp_solver;

	#define			ISL_ILP_GBR	0
	#define			ISL_ILP_PIP	1
	unsigned		ilp_solver;

	#define			ISL_PIP_TAB	0
	#define			ISL_PIP_PIP	1
	unsigned		pip;

	#define			ISL_CONTEXT_GBR		0
	#define			ISL_CONTEXT_LEXMIN	1
	unsigned		context;

	#define			ISL_GBR_NEVER	0
	#define			ISL_GBR_ONCE	1
	#define			ISL_GBR_ALWAYS	2
	unsigned		gbr;
	unsigned		gbr_only_first;

	#define			ISL_CLOSURE_ISL		0
	#define			ISL_CLOSURE_BOX		1
	unsigned		closure;

	#define			ISL_BOUND_BERNSTEIN	0
	#define			ISL_BOUND_RANGE		1
	int			bound;

	#define			ISL_BERNSTEIN_FACTORS	1
	#define			ISL_BERNSTEIN_INTERVALS	2
	int			bernstein_recurse;

	int			bernstein_triangulate;

	int			pip_symmetry;

	#define			ISL_CONVEX_HULL_WRAP	0
	#define			ISL_CONVEX_HULL_FM	1
	int			convex;

	int			schedule_parametric;
	int			schedule_outer_zero_distance;
	int			schedule_maximize_band_depth;
	int			schedule_split_parallel;
};

ISL_ARG_DECL(isl_options, struct isl_options, isl_options_arg)

extern struct isl_arg isl_options_arg[];

#if defined(__cplusplus)
}
#endif

#endif
