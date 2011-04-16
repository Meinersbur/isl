#ifndef ISL_LOCAL_SPACE_PRIVATE_H
#define ISL_LOCAL_SPACE_PRIVATE_H

#include <isl/div.h>
#include <isl/mat.h>
#include <isl/local_space.h>

struct isl_local_space {
	int ref;

	isl_dim *dim;
	isl_mat *div;
};

__isl_give isl_local_space *isl_local_space_alloc(__isl_take isl_dim *dim,
	unsigned n_div);

__isl_give isl_local_space *isl_local_space_add_div(
	__isl_take isl_local_space *ls, __isl_take isl_vec *div);

unsigned isl_local_space_offset(__isl_keep isl_local_space *ls,
	enum isl_dim_type type);

__isl_give isl_local_space *isl_local_space_replace_divs(
	__isl_take isl_local_space *ls, __isl_take isl_mat *div);

#endif
