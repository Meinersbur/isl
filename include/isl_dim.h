#ifndef ISL_DIM_H
#define ISL_DIM_H

#include <isl_ctx.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_name;
struct isl_dim {
	int ref;

	struct isl_ctx *ctx;

	unsigned nparam;
	unsigned n_in;		/* zero for sets */
	unsigned n_out;		/* dim for sets */

	unsigned n_name;
	struct isl_name **names;
};

enum isl_dim_type {
	isl_dim_param,
	isl_dim_in,
	isl_dim_out,
	isl_dim_set = isl_dim_out,
};

struct isl_dim *isl_dim_alloc(struct isl_ctx *ctx,
			unsigned nparam, unsigned n_in, unsigned n_out);
struct isl_dim *isl_dim_set_alloc(struct isl_ctx *ctx,
			unsigned nparam, unsigned dim);
struct isl_dim *isl_dim_copy(struct isl_dim *dim);
struct isl_dim *isl_dim_cow(struct isl_dim *dim);
void isl_dim_free(struct isl_dim *dim);

struct isl_dim *isl_dim_set_name(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos,
				 const char *name);
const char *isl_dim_get_name(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos);

struct isl_dim *isl_dim_extend(struct isl_dim *dim,
			unsigned nparam, unsigned n_in, unsigned n_out);
struct isl_dim *isl_dim_join(struct isl_dim *left, struct isl_dim *right);
struct isl_dim *isl_dim_map(struct isl_dim *dim);
struct isl_dim *isl_dim_reverse(struct isl_dim *dim);
struct isl_dim *isl_dim_drop_inputs(struct isl_dim *dim,
		unsigned first, unsigned n);
struct isl_dim *isl_dim_drop_outputs(struct isl_dim *dim,
		unsigned first, unsigned n);

int isl_dim_equal(struct isl_dim *dim1, struct isl_dim *dim2);
int isl_dim_compatible(struct isl_dim *dim1, struct isl_dim *dim2);
unsigned isl_dim_total(struct isl_dim *dim);

#if defined(__cplusplus)
}
#endif

#endif
