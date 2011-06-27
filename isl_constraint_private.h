#ifndef ISL_CONSTRAINT_PRIVATE_H
#define ISL_CONSTRAINT_PRIVATE_H

#include <isl/constraint.h>

struct isl_constraint {
	int ref;
	struct isl_ctx *ctx;

	struct isl_basic_map	*bmap;
	isl_int			**line;
};

struct isl_constraint *isl_basic_set_constraint(struct isl_basic_set *bset,
	isl_int **line);

#endif
