/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl_ctx_private.h>
#include <isl/vec.h>

static struct isl_options *find_nested_options(struct isl_arg *arg,
	void *opt, struct isl_arg *wanted)
{
	int i;
	struct isl_options *options;

	if (arg == wanted)
		return opt;

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		if (arg[i].type != isl_arg_child)
			continue;
		options = find_nested_options(arg[i].u.child.child,
				*(void **)(((char *)opt) + arg->offset), wanted);
		if (options)
			return options;
	}

	return NULL;
}

static struct isl_options *find_nested_isl_options(struct isl_arg *arg,
	void *opt)
{
	return find_nested_options(arg, opt, isl_options_arg);
}

void *isl_ctx_peek_options(isl_ctx *ctx, struct isl_arg *arg)
{
	if (!ctx)
		return NULL;
	return find_nested_options(ctx->user_arg, ctx->user_opt, arg);
}

isl_ctx *isl_ctx_alloc_with_options(struct isl_arg *arg, void *user_opt)
{
	struct isl_ctx *ctx = NULL;
	struct isl_options *opt = NULL;
	int opt_allocated = 0;

	if (!user_opt)
		return NULL;

	opt = find_nested_isl_options(arg, user_opt);
	if (!opt) {
		opt = isl_options_new_with_defaults();
		if (!opt)
			goto error;
		opt_allocated = 1;
	}

	ctx = isl_calloc_type(NULL, struct isl_ctx);
	if (!ctx)
		goto error;

	if (isl_hash_table_init(ctx, &ctx->name_hash, 0))
		goto error;

	ctx->stats = isl_calloc_type(ctx, struct isl_stats);
	if (!ctx->stats)
		goto error;

	ctx->user_arg = arg;
	ctx->user_opt = user_opt;
	ctx->opt_allocated = opt_allocated;
	ctx->opt = opt;
	ctx->ref = 0;

	isl_int_init(ctx->zero);
	isl_int_set_si(ctx->zero, 0);

	isl_int_init(ctx->one);
	isl_int_set_si(ctx->one, 1);

	isl_int_init(ctx->two);
	isl_int_set_si(ctx->two, 2);

	isl_int_init(ctx->negone);
	isl_int_set_si(ctx->negone, -1);

	isl_int_init(ctx->normalize_gcd);

	ctx->n_cached = 0;
	ctx->n_miss = 0;

	ctx->error = isl_error_none;

	return ctx;
error:
	isl_arg_free(arg, user_opt);
	if (opt_allocated)
		isl_options_free(opt);
	free(ctx);
	return NULL;
}

struct isl_ctx *isl_ctx_alloc()
{
	struct isl_options *opt;

	opt = isl_options_new_with_defaults();

	return isl_ctx_alloc_with_options(isl_options_arg, opt);
}

void isl_ctx_ref(struct isl_ctx *ctx)
{
	ctx->ref++;
}

void isl_ctx_deref(struct isl_ctx *ctx)
{
	isl_assert(ctx, ctx->ref > 0, return);
	ctx->ref--;
}

void isl_ctx_free(struct isl_ctx *ctx)
{
	if (!ctx)
		return;
	isl_assert(ctx, ctx->ref == 0, return);
	isl_hash_table_clear(&ctx->name_hash);
	isl_blk_clear_cache(ctx);
	isl_int_clear(ctx->zero);
	isl_int_clear(ctx->one);
	isl_int_clear(ctx->two);
	isl_int_clear(ctx->negone);
	isl_int_clear(ctx->normalize_gcd);
	isl_arg_free(ctx->user_arg, ctx->user_opt);
	if (ctx->opt_allocated)
		free(ctx->opt);
	free(ctx->stats);
	free(ctx);
}

struct isl_options *isl_ctx_options(isl_ctx *ctx)
{
	if (!ctx)
		return NULL;
	return ctx->opt;
}

enum isl_error isl_ctx_last_error(isl_ctx *ctx)
{
	return ctx->error;
}

void isl_ctx_reset_error(isl_ctx *ctx)
{
	ctx->error = isl_error_none;
}

void isl_ctx_set_error(isl_ctx *ctx, enum isl_error error)
{
	if (ctx)
		ctx->error = error;
}

void isl_ctx_abort(isl_ctx *ctx)
{
	if (ctx)
		ctx->abort = 1;
}

void isl_ctx_resume(isl_ctx *ctx)
{
	if (ctx)
		ctx->abort = 0;
}

int isl_ctx_aborted(isl_ctx *ctx)
{
	return ctx ? ctx->abort : -1;
}
