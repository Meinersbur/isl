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

#include <assert.h>
#include <stdlib.h>

#include <isl_int.h>
#include <isl_options.h>
#include <isl_blk.h>
#include <isl_hash.h>
#include <isl_config.h>

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
 * The only exception is the isl_ctx argument, which shoud never be NULL.
 */
struct isl_stats {
	long	gbr_solved_lps;
};
struct isl_ctx {
	int			ref;

	struct isl_stats	*stats;

	int			 opt_allocated;
	struct isl_options	*opt;
	void			*user_opt;
	struct isl_arg		*user_arg;

	isl_int			zero;
	isl_int			one;
	isl_int			negone;

	isl_int			normalize_gcd;

	int			n_cached;
	struct isl_blk		cache[ISL_BLK_CACHE_SIZE];
	struct isl_hash_table	name_hash;
};
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

#define isl_alloc(ctx,type,size)	(type *)malloc(size)
#define isl_calloc(ctx,type,size)	(type *)calloc(1, size)
#define isl_realloc(ctx,ptr,type,size)	(type *)realloc(ptr,size)
#define isl_alloc_type(ctx,type)	isl_alloc(ctx,type,sizeof(type))
#define isl_calloc_type(ctx,type)	isl_calloc(ctx,type,sizeof(type))
#define isl_realloc_type(ctx,ptr,type)	isl_realloc(ctx,ptr,type,sizeof(type))
#define isl_alloc_array(ctx,type,n)	isl_alloc(ctx,type,(n)*sizeof(type))
#define isl_calloc_array(ctx,type,n)	(type *)calloc(n, sizeof(type))
#define isl_realloc_array(ctx,ptr,type,n) \
				    isl_realloc(ctx,ptr,type,(n)*sizeof(type))

#define isl_assert(ctx,test,code)					\
	do {								\
		assert(test);						\
		if (0 && !ctx) {					\
			code;						\
		}							\
	} while(0)

#define isl_min(a,b)			((a < b) ? (a) : (b))

/* struct isl_ctx functions */

struct isl_options *isl_ctx_options(isl_ctx *ctx);

isl_ctx *isl_ctx_alloc_with_options(struct isl_arg *arg, __isl_take void *opt);
isl_ctx *isl_ctx_alloc();
void isl_ctx_ref(struct isl_ctx *ctx);
void isl_ctx_deref(struct isl_ctx *ctx);
void isl_ctx_free(isl_ctx *ctx);

#if defined(__cplusplus)
}
#endif

#endif
