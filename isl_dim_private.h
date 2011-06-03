#include <isl/dim.h>
#include <isl/hash.h>

struct isl_name;
struct isl_dim {
	int ref;

	struct isl_ctx *ctx;

	unsigned nparam;
	unsigned n_in;		/* zero for sets */
	unsigned n_out;		/* dim for sets */

	struct isl_name *tuple_name[2];
	struct isl_dim *nested[2];

	unsigned n_name;
	struct isl_name **names;
};

uint32_t isl_dim_get_hash(__isl_keep isl_dim *dim);

__isl_give isl_dim *isl_dim_as_set_dim(__isl_take isl_dim *dim);

unsigned isl_dim_offset(__isl_keep isl_dim *dim, enum isl_dim_type type);

int isl_dim_may_be_set(__isl_keep isl_dim *dim);
int isl_dim_is_named_or_nested(__isl_keep isl_dim *dim, enum isl_dim_type type);
int isl_dim_has_named_params(__isl_keep isl_dim *dim);
__isl_give isl_dim *isl_dim_reset(__isl_take isl_dim *dim,
	enum isl_dim_type type);
__isl_give isl_dim *isl_dim_flatten(__isl_take isl_dim *dim);
__isl_give isl_dim *isl_dim_flatten_range(__isl_take isl_dim *dim);

__isl_give isl_dim *isl_dim_replace(__isl_take isl_dim *dst,
	enum isl_dim_type type, __isl_keep isl_dim *src);

__isl_give isl_dim *isl_dim_lift(__isl_take isl_dim *dim, unsigned n_local);
