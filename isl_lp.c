#include "isl_ctx.h"
#include "isl_lp.h"
#include "isl_lp_piplib.h"

enum isl_lp_result isl_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom)
{
	return isl_pip_solve_lp(bmap, maximize, f, denom, opt, opt_denom);
}
