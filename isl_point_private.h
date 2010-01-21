#include <isl/dim.h>
#include <isl/point.h>
#include <isl/vec.h>

struct isl_point {
	int		ref;
	struct isl_dim	*dim;
	struct isl_vec	*vec;
};

__isl_give isl_point *isl_point_alloc(__isl_take isl_dim *dim,
	__isl_take isl_vec *vec);
