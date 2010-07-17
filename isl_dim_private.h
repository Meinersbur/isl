#include <isl_dim.h>

__isl_give isl_dim *isl_dim_as_set_dim(__isl_take isl_dim *dim);

unsigned isl_dim_offset(__isl_keep isl_dim *dim, enum isl_dim_type type);
int isl_dim_tuple_match(__isl_keep isl_dim *dim1, enum isl_dim_type dim1_type,
			__isl_keep isl_dim *dim2, enum isl_dim_type dim2_type);
