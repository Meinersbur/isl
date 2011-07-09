#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xS(TYPE,NAME) struct TYPE ## _ ## NAME
#define S(TYPE,NAME) xS(TYPE,NAME)

#ifdef HAS_TYPE
static __isl_give PW *FN(PW,alloc_)(__isl_take isl_dim *dim,
	enum isl_fold type, int n)
#else
static __isl_give PW *FN(PW,alloc_)(__isl_take isl_dim *dim, int n)
#endif
{
	isl_ctx *ctx;
	struct PW *pw;

	if (!dim)
		return NULL;
	ctx = isl_dim_get_ctx(dim);
	isl_assert(ctx, n >= 0, goto error);
	pw = isl_alloc(ctx, struct PW,
			sizeof(struct PW) + (n - 1) * sizeof(S(PW,piece)));
	if (!pw)
		goto error;

	pw->ref = 1;
#ifdef HAS_TYPE
	pw->type = type;
#endif
	pw->size = n;
	pw->n = 0;
	pw->dim = dim;
	return pw;
error:
	isl_dim_free(dim);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give PW *FN(PW,ZERO)(__isl_take isl_dim *dim, enum isl_fold type)
{
	return FN(PW,alloc_)(dim, type, 0);
}
#else
__isl_give PW *FN(PW,ZERO)(__isl_take isl_dim *dim)
{
	return FN(PW,alloc_)(dim, 0);
}
#endif

__isl_give PW *FN(PW,add_piece)(__isl_take PW *pw,
	__isl_take isl_set *set, __isl_take EL *el)
{
	isl_ctx *ctx;
	isl_dim *el_dim = NULL;

	if (!pw || !set || !el)
		goto error;

	if (isl_set_plain_is_empty(set) || FN(EL,EL_IS_ZERO)(el)) {
		isl_set_free(set);
		FN(EL,free)(el);
		return pw;
	}

	ctx = isl_set_get_ctx(set);
#ifdef HAS_TYPE
	if (pw->type != el->type)
		isl_die(ctx, isl_error_invalid, "fold types don't match",
			goto error);
#endif
	el_dim = FN(EL,get_dim(el));
	isl_assert(ctx, isl_dim_equal(pw->dim, el_dim), goto error);
	isl_assert(ctx, pw->n < pw->size, goto error);

	pw->p[pw->n].set = set;
	pw->p[pw->n].FIELD = el;
	pw->n++;
	
	isl_dim_free(el_dim);
	return pw;
error:
	isl_dim_free(el_dim);
	FN(PW,free)(pw);
	isl_set_free(set);
	FN(EL,free)(el);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give PW *FN(PW,alloc)(enum isl_fold type,
	__isl_take isl_set *set, __isl_take EL *el)
#else
__isl_give PW *FN(PW,alloc)(__isl_take isl_set *set, __isl_take EL *el)
#endif
{
	PW *pw;

	if (!set || !el)
		goto error;

#ifdef HAS_TYPE
	pw = FN(PW,alloc_)(isl_set_get_dim(set), type, 1);
#else
	pw = FN(PW,alloc_)(isl_set_get_dim(set), 1);
#endif

	return FN(PW,add_piece)(pw, set, el);
error:
	isl_set_free(set);
	FN(EL,free)(el);
	return NULL;
}

__isl_give PW *FN(PW,dup)(__isl_keep PW *pw)
{
	int i;
	PW *dup;

	if (!pw)
		return NULL;

#ifdef HAS_TYPE
	dup = FN(PW,alloc_)(isl_dim_copy(pw->dim), pw->type, pw->n);
#else
	dup = FN(PW,alloc_)(isl_dim_copy(pw->dim), pw->n);
#endif
	if (!dup)
		return NULL;

	for (i = 0; i < pw->n; ++i)
		dup = FN(PW,add_piece)(dup, isl_set_copy(pw->p[i].set),
					    FN(EL,copy)(pw->p[i].FIELD));

	return dup;
}

__isl_give PW *FN(PW,cow)(__isl_take PW *pw)
{
	if (!pw)
		return NULL;

	if (pw->ref == 1)
		return pw;
	pw->ref--;
	return FN(PW,dup)(pw);
}

__isl_give PW *FN(PW,copy)(__isl_keep PW *pw)
{
	if (!pw)
		return NULL;

	pw->ref++;
	return pw;
}

void *FN(PW,free)(__isl_take PW *pw)
{
	int i;

	if (!pw)
		return NULL;
	if (--pw->ref > 0)
		return NULL;

	for (i = 0; i < pw->n; ++i) {
		isl_set_free(pw->p[i].set);
		FN(EL,free)(pw->p[i].FIELD);
	}
	isl_dim_free(pw->dim);
	free(pw);

	return NULL;
}

int FN(PW,IS_ZERO)(__isl_keep PW *pw)
{
	if (!pw)
		return -1;

	return pw->n == 0;
}

__isl_give PW *FN(PW,add)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	int i, j, n;
	struct PW *res;
	isl_ctx *ctx;
	isl_set *set;

	if (!pw1 || !pw2)
		goto error;

	ctx = isl_dim_get_ctx(pw1->dim);
#ifdef HAS_TYPE
	if (pw1->type != pw2->type)
		isl_die(ctx, isl_error_invalid,
			"fold types don't match", goto error);
#endif
	isl_assert(ctx, isl_dim_equal(pw1->dim, pw2->dim), goto error);

	if (FN(PW,IS_ZERO)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,IS_ZERO)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

	n = (pw1->n + 1) * (pw2->n + 1);
#ifdef HAS_TYPE
	res = FN(PW,alloc_)(isl_dim_copy(pw1->dim), pw1->type, n);
#else
	res = FN(PW,alloc_)(isl_dim_copy(pw1->dim), n);
#endif

	for (i = 0; i < pw1->n; ++i) {
		set = isl_set_copy(pw1->p[i].set);
		for (j = 0; j < pw2->n; ++j) {
			struct isl_set *common;
			EL *sum;
			set = isl_set_subtract(set,
					isl_set_copy(pw2->p[j].set));
			common = isl_set_intersect(isl_set_copy(pw1->p[i].set),
						isl_set_copy(pw2->p[j].set));
			if (isl_set_plain_is_empty(common)) {
				isl_set_free(common);
				continue;
			}

			sum = FN(EL,add_on_domain)(common,
						   FN(EL,copy)(pw1->p[i].FIELD),
						   FN(EL,copy)(pw2->p[j].FIELD));

			res = FN(PW,add_piece)(res, common, sum);
		}
		res = FN(PW,add_piece)(res, set, FN(EL,copy)(pw1->p[i].FIELD));
	}

	for (j = 0; j < pw2->n; ++j) {
		set = isl_set_copy(pw2->p[j].set);
		for (i = 0; i < pw1->n; ++i)
			set = isl_set_subtract(set,
					isl_set_copy(pw1->p[i].set));
		res = FN(PW,add_piece)(res, set, FN(EL,copy)(pw2->p[j].FIELD));
	}

	FN(PW,free)(pw1);
	FN(PW,free)(pw2);

	return res;
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return NULL;
}

__isl_give PW *FN(PW,add_disjoint)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	int i;
	isl_ctx *ctx;
	PW *res;

	if (!pw1 || !pw2)
		goto error;

	ctx = isl_dim_get_ctx(pw1->dim);
#ifdef HAS_TYPE
	if (pw1->type != pw2->type)
		isl_die(ctx, isl_error_invalid,
			"fold types don't match", goto error);
#endif
	isl_assert(ctx, isl_dim_equal(pw1->dim, pw2->dim), goto error);

	if (FN(PW,IS_ZERO)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,IS_ZERO)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

#ifdef HAS_TYPE
	res = FN(PW,alloc_)(isl_dim_copy(pw1->dim), pw1->type, pw1->n + pw2->n);
#else
	res = FN(PW,alloc_)(isl_dim_copy(pw1->dim), pw1->n + pw2->n);
#endif

	for (i = 0; i < pw1->n; ++i)
		res = FN(PW,add_piece)(res,
				isl_set_copy(pw1->p[i].set),
				FN(EL,copy)(pw1->p[i].FIELD));

	for (i = 0; i < pw2->n; ++i)
		res = FN(PW,add_piece)(res,
				isl_set_copy(pw2->p[i].set),
				FN(EL,copy)(pw2->p[i].FIELD));

	FN(PW,free)(pw1);
	FN(PW,free)(pw2);

	return res;
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return NULL;
}

#ifndef NO_NEG
__isl_give PW *FN(PW,neg)(__isl_take PW *pw)
{
	int i;

	if (!pw)
		return NULL;

	if (FN(PW,IS_ZERO)(pw))
		return pw;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].FIELD = FN(EL,neg)(pw->p[i].FIELD);
		if (!pw->p[i].FIELD)
			return FN(PW,free)(pw);
	}

	return pw;
}

__isl_give PW *FN(PW,sub)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	return FN(PW,add)(pw1, FN(PW,neg)(pw2));
}
#endif

#ifndef NO_EVAL
__isl_give isl_qpolynomial *FN(PW,eval)(__isl_take PW *pw,
	__isl_take isl_point *pnt)
{
	int i;
	int found = 0;
	isl_ctx *ctx;
	isl_dim *pnt_dim = NULL;
	isl_qpolynomial *qp;

	if (!pw || !pnt)
		goto error;
	ctx = isl_point_get_ctx(pnt);
	pnt_dim = isl_point_get_dim(pnt);
	isl_assert(ctx, isl_dim_equal(pnt_dim, pw->dim), goto error);

	for (i = 0; i < pw->n; ++i) {
		found = isl_set_contains_point(pw->p[i].set, pnt);
		if (found < 0)
			goto error;
		if (found)
			break;
	}
	if (found)
		qp = FN(EL,eval)(FN(EL,copy)(pw->p[i].FIELD),
					    isl_point_copy(pnt));
	else
		qp = isl_qpolynomial_zero(isl_dim_copy(pw->dim));
	FN(PW,free)(pw);
	isl_dim_free(pnt_dim);
	isl_point_free(pnt);
	return qp;
error:
	FN(PW,free)(pw);
	isl_dim_free(pnt_dim);
	isl_point_free(pnt);
	return NULL;
}
#endif

__isl_give isl_set *FN(PW,domain)(__isl_take PW *pw)
{
	int i;
	isl_set *dom;

	if (!pw)
		return NULL;

	dom = isl_set_empty(isl_dim_copy(pw->dim));
	for (i = 0; i < pw->n; ++i)
		dom = isl_set_union_disjoint(dom, isl_set_copy(pw->p[i].set));

	FN(PW,free)(pw);

	return dom;
}

__isl_give PW *FN(PW,intersect_domain)(__isl_take PW *pw, __isl_take isl_set *set)
{
	int i;

	if (!pw || !set)
		goto error;

	if (pw->n == 0) {
		isl_set_free(set);
		return pw;
	}

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;

	for (i = pw->n - 1; i >= 0; --i) {
		isl_basic_set *aff;
		pw->p[i].set = isl_set_intersect(pw->p[i].set, isl_set_copy(set));
		if (!pw->p[i].set)
			goto error;
		aff = isl_set_affine_hull(isl_set_copy(pw->p[i].set));
		pw->p[i].FIELD = FN(EL,substitute_equalities)(pw->p[i].FIELD,
								aff);
		if (isl_set_plain_is_empty(pw->p[i].set)) {
			isl_set_free(pw->p[i].set);
			FN(EL,free)(pw->p[i].FIELD);
			if (i != pw->n - 1)
				pw->p[i] = pw->p[pw->n - 1];
			pw->n--;
		}
	}
	
	isl_set_free(set);
	return pw;
error:
	isl_set_free(set);
	FN(PW,free)(pw);
	return NULL;
}

__isl_give PW *FN(PW,gist)(__isl_take PW *pw, __isl_take isl_set *context)
{
	int i;
	isl_basic_set *hull = NULL;

	if (!pw || !context)
		goto error;

	if (pw->n == 0) {
		isl_set_free(context);
		return pw;
	}

	context = isl_set_compute_divs(context);
	hull = isl_set_simple_hull(isl_set_copy(context));

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;

	for (i = pw->n - 1; i >= 0; --i) {
		pw->p[i].set = isl_set_intersect(pw->p[i].set,
						 isl_set_copy(context));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,gist)(pw->p[i].FIELD,
					     isl_set_copy(pw->p[i].set));
		pw->p[i].set = isl_set_gist_basic_set(pw->p[i].set,
						isl_basic_set_copy(hull));
		if (!pw->p[i].set)
			goto error;
		if (isl_set_plain_is_empty(pw->p[i].set)) {
			isl_set_free(pw->p[i].set);
			FN(EL,free)(pw->p[i].FIELD);
			if (i != pw->n - 1)
				pw->p[i] = pw->p[pw->n - 1];
			pw->n--;
		}
	}

	isl_basic_set_free(hull);
	isl_set_free(context);

	return pw;
error:
	FN(PW,free)(pw);
	isl_basic_set_free(hull);
	isl_set_free(context);
	return NULL;
}

__isl_give PW *FN(PW,coalesce)(__isl_take PW *pw)
{
	int i, j;

	if (!pw)
		return NULL;
	if (pw->n == 0)
		return pw;

	for (i = pw->n - 1; i >= 0; --i) {
		for (j = i - 1; j >= 0; --j) {
			if (!FN(EL,plain_is_equal)(pw->p[i].FIELD,
							pw->p[j].FIELD))
				continue;
			pw->p[j].set = isl_set_union(pw->p[j].set,
							pw->p[i].set);
			FN(EL,free)(pw->p[i].FIELD);
			if (i != pw->n - 1)
				pw->p[i] = pw->p[pw->n - 1];
			pw->n--;
			break;
		}
		if (j >= 0)
			continue;
		pw->p[i].set = isl_set_coalesce(pw->p[i].set);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

isl_ctx *FN(PW,get_ctx)(__isl_keep PW *pw)
{
	return pw ? isl_dim_get_ctx(pw->dim) : NULL;
}

#ifndef NO_INVOLVES_DIMS
int FN(PW,involves_dims)(__isl_keep PW *pw, enum isl_dim_type type,
	unsigned first, unsigned n)
{
	int i;

	if (!pw)
		return -1;
	if (pw->n == 0 || n == 0)
		return 0;
	for (i = 0; i < pw->n; ++i) {
		int involves = FN(EL,involves_dims)(pw->p[i].FIELD,
							type, first, n);
		if (involves < 0 || involves)
			return involves;
		involves = isl_set_involves_dims(pw->p[i].set, type, first, n);
		if (involves < 0 || involves)
			return involves;
	}
	return 0;
}
#endif

__isl_give PW *FN(PW,set_dim_name)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	pw->dim = isl_dim_set_name(pw->dim, type, pos, s);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_set_dim_name(pw->p[i].set, type, pos, s);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,set_dim_name)(pw->p[i].FIELD, type, pos, s);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

#ifndef NO_DROP_DIMS
__isl_give PW *FN(PW,drop_dims)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	if (!pw)
		return NULL;
	if (n == 0 && !isl_dim_get_tuple_name(pw->dim, type))
		return pw;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	pw->dim = isl_dim_drop(pw->dim, type, first, n);
	if (!pw->dim)
		goto error;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_drop(pw->p[i].set, type, first, n);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,drop_dims)(pw->p[i].FIELD, type, first, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}
#endif

#ifndef NO_INSERT_DIMS
__isl_give PW *FN(PW,insert_dims)(__isl_take PW *pw, enum isl_dim_type type,
	unsigned first, unsigned n)
{
	int i;

	if (!pw)
		return NULL;
	if (n == 0 && !isl_dim_is_named_or_nested(pw->dim, type))
		return pw;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	pw->dim = isl_dim_insert(pw->dim, type, first, n);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_insert(pw->p[i].set, type, first, n);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,insert_dims)(pw->p[i].FIELD,
								type, first, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}
#endif

__isl_give PW *FN(PW,fix_dim)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned pos, isl_int v)
{
	int i;

	if (!pw)
		return NULL;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_fix(pw->p[i].set, type, pos, v);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

unsigned FN(PW,dim)(__isl_keep PW *pw, enum isl_dim_type type)
{
	return pw ? isl_dim_size(pw->dim, type) : 0;
}

__isl_give PW *FN(PW,split_dims)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	if (!pw)
		return NULL;
	if (n == 0)
		return pw;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	if (!pw->dim)
		goto error;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_split_dims(pw->p[i].set, type, first, n);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

#ifndef NO_OPT
/* Compute the maximal value attained by the piecewise quasipolynomial
 * on its domain or zero if the domain is empty.
 * In the worst case, the domain is scanned completely,
 * so the domain is assumed to be bounded.
 */
__isl_give isl_qpolynomial *FN(PW,opt)(__isl_take PW *pw, int max)
{
	int i;
	isl_qpolynomial *opt;

	if (!pw)
		return NULL;

	if (pw->n == 0) {
		isl_dim *dim = isl_dim_copy(pw->dim);
		FN(PW,free)(pw);
		return isl_qpolynomial_zero(dim);
	}

	opt = FN(EL,opt_on_domain)(FN(EL,copy)(pw->p[0].FIELD),
					isl_set_copy(pw->p[0].set), max);
	for (i = 1; i < pw->n; ++i) {
		isl_qpolynomial *opt_i;
		opt_i = FN(EL,opt_on_domain)(FN(EL,copy)(pw->p[i].FIELD),
						isl_set_copy(pw->p[i].set), max);
		if (max)
			opt = isl_qpolynomial_max_cst(opt, opt_i);
		else
			opt = isl_qpolynomial_min_cst(opt, opt_i);
	}

	FN(PW,free)(pw);
	return opt;
}

__isl_give isl_qpolynomial *FN(PW,max)(__isl_take PW *pw)
{
	return FN(PW,opt)(pw, 1);
}

__isl_give isl_qpolynomial *FN(PW,min)(__isl_take PW *pw)
{
	return FN(PW,opt)(pw, 0);
}
#endif

__isl_give isl_dim *FN(PW,get_dim)(__isl_keep PW *pw)
{
	return pw ? isl_dim_copy(pw->dim) : NULL;
}

#ifndef NO_RESET_DIM
__isl_give PW *FN(PW,reset_dim)(__isl_take PW *pw, __isl_take isl_dim *dim)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw || !dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_reset_dim(pw->p[i].set,
						 isl_dim_copy(dim));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,reset_dim)(pw->p[i].FIELD,
						  isl_dim_copy(dim));
		if (!pw->p[i].FIELD)
			goto error;
	}
	isl_dim_free(pw->dim);
	pw->dim = dim;

	return pw;
error:
	isl_dim_free(dim);
	FN(PW,free)(pw);
	return NULL;
}
#endif

int FN(PW,has_equal_dim)(__isl_keep PW *pw1, __isl_keep PW *pw2)
{
	if (!pw1 || !pw2)
		return -1;

	return isl_dim_equal(pw1->dim, pw2->dim);
}

#ifndef NO_MORPH
__isl_give PW *FN(PW,morph)(__isl_take PW *pw, __isl_take isl_morph *morph)
{
	int i;
	isl_ctx *ctx;

	if (!pw || !morph)
		goto error;

	ctx = isl_dim_get_ctx(pw->dim);
	isl_assert(ctx, isl_dim_equal(pw->dim, morph->dom->dim),
		goto error);

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;
	isl_dim_free(pw->dim);
	pw->dim = isl_dim_copy(morph->ran->dim);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_morph_set(isl_morph_copy(morph), pw->p[i].set);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,morph)(pw->p[i].FIELD,
						isl_morph_copy(morph));
		if (!pw->p[i].FIELD)
			goto error;
	}

	isl_morph_free(morph);

	return pw;
error:
	FN(PW,free)(pw);
	isl_morph_free(morph);
	return NULL;
}
#endif

int FN(PW,foreach_piece)(__isl_keep PW *pw,
	int (*fn)(__isl_take isl_set *set, __isl_take EL *el, void *user),
	void *user)
{
	int i;

	if (!pw)
		return -1;

	for (i = 0; i < pw->n; ++i)
		if (fn(isl_set_copy(pw->p[i].set),
				FN(EL,copy)(pw->p[i].FIELD), user) < 0)
			return -1;

	return 0;
}

#ifndef NO_LIFT
static int any_divs(__isl_keep isl_set *set)
{
	int i;

	if (!set)
		return -1;

	for (i = 0; i < set->n; ++i)
		if (set->p[i]->n_div > 0)
			return 1;

	return 0;
}

static int foreach_lifted_subset(__isl_take isl_set *set, __isl_take EL *el,
	int (*fn)(__isl_take isl_set *set, __isl_take EL *el,
		    void *user), void *user)
{
	int i;

	if (!set || !el)
		goto error;

	for (i = 0; i < set->n; ++i) {
		isl_set *lift;
		EL *copy;

		lift = isl_set_from_basic_set(isl_basic_set_copy(set->p[i]));
		lift = isl_set_lift(lift);

		copy = FN(EL,copy)(el);
		copy = FN(EL,lift)(copy, isl_set_get_dim(lift));

		if (fn(lift, copy, user) < 0)
			goto error;
	}

	isl_set_free(set);
	FN(EL,free)(el);

	return 0;
error:
	isl_set_free(set);
	FN(EL,free)(el);
	return -1;
}

int FN(PW,foreach_lifted_piece)(__isl_keep PW *pw,
	int (*fn)(__isl_take isl_set *set, __isl_take EL *el,
		    void *user), void *user)
{
	int i;

	if (!pw)
		return -1;

	for (i = 0; i < pw->n; ++i) {
		isl_set *set;
		EL *el;

		set = isl_set_copy(pw->p[i].set);
		el = FN(EL,copy)(pw->p[i].FIELD);
		if (!any_divs(set)) {
			if (fn(set, el, user) < 0)
				return -1;
			continue;
		}
		if (foreach_lifted_subset(set, el, fn, user) < 0)
			return -1;
	}

	return 0;
}
#endif

#ifndef NO_MOVE_DIMS
__isl_give PW *FN(PW,move_dims)(__isl_take PW *pw,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	pw->dim = isl_dim_move(pw->dim, dst_type, dst_pos, src_type, src_pos, n);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_move_dims(pw->p[i].set,
						dst_type, dst_pos,
						src_type, src_pos, n);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,move_dims)(pw->p[i].FIELD,
					dst_type, dst_pos, src_type, src_pos, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}
#endif

#ifndef NO_REALIGN
__isl_give PW *FN(PW,realign)(__isl_take PW *pw, __isl_take isl_reordering *exp)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw || !exp)
		return NULL;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_realign(pw->p[i].set,
						    isl_reordering_copy(exp));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,realign)(pw->p[i].FIELD,
						    isl_reordering_copy(exp));
		if (!pw->p[i].FIELD)
			goto error;
	}

	pw = FN(PW,reset_dim)(pw, isl_dim_copy(exp->dim));

	isl_reordering_free(exp);
	return pw;
error:
	isl_reordering_free(exp);
	FN(PW,free)(pw);
	return NULL;
}

/* Align the parameters of "pw" to those of "model".
 */
__isl_give PW *FN(PW,align_params)(__isl_take PW *pw, __isl_take isl_dim *model)
{
	isl_ctx *ctx;

	if (!pw || !model)
		goto error;

	ctx = isl_dim_get_ctx(model);
	if (!isl_dim_has_named_params(model))
		isl_die(ctx, isl_error_invalid,
			"model has unnamed parameters", goto error);
	if (!isl_dim_has_named_params(pw->dim))
		isl_die(ctx, isl_error_invalid,
			"input has unnamed parameters", goto error);
	if (!isl_dim_match(pw->dim, isl_dim_param, model, isl_dim_param)) {
		isl_reordering *exp;

		model = isl_dim_drop(model, isl_dim_in,
					0, isl_dim_size(model, isl_dim_in));
		model = isl_dim_drop(model, isl_dim_out,
					0, isl_dim_size(model, isl_dim_out));
		exp = isl_parameter_alignment_reordering(pw->dim, model);
		exp = isl_reordering_extend_dim(exp, FN(PW,get_dim)(pw));
		pw = FN(PW,realign)(pw, exp);
	}

	isl_dim_free(model);
	return pw;
error:
	isl_dim_free(model);
	FN(PW,free)(pw);
	return NULL;
}
#endif

__isl_give PW *FN(PW,mul_isl_int)(__isl_take PW *pw, isl_int v)
{
	int i;

	if (isl_int_is_one(v))
		return pw;
	if (pw && isl_int_is_zero(v)) {
		PW *zero;
		isl_dim *dim = FN(PW,get_dim)(pw);
#ifdef HAS_TYPE
		zero = FN(PW,ZERO)(dim, pw->type);
#else
		zero = FN(PW,ZERO)(dim);
#endif
		FN(PW,free)(pw);
		return zero;
	}
	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	if (pw->n == 0)
		return pw;

#ifdef HAS_TYPE
	if (isl_int_is_neg(v))
		pw->type = isl_fold_type_negate(pw->type);
#endif
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].FIELD = FN(EL,scale)(pw->p[i].FIELD, v);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

__isl_give PW *FN(PW,scale)(__isl_take PW *pw, isl_int v)
{
	return FN(PW,mul_isl_int)(pw, v);
}
