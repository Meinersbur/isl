#include "isl_ctx.h"
#include "isl_vec.h"
#ifdef ISL_POLYLIB
#include <polylib/polylibgmp.h>
#endif

struct isl_ctx *isl_ctx_alloc()
{
	struct isl_ctx *ctx = NULL;

	ctx = isl_alloc_type(NULL, struct isl_ctx);
	if (!ctx)
		goto error;

	isl_int_init(ctx->one);
	isl_int_set_si(ctx->one, 1);

#ifdef ISL_POLYLIB
	ctx->MaxRays = POL_NO_DUAL | POL_INTEGER;
#endif

	return ctx;
error:
	free(ctx);
	return NULL;
}

void isl_ctx_free(struct isl_ctx *ctx)
{
	if (!ctx)
		return;
	isl_int_clear(ctx->one);
	free(ctx);
}
