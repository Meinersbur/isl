#ifndef ISL_EQUALITIES_H
#define ISL_EQUALITIES_H

#include <isl_set.h>
#include <isl_mat.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_basic_set *isl_basic_set_remove_equalities(
	struct isl_basic_set *bset, struct isl_mat **T, struct isl_mat **T2);

#if defined(__cplusplus)
}
#endif

#endif
