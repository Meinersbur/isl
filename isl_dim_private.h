#include <isl_dim.h>

__isl_give isl_dim *isl_dim_move(__isl_take isl_dim *dim,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);
