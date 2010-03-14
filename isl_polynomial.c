#include <isl_seq.h>
#include <isl_polynomial_private.h>
#include <isl_point_private.h>
#include <isl_dim_private.h>
#include <isl_map_private.h>

int isl_upoly_is_cst(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return -1;

	return up->var < 0;
}

__isl_keep struct isl_upoly_cst *isl_upoly_as_cst(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return NULL;

	isl_assert(up->ctx, up->var < 0, return NULL);

	return (struct isl_upoly_cst *)up;
}

__isl_keep struct isl_upoly_rec *isl_upoly_as_rec(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return NULL;

	isl_assert(up->ctx, up->var >= 0, return NULL);

	return (struct isl_upoly_rec *)up;
}

int isl_upoly_is_zero(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_zero(cst->n) && isl_int_is_pos(cst->d);
}

int isl_upoly_is_nan(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_zero(cst->n) && isl_int_is_zero(cst->d);
}

int isl_upoly_is_infty(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_pos(cst->n) && isl_int_is_zero(cst->d);
}

int isl_upoly_is_neginfty(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_neg(cst->n) && isl_int_is_zero(cst->d);
}

int isl_upoly_is_one(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_eq(cst->n, cst->d) && isl_int_is_pos(cst->d);
}

int isl_upoly_is_negone(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (!up)
		return -1;
	if (!isl_upoly_is_cst(up))
		return 0;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return -1;

	return isl_int_is_negone(cst->n) && isl_int_is_one(cst->d);
}

__isl_give struct isl_upoly_cst *isl_upoly_cst_alloc(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_alloc_type(ctx, struct isl_upoly_cst);
	if (!cst)
		return NULL;

	cst->up.ref = 1;
	cst->up.ctx = ctx;
	isl_ctx_ref(ctx);
	cst->up.var = -1;

	isl_int_init(cst->n);
	isl_int_init(cst->d);

	return cst;
}

__isl_give struct isl_upoly *isl_upoly_zero(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, 0);
	isl_int_set_si(cst->d, 1);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_infty(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, 1);
	isl_int_set_si(cst->d, 0);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_nan(struct isl_ctx *ctx)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set_si(cst->n, 0);
	isl_int_set_si(cst->d, 0);

	return &cst->up;
}

__isl_give struct isl_upoly *isl_upoly_rat_cst(struct isl_ctx *ctx,
	isl_int n, isl_int d)
{
	struct isl_upoly_cst *cst;

	cst = isl_upoly_cst_alloc(ctx);
	if (!cst)
		return NULL;

	isl_int_set(cst->n, n);
	isl_int_set(cst->d, d);

	return &cst->up;
}

__isl_give struct isl_upoly_rec *isl_upoly_alloc_rec(struct isl_ctx *ctx,
	int var, int size)
{
	struct isl_upoly_rec *rec;

	isl_assert(ctx, var >= 0, return NULL);
	isl_assert(ctx, size >= 0, return NULL);
	rec = isl_calloc(dim->ctx, struct isl_upoly_rec,
			sizeof(struct isl_upoly_rec) +
			(size - 1) * sizeof(struct isl_upoly *));
	if (!rec)
		return NULL;

	rec->up.ref = 1;
	rec->up.ctx = ctx;
	isl_ctx_ref(ctx);
	rec->up.var = var;

	rec->n = 0;
	rec->size = size;

	return rec;
error:
	isl_upoly_free(&rec->up);
	return NULL;
}

int isl_qpolynomial_is_zero(__isl_keep isl_qpolynomial *qp)
{
	struct isl_upoly_cst *cst;

	if (!qp)
		return -1;

	return isl_upoly_is_zero(qp->upoly);
}

int isl_qpolynomial_is_one(__isl_keep isl_qpolynomial *qp)
{
	struct isl_upoly_cst *cst;

	if (!qp)
		return -1;

	return isl_upoly_is_one(qp->upoly);
}

static void upoly_free_cst(__isl_take struct isl_upoly_cst *cst)
{
	isl_int_clear(cst->n);
	isl_int_clear(cst->d);
}

static void upoly_free_rec(__isl_take struct isl_upoly_rec *rec)
{
	int i;

	for (i = 0; i < rec->n; ++i)
		isl_upoly_free(rec->p[i]);
}

__isl_give struct isl_upoly *isl_upoly_copy(__isl_keep struct isl_upoly *up)
{
	if (!up)
		return NULL;

	up->ref++;
	return up;
}

__isl_give struct isl_upoly *isl_upoly_dup_cst(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;
	struct isl_upoly_cst *dup;

	cst = isl_upoly_as_cst(up);
	if (!cst)
		return NULL;

	dup = isl_upoly_as_cst(isl_upoly_zero(up->ctx));
	if (!dup)
		return NULL;
	isl_int_set(dup->n, cst->n);
	isl_int_set(dup->d, cst->d);

	return &dup->up;
}

__isl_give struct isl_upoly *isl_upoly_dup_rec(__isl_keep struct isl_upoly *up)
{
	int i;
	struct isl_upoly_rec *rec;
	struct isl_upoly_rec *dup;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		return NULL;

	dup = isl_upoly_alloc_rec(up->ctx, up->var, rec->n);
	if (!dup)
		return NULL;

	for (i = 0; i < rec->n; ++i) {
		dup->p[i] = isl_upoly_copy(rec->p[i]);
		if (!dup->p[i])
			goto error;
		dup->n++;
	}

	return &dup->up;
error:
	isl_upoly_free(&dup->up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_dup(__isl_keep struct isl_upoly *up)
{
	struct isl_upoly *dup;

	if (!up)
		return NULL;

	if (isl_upoly_is_cst(up))
		return isl_upoly_dup_cst(up);
	else
		return isl_upoly_dup_rec(up);
}

__isl_give struct isl_upoly *isl_upoly_cow(__isl_take struct isl_upoly *up)
{
	if (!up)
		return NULL;

	if (up->ref == 1)
		return up;
	up->ref--;
	return isl_upoly_dup(up);
}

void isl_upoly_free(__isl_take struct isl_upoly *up)
{
	if (!up)
		return;

	if (--up->ref > 0)
		return;

	if (up->var < 0)
		upoly_free_cst((struct isl_upoly_cst *)up);
	else
		upoly_free_rec((struct isl_upoly_rec *)up);

	isl_ctx_deref(up->ctx);
	free(up);
}

static void isl_upoly_cst_reduce(__isl_keep struct isl_upoly_cst *cst)
{
	isl_int gcd;

	isl_int_init(gcd);
	isl_int_gcd(gcd, cst->n, cst->d);
	if (!isl_int_is_zero(gcd) && !isl_int_is_one(gcd)) {
		isl_int_divexact(cst->n, cst->n, gcd);
		isl_int_divexact(cst->d, cst->d, gcd);
	}
	isl_int_clear(gcd);
}

__isl_give struct isl_upoly *isl_upoly_sum_cst(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	struct isl_upoly_cst *cst1;
	struct isl_upoly_cst *cst2;

	up1 = isl_upoly_cow(up1);
	if (!up1 || !up2)
		goto error;

	cst1 = isl_upoly_as_cst(up1);
	cst2 = isl_upoly_as_cst(up2);

	if (isl_int_eq(cst1->d, cst2->d))
		isl_int_add(cst1->n, cst1->n, cst2->n);
	else {
		isl_int_mul(cst1->n, cst1->n, cst2->d);
		isl_int_addmul(cst1->n, cst2->n, cst1->d);
		isl_int_mul(cst1->d, cst1->d, cst2->d);
	}

	isl_upoly_cst_reduce(cst1);

	isl_upoly_free(up2);
	return up1;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

static __isl_give struct isl_upoly *replace_by_zero(
	__isl_take struct isl_upoly *up)
{
	struct isl_ctx *ctx;

	if (!up)
		return NULL;
	ctx = up->ctx;
	isl_upoly_free(up);
	return isl_upoly_zero(ctx);
}

static __isl_give struct isl_upoly *replace_by_constant_term(
	__isl_take struct isl_upoly *up)
{
	struct isl_upoly_rec *rec;
	struct isl_upoly *cst;

	if (!up)
		return NULL;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;
	isl_assert(up->ctx, rec->n == 1, goto error);
	cst = isl_upoly_copy(rec->p[0]);
	isl_upoly_free(up);
	return cst;
error:
	isl_upoly_free(up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_sum(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	int i;
	struct isl_upoly_rec *rec1, *rec2;

	if (!up1 || !up2)
		goto error;

	if (isl_upoly_is_nan(up1)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_nan(up2)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_zero(up1)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_zero(up2)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (up1->var < up2->var)
		return isl_upoly_sum(up2, up1);

	if (up2->var < up1->var) {
		struct isl_upoly_rec *rec;
		if (isl_upoly_is_infty(up2) || isl_upoly_is_neginfty(up2)) {
			isl_upoly_free(up1);
			return up2;
		}
		up1 = isl_upoly_cow(up1);
		rec = isl_upoly_as_rec(up1);
		if (!rec)
			goto error;
		rec->p[0] = isl_upoly_sum(rec->p[0], up2);
		if (rec->n == 1)
			up1 = replace_by_constant_term(up1);
		return up1;
	}

	if (isl_upoly_is_cst(up1))
		return isl_upoly_sum_cst(up1, up2);

	rec1 = isl_upoly_as_rec(up1);
	rec2 = isl_upoly_as_rec(up2);
	if (!rec1 || !rec2)
		goto error;

	if (rec1->n < rec2->n)
		return isl_upoly_sum(up2, up1);

	up1 = isl_upoly_cow(up1);
	rec1 = isl_upoly_as_rec(up1);
	if (!rec1)
		goto error;

	for (i = rec2->n - 1; i >= 0; --i) {
		rec1->p[i] = isl_upoly_sum(rec1->p[i],
					    isl_upoly_copy(rec2->p[i]));
		if (!rec1->p[i])
			goto error;
		if (i == rec1->n - 1 && isl_upoly_is_zero(rec1->p[i])) {
			isl_upoly_free(rec1->p[i]);
			rec1->n--;
		}
	}

	if (rec1->n == 0)
		up1 = replace_by_zero(up1);
	else if (rec1->n == 1)
		up1 = replace_by_constant_term(up1);

	isl_upoly_free(up2);

	return up1;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_neg_cst(__isl_take struct isl_upoly *up)
{
	struct isl_upoly_cst *cst;

	if (isl_upoly_is_zero(up))
		return up;

	up = isl_upoly_cow(up);
	if (!up)
		return NULL;

	cst = isl_upoly_as_cst(up);

	isl_int_neg(cst->n, cst->n);

	return up;
}

__isl_give struct isl_upoly *isl_upoly_neg(__isl_take struct isl_upoly *up)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up)
		return NULL;

	if (isl_upoly_is_cst(up))
		return isl_upoly_neg_cst(up);

	up = isl_upoly_cow(up);
	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		rec->p[i] = isl_upoly_neg(rec->p[i]);
		if (!rec->p[i])
			goto error;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_mul_cst(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	struct isl_upoly_cst *cst1;
	struct isl_upoly_cst *cst2;

	up1 = isl_upoly_cow(up1);
	if (!up1 || !up2)
		goto error;

	cst1 = isl_upoly_as_cst(up1);
	cst2 = isl_upoly_as_cst(up2);

	isl_int_mul(cst1->n, cst1->n, cst2->n);
	isl_int_mul(cst1->d, cst1->d, cst2->d);

	isl_upoly_cst_reduce(cst1);

	isl_upoly_free(up2);
	return up1;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_mul_rec(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	struct isl_upoly_rec *rec1;
	struct isl_upoly_rec *rec2;
	struct isl_upoly_rec *res;
	int i, j;
	int size;

	rec1 = isl_upoly_as_rec(up1);
	rec2 = isl_upoly_as_rec(up2);
	if (!rec1 || !rec2)
		goto error;
	size = rec1->n + rec2->n - 1;
	res = isl_upoly_alloc_rec(up1->ctx, up1->var, size);
	if (!res)
		goto error;

	for (i = 0; i < rec1->n; ++i) {
		res->p[i] = isl_upoly_mul(isl_upoly_copy(rec2->p[0]),
					    isl_upoly_copy(rec1->p[i]));
		if (!res->p[i])
			goto error;
		res->n++;
	}
	for (; i < size; ++i) {
		res->p[i] = isl_upoly_zero(up1->ctx);
		if (!res->p[i])
			goto error;
		res->n++;
	}
	for (i = 0; i < rec1->n; ++i) {
		for (j = 1; j < rec2->n; ++j) {
			struct isl_upoly *up;
			up = isl_upoly_mul(isl_upoly_copy(rec2->p[j]),
					    isl_upoly_copy(rec1->p[i]));
			res->p[i + j] = isl_upoly_sum(res->p[i + j], up);
			if (!res->p[i + j])
				goto error;
		}
	}

	isl_upoly_free(up1);
	isl_upoly_free(up2);

	return &res->up;
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	isl_upoly_free(&res->up);
	return NULL;
}

__isl_give struct isl_upoly *isl_upoly_mul(__isl_take struct isl_upoly *up1,
	__isl_take struct isl_upoly *up2)
{
	if (!up1 || !up2)
		goto error;

	if (isl_upoly_is_nan(up1)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_nan(up2)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_zero(up1)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_zero(up2)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_one(up1)) {
		isl_upoly_free(up1);
		return up2;
	}

	if (isl_upoly_is_one(up2)) {
		isl_upoly_free(up2);
		return up1;
	}

	if (up1->var < up2->var)
		return isl_upoly_mul(up2, up1);

	if (up2->var < up1->var) {
		int i;
		struct isl_upoly_rec *rec;
		if (isl_upoly_is_infty(up2) || isl_upoly_is_neginfty(up2)) {
			isl_ctx *ctx = up1->ctx;
			isl_upoly_free(up1);
			isl_upoly_free(up2);
			return isl_upoly_nan(ctx);
		}
		up1 = isl_upoly_cow(up1);
		rec = isl_upoly_as_rec(up1);
		if (!rec)
			goto error;

		for (i = 0; i < rec->n; ++i) {
			rec->p[i] = isl_upoly_mul(rec->p[i],
						    isl_upoly_copy(up2));
			if (!rec->p[i])
				goto error;
		}
		isl_upoly_free(up2);
		return up1;
	}

	if (isl_upoly_is_cst(up1))
		return isl_upoly_mul_cst(up1, up2);

	return isl_upoly_mul_rec(up1, up2);
error:
	isl_upoly_free(up1);
	isl_upoly_free(up2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_alloc(__isl_take isl_dim *dim,
	unsigned n_div)
{
	struct isl_qpolynomial *qp;
	unsigned total;

	if (!dim)
		return NULL;

	total = isl_dim_total(dim);

	qp = isl_calloc_type(dim->ctx, struct isl_qpolynomial);
	if (!qp)
		return NULL;

	qp->ref = 1;
	qp->div = isl_mat_alloc(dim->ctx, n_div, 1 + 1 + total + n_div);
	if (!qp->div)
		goto error;

	qp->dim = dim;

	return qp;
error:
	isl_dim_free(dim);
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_copy(__isl_keep isl_qpolynomial *qp)
{
	if (!qp)
		return NULL;

	qp->ref++;
	return qp;
}

__isl_give isl_qpolynomial *isl_qpolynomial_dup(__isl_keep isl_qpolynomial *qp)
{
	struct isl_qpolynomial *dup;

	if (!qp)
		return NULL;

	dup = isl_qpolynomial_alloc(isl_dim_copy(qp->dim), qp->div->n_row);
	if (!dup)
		return NULL;
	isl_mat_free(dup->div);
	dup->div = isl_mat_copy(qp->div);
	if (!dup->div)
		goto error;
	dup->upoly = isl_upoly_copy(qp->upoly);
	if (!dup->upoly)
		goto error;

	return dup;
error:
	isl_qpolynomial_free(dup);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_cow(__isl_take isl_qpolynomial *qp)
{
	if (!qp)
		return NULL;

	if (qp->ref == 1)
		return qp;
	qp->ref--;
	return isl_qpolynomial_dup(qp);
}

void isl_qpolynomial_free(__isl_take isl_qpolynomial *qp)
{
	if (!qp)
		return;

	if (--qp->ref > 0)
		return;

	isl_dim_free(qp->dim);
	isl_mat_free(qp->div);
	isl_upoly_free(qp->upoly);

	free(qp);
}

static int compatible_divs(__isl_keep isl_mat *div1, __isl_keep isl_mat *div2)
{
	int n_row, n_col;
	int equal;

	isl_assert(div1->ctx, div1->n_row >= div2->n_row &&
				div1->n_col >= div2->n_col, return -1);

	if (div1->n_row == div2->n_row)
		return isl_mat_is_equal(div1, div2);

	n_row = div1->n_row;
	n_col = div1->n_col;
	div1->n_row = div2->n_row;
	div1->n_col = div2->n_col;

	equal = isl_mat_is_equal(div1, div2);

	div1->n_row = n_row;
	div1->n_col = n_col;

	return equal;
}

static void expand_row(__isl_keep isl_mat *dst, int d,
	__isl_keep isl_mat *src, int s, int *exp)
{
	int i;
	unsigned c = src->n_col - src->n_row;

	isl_seq_cpy(dst->row[d], src->row[s], c);
	isl_seq_clr(dst->row[d] + c, dst->n_col - c);

	for (i = 0; i < s; ++i)
		isl_int_set(dst->row[d][c + exp[i]], src->row[s][c + i]);
}

static int cmp_row(__isl_keep isl_mat *div, int i, int j)
{
	int li, lj;

	li = isl_seq_last_non_zero(div->row[i], div->n_col);
	lj = isl_seq_last_non_zero(div->row[j], div->n_col);

	if (li != lj)
		return li - lj;

	return isl_seq_cmp(div->row[i], div->row[j], div->n_col);
}

static __isl_give isl_mat *merge_divs(__isl_keep isl_mat *div1,
	__isl_keep isl_mat *div2, int *exp1, int *exp2)
{
	int i, j, k;
	isl_mat *div = NULL;
	unsigned d = div1->n_col - div1->n_row;

	div = isl_mat_alloc(div1->ctx, 1 + div1->n_row + div2->n_row,
				d + div1->n_row + div2->n_row);
	if (!div)
		return NULL;

	for (i = 0, j = 0, k = 0; i < div1->n_row && j < div2->n_row; ++k) {
		int cmp;

		expand_row(div, k, div1, i, exp1);
		expand_row(div, k + 1, div2, j, exp2);

		cmp = cmp_row(div, k, k + 1);
		if (cmp == 0) {
			exp1[i++] = k;
			exp2[j++] = k;
		} else if (cmp < 0) {
			exp1[i++] = k;
		} else {
			exp2[j++] = k;
			isl_seq_cpy(div->row[k], div->row[k + 1], div->n_col);
		}
	}
	for (; i < div1->n_row; ++i, ++k) {
		expand_row(div, k, div1, i, exp1);
		exp1[i] = k;
	}
	for (; j < div2->n_row; ++j, ++k) {
		expand_row(div, k, div2, j, exp2);
		exp2[j] = k;
	}

	div->n_row = k;
	div->n_col = d + k;

	return div;
}

static __isl_give struct isl_upoly *expand(__isl_take struct isl_upoly *up,
	int *exp, int first)
{
	int i;
	struct isl_upoly_rec *rec;

	if (isl_upoly_is_cst(up))
		return up;

	if (up->var < first)
		return up;

	if (exp[up->var - first] == up->var - first)
		return up;

	up = isl_upoly_cow(up);
	if (!up)
		goto error;

	up->var = exp[up->var - first] + first;

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		rec->p[i] = expand(rec->p[i], exp, first);
		if (!rec->p[i])
			goto error;
	}

	return up;
error:
	isl_upoly_free(up);
	return NULL;
}

static __isl_give isl_qpolynomial *with_merged_divs(
	__isl_give isl_qpolynomial *(*fn)(__isl_take isl_qpolynomial *qp1,
					  __isl_take isl_qpolynomial *qp2),
	__isl_take isl_qpolynomial *qp1, __isl_take isl_qpolynomial *qp2)
{
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_mat *div = NULL;

	qp1 = isl_qpolynomial_cow(qp1);
	qp2 = isl_qpolynomial_cow(qp2);

	if (!qp1 || !qp2)
		goto error;

	isl_assert(qp1->div->ctx, qp1->div->n_row >= qp2->div->n_row &&
				qp1->div->n_col >= qp2->div->n_col, goto error);

	exp1 = isl_alloc_array(qp1->div->ctx, int, qp1->div->n_row);
	exp2 = isl_alloc_array(qp2->div->ctx, int, qp2->div->n_row);
	if (!exp1 || !exp2)
		goto error;

	div = merge_divs(qp1->div, qp2->div, exp1, exp2);
	if (!div)
		goto error;

	isl_mat_free(qp1->div);
	qp1->div = isl_mat_copy(div);
	isl_mat_free(qp2->div);
	qp2->div = isl_mat_copy(div);

	qp1->upoly = expand(qp1->upoly, exp1, div->n_col - div->n_row - 2);
	qp2->upoly = expand(qp2->upoly, exp2, div->n_col - div->n_row - 2);

	if (!qp1->upoly || !qp2->upoly)
		goto error;

	isl_mat_free(div);
	free(exp1);
	free(exp2);

	return fn(qp1, qp2);
error:
	isl_mat_free(div);
	free(exp1);
	free(exp2);
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_add(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2)
{
	qp1 = isl_qpolynomial_cow(qp1);

	if (!qp1 || !qp2)
		goto error;

	if (qp1->div->n_row < qp2->div->n_row)
		return isl_qpolynomial_add(qp2, qp1);

	isl_assert(qp1->dim->ctx, isl_dim_equal(qp1->dim, qp2->dim), goto error);
	if (!compatible_divs(qp1->div, qp2->div))
		return with_merged_divs(isl_qpolynomial_add, qp1, qp2);

	qp1->upoly = isl_upoly_sum(qp1->upoly, isl_upoly_copy(qp2->upoly));
	if (!qp1->upoly)
		goto error;

	isl_qpolynomial_free(qp2);

	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_sub(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2)
{
	return isl_qpolynomial_add(qp1, isl_qpolynomial_neg(qp2));
}

__isl_give isl_qpolynomial *isl_qpolynomial_neg(__isl_take isl_qpolynomial *qp)
{
	qp = isl_qpolynomial_cow(qp);

	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_neg(qp->upoly);
	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_mul(__isl_take isl_qpolynomial *qp1,
	__isl_take isl_qpolynomial *qp2)
{
	qp1 = isl_qpolynomial_cow(qp1);

	if (!qp1 || !qp2)
		goto error;

	if (qp1->div->n_row < qp2->div->n_row)
		return isl_qpolynomial_mul(qp2, qp1);

	isl_assert(qp1->dim->ctx, isl_dim_equal(qp1->dim, qp2->dim), goto error);
	if (!compatible_divs(qp1->div, qp2->div))
		return with_merged_divs(isl_qpolynomial_mul, qp1, qp2);

	qp1->upoly = isl_upoly_mul(qp1->upoly, isl_upoly_copy(qp2->upoly));
	if (!qp1->upoly)
		goto error;

	isl_qpolynomial_free(qp2);

	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_zero(__isl_take isl_dim *dim)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_zero(dim->ctx);
	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_infty(__isl_take isl_dim *dim)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_infty(dim->ctx);
	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_nan(__isl_take isl_dim *dim)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_nan(dim->ctx);
	if (!qp->upoly)
		goto error;

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_cst(__isl_take isl_dim *dim,
	isl_int v)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_zero(dim->ctx);
	if (!qp->upoly)
		goto error;
	cst = isl_upoly_as_cst(qp->upoly);
	isl_int_set(cst->n, v);

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

int isl_qpolynomial_is_cst(__isl_keep isl_qpolynomial *qp,
	isl_int *n, isl_int *d)
{
	struct isl_upoly_cst *cst;

	if (!qp)
		return -1;

	if (!isl_upoly_is_cst(qp->upoly))
		return 0;

	cst = isl_upoly_as_cst(qp->upoly);
	if (!cst)
		return -1;

	if (n)
		isl_int_set(*n, cst->n);
	if (d)
		isl_int_set(*d, cst->d);

	return 1;
}

__isl_give isl_qpolynomial *isl_qpolynomial_pow(__isl_take isl_dim *dim,
	int pos, int power)
{
	int i;
	struct isl_qpolynomial *qp;
	struct isl_upoly_rec *rec;
	struct isl_upoly_cst *cst;
	struct isl_ctx *ctx;

	if (!dim)
		return NULL;

	ctx = dim->ctx;

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		return NULL;

	qp->upoly = &isl_upoly_alloc_rec(ctx, pos, 1 + power)->up;
	if (!qp->upoly)
		goto error;
	rec = isl_upoly_as_rec(qp->upoly);
	for (i = 0; i < 1 + power; ++i) {
		rec->p[i] = isl_upoly_zero(ctx);
		if (!rec->p[i])
			goto error;
		rec->n++;
	}
	cst = isl_upoly_as_cst(rec->p[power]);
	isl_int_set_si(cst->n, 1);

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_var(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned pos)
{
	if (!dim)
		return NULL;

	isl_assert(dim->ctx, isl_dim_size(dim, isl_dim_in) == 0, goto error);
	isl_assert(dim->ctx, pos < isl_dim_size(dim, type), goto error);

	if (type == isl_dim_set)
		pos += isl_dim_size(dim, isl_dim_param);

	return isl_qpolynomial_pow(dim, pos, 1);
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_div_pow(__isl_take isl_div *div,
	int power)
{
	struct isl_qpolynomial *qp = NULL;
	struct isl_upoly_rec *rec;
	struct isl_upoly_cst *cst;
	int i;
	int pos;

	if (!div)
		return NULL;
	isl_assert(div->ctx, div->bmap->n_div == 1, goto error);

	qp = isl_qpolynomial_alloc(isl_basic_map_get_dim(div->bmap), 1);
	if (!qp)
		goto error;

	isl_seq_cpy(qp->div->row[0], div->line[0], qp->div->n_col - 1);
	isl_int_set_si(qp->div->row[0][qp->div->n_col - 1], 0);

	pos = isl_dim_total(qp->dim);
	qp->upoly = &isl_upoly_alloc_rec(div->ctx, pos, 1 + power)->up;
	if (!qp->upoly)
		goto error;
	rec = isl_upoly_as_rec(qp->upoly);
	for (i = 0; i < 1 + power; ++i) {
		rec->p[i] = isl_upoly_zero(div->ctx);
		if (!rec->p[i])
			goto error;
		rec->n++;
	}
	cst = isl_upoly_as_cst(rec->p[power]);
	isl_int_set_si(cst->n, 1);

	isl_div_free(div);

	return qp;
error:
	isl_qpolynomial_free(qp);
	isl_div_free(div);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_div(__isl_take isl_div *div)
{
	return isl_qpolynomial_div_pow(div, 1);
}

__isl_give isl_qpolynomial *isl_qpolynomial_rat_cst(__isl_take isl_dim *dim,
	const isl_int n, const isl_int d)
{
	struct isl_qpolynomial *qp;
	struct isl_upoly_cst *cst;

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		return NULL;

	qp->upoly = isl_upoly_zero(dim->ctx);
	if (!qp->upoly)
		goto error;
	cst = isl_upoly_as_cst(qp->upoly);
	isl_int_set(cst->n, n);
	isl_int_set(cst->d, d);

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

#undef PW
#define PW isl_pw_qpolynomial
#undef EL
#define EL isl_qpolynomial
#undef IS_ZERO
#define IS_ZERO is_zero
#undef FIELD
#define FIELD qp
#undef ADD
#define ADD add

#include <isl_pw_templ.c>

#undef PW
#define PW isl_pw_qpolynomial_fold
#undef EL
#define EL isl_qpolynomial_fold
#undef IS_ZERO
#define IS_ZERO is_empty
#undef FIELD
#define FIELD fold
#undef ADD
#define ADD fold

#include <isl_pw_templ.c>

int isl_pw_qpolynomial_is_one(__isl_keep isl_pw_qpolynomial *pwqp)
{
	if (!pwqp)
		return -1;

	if (pwqp->n != -1)
		return 0;

	if (!isl_set_fast_is_universe(pwqp->p[0].set))
		return 0;

	return isl_qpolynomial_is_one(pwqp->p[0].qp);
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_mul(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2)
{
	int i, j, n;
	struct isl_pw_qpolynomial *res;
	isl_set *set;

	if (!pwqp1 || !pwqp2)
		goto error;

	isl_assert(pwqp1->dim->ctx, isl_dim_equal(pwqp1->dim, pwqp2->dim),
			goto error);

	if (isl_pw_qpolynomial_is_zero(pwqp1)) {
		isl_pw_qpolynomial_free(pwqp2);
		return pwqp1;
	}

	if (isl_pw_qpolynomial_is_zero(pwqp2)) {
		isl_pw_qpolynomial_free(pwqp1);
		return pwqp2;
	}

	if (isl_pw_qpolynomial_is_one(pwqp1)) {
		isl_pw_qpolynomial_free(pwqp1);
		return pwqp2;
	}

	if (isl_pw_qpolynomial_is_one(pwqp2)) {
		isl_pw_qpolynomial_free(pwqp2);
		return pwqp1;
	}

	n = pwqp1->n * pwqp2->n;
	res = isl_pw_qpolynomial_alloc_(isl_dim_copy(pwqp1->dim), n);

	for (i = 0; i < pwqp1->n; ++i) {
		for (j = 0; j < pwqp2->n; ++j) {
			struct isl_set *common;
			struct isl_qpolynomial *prod;
			common = isl_set_intersect(isl_set_copy(pwqp1->p[i].set),
						isl_set_copy(pwqp2->p[j].set));
			if (isl_set_fast_is_empty(common)) {
				isl_set_free(common);
				continue;
			}

			prod = isl_qpolynomial_mul(
				isl_qpolynomial_copy(pwqp1->p[i].qp),
				isl_qpolynomial_copy(pwqp2->p[j].qp));

			res = isl_pw_qpolynomial_add_piece(res, common, prod);
		}
	}

	isl_pw_qpolynomial_free(pwqp1);
	isl_pw_qpolynomial_free(pwqp2);

	return res;
error:
	isl_pw_qpolynomial_free(pwqp1);
	isl_pw_qpolynomial_free(pwqp2);
	return NULL;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_neg(
	__isl_take isl_pw_qpolynomial *pwqp)
{
	int i, j, n;
	struct isl_pw_qpolynomial *res;
	isl_set *set;

	if (!pwqp)
		return NULL;

	if (isl_pw_qpolynomial_is_zero(pwqp))
		return pwqp;

	pwqp = isl_pw_qpolynomial_cow(pwqp);

	for (i = 0; i < pwqp->n; ++i) {
		pwqp->p[i].qp = isl_qpolynomial_neg(pwqp->p[i].qp);
		if (!pwqp->p[i].qp)
			goto error;
	}

	return pwqp;
error:
	isl_pw_qpolynomial_free(pwqp);
	return NULL;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_sub(
	__isl_take isl_pw_qpolynomial *pwqp1,
	__isl_take isl_pw_qpolynomial *pwqp2)
{
	return isl_pw_qpolynomial_add(pwqp1, isl_pw_qpolynomial_neg(pwqp2));
}

__isl_give struct isl_upoly *isl_upoly_eval(
	__isl_take struct isl_upoly *up, __isl_take isl_vec *vec)
{
	int i;
	struct isl_upoly_rec *rec;
	struct isl_upoly *res;
	struct isl_upoly *base;

	if (isl_upoly_is_cst(up)) {
		isl_vec_free(vec);
		return up;
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	isl_assert(up->ctx, rec->n >= 1, goto error);

	base = isl_upoly_rat_cst(up->ctx, vec->el[1 + up->var], vec->el[0]);

	res = isl_upoly_eval(isl_upoly_copy(rec->p[rec->n - 1]),
				isl_vec_copy(vec));

	for (i = rec->n - 2; i >= 0; --i) {
		res = isl_upoly_mul(res, isl_upoly_copy(base));
		res = isl_upoly_sum(res, 
			    isl_upoly_eval(isl_upoly_copy(rec->p[i]),
							    isl_vec_copy(vec)));
	}

	isl_upoly_free(base);
	isl_upoly_free(up);
	isl_vec_free(vec);
	return res;
error:
	isl_upoly_free(up);
	isl_vec_free(vec);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_eval(
	__isl_take isl_qpolynomial *qp, __isl_take isl_point *pnt)
{
	isl_vec *ext;
	struct isl_upoly *up;
	isl_dim *dim;

	if (!qp || !pnt)
		goto error;
	isl_assert(pnt->dim->ctx, isl_dim_equal(pnt->dim, qp->dim), goto error);

	if (qp->div->n_row == 0)
		ext = isl_vec_copy(pnt->vec);
	else {
		int i;
		unsigned dim = isl_dim_total(qp->dim);
		ext = isl_vec_alloc(qp->dim->ctx, 1 + dim + qp->div->n_row);
		if (!ext)
			goto error;

		isl_seq_cpy(ext->el, pnt->vec->el, pnt->vec->size);
		for (i = 0; i < qp->div->n_row; ++i) {
			isl_seq_inner_product(qp->div->row[i] + 1, ext->el,
						1 + dim + i, &ext->el[1+dim+i]);
			isl_int_fdiv_q(ext->el[1+dim+i], ext->el[1+dim+i],
					qp->div->row[i][0]);
		}
	}

	up = isl_upoly_eval(isl_upoly_copy(qp->upoly), ext);
	if (!up)
		goto error;

	dim = isl_dim_copy(qp->dim);
	isl_qpolynomial_free(qp);
	isl_point_free(pnt);

	qp = isl_qpolynomial_alloc(dim, 0);
	if (!qp)
		isl_upoly_free(up);
	else
		qp->upoly = up;

	return qp;
error:
	isl_qpolynomial_free(qp);
	isl_point_free(pnt);
	return NULL;
}

int isl_upoly_cmp(__isl_keep struct isl_upoly_cst *cst1,
	__isl_keep struct isl_upoly_cst *cst2)
{
	int cmp;
	isl_int t;
	isl_int_init(t);
	isl_int_mul(t, cst1->n, cst2->d);
	isl_int_submul(t, cst2->n, cst1->d);
	cmp = isl_int_sgn(t);
	isl_int_clear(t);
	return cmp;
}

static __isl_give isl_qpolynomial *qpolynomial_min(
	__isl_take isl_qpolynomial *qp1, __isl_take isl_qpolynomial *qp2)
{
	struct isl_upoly_cst *cst1, *cst2;
	int cmp;

	if (!qp1 || !qp2)
		goto error;
	isl_assert(qp1->dim->ctx, isl_upoly_is_cst(qp1->upoly), goto error);
	isl_assert(qp2->dim->ctx, isl_upoly_is_cst(qp2->upoly), goto error);
	cst1 = isl_upoly_as_cst(qp1->upoly);
	cst2 = isl_upoly_as_cst(qp2->upoly);
	cmp = isl_upoly_cmp(cst1, cst2);

	if (cmp <= 0) {
		isl_qpolynomial_free(qp2);
	} else {
		isl_qpolynomial_free(qp1);
		qp1 = qp2;
	}
	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

static __isl_give isl_qpolynomial *qpolynomial_max(
	__isl_take isl_qpolynomial *qp1, __isl_take isl_qpolynomial *qp2)
{
	struct isl_upoly_cst *cst1, *cst2;
	int cmp;

	if (!qp1 || !qp2)
		goto error;
	isl_assert(qp1->dim->ctx, isl_upoly_is_cst(qp1->upoly), goto error);
	isl_assert(qp2->dim->ctx, isl_upoly_is_cst(qp2->upoly), goto error);
	cst1 = isl_upoly_as_cst(qp1->upoly);
	cst2 = isl_upoly_as_cst(qp2->upoly);
	cmp = isl_upoly_cmp(cst1, cst2);

	if (cmp >= 0) {
		isl_qpolynomial_free(qp2);
	} else {
		isl_qpolynomial_free(qp1);
		qp1 = qp2;
	}
	return qp1;
error:
	isl_qpolynomial_free(qp1);
	isl_qpolynomial_free(qp2);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_fold_eval(
	__isl_take isl_qpolynomial_fold *fold, __isl_take isl_point *pnt)
{
	isl_qpolynomial *qp;

	if (!fold || !pnt)
		goto error;
	isl_assert(pnt->dim->ctx, isl_dim_equal(pnt->dim, fold->dim), goto error);
	isl_assert(pnt->dim->ctx,
		fold->type == isl_fold_max || fold->type == isl_fold_min,
		goto error);

	if (fold->n == 0)
		qp = isl_qpolynomial_zero(isl_dim_copy(fold->dim));
	else {
		int i;
		qp = isl_qpolynomial_eval(isl_qpolynomial_copy(fold->qp[0]),
						isl_point_copy(pnt));
		for (i = 1; i < fold->n; ++i) {
			isl_qpolynomial *qp_i;
			qp_i = isl_qpolynomial_eval(
					    isl_qpolynomial_copy(fold->qp[i]),
					    isl_point_copy(pnt));
			if (fold->type == isl_fold_max)
				qp = qpolynomial_max(qp, qp_i);
			else
				qp = qpolynomial_min(qp, qp_i);
		}
	}
	isl_qpolynomial_fold_free(fold);
	isl_point_free(pnt);

	return qp;
error:
	isl_qpolynomial_fold_free(fold);
	isl_point_free(pnt);
	return NULL;
}

__isl_give isl_qpolynomial *isl_qpolynomial_move(__isl_take isl_qpolynomial *qp,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	if (!qp)
		return NULL;

	qp->dim = isl_dim_move(qp->dim, dst_type, dst_pos, src_type, src_pos, n);
	if (!qp->dim)
		goto error;
	
	/* Do something to polynomials when needed; later */

	return qp;
error:
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_pw_qpolynomial *isl_pw_qpolynomial_move(
	__isl_take isl_pw_qpolynomial *pwqp,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	int i;

	if (!pwqp)
		return NULL;

	pwqp->dim = isl_dim_move(pwqp->dim,
				    dst_type, dst_pos, src_type, src_pos, n);
	if (!pwqp->dim)
		goto error;

	for (i = 0; i < pwqp->n; ++i) {
		pwqp->p[i].set = isl_set_move(pwqp->p[i].set, dst_type, dst_pos,
						src_type, src_pos, n);
		if (!pwqp->p[i].set)
			goto error;
		pwqp->p[i].qp = isl_qpolynomial_move(pwqp->p[i].qp,
					dst_type, dst_pos, src_type, src_pos, n);
		if (!pwqp->p[i].qp)
			goto error;
	}

	return pwqp;
error:
	isl_pw_qpolynomial_free(pwqp);
	return NULL;
}

__isl_give isl_dim *isl_pw_qpolynomial_get_dim(
	__isl_keep isl_pw_qpolynomial *pwqp)
{
	if (!pwqp)
		return NULL;

	return isl_dim_copy(pwqp->dim);
}

unsigned isl_pw_qpolynomial_dim(__isl_keep isl_pw_qpolynomial *pwqp,
	enum isl_dim_type type)
{
	return pwqp ? isl_dim_size(pwqp->dim, type) : 0;
}

__isl_give isl_term *isl_term_alloc(__isl_take isl_dim *dim,
	__isl_take isl_mat *div)
{
	isl_term *term;
	int n;

	if (!dim || !div)
		goto error;

	n = isl_dim_total(dim) + div->n_row;

	term = isl_calloc(dim->ctx, struct isl_term,
			sizeof(struct isl_term) + (n - 1) * sizeof(int));
	if (!term)
		goto error;

	term->ref = 1;
	term->dim = dim;
	term->div = div;
	isl_int_init(term->n);
	isl_int_init(term->d);
	
	return term;
error:
	isl_dim_free(dim);
	isl_mat_free(div);
	return NULL;
}

__isl_give isl_term *isl_term_copy(__isl_keep isl_term *term)
{
	if (!term)
		return NULL;

	term->ref++;
	return term;
}

__isl_give isl_term *isl_term_dup(__isl_keep isl_term *term)
{
	int i;
	isl_term *dup;
	unsigned total;

	if (term)
		return NULL;

	total = isl_dim_total(term->dim) + term->div->n_row;

	dup = isl_term_alloc(isl_dim_copy(term->dim), isl_mat_copy(term->div));
	if (!dup)
		return NULL;

	isl_int_set(dup->n, term->n);
	isl_int_set(dup->d, term->d);

	for (i = 0; i < total; ++i)
		dup->pow[i] = term->pow[i];

	return dup;
}

__isl_give isl_term *isl_term_cow(__isl_take isl_term *term)
{
	if (!term)
		return NULL;

	if (term->ref == 1)
		return term;
	term->ref--;
	return isl_term_dup(term);
}

void isl_term_free(__isl_take isl_term *term)
{
	if (!term)
		return;

	if (--term->ref > 0)
		return;

	isl_dim_free(term->dim);
	isl_mat_free(term->div);
	isl_int_clear(term->n);
	isl_int_clear(term->d);
	free(term);
}

unsigned isl_term_dim(__isl_keep isl_term *term, enum isl_dim_type type)
{
	if (!term)
		return 0;

	switch (type) {
	case isl_dim_param:
	case isl_dim_in:
	case isl_dim_out:	return isl_dim_size(term->dim, type);
	case isl_dim_div:	return term->div->n_row;
	case isl_dim_all:	return isl_dim_total(term->dim) + term->div->n_row;
	}
}

isl_ctx *isl_term_get_ctx(__isl_keep isl_term *term)
{
	return term ? term->dim->ctx : NULL;
}

void isl_term_get_num(__isl_keep isl_term *term, isl_int *n)
{
	if (!term)
		return;
	isl_int_set(*n, term->n);
}

void isl_term_get_den(__isl_keep isl_term *term, isl_int *d)
{
	if (!term)
		return;
	isl_int_set(*d, term->d);
}

int isl_term_get_exp(__isl_keep isl_term *term,
	enum isl_dim_type type, unsigned pos)
{
	if (!term)
		return -1;

	isl_assert(term->dim->ctx, pos < isl_term_dim(term, type), return -1);

	if (type >= isl_dim_set)
		pos += isl_dim_size(term->dim, isl_dim_param);
	if (type >= isl_dim_div)
		pos += isl_dim_size(term->dim, isl_dim_set);

	return term->pow[pos];
}

__isl_give isl_div *isl_term_get_div(__isl_keep isl_term *term, unsigned pos)
{
	isl_basic_map *bmap;
	unsigned total;
	int k;

	if (!term)
		return NULL;

	isl_assert(term->dim->ctx, pos < isl_term_dim(term, isl_dim_div),
			return NULL);

	total = term->div->n_col - term->div->n_row - 2;
	/* No nested divs for now */
	isl_assert(term->dim->ctx,
		isl_seq_first_non_zero(term->div->row[pos] + 2 + total,
					term->div->n_row) == -1,
		return NULL);

	bmap = isl_basic_map_alloc_dim(isl_dim_copy(term->dim), 1, 0, 0);
	if ((k = isl_basic_map_alloc_div(bmap)) < 0)
		goto error;

	isl_seq_cpy(bmap->div[k], term->div->row[pos], 2 + total);

	return isl_basic_map_div(bmap, k);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

__isl_give isl_term *isl_upoly_foreach_term(__isl_keep struct isl_upoly *up,
	int (*fn)(__isl_take isl_term *term, void *user),
	__isl_take isl_term *term, void *user)
{
	int i;
	struct isl_upoly_rec *rec;

	if (!up || !term)
		goto error;

	if (isl_upoly_is_zero(up))
		return term;

	isl_assert(up->ctx, !isl_upoly_is_nan(up), goto error);
	isl_assert(up->ctx, !isl_upoly_is_infty(up), goto error);
	isl_assert(up->ctx, !isl_upoly_is_neginfty(up), goto error);

	if (isl_upoly_is_cst(up)) {
		struct isl_upoly_cst *cst;
		cst = isl_upoly_as_cst(up);
		if (!cst)
			goto error;
		term = isl_term_cow(term);
		if (!term)
			goto error;
		isl_int_set(term->n, cst->n);
		isl_int_set(term->d, cst->d);
		if (fn(isl_term_copy(term), user) < 0)
			goto error;
		return term;
	}

	rec = isl_upoly_as_rec(up);
	if (!rec)
		goto error;

	for (i = 0; i < rec->n; ++i) {
		term = isl_term_cow(term);
		if (!term)
			goto error;
		term->pow[up->var] = i;
		term = isl_upoly_foreach_term(rec->p[i], fn, term, user);
		if (!term)
			goto error;
	}
	term->pow[up->var] = 0;

	return term;
error:
	isl_term_free(term);
	return NULL;
}

int isl_qpolynomial_foreach_term(__isl_keep isl_qpolynomial *qp,
	int (*fn)(__isl_take isl_term *term, void *user), void *user)
{
	isl_term *term;

	if (!qp)
		return -1;

	term = isl_term_alloc(isl_dim_copy(qp->dim), isl_mat_copy(qp->div));
	if (!term)
		return -1;

	term = isl_upoly_foreach_term(qp->upoly, fn, term, user);

	isl_term_free(term);

	return term ? 0 : -1;
}

int isl_pw_qpolynomial_foreach_piece(__isl_keep isl_pw_qpolynomial *pwqp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user)
{
	int i;

	if (!pwqp)
		return -1;

	for (i = 0; i < pwqp->n; ++i)
		if (fn(isl_set_copy(pwqp->p[i].set),
				isl_qpolynomial_copy(pwqp->p[i].qp), user) < 0)
			return -1;

	return 0;
}

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

__isl_give isl_qpolynomial *isl_qpolynomial_lift(__isl_take isl_qpolynomial *qp,
	__isl_take isl_dim *dim)
{
	if (!qp || !dim)
		goto error;

	if (isl_dim_equal(qp->dim, dim)) {
		isl_dim_free(dim);
		return qp;
	}

	qp = isl_qpolynomial_cow(qp);
	if (!qp)
		goto error;

	if (qp->div->n_row) {
		int i;
		int extra;
		unsigned total;
		int *exp;

		extra = isl_dim_size(dim, isl_dim_set) -
				isl_dim_size(qp->dim, isl_dim_set);
		total = isl_dim_total(qp->dim);
		exp = isl_alloc_array(qp->div->ctx, int, qp->div->n_row);
		if (!exp)
			goto error;
		for (i = 0; i < qp->div->n_row; ++i)
			exp[i] = extra + i;
		qp->upoly = expand(qp->upoly, exp, total);
		free(exp);
		if (!qp->upoly)
			goto error;
		qp->div = isl_mat_insert_cols(qp->div, 2 + total, extra);
		if (!qp->div)
			goto error;
		for (i = 0; i < qp->div->n_row; ++i)
			isl_seq_clr(qp->div->row[i] + 2 + total, extra);
	}

	isl_dim_free(qp->dim);
	qp->dim = dim;

	return qp;
error:
	isl_dim_free(dim);
	isl_qpolynomial_free(qp);
	return NULL;
}

static int foreach_lifted_subset(__isl_take isl_set *set,
	__isl_take isl_qpolynomial *qp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user)
{
	int i;

	if (!set || !qp)
		goto error;

	for (i = 0; i < set->n; ++i) {
		isl_set *lift;
		isl_qpolynomial *copy;

		lift = isl_set_from_basic_set(isl_basic_set_copy(set->p[i]));
		lift = isl_set_lift(lift);

		copy = isl_qpolynomial_copy(qp);
		copy = isl_qpolynomial_lift(copy, isl_set_get_dim(lift));

		if (fn(lift, copy, user) < 0)
			goto error;
	}

	isl_set_free(set);
	isl_qpolynomial_free(qp);

	return 0;
error:
	isl_set_free(set);
	isl_qpolynomial_free(qp);
	return -1;
}

int isl_pw_qpolynomial_foreach_lifted_piece(__isl_keep isl_pw_qpolynomial *pwqp,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_qpolynomial *qp,
		    void *user), void *user)
{
	int i;

	if (!pwqp)
		return -1;

	for (i = 0; i < pwqp->n; ++i) {
		isl_set *set;
		isl_qpolynomial *qp;

		set = isl_set_copy(pwqp->p[i].set);
		qp = isl_qpolynomial_copy(pwqp->p[i].qp);
		if (!any_divs(set)) {
			if (fn(set, qp, user) < 0)
				return -1;
			continue;
		}
		if (foreach_lifted_subset(set, qp, fn, user) < 0)
			return -1;
	}

	return 0;
}

static __isl_give isl_qpolynomial_fold *qpolynomial_fold_alloc(
	enum isl_fold type, __isl_take isl_dim *dim, int n)
{
	isl_qpolynomial_fold *fold;

	if (!dim)
		goto error;

	isl_assert(dim->ctx, n >= 0, goto error);
	fold = isl_alloc(dim->ctx, struct isl_qpolynomial_fold,
			sizeof(struct isl_qpolynomial_fold) +
			(n - 1) * sizeof(struct isl_qpolynomial *));
	if (!fold)
		goto error;

	fold->ref = 1;
	fold->size = n;
	fold->n = 0;
	fold->type = type;
	fold->dim = dim;

	return fold;
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_empty(enum isl_fold type,
	__isl_take isl_dim *dim)
{
	return qpolynomial_fold_alloc(type, dim, 0);
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_alloc(
	enum isl_fold type, __isl_take isl_qpolynomial *qp)
{
	isl_qpolynomial_fold *fold;

	if (!qp)
		return NULL;

	fold = qpolynomial_fold_alloc(type, isl_dim_copy(qp->dim), 1);
	if (!fold)
		goto error;

	fold->qp[0] = qp;
	fold->n++;

	return fold;
error:
	isl_qpolynomial_fold_free(fold);
	isl_qpolynomial_free(qp);
	return NULL;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_copy(
	__isl_keep isl_qpolynomial_fold *fold)
{
	if (!fold)
		return NULL;

	fold->ref++;
	return fold;
}

void isl_qpolynomial_fold_free(__isl_take isl_qpolynomial_fold *fold)
{
	int i;

	if (!fold)
		return;
	if (--fold->ref > 0)
		return;

	for (i = 0; i < fold->n; ++i)
		isl_qpolynomial_free(fold->qp[i]);
	isl_dim_free(fold->dim);
	free(fold);
}

int isl_qpolynomial_fold_is_empty(__isl_keep isl_qpolynomial_fold *fold)
{
	if (!fold)
		return -1;

	return fold->n == 0;
}

__isl_give isl_qpolynomial_fold *isl_qpolynomial_fold_fold(
	__isl_take isl_qpolynomial_fold *fold1,
	__isl_take isl_qpolynomial_fold *fold2)
{
	int i;
	struct isl_qpolynomial_fold *res = NULL;

	if (!fold1 || !fold2)
		goto error;

	isl_assert(fold1->dim->ctx, fold1->type == fold2->type, goto error);
	isl_assert(fold1->dim->ctx, isl_dim_equal(fold1->dim, fold2->dim),
			goto error);

	if (isl_qpolynomial_fold_is_empty(fold1)) {
		isl_qpolynomial_fold_free(fold1);
		return fold2;
	}

	if (isl_qpolynomial_fold_is_empty(fold2)) {
		isl_qpolynomial_fold_free(fold2);
		return fold1;
	}

	res = qpolynomial_fold_alloc(fold1->type, isl_dim_copy(fold1->dim),
					fold1->n + fold2->n);
	if (!res)
		goto error;

	for (i = 0; i < fold1->n; ++i) {
		res->qp[res->n] = isl_qpolynomial_copy(fold1->qp[i]);
		if (!res->qp[res->n])
			goto error;
		res->n++;
	}

	for (i = 0; i < fold2->n; ++i) {
		res->qp[res->n] = isl_qpolynomial_copy(fold2->qp[i]);
		if (!res->qp[res->n])
			goto error;
		res->n++;
	}

	isl_qpolynomial_fold_free(fold1);
	isl_qpolynomial_fold_free(fold2);

	return res;
error:
	isl_qpolynomial_fold_free(res);
	isl_qpolynomial_fold_free(fold1);
	isl_qpolynomial_fold_free(fold2);
	return NULL;
}

__isl_give isl_pw_qpolynomial_fold *isl_pw_qpolynomial_fold_from_pw_qpolynomial(
	enum isl_fold type, __isl_take isl_pw_qpolynomial *pwqp)
{
	int i;
	isl_pw_qpolynomial_fold *pwf;

	if (!pwqp)
		return NULL;
	
	pwf = isl_pw_qpolynomial_fold_alloc_(isl_dim_copy(pwqp->dim), pwqp->n);

	for (i = 0; i < pwqp->n; ++i)
		pwf = isl_pw_qpolynomial_fold_add_piece(pwf,
			isl_set_copy(pwqp->p[i].set),
			isl_qpolynomial_fold_alloc(type,
				isl_qpolynomial_copy(pwqp->p[i].qp)));

	isl_pw_qpolynomial_free(pwqp);

	return pwf;
}

static int up_set_active(__isl_keep struct isl_upoly *up, int *active, int d)
{
	struct isl_upoly_rec *rec;
	int i;

	if (!up)
		return -1;

	if (isl_upoly_is_cst(up))
		return 0;

	if (up->var < d)
		active[up->var] = 1;

	rec = isl_upoly_as_rec(up);
	for (i = 0; i < rec->n; ++i)
		if (up_set_active(rec->p[i], active, d) < 0)
			return -1;

	return 0;
}

static int set_active(__isl_keep isl_qpolynomial *qp, int *active)
{
	int i, j;
	int d = isl_dim_total(qp->dim);

	for (i = 0; i < d; ++i)
		for (j = 0; j < qp->div->n_row; ++j) {
			if (isl_int_is_zero(qp->div->row[j][2 + i]))
				continue;
			active[i] = 1;
			break;
		}

	return up_set_active(qp->upoly, active, d);
}

/* For each parameter or variable that does not appear in qp,
 * first eliminate the variable from all constraints and then set it to zero.
 */
static __isl_give isl_set *fix_inactive(__isl_take isl_set *set,
	__isl_keep isl_qpolynomial *qp)
{
	int *active = NULL;
	int i;
	int d;
	unsigned nparam;
	unsigned nvar;

	if (!set || !qp)
		goto error;

	d = isl_dim_total(set->dim);
	active = isl_calloc_array(set->ctx, int, d);
	if (set_active(qp, active) < 0)
		goto error;

	for (i = 0; i < d; ++i)
		if (!active[i])
			break;

	if (i == d) {
		free(active);
		return set;
	}

	nparam = isl_dim_size(set->dim, isl_dim_param);
	nvar = isl_dim_size(set->dim, isl_dim_set);
	for (i = 0; i < nparam; ++i) {
		if (active[i])
			continue;
		set = isl_set_eliminate(set, isl_dim_param, i, 1);
		set = isl_set_fix_si(set, isl_dim_param, i, 0);
	}
	for (i = 0; i < nvar; ++i) {
		if (active[i])
			continue;
		set = isl_set_eliminate(set, isl_dim_set, i, 1);
		set = isl_set_fix_si(set, isl_dim_set, i, 0);
	}

	free(active);

	return set;
error:
	free(active);
	isl_set_free(set);
	return NULL;
}

struct isl_max_data {
	isl_qpolynomial *qp;
	int first;
	isl_qpolynomial *max;
};

static int max_fn(__isl_take isl_point *pnt, void *user)
{
	struct isl_max_data *data = (struct isl_max_data *)user;
	isl_qpolynomial *val;

	val = isl_qpolynomial_eval(isl_qpolynomial_copy(data->qp), pnt);
	if (data->first) {
		data->first = 0;
		data->max = val;
	} else {
		data->max = qpolynomial_max(data->max, val);
	}

	return 0;
}

static __isl_give isl_qpolynomial *guarded_qpolynomial_max(
	__isl_take isl_set *set, __isl_take isl_qpolynomial *qp)
{
	struct isl_max_data data = { NULL, 1, NULL };

	if (!set || !qp)
		goto error;

	if (isl_upoly_is_cst(qp->upoly)) {
		isl_set_free(set);
		return qp;
	}

	set = fix_inactive(set, qp);

	data.qp = qp;
	if (isl_set_foreach_point(set, max_fn, &data) < 0)
		goto error;

	isl_set_free(set);
	isl_qpolynomial_free(qp);
	return data.max;
error:
	isl_set_free(set);
	isl_qpolynomial_free(qp);
	isl_qpolynomial_free(data.max);
	return NULL;
}

/* Compute the maximal value attained by the piecewise quasipolynomial
 * on its domain or zero if the domain is empty.
 * In the worst case, the domain is scanned completely,
 * so the domain is assumed to be bounded.
 */
__isl_give isl_qpolynomial *isl_pw_qpolynomial_max(
	__isl_take isl_pw_qpolynomial *pwqp)
{
	int i;
	isl_qpolynomial *max;

	if (!pwqp)
		return NULL;

	if (pwqp->n == 0) {
		isl_dim *dim = isl_dim_copy(pwqp->dim);
		isl_pw_qpolynomial_free(pwqp);
		return isl_qpolynomial_zero(dim);
	}

	max = guarded_qpolynomial_max(isl_set_copy(pwqp->p[0].set),
					isl_qpolynomial_copy(pwqp->p[0].qp));
	for (i = 1; i < pwqp->n; ++i) {
		isl_qpolynomial *max_i;
		max_i = guarded_qpolynomial_max(isl_set_copy(pwqp->p[i].set),
					    isl_qpolynomial_copy(pwqp->p[i].qp));
		max = qpolynomial_max(max, max_i);
	}

	isl_pw_qpolynomial_free(pwqp);
	return max;
}
