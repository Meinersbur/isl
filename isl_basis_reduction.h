#ifndef ISL_BASIS_REDUCTION_H
#define ISL_BASIS_REDUCTION_H

#include "isl_set.h"
#include "isl_mat.h"
#include "isl_tab.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_tab *isl_tab_compute_reduced_basis(struct isl_tab *tab);
struct isl_mat *isl_basic_set_reduced_basis(struct isl_basic_set *bset);

#if defined(__cplusplus)
}
#endif

#endif
