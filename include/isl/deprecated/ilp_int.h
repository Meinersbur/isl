#ifndef ISL_DEPRECATED_ILP_INT_H
#define ISL_DEPRECATED_ILP_INT_H

#include <isl/deprecated/int.h>
#include <isl/lp.h>
#include <isl/ilp.h>

enum isl_lp_result isl_basic_set_max(__isl_keep isl_basic_set *bset,
	__isl_keep isl_aff *obj, isl_int *opt);
enum isl_lp_result isl_set_min(__isl_keep isl_set *set,
	__isl_keep isl_aff *obj, isl_int *opt);
enum isl_lp_result isl_set_max(__isl_keep isl_set *set,
	__isl_keep isl_aff *obj, isl_int *opt);

#endif
