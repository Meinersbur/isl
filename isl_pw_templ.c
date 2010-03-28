#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xS(TYPE,NAME) struct TYPE ## _ ## NAME
#define S(TYPE,NAME) xS(TYPE,NAME)

static __isl_give PW *FN(PW,alloc_)(__isl_take isl_dim *dim, int n)
{
	struct PW *pw;

	if (!dim)
		return NULL;
	isl_assert(dim->ctx, n >= 0, goto error);
	pw = isl_alloc(dim->ctx, struct PW,
			sizeof(struct PW) + (n - 1) * sizeof(S(PW,piece)));
	if (!pw)
		goto error;

	pw->ref = 1;
	pw->size = n;
	pw->n = 0;
	pw->dim = dim;
	return pw;
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give PW *FN(PW,zero)(__isl_take isl_dim *dim)
{
	return FN(PW,alloc_)(dim, 0);
}

__isl_give PW *FN(PW,add_piece)(__isl_take PW *pw,
	__isl_take isl_set *set, __isl_take EL *el)
{
	if (!pw || !set || !el)
		goto error;

	if (isl_set_fast_is_empty(set) || FN(EL,IS_ZERO)(el)) {
		isl_set_free(set);
		FN(EL,free)(el);
		return pw;
	}

	isl_assert(set->ctx, isl_dim_equal(pw->dim, el->dim), goto error);
	isl_assert(set->ctx, pw->n < pw->size, goto error);

	pw->p[pw->n].set = set;
	pw->p[pw->n].FIELD = el;
	pw->n++;
	
	return pw;
error:
	FN(PW,free)(pw);
	isl_set_free(set);
	FN(EL,free)(el);
	return NULL;
}

__isl_give PW *FN(PW,alloc)(__isl_take isl_set *set, __isl_take EL *el)
{
	PW *pw;

	if (!set || !el)
		goto error;

	pw = FN(PW,alloc_)(isl_set_get_dim(set), 1);

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

	dup = FN(PW,alloc_)(isl_dim_copy(pw->dim), pw->n);
	if (!dup)
		return NULL;

	for (i = 0; i < pw->n; ++i)
		dup = FN(PW,add_piece)(dup, isl_set_copy(pw->p[i].set),
					    FN(EL,copy)(pw->p[i].FIELD));

	return dup;
error:
	FN(PW,free)(dup);
	return NULL;
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

void FN(PW,free)(__isl_take PW *pw)
{
	int i;

	if (!pw)
		return;
	if (--pw->ref > 0)
		return;

	for (i = 0; i < pw->n; ++i) {
		isl_set_free(pw->p[i].set);
		FN(EL,free)(pw->p[i].FIELD);
	}
	isl_dim_free(pw->dim);
	free(pw);
}

int FN(PW,is_zero)(__isl_keep PW *pw)
{
	if (!pw)
		return -1;

	return pw->n == 0;
}

__isl_give PW *FN(PW,add)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	int i, j, n;
	struct PW *res;
	isl_set *set;

	if (!pw1 || !pw2)
		goto error;

	isl_assert(pw1->dim->ctx, isl_dim_equal(pw1->dim, pw2->dim), goto error);

	if (FN(PW,is_zero)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,is_zero)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

	n = (pw1->n + 1) * (pw2->n + 1);
	res = FN(PW,alloc_)(isl_dim_copy(pw1->dim), n);

	for (i = 0; i < pw1->n; ++i) {
		set = isl_set_copy(pw1->p[i].set);
		for (j = 0; j < pw2->n; ++j) {
			struct isl_set *common;
			EL *sum;
			set = isl_set_subtract(set,
					isl_set_copy(pw2->p[j].set));
			common = isl_set_intersect(isl_set_copy(pw1->p[i].set),
						isl_set_copy(pw2->p[j].set));
			if (isl_set_fast_is_empty(common)) {
				isl_set_free(common);
				continue;
			}

			sum = ADD(common, FN(EL,copy)(pw1->p[i].FIELD),
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
	PW *res;

	if (!pw1 || !pw2)
		goto error;

	isl_assert(pw1->dim->ctx, isl_dim_equal(pw1->dim, pw2->dim), goto error);

	if (FN(PW,is_zero)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,is_zero)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

	res = FN(PW,alloc_)(isl_dim_copy(pw1->dim), pw1->n + pw2->n);

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

__isl_give isl_qpolynomial *FN(PW,eval)(__isl_take PW *pw,
	__isl_take isl_point *pnt)
{
	int i;
	int found;
	isl_qpolynomial *qp;

	if (!pw || !pnt)
		goto error;
	isl_assert(pnt->dim->ctx, isl_dim_equal(pnt->dim, pw->dim), goto error);

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
	isl_point_free(pnt);
	return qp;
error:
	FN(PW,free)(pw);
	isl_point_free(pnt);
	return NULL;
}

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

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_intersect(pw->p[i].set, isl_set_copy(set));
		if (!pw->p[i].set)
			goto error;
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

	hull = isl_set_convex_hull(isl_set_copy(context));

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_gist(pw->p[i].set,
						isl_basic_set_copy(hull));
		if (!pw->p[i].set)
			goto error;
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
			if (!FN(EL,is_equal)(pw->p[i].FIELD, pw->p[j].FIELD))
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
	return pw ? pw->dim->ctx : NULL;
}

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
	}
	return 0;
}

__isl_give PW *FN(PW,drop_dims)(__isl_take PW *pw,
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
