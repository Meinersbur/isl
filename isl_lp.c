#include "isl_ctx.h"
#include "isl_lp.h"
#ifdef ISL_PIPLIB
#include "isl_lp_piplib.h"
#endif

enum isl_lp_result isl_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt)
{
#ifdef ISL_PIPLIB
	return isl_pip_solve_lp(bmap, maximize, f, denom, opt);
#else
	return isl_lp_error;
#endif
}
