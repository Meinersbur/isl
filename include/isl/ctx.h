/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_CTX_H
#define ISL_CTX_H

#include <stdio.h>
#include <stdlib.h>

#include <isl/int.h>
#include <isl/options.h>
#include <isl/blk.h>
#include <isl/hash.h>
#include <isl/config.h>

#define __isl_give
#define __isl_take
#define __isl_keep

#ifdef GCC_WARN_UNUSED_RESULT
#define	WARN_UNUSED	GCC_WARN_UNUSED_RESULT
#else
#define WARN_UNUSED
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* Nearly all isa functions require a struct isl_ctx allocated using
 * isl_ctx_alloc.  This ctx contains (or will contain) options that
 * control the behavior of the library and some caches.
 *
 * An object allocated within a given ctx should never be used inside
 * another ctx.  Functions for moving objects from one ctx to another
 * will be added as the need arises.
 *
 * A given context should only be used inside a single thread.
 * A global context for synchronization between different threads
 * as well as functions for moving a context to a different thread
 * will be added as the need arises.
 *
 * If anything goes wrong (out of memory, failed assertion), then
 * the library will currently simply abort.  This will be made
 * configurable in the future.
 * Users of the library should expect functions that return
 * a pointer to a structure, to return NULL, indicating failure.
 * Any function accepting a pointer to a structure will treat
 * a NULL argument as a failure, resulting in the function freeing
 * the remaining structures (if any) and returning NULL itself
 * (in case of pointer return type).
 * The only exception is the isl_ctx argument, which should never be NULL.
 */
struct isl_stats {
	long	gbr_solved_lps;
};
enum isl_error {
	isl_error_none = 0,
	isl_error_abort,
	isl_error_unknown,
	isl_error_internal,
	isl_error_invalid,
	isl_error_unsupported
};
struct isl_ctx;
typedef struct isl_ctx isl_ctx;

/* Some helper macros */

#define ISL_FL_INIT(l, f)   (l) = (f)               /* Specific flags location. */
#define ISL_FL_SET(l, f)    ((l) |= (f))
#define ISL_FL_CLR(l, f)    ((l) &= ~(f))
#define ISL_FL_ISSET(l, f)  (!!((l) & (f)))

#define ISL_F_INIT(p, f)    ISL_FL_INIT((p)->flags, f)  /* Structure element flags. */
#define ISL_F_SET(p, f)     ISL_FL_SET((p)->flags, f)
#define ISL_F_CLR(p, f)     ISL_FL_CLR((p)->flags, f)
#define ISL_F_ISSET(p, f)   ISL_FL_ISSET((p)->flags, f)

/* isl_check_ctx() checks at compile time if 'ctx' is of type 'isl_ctx *' and
 * returns the value of 'expr'. It is used to ensure, that always an isl_ctx is
 * passed to the following macros, even if they currently do not use it.
 */
#define isl_check_ctx(ctx, expr)	(ctx != (isl_ctx *) 0) ? expr : expr

#define isl_alloc(ctx,type,size)	isl_check_ctx(ctx, (type *)malloc(size))
#define isl_calloc(ctx,type,size)	isl_check_ctx(ctx, \
						(type *)calloc(1, size))
#define isl_realloc(ctx,ptr,type,size)	isl_check_ctx(ctx, \
						(type *)realloc(ptr,size))
#define isl_alloc_type(ctx,type)	isl_alloc(ctx,type,sizeof(type))
#define isl_calloc_type(ctx,type)	isl_calloc(ctx,type,sizeof(type))
#define isl_realloc_type(ctx,ptr,type)	isl_realloc(ctx,ptr,type,sizeof(type))
#define isl_alloc_array(ctx,type,n)	isl_alloc(ctx,type,(n)*sizeof(type))
#define isl_calloc_array(ctx,type,n)	isl_check_ctx(ctx,\
						(type *)calloc(n, sizeof(type)))
#define isl_realloc_array(ctx,ptr,type,n) \
				    isl_realloc(ctx,ptr,type,(n)*sizeof(type))

#define isl_die(ctx,errno,msg,code)					\
	do {								\
		isl_ctx_set_error(ctx, errno);				\
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg);	\
		code;							\
	} while (0)
#define isl_assert4(ctx,test,code,errno)				\
	do {								\
		if (test)						\
			break;						\
		isl_die(ctx, errno, "Assertion \"" #test "\" failed", code);	\
	} while (0)
#define isl_assert(ctx,test,code)					\
	isl_assert4(ctx,test,code,isl_error_unknown)

#define isl_min(a,b)			((a < b) ? (a) : (b))

/* struct isl_ctx functions */

struct isl_options *isl_ctx_options(isl_ctx *ctx);

isl_ctx *isl_ctx_alloc_with_options(struct isl_arg *arg, __isl_take void *opt);
isl_ctx *isl_ctx_alloc();
void *isl_ctx_peek_options(isl_ctx *ctx, struct isl_arg *arg);
void isl_ctx_ref(struct isl_ctx *ctx);
void isl_ctx_deref(struct isl_ctx *ctx);
void isl_ctx_free(isl_ctx *ctx);

void isl_ctx_abort(isl_ctx *ctx);
void isl_ctx_resume(isl_ctx *ctx);
int isl_ctx_aborted(isl_ctx *ctx);

#define ISL_ARG_CTX_DECL(prefix,st,arg)					\
st *isl_ctx_peek_ ## prefix(isl_ctx *ctx);

#define ISL_ARG_CTX_DEF(prefix,st,arg)					\
st *isl_ctx_peek_ ## prefix(isl_ctx *ctx)				\
{									\
	return (st *)isl_ctx_peek_options(ctx, arg);			\
}

enum isl_error isl_ctx_last_error(isl_ctx *ctx);
void isl_ctx_reset_error(isl_ctx *ctx);
void isl_ctx_set_error(isl_ctx *ctx, enum isl_error error);

#if defined(__cplusplus)
}
#endif

#endif
