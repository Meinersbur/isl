/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xS(TYPE,NAME) struct TYPE ## _ ## NAME
#define S(TYPE,NAME) xS(TYPE,NAME)

struct UNION {
	int ref;
#ifdef HAS_TYPE
	enum isl_fold type;
#endif
	isl_dim *dim;

	struct isl_hash_table	table;
};

__isl_give UNION *FN(UNION,cow)(__isl_take UNION *u);

isl_ctx *FN(UNION,get_ctx)(__isl_keep UNION *u)
{
	return u ? u->dim->ctx : NULL;
}

__isl_give isl_dim *FN(UNION,get_dim)(__isl_keep UNION *u)
{
	if (!u)
		return NULL;
	return isl_dim_copy(u->dim);
}

#ifdef HAS_TYPE
static __isl_give UNION *FN(UNION,alloc)(__isl_take isl_dim *dim,
	enum isl_fold type, int size)
#else
static __isl_give UNION *FN(UNION,alloc)(__isl_take isl_dim *dim, int size)
#endif
{
	UNION *u;

	if (!dim)
		return NULL;

	u = isl_calloc_type(dim->ctx, UNION);
	if (!u)
		return NULL;

	u->ref = 1;
#ifdef HAS_TYPE
	u->type = type;
#endif
	u->dim = dim;
	if (isl_hash_table_init(dim->ctx, &u->table, size) < 0)
		goto error;

	return u;
error:
	isl_dim_free(dim);
	FN(UNION,free)(u);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give UNION *FN(UNION,zero)(__isl_take isl_dim *dim, enum isl_fold type)
{
	return FN(UNION,alloc)(dim, type, 16);
}
#else
__isl_give UNION *FN(UNION,zero)(__isl_take isl_dim *dim)
{
	return FN(UNION,alloc)(dim, 16);
}
#endif

__isl_give UNION *FN(UNION,copy)(__isl_keep UNION *u)
{
	if (!u)
		return NULL;

	u->ref++;
	return u;
}

S(UNION,foreach_data)
{
	int (*fn)(__isl_take PART *part, void *user);
	void *user;
};

static int call_on_copy(void **entry, void *user)
{
	PART *part = *entry;
	S(UNION,foreach_data) *data = (S(UNION,foreach_data) *)user;

	return data->fn(FN(PART,copy)(part), data->user);
}

int FN(FN(UNION,foreach),PARTS)(__isl_keep UNION *u,
	int (*fn)(__isl_take PART *part, void *user), void *user)
{
	S(UNION,foreach_data) data = { fn, user };

	if (!u)
		return -1;

	return isl_hash_table_foreach(u->dim->ctx, &u->table,
				      &call_on_copy, &data);
}

static int has_dim(const void *entry, const void *val)
{
	PART *part = (PART *)entry;
	isl_dim *dim = (isl_dim *)val;

	return isl_dim_equal(part->dim, dim);
}

__isl_give PART *FN(FN(UNION,extract),PARTS)(__isl_keep UNION *u,
	__isl_take isl_dim *dim)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!u || !dim)
		goto error;

	hash = isl_dim_get_hash(dim);
	entry = isl_hash_table_find(u->dim->ctx, &u->table, hash,
				    &has_dim, dim, 0);
	if (!entry)
#ifdef HAS_TYPE
		return FN(PART,zero)(dim, u->type);
#else
		return FN(PART,zero)(dim);
#endif
	isl_dim_free(dim);
	return FN(PART,copy)(entry->data);
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give UNION *FN(FN(UNION,add),PARTS)(__isl_take UNION *u,
	__isl_take PART *part)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!part)
		goto error;

	if (FN(PART,is_zero)(part)) {
		FN(PART,free)(part);
		return u;
	}

	u = FN(UNION,cow)(u);

	if (!u)
		goto error;

	isl_assert(u->dim->ctx, isl_dim_match(part->dim, isl_dim_param, u->dim,
					      isl_dim_param), goto error);

	hash = isl_dim_get_hash(part->dim);
	entry = isl_hash_table_find(u->dim->ctx, &u->table, hash,
				    &has_dim, part->dim, 1);
	if (!entry)
		goto error;

	if (!entry->data)
		entry->data = part;
	else {
		entry->data = FN(PART,add)(entry->data, FN(PART,copy)(part));
		if (!entry->data)
			goto error;
		FN(PART,free)(part);
		if (FN(PART,is_zero)(entry->data)) {
			FN(PART,free)(entry->data);
			isl_hash_table_remove(u->dim->ctx, &u->table, entry);
		}
	}

	return u;
error:
	FN(PART,free)(part);
	FN(UNION,free)(u);
	return NULL;
}

static int add_part(__isl_take PART *part, void *user)
{
	UNION **u = (UNION **)user;

	*u = FN(FN(UNION,add),PARTS)(*u, part);

	return 0;
}

__isl_give UNION *FN(UNION,dup)(__isl_keep UNION *u)
{
	UNION *dup;

	if (!u)
		return NULL;

#ifdef HAS_TYPE
	dup = FN(UNION,zero)(isl_dim_copy(u->dim), u->type);
#else
	dup = FN(UNION,zero)(isl_dim_copy(u->dim));
#endif
	if (FN(FN(UNION,foreach),PARTS)(u, &add_part, &dup) < 0)
		goto error;
	return dup;
error:
	FN(UNION,free)(dup);
	return NULL;
}

__isl_give UNION *FN(UNION,cow)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (u->ref == 1)
		return u;
	u->ref--;
	return FN(UNION,dup)(u);
}

static int free_u_entry(void **entry, void *user)
{
	PART *part = *entry;
	FN(PART,free)(part);
	return 0;
}

void FN(UNION,free)(__isl_take UNION *u)
{
	if (!u)
		return;

	if (--u->ref > 0)
		return;

	isl_hash_table_foreach(u->dim->ctx, &u->table, &free_u_entry, NULL);
	isl_hash_table_clear(&u->table);
	isl_dim_free(u->dim);
	free(u);
}

S(UNION,align) {
	isl_reordering *exp;
	UNION *res;
};

static int align_entry(__isl_take PART *part, void *user)
{
	isl_reordering *exp;
	S(UNION,align) *data = user;

	exp = isl_reordering_extend_dim(isl_reordering_copy(data->exp),
				    FN(PART,get_dim)(part));

	data->res = FN(FN(UNION,add),PARTS)(data->res,
					    FN(PART,realign)(part, exp));

	return 0;
}

__isl_give UNION *FN(UNION,align_params)(__isl_take UNION *u,
	__isl_take isl_dim *model)
{
	S(UNION,align) data = { NULL, NULL };

	if (!u || !model)
		goto error;

	if (isl_dim_match(u->dim, isl_dim_param, model, isl_dim_param)) {
		isl_dim_free(model);
		return u;
	}

	data.exp = isl_parameter_alignment_reordering(u->dim, model);
	if (!data.exp)
		goto error;

#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_dim_copy(data.exp->dim),
						u->type, u->table.n);
#else
	data.res = FN(UNION,alloc)(isl_dim_copy(data.exp->dim), u->table.n);
#endif
	if (FN(FN(UNION,foreach),PARTS)(u, &align_entry, &data) < 0)
		goto error;

	isl_reordering_free(data.exp);
	FN(UNION,free)(u);
	isl_dim_free(model);
	return data.res;
error:
	isl_reordering_free(data.exp);
	FN(UNION,free)(u);
	FN(UNION,free)(data.res);
	isl_dim_free(model);
	return NULL;
}

__isl_give UNION *FN(UNION,add)(__isl_take UNION *u1, __isl_take UNION *u2)
{
	u1 = FN(UNION,align_params)(u1, FN(UNION,get_dim)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_dim)(u1));

	u1 = FN(UNION,cow)(u1);

	if (!u1 || !u2)
		goto error;

	if (FN(FN(UNION,foreach),PARTS)(u2, &add_part, &u1) < 0)
		goto error;

	FN(UNION,free)(u2);

	return u1;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return NULL;
}

__isl_give UNION *FN(FN(UNION,from),PARTS)(__isl_take PART *part)
{
	isl_dim *dim;
	UNION *u;

	if (!part)
		return NULL;

	dim = FN(PART,get_dim)(part);
	dim = isl_dim_drop(dim, isl_dim_in, 0, isl_dim_size(dim, isl_dim_in));
	dim = isl_dim_drop(dim, isl_dim_out, 0, isl_dim_size(dim, isl_dim_out));
#ifdef HAS_TYPE
	u = FN(UNION,zero)(dim, part->type);
#else
	u = FN(UNION,zero)(dim);
#endif
	u = FN(FN(UNION,add),PARTS)(u, part);

	return u;
}

S(UNION,match_bin_data) {
	UNION *u2;
	UNION *res;
};

/* This function is currently only used from isl_polynomial.c
 * and not from isl_fold.c.
 */
static __isl_give UNION *match_bin_op(__isl_take UNION *u1,
	__isl_take UNION *u2,
	int (*fn)(void **, void *)) __attribute__ ((unused));
static __isl_give UNION *match_bin_op(__isl_take UNION *u1,
	__isl_take UNION *u2, int (*fn)(void **, void *))
{
	S(UNION,match_bin_data) data = { NULL, NULL };

	u1 = FN(UNION,align_params)(u1, FN(UNION,get_dim)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_dim)(u1));

	if (!u1 || !u2)
		goto error;

	data.u2 = u2;
#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_dim_copy(u1->dim), u1->type, u1->table.n);
#else
	data.res = FN(UNION,alloc)(isl_dim_copy(u1->dim), u1->table.n);
#endif
	if (isl_hash_table_foreach(u1->dim->ctx, &u1->table, fn, &data) < 0)
		goto error;

	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return data.res;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	FN(UNION,free)(data.res);
	return NULL;
}

S(UNION,match_set_data) {
	isl_union_set *uset;
	UNION *res;
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*);
};

static int set_has_dim(const void *entry, const void *val)
{
	isl_set *set = (isl_set *)entry;
	isl_dim *dim = (isl_dim *)val;

	return isl_dim_equal(set->dim, dim);
}

static int match_set_entry(void **entry, void *user)
{
	S(UNION,match_set_data) *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	PW *pw = *entry;
	int empty;

	hash = isl_dim_get_hash(pw->dim);
	entry2 = isl_hash_table_find(data->uset->dim->ctx, &data->uset->table,
				     hash, &set_has_dim, pw->dim, 0);
	if (!entry2)
		return 0;

	pw = FN(PW,copy)(pw);
	pw = data->fn(pw, isl_set_copy(entry2->data));

	empty = FN(PW,is_zero)(pw);
	if (empty < 0) {
		FN(PW,free)(pw);
		return -1;
	}
	if (empty) {
		FN(PW,free)(pw);
		return 0;
	}

	data->res = FN(FN(UNION,add),PARTS)(data->res, pw);

	return 0;
}

static __isl_give UNION *match_set_op(__isl_take UNION *u,
	__isl_take isl_union_set *uset,
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*))
{
	S(UNION,match_set_data) data = { NULL, NULL, fn };

	u = FN(UNION,align_params)(u, isl_union_set_get_dim(uset));
	uset = isl_union_set_align_params(uset, FN(UNION,get_dim)(u));

	if (!u || !uset)
		goto error;

	data.uset = uset;
#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_dim_copy(u->dim), u->type, u->table.n);
#else
	data.res = FN(UNION,alloc)(isl_dim_copy(u->dim), u->table.n);
#endif
	if (isl_hash_table_foreach(u->dim->ctx, &u->table,
				   &match_set_entry, &data) < 0)
		goto error;

	FN(UNION,free)(u);
	isl_union_set_free(uset);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_union_set_free(uset);
	FN(UNION,free)(data.res);
	return NULL;
}

__isl_give UNION *FN(UNION,intersect_domain)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	return match_set_op(u, uset, &FN(PW,intersect_domain));
}

__isl_give UNION *FN(UNION,gist)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	return match_set_op(u, uset, &FN(PW,gist));
}

__isl_give isl_qpolynomial *FN(UNION,eval)(__isl_take UNION *u,
	__isl_take isl_point *pnt)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;
	isl_qpolynomial *qp;

	if (!u || !pnt)
		goto error;

	hash = isl_dim_get_hash(pnt->dim);
	entry = isl_hash_table_find(u->dim->ctx, &u->table,
				    hash, &has_dim, pnt->dim, 0);
	if (!entry) {
		qp = isl_qpolynomial_zero(isl_dim_copy(pnt->dim));
		isl_point_free(pnt);
	} else {
		qp = FN(PART,eval)(FN(PART,copy)(entry->data), pnt);
	}
	FN(UNION,free)(u);
	return qp;
error:
	FN(UNION,free)(u);
	isl_point_free(pnt);
	return NULL;
}

static int coalesce_entry(void **entry, void *user)
{
	PW **pw = (PW **)entry;

	*pw = FN(PW,coalesce)(*pw);
	if (!*pw)
		return -1;

	return 0;
}

__isl_give UNION *FN(UNION,coalesce)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (isl_hash_table_foreach(u->dim->ctx, &u->table,
				   &coalesce_entry, NULL) < 0)
		goto error;

	return u;
error:
	FN(UNION,free)(u);
	return NULL;
}

static int domain(__isl_take PART *part, void *user)
{
	isl_union_set **uset = (isl_union_set **)user;

	*uset = isl_union_set_add_set(*uset, FN(PART,domain)(part));

	return 0;
}

__isl_give isl_union_set *FN(UNION,domain)(__isl_take UNION *u)
{
	isl_union_set *uset;

	uset = isl_union_set_empty(FN(UNION,get_dim)(u));
	if (FN(FN(UNION,foreach),PARTS)(u, &domain, &uset) < 0)
		goto error;

	FN(UNION,free)(u);
	
	return uset;
error:
	isl_union_set_free(uset);
	FN(UNION,free)(u);
	return NULL;
}

static int mul_isl_int(void **entry, void *user)
{
	PW **pw = (PW **)entry;
	isl_int *v = user;

	*pw = FN(PW,mul_isl_int)(*pw, *v);
	if (!*pw)
		return -1;

	return 0;
}

__isl_give UNION *FN(UNION,mul_isl_int)(__isl_take UNION *u, isl_int v)
{
	if (isl_int_is_one(v))
		return u;

	if (u && isl_int_is_zero(v)) {
		UNION *zero;
		isl_dim *dim = FN(UNION,get_dim)(u);
#ifdef HAS_TYPE
		zero = FN(UNION,zero)(dim, u->type);
#else
		zero = FN(UNION,zero)(dim);
#endif
		FN(UNION,free)(u);
		return zero;
	}

	u = FN(UNION,cow)(u);
	if (!u)
		return NULL;

#ifdef HAS_TYPE
	if (isl_int_is_neg(v))
		u->type = isl_fold_type_negate(u->type);
#endif
	if (isl_hash_table_foreach(u->dim->ctx, &u->table, &mul_isl_int, v) < 0)
		goto error;

	return u;
error:
	FN(UNION,free)(u);
	return NULL;
}
