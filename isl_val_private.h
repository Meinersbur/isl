#ifndef ISL_VAL_PRIVATE_H
#define ISL_VAL_PRIVATE_H

#include <isl/int.h>
#include <isl/val.h>

/* Represents a "value", which may be an integer value, a rational value,
 * plus or minus infinity or "not a number".
 *
 * Internally, +infinity is represented as 1/0,
 * -infinity as -1/0 and NaN as 0/0.
 *
 * A rational value is always normalized before it is passed to the user.
 */
struct isl_val {
	int ref;
	isl_ctx *ctx;

	isl_int n;
	isl_int d;
};

#undef EL
#define EL isl_val

#include <isl_list_templ.h>

__isl_give isl_val *isl_val_alloc(isl_ctx *ctx);
__isl_give isl_val *isl_val_normalize(__isl_take isl_val *v);
__isl_give isl_val *isl_val_int_from_isl_int(isl_ctx *ctx, isl_int n);
__isl_give isl_val *isl_val_rat_from_isl_int(isl_ctx *ctx,
	isl_int n, isl_int d);
__isl_give isl_val *isl_val_cow(__isl_take isl_val *val);

#endif
