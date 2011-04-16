#ifndef ISL_AFF_PRIVATE_H
#define ISL_AFF_PRIVATE_H

#include <isl/aff.h>
#include <isl/vec.h>
#include <isl/mat.h>
#include <isl/local_space.h>

struct isl_aff {
	int ref;

	isl_local_space	*ls;
	isl_vec		*v;
};

__isl_give isl_aff *isl_aff_alloc(__isl_take isl_local_space *ls);

__isl_give isl_aff *isl_aff_expand_divs( __isl_take isl_aff *aff,
	__isl_take isl_mat *div, int *exp);

#endif
