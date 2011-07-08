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

struct isl_pw_aff_piece {
	struct isl_set *set;
	struct isl_aff *aff;
};

struct isl_pw_aff {
	int ref;

	isl_dim *dim;

	int n;

	size_t size;
	struct isl_pw_aff_piece p[1];
};

__isl_give isl_aff *isl_aff_alloc(__isl_take isl_local_space *ls);

__isl_give isl_aff *isl_aff_reset_dim(__isl_take isl_aff *aff,
	__isl_take isl_dim *dim);
__isl_give isl_aff *isl_aff_realign(__isl_take isl_aff *aff,
	__isl_take isl_reordering *r);

__isl_give isl_aff *isl_aff_expand_divs( __isl_take isl_aff *aff,
	__isl_take isl_mat *div, int *exp);

__isl_give isl_pw_aff *isl_pw_aff_add_disjoint(
	__isl_take isl_pw_aff *pwaff1, __isl_take isl_pw_aff *pwaff2);

#endif
