#ifndef ISL_ILP_H
#define ISL_ILP_H

#include <isl_lp.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum isl_lp_result isl_basic_set_solve_ilp(struct isl_basic_set *bset, int max,
				      isl_int *f, isl_int *opt,
				      struct isl_vec **sol_p);

#if defined(__cplusplus)
}
#endif

#endif
