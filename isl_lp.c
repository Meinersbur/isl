#include "isl_ctx.h"
#include "isl_lp.h"
#include "isl_lp_piplib.h"

/* Given a basic map "bmap" and an affine combination of the variables "f"
 * with denominator "denom", set *opt/*opt_denom to the minimal
 * (or maximal if "maximize" is true) value attained by f/d over "bmap",
 * assuming the basic map is not empty and the expression cannot attain
 * arbitrarily small (or large) values.
 * If opt_denom is NULL, then *opt is rounded up (or down)
 * to the nearest integer.
 * The return value reflects the nature of the result (empty, unbounded,
 * minmimal or maximal value returned in *opt).
 */
enum isl_lp_result isl_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom)
{
	return isl_pip_solve_lp(bmap, maximize, f, denom, opt, opt_denom);
}
