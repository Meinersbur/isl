#ifndef ISL_CONSTRAINT_PRIVATE_H
#define ISL_CONSTRAINT_PRIVATE_H

#include <isl/aff.h>
#include <isl/constraint.h>

struct isl_constraint {
	int ref;

	int eq;
	isl_aff *aff;
};

struct isl_constraint *isl_basic_set_constraint(struct isl_basic_set *bset,
	isl_int **line);

#endif
