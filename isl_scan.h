#ifndef ISL_SCAN_H
#define ISL_SCAN_H

#include "isl_set.h"
#include "isl_vec.h"

struct isl_scan_callback {
	int (*add)(struct isl_scan_callback *cb, __isl_take isl_vec *sample);
};

int isl_basic_set_scan(struct isl_basic_set *bset,
	struct isl_scan_callback *callback);

#endif
