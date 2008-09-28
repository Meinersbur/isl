#include <string.h>
#include <strings.h>
#include "isl_ctx.h"
#include "isl_blk.h"
#include "isl_equalities.h"
#include "isl_list.h"
#include "isl_lp.h"
#include "isl_seq.h"
#include "isl_set.h"
#include "isl_map.h"
#include "isl_map_private.h"
#include "isl_map_piplib.h"
#include "isl_sample.h"
#include "isl_vec.h"

static struct isl_basic_map *basic_map_init(struct isl_ctx *ctx,
		struct isl_basic_map *bmap,
		unsigned nparam, unsigned n_in, unsigned n_out, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	int i;
	size_t row_size = 1 + nparam + n_in + n_out + extra;

	bmap->block = isl_blk_alloc(ctx, (n_eq + n_ineq) * row_size);
	if (isl_blk_is_error(bmap->block)) {
		free(bmap);
		return NULL;
	}

	bmap->eq = isl_alloc_array(ctx, isl_int *, n_eq + n_ineq);
	if (!bmap->eq) {
		isl_blk_free(ctx, bmap->block);
		free(bmap);
		return NULL;
	}

	if (extra == 0) {
		bmap->block2 = isl_blk_empty();
		bmap->div = NULL;
	} else {
		bmap->block2 = isl_blk_alloc(ctx, extra * (1 + row_size));
		if (isl_blk_is_error(bmap->block2)) {
			free(bmap->eq);
			isl_blk_free(ctx, bmap->block);
			free(bmap);
			return NULL;
		}

		bmap->div = isl_alloc_array(ctx, isl_int *, extra);
		if (!bmap->div) {
			isl_blk_free(ctx, bmap->block2);
			free(bmap->eq);
			isl_blk_free(ctx, bmap->block);
			free(bmap);
			return NULL;
		}
	}

	for (i = 0; i < n_eq + n_ineq; ++i)
		bmap->eq[i] = bmap->block.data + i * row_size;

	for (i = 0; i < extra; ++i)
		bmap->div[i] = bmap->block2.data + i * (1 + row_size);

	bmap->ctx = ctx;
	isl_ctx_ref(ctx);
	bmap->ref = 1;
	bmap->flags = 0;
	bmap->c_size = n_eq + n_ineq;
	bmap->ineq = bmap->eq + n_eq;
	bmap->nparam = nparam;
	bmap->n_in = n_in;
	bmap->n_out = n_out;
	bmap->extra = extra;
	bmap->n_eq = 0;
	bmap->n_ineq = 0;
	bmap->n_div = 0;
	bmap->sample = NULL;

	return bmap;
}

struct isl_basic_set *isl_basic_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_alloc(ctx, nparam, 0, dim, extra, n_eq, n_ineq);
	return (struct isl_basic_set *)bmap;
}

struct isl_basic_map *isl_basic_map_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	struct isl_basic_map *bmap;

	bmap = isl_alloc_type(ctx, struct isl_basic_map);
	if (!bmap)
		return NULL;

	return basic_map_init(ctx, bmap,
			       nparam, in, out, extra, n_eq, n_ineq);
}

static void dup_constraints(
		struct isl_basic_map *dst, struct isl_basic_map *src)
{
	int i;
	unsigned total = src->nparam + src->n_in + src->n_out + src->n_div;

	for (i = 0; i < src->n_eq; ++i) {
		int j = isl_basic_map_alloc_equality(dst);
		isl_seq_cpy(dst->eq[j], src->eq[i], 1+total);
	}

	for (i = 0; i < src->n_ineq; ++i) {
		int j = isl_basic_map_alloc_inequality(dst);
		isl_seq_cpy(dst->ineq[j], src->ineq[i], 1+total);
	}

	for (i = 0; i < src->n_div; ++i) {
		int j = isl_basic_map_alloc_div(dst);
		isl_seq_cpy(dst->div[j], src->div[i], 1+1+total);
	}
	F_SET(dst, ISL_BASIC_SET_FINAL);
}

struct isl_basic_map *isl_basic_map_dup(struct isl_basic_map *bmap)
{
	struct isl_basic_map *dup;

	if (!bmap)
		return NULL;
	dup = isl_basic_map_alloc(bmap->ctx, bmap->nparam,
			bmap->n_in, bmap->n_out,
			bmap->n_div, bmap->n_eq, bmap->n_ineq);
	if (!dup)
		return NULL;
	dup->flags = bmap->flags;
	dup_constraints(dup, bmap);
	dup->sample = isl_vec_copy(bmap->ctx, bmap->sample);
	return dup;
}

struct isl_basic_set *isl_basic_set_dup(struct isl_basic_set *bset)
{
	struct isl_basic_map *dup;

	dup = isl_basic_map_dup((struct isl_basic_map *)bset);
	return (struct isl_basic_set *)dup;
}

struct isl_basic_set *isl_basic_set_copy(struct isl_basic_set *bset)
{
	if (!bset)
		return NULL;

	if (F_ISSET(bset, ISL_BASIC_SET_FINAL)) {
		bset->ref++;
		return bset;
	}
	return isl_basic_set_dup(bset);
}

struct isl_set *isl_set_copy(struct isl_set *set)
{
	if (!set)
		return NULL;

	set->ref++;
	return set;
}

struct isl_basic_map *isl_basic_map_copy(struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;

	if (F_ISSET(bmap, ISL_BASIC_SET_FINAL)) {
		bmap->ref++;
		return bmap;
	}
	return isl_basic_map_dup(bmap);
}

struct isl_map *isl_map_copy(struct isl_map *map)
{
	if (!map)
		return NULL;

	map->ref++;
	return map;
}

void isl_basic_map_free(struct isl_basic_map *bmap)
{
	if (!bmap)
		return;

	if (--bmap->ref > 0)
		return;

	isl_ctx_deref(bmap->ctx);
	free(bmap->div);
	isl_blk_free(bmap->ctx, bmap->block2);
	free(bmap->eq);
	isl_blk_free(bmap->ctx, bmap->block);
	isl_vec_free(bmap->ctx, bmap->sample);
	free(bmap);
}

void isl_basic_set_free(struct isl_basic_set *bset)
{
	isl_basic_map_free((struct isl_basic_map *)bset);
}

int isl_basic_map_alloc_equality(struct isl_basic_map *bmap)
{
	struct isl_ctx *ctx;
	if (!bmap)
		return -1;
	ctx = bmap->ctx;
	isl_assert(ctx, bmap->n_eq + bmap->n_ineq < bmap->c_size, return -1);
	isl_assert(ctx, bmap->eq + bmap->n_eq <= bmap->ineq, return -1);
	if (bmap->eq + bmap->n_eq == bmap->ineq) {
		isl_int *t;
		int j = isl_basic_map_alloc_inequality(bmap);
		if (j < 0)
			return -1;
		t = bmap->ineq[0];
		bmap->ineq[0] = bmap->ineq[j];
		bmap->ineq[j] = t;
		bmap->n_ineq--;
		bmap->ineq++;
	}
	return bmap->n_eq++;
}

int isl_basic_set_alloc_equality(struct isl_basic_set *bset)
{
	return isl_basic_map_alloc_equality((struct isl_basic_map *)bset);
}

int isl_basic_map_free_equality(struct isl_basic_map *bmap, unsigned n)
{
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, n <= bmap->n_eq, return -1);
	bmap->n_eq -= n;
	return 0;
}

int isl_basic_map_drop_equality(struct isl_basic_map *bmap, unsigned pos)
{
	isl_int *t;
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, pos < bmap->n_eq, return -1);

	if (pos != bmap->n_eq - 1) {
		t = bmap->eq[pos];
		bmap->eq[pos] = bmap->eq[bmap->n_eq - 1];
		bmap->eq[bmap->n_eq - 1] = t;
	}
	bmap->n_eq--;
	return 0;
}

int isl_basic_set_drop_equality(struct isl_basic_set *bset, unsigned pos)
{
	return isl_basic_map_drop_equality((struct isl_basic_map *)bset, pos);
}

void isl_basic_map_inequality_to_equality(
		struct isl_basic_map *bmap, unsigned pos)
{
	isl_int *t;

	t = bmap->ineq[pos];
	bmap->ineq[pos] = bmap->ineq[0];
	bmap->ineq[0] = bmap->eq[bmap->n_eq];
	bmap->eq[bmap->n_eq] = t;
	bmap->n_eq++;
	bmap->n_ineq--;
	bmap->ineq++;
	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
}

int isl_basic_map_alloc_inequality(struct isl_basic_map *bmap)
{
	struct isl_ctx *ctx;
	if (!bmap)
		return -1;
	ctx = bmap->ctx;
	isl_assert(ctx, (bmap->ineq - bmap->eq) + bmap->n_ineq < bmap->c_size,
			return -1);
	F_CLR(bmap, ISL_BASIC_MAP_NO_IMPLICIT);
	F_CLR(bmap, ISL_BASIC_MAP_NO_REDUNDANT);
	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	return bmap->n_ineq++;
}

int isl_basic_set_alloc_inequality(struct isl_basic_set *bset)
{
	return isl_basic_map_alloc_inequality((struct isl_basic_map *)bset);
}

int isl_basic_map_free_inequality(struct isl_basic_map *bmap, unsigned n)
{
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, n <= bmap->n_ineq, return -1);
	bmap->n_ineq -= n;
	return 0;
}

int isl_basic_set_free_inequality(struct isl_basic_set *bset, unsigned n)
{
	return isl_basic_map_free_inequality((struct isl_basic_map *)bset, n);
}

int isl_basic_map_drop_inequality(struct isl_basic_map *bmap, unsigned pos)
{
	isl_int *t;
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, pos < bmap->n_ineq, return -1);

	if (pos != bmap->n_ineq - 1) {
		t = bmap->ineq[pos];
		bmap->ineq[pos] = bmap->ineq[bmap->n_ineq - 1];
		bmap->ineq[bmap->n_ineq - 1] = t;
		F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	}
	bmap->n_ineq--;
	return 0;
}

int isl_basic_set_drop_inequality(struct isl_basic_set *bset, unsigned pos)
{
	return isl_basic_map_drop_inequality((struct isl_basic_map *)bset, pos);
}

int isl_basic_map_alloc_div(struct isl_basic_map *bmap)
{
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, bmap->n_div < bmap->extra, return -1);
	return bmap->n_div++;
}

int isl_basic_map_free_div(struct isl_basic_map *bmap, unsigned n)
{
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, n <= bmap->n_div, return -1);
	bmap->n_div -= n;
	return 0;
}

/* Copy constraint from src to dst, putting the vars of src at offset
 * dim_off in dst and the divs of src at offset div_off in dst.
 * If both sets are actually map, then dim_off applies to the input
 * variables.
 */
static void copy_constraint(struct isl_basic_map *dst_map, isl_int *dst,
			    struct isl_basic_map *src_map, isl_int *src,
			    unsigned in_off, unsigned out_off, unsigned div_off)
{
	isl_int_set(dst[0], src[0]);
	isl_seq_cpy(dst+1, src+1, isl_min(dst_map->nparam, src_map->nparam));
	if (dst_map->nparam > src_map->nparam)
		isl_seq_clr(dst+1+src_map->nparam,
				dst_map->nparam - src_map->nparam);
	isl_seq_clr(dst+1+dst_map->nparam, in_off);
	isl_seq_cpy(dst+1+dst_map->nparam+in_off,
		    src+1+src_map->nparam,
		    isl_min(dst_map->n_in-in_off, src_map->n_in));
	if (dst_map->n_in-in_off > src_map->n_in)
		isl_seq_clr(dst+1+dst_map->nparam+in_off+src_map->n_in,
				dst_map->n_in - in_off - src_map->n_in);
	isl_seq_clr(dst+1+dst_map->nparam+dst_map->n_in, out_off);
	isl_seq_cpy(dst+1+dst_map->nparam+dst_map->n_in+out_off,
		    src+1+src_map->nparam+src_map->n_in,
		    isl_min(dst_map->n_out-out_off, src_map->n_out));
	if (dst_map->n_out-out_off > src_map->n_out)
		isl_seq_clr(dst+1+dst_map->nparam+dst_map->n_in+out_off+src_map->n_out,
				dst_map->n_out - out_off - src_map->n_out);
	isl_seq_clr(dst+1+dst_map->nparam+dst_map->n_in+dst_map->n_out, div_off);
	isl_seq_cpy(dst+1+dst_map->nparam+dst_map->n_in+dst_map->n_out+div_off,
		    src+1+src_map->nparam+src_map->n_in+src_map->n_out,
		    isl_min(dst_map->extra-div_off, src_map->extra));
	if (dst_map->extra-div_off > src_map->extra)
		isl_seq_clr(dst+1+dst_map->nparam+dst_map->n_in+dst_map->n_out+
				div_off+src_map->extra,
				dst_map->extra - div_off - src_map->extra);
}

static void copy_div(struct isl_basic_map *dst_map, isl_int *dst,
		     struct isl_basic_map *src_map, isl_int *src,
		     unsigned in_off, unsigned out_off, unsigned div_off)
{
	isl_int_set(dst[0], src[0]);
	copy_constraint(dst_map, dst+1, src_map, src+1, in_off, out_off, div_off);
}

static struct isl_basic_map *add_constraints(struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2, unsigned i_pos, unsigned o_pos)
{
	int i;
	unsigned div_off;

	if (!bmap1 || !bmap2)
		goto error;

	div_off = bmap1->n_div;

	for (i = 0; i < bmap2->n_eq; ++i) {
		int i1 = isl_basic_map_alloc_equality(bmap1);
		if (i1 < 0)
			goto error;
		copy_constraint(bmap1, bmap1->eq[i1], bmap2, bmap2->eq[i],
				i_pos, o_pos, div_off);
	}

	for (i = 0; i < bmap2->n_ineq; ++i) {
		int i1 = isl_basic_map_alloc_inequality(bmap1);
		if (i1 < 0)
			goto error;
		copy_constraint(bmap1, bmap1->ineq[i1], bmap2, bmap2->ineq[i],
				i_pos, o_pos, div_off);
	}

	for (i = 0; i < bmap2->n_div; ++i) {
		int i1 = isl_basic_map_alloc_div(bmap1);
		if (i1 < 0)
			goto error;
		copy_div(bmap1, bmap1->div[i1], bmap2, bmap2->div[i],
			 i_pos, o_pos, div_off);
	}

	isl_basic_map_free(bmap2);

	return bmap1;

error:
	isl_basic_map_free(bmap1);
	isl_basic_map_free(bmap2);
	return NULL;
}

static struct isl_basic_set *set_add_constraints(struct isl_basic_set *bset1,
		struct isl_basic_set *bset2, unsigned pos)
{
	return (struct isl_basic_set *)
		add_constraints((struct isl_basic_map *)bset1,
				(struct isl_basic_map *)bset2, 0, pos);
}

struct isl_basic_map *isl_basic_map_extend(struct isl_basic_map *base,
		unsigned nparam, unsigned n_in, unsigned n_out, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	struct isl_basic_map *ext;
	unsigned flags;
	int dims_ok;

	base = isl_basic_map_cow(base);
	if (!base)
		goto error;

	dims_ok = base && base->nparam == nparam &&
		  base->n_in == n_in && base->n_out == n_out &&
		  base->extra >= base->n_div + extra;

	if (dims_ok && n_eq == 0 && n_ineq == 0)
		return base;

	isl_assert(base->ctx, base->nparam <= nparam, goto error);
	isl_assert(base->ctx, base->n_in <= n_in, goto error);
	isl_assert(base->ctx, base->n_out <= n_out, goto error);
	extra += base->extra;
	n_eq += base->n_eq;
	n_ineq += base->n_ineq;

	ext = isl_basic_map_alloc(base->ctx, nparam, n_in, n_out,
					extra, n_eq, n_ineq);
	if (!base)
		return ext;
	if (!ext)
		goto error;

	flags = base->flags;
	ext = add_constraints(ext, base, 0, 0);
	if (ext) {
		ext->flags = flags;
		F_CLR(ext, ISL_BASIC_SET_FINAL);
	}

	return ext;

error:
	isl_basic_map_free(base);
	return NULL;
}

struct isl_basic_set *isl_basic_set_extend(struct isl_basic_set *base,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	return (struct isl_basic_set *)
		isl_basic_map_extend((struct isl_basic_map *)base,
					nparam, 0, dim, extra, n_eq, n_ineq);
}

struct isl_basic_set *isl_basic_set_cow(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_cow((struct isl_basic_map *)bset);
}

struct isl_basic_map *isl_basic_map_cow(struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;

	if (bmap->ref > 1) {
		bmap->ref--;
		bmap = isl_basic_map_dup(bmap);
	}
	F_CLR(bmap, ISL_BASIC_SET_FINAL);
	return bmap;
}

struct isl_set *isl_set_cow(struct isl_set *set)
{
	if (!set)
		return NULL;

	if (set->ref == 1)
		return set;
	set->ref--;
	return isl_set_dup(set);
}

struct isl_map *isl_map_cow(struct isl_map *map)
{
	if (!map)
		return NULL;

	if (map->ref == 1)
		return map;
	map->ref--;
	return isl_map_dup(map);
}

static void swap_vars(struct isl_blk blk, isl_int *a,
			unsigned a_len, unsigned b_len)
{
	isl_seq_cpy(blk.data, a+a_len, b_len);
	isl_seq_cpy(blk.data+b_len, a, a_len);
	isl_seq_cpy(a, blk.data, b_len+a_len);
}

struct isl_basic_set *isl_basic_set_swap_vars(
		struct isl_basic_set *bset, unsigned n)
{
	int i;
	struct isl_blk blk;

	if (!bset)
		goto error;

	isl_assert(bset->ctx, n <= bset->dim, goto error);

	if (n == bset->dim)
		return bset;

	bset = isl_basic_set_cow(bset);
	if (!bset)
		return NULL;

	blk = isl_blk_alloc(bset->ctx, bset->dim);
	if (isl_blk_is_error(blk))
		goto error;

	for (i = 0; i < bset->n_eq; ++i)
		swap_vars(blk,
			  bset->eq[i]+1+bset->nparam, n, bset->dim - n);

	for (i = 0; i < bset->n_ineq; ++i)
		swap_vars(blk,
			  bset->ineq[i]+1+bset->nparam, n, bset->dim - n);

	for (i = 0; i < bset->n_div; ++i)
		swap_vars(blk,
			  bset->div[i]+1+1+bset->nparam, n, bset->dim - n);

	isl_blk_free(bset->ctx, blk);

	F_CLR(bset, ISL_BASIC_SET_NORMALIZED);
	return bset;

error:
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_set *isl_set_swap_vars(struct isl_set *set, unsigned n)
{
	int i;
	set = isl_set_cow(set);
	if (!set)
		return NULL;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_swap_vars(set->p[i], n);
		if (!set->p[i]) {
			isl_set_free(set);
			return NULL;
		}
	}
	F_CLR(set, ISL_SET_NORMALIZED);
	return set;
}

static void constraint_drop_vars(isl_int *c, unsigned n, unsigned rem)
{
	isl_seq_cpy(c, c + n, rem);
	isl_seq_clr(c + rem, n);
}

struct isl_basic_set *isl_basic_set_drop_dims(
		struct isl_basic_set *bset, unsigned first, unsigned n)
{
	int i;

	if (!bset)
		goto error;

	isl_assert(bset->ctx, first + n <= bset->dim, goto error);

	if (n == 0)
		return bset;

	bset = isl_basic_set_cow(bset);
	if (!bset)
		return NULL;

	for (i = 0; i < bset->n_eq; ++i)
		constraint_drop_vars(bset->eq[i]+1+bset->nparam+first, n,
				     (bset->dim-first-n)+bset->extra);

	for (i = 0; i < bset->n_ineq; ++i)
		constraint_drop_vars(bset->ineq[i]+1+bset->nparam+first, n,
				     (bset->dim-first-n)+bset->extra);

	for (i = 0; i < bset->n_div; ++i)
		constraint_drop_vars(bset->div[i]+1+1+bset->nparam+first, n,
				     (bset->dim-first-n)+bset->extra);

	bset->dim -= n;
	bset->extra += n;

	F_CLR(bset, ISL_BASIC_SET_NORMALIZED);
	bset = isl_basic_set_simplify(bset);
	return isl_basic_set_finalize(bset);
error:
	isl_basic_set_free(bset);
	return NULL;
}

static struct isl_set *isl_set_drop_dims(
		struct isl_set *set, unsigned first, unsigned n)
{
	int i;

	if (!set)
		goto error;

	isl_assert(set->ctx, first + n <= set->dim, goto error);

	if (n == 0)
		return set;
	set = isl_set_cow(set);
	if (!set)
		goto error;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_drop_dims(set->p[i], first, n);
		if (!set->p[i])
			goto error;
	}
	set->dim -= n;

	F_CLR(set, ISL_SET_NORMALIZED);
	return set;
error:
	isl_set_free(set);
	return NULL;
}

struct isl_basic_map *isl_basic_map_drop_inputs(
		struct isl_basic_map *bmap, unsigned first, unsigned n)
{
	int i;

	if (!bmap)
		goto error;

	isl_assert(bmap->ctx, first + n <= bmap->n_in, goto error);

	if (n == 0)
		return bmap;

	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		return NULL;

	for (i = 0; i < bmap->n_eq; ++i)
		constraint_drop_vars(bmap->eq[i]+1+bmap->nparam+first, n,
				 (bmap->n_in-first-n)+bmap->n_out+bmap->extra);

	for (i = 0; i < bmap->n_ineq; ++i)
		constraint_drop_vars(bmap->ineq[i]+1+bmap->nparam+first, n,
				 (bmap->n_in-first-n)+bmap->n_out+bmap->extra);

	for (i = 0; i < bmap->n_div; ++i)
		constraint_drop_vars(bmap->div[i]+1+1+bmap->nparam+first, n,
				 (bmap->n_in-first-n)+bmap->n_out+bmap->extra);

	bmap->n_in -= n;
	bmap->extra += n;

	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	bmap = isl_basic_map_simplify(bmap);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_map *isl_map_drop_inputs(
		struct isl_map *map, unsigned first, unsigned n)
{
	int i;

	if (!map)
		goto error;

	isl_assert(map->ctx, first + n <= map->n_in, goto error);

	if (n == 0)
		return map;
	map = isl_map_cow(map);
	if (!map)
		goto error;

	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_drop_inputs(map->p[i], first, n);
		if (!map->p[i])
			goto error;
	}
	map->n_in -= n;
	F_CLR(map, ISL_MAP_NORMALIZED);

	return map;
error:
	isl_map_free(map);
	return NULL;
}

/*
 * We don't cow, as the div is assumed to be redundant.
 */
static struct isl_basic_map *isl_basic_map_drop_div(
		struct isl_basic_map *bmap, unsigned div)
{
	int i;
	unsigned pos;

	if (!bmap)
		goto error;

	pos = 1 + bmap->nparam + bmap->n_in + bmap->n_out + div;

	isl_assert(bmap->ctx, div < bmap->n_div, goto error);

	for (i = 0; i < bmap->n_eq; ++i)
		constraint_drop_vars(bmap->eq[i]+pos, 1, bmap->extra-div-1);

	for (i = 0; i < bmap->n_ineq; ++i) {
		if (!isl_int_is_zero(bmap->ineq[i][pos])) {
			isl_basic_map_drop_inequality(bmap, i);
			--i;
			continue;
		}
		constraint_drop_vars(bmap->ineq[i]+pos, 1, bmap->extra-div-1);
	}

	for (i = 0; i < bmap->n_div; ++i)
		constraint_drop_vars(bmap->div[i]+1+pos, 1, bmap->extra-div-1);

	if (div != bmap->n_div - 1) {
		int j;
		isl_int *t = bmap->div[div];

		for (j = div; j < bmap->n_div - 1; ++j)
			bmap->div[j] = bmap->div[j+1];

		bmap->div[bmap->n_div - 1] = t;
	}
	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	isl_basic_map_free_div(bmap, 1);

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static void eliminate_div(struct isl_basic_map *bmap, isl_int *eq, unsigned div)
{
	int i;
	unsigned pos = 1 + bmap->nparam + bmap->n_in + bmap->n_out + div;
	unsigned len;
	len = 1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;

	for (i = 0; i < bmap->n_eq; ++i)
		if (bmap->eq[i] != eq)
			isl_seq_elim(bmap->eq[i], eq, pos, len, NULL);

	for (i = 0; i < bmap->n_ineq; ++i)
		isl_seq_elim(bmap->ineq[i], eq, pos, len, NULL);

	/* We need to be careful about circular definitions,
	 * so for now we just remove the definitions of other divs that
	 * depend on this div and (possibly) recompute them later.
	 */
	for (i = 0; i < bmap->n_div; ++i)
		if (!isl_int_is_zero(bmap->div[i][0]) &&
		    !isl_int_is_zero(bmap->div[i][1 + pos]))
			isl_seq_clr(bmap->div[i], 1 + len);

	isl_basic_map_drop_div(bmap, div);
}

struct isl_basic_map *isl_basic_map_set_to_empty(struct isl_basic_map *bmap)
{
	int i = 0;
	unsigned total;
	if (!bmap)
		goto error;
	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra;
	isl_basic_map_free_div(bmap, bmap->n_div);
	isl_basic_map_free_inequality(bmap, bmap->n_ineq);
	if (bmap->n_eq > 0)
		isl_basic_map_free_equality(bmap, bmap->n_eq-1);
	else {
		isl_basic_map_alloc_equality(bmap);
		if (i < 0)
			goto error;
	}
	isl_int_set_si(bmap->eq[i][0], 1);
	isl_seq_clr(bmap->eq[i]+1, total);
	F_SET(bmap, ISL_BASIC_MAP_EMPTY);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_set_to_empty(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_set_to_empty((struct isl_basic_map *)bset);
}

static void swap_equality(struct isl_basic_map *bmap, int a, int b)
{
	isl_int *t = bmap->eq[a];
	bmap->eq[a] = bmap->eq[b];
	bmap->eq[b] = t;
}

static void swap_inequality(struct isl_basic_map *bmap, int a, int b)
{
	if (a != b) {
		isl_int *t = bmap->ineq[a];
		bmap->ineq[a] = bmap->ineq[b];
		bmap->ineq[b] = t;
	}
}

static void set_swap_inequality(struct isl_basic_set *bset, int a, int b)
{
	swap_inequality((struct isl_basic_map *)bset, a, b);
}

static void swap_div(struct isl_basic_map *bmap, int a, int b)
{
	int i;
	unsigned off = bmap->nparam + bmap->n_in + bmap->n_out;
	isl_int *t = bmap->div[a];
	bmap->div[a] = bmap->div[b];
	bmap->div[b] = t;

	for (i = 0; i < bmap->n_eq; ++i)
		isl_int_swap(bmap->eq[i][1+off+a], bmap->eq[i][1+off+b]);

	for (i = 0; i < bmap->n_ineq; ++i)
		isl_int_swap(bmap->ineq[i][1+off+a], bmap->ineq[i][1+off+b]);

	for (i = 0; i < bmap->n_div; ++i)
		isl_int_swap(bmap->div[i][1+1+off+a], bmap->div[i][1+1+off+b]);
	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
}

static void eliminate_var_using_equality(struct isl_basic_map *bmap,
	unsigned pos, isl_int *eq, int *progress)
{
	unsigned total;
	int k;

	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	for (k = 0; k < bmap->n_eq; ++k) {
		if (bmap->eq[k] == eq)
			continue;
		if (isl_int_is_zero(bmap->eq[k][1+pos]))
			continue;
		if (progress)
			*progress = 1;
		isl_seq_elim(bmap->eq[k], eq, 1+pos, 1+total, NULL);
	}

	for (k = 0; k < bmap->n_ineq; ++k) {
		if (isl_int_is_zero(bmap->ineq[k][1+pos]))
			continue;
		if (progress)
			*progress = 1;
		isl_seq_elim(bmap->ineq[k], eq, 1+pos, 1+total, NULL);
		F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	}

	for (k = 0; k < bmap->n_div; ++k) {
		if (isl_int_is_zero(bmap->div[k][0]))
			continue;
		if (isl_int_is_zero(bmap->div[k][1+1+pos]))
			continue;
		if (progress)
			*progress = 1;
		isl_seq_elim(bmap->div[k]+1, eq,
				1+pos, 1+total, &bmap->div[k][0]);
		F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	}
}
	

struct isl_basic_map *isl_basic_map_gauss(
	struct isl_basic_map *bmap, int *progress)
{
	int k;
	int done;
	int last_var;
	unsigned total_var;
	unsigned total;

	if (!bmap)
		return NULL;

	total_var = bmap->nparam + bmap->n_in + bmap->n_out;
	total = total_var + bmap->n_div;

	last_var = total - 1;
	for (done = 0; done < bmap->n_eq; ++done) {
		for (; last_var >= 0; --last_var) {
			for (k = done; k < bmap->n_eq; ++k)
				if (!isl_int_is_zero(bmap->eq[k][1+last_var]))
					break;
			if (k < bmap->n_eq)
				break;
		}
		if (last_var < 0)
			break;
		if (k != done)
			swap_equality(bmap, k, done);
		if (isl_int_is_neg(bmap->eq[done][1+last_var]))
			isl_seq_neg(bmap->eq[done], bmap->eq[done], 1+total);

		eliminate_var_using_equality(bmap, last_var, bmap->eq[done],
						progress);

		if (last_var >= total_var &&
		    isl_int_is_zero(bmap->div[last_var - total_var][0])) {
			unsigned div = last_var - total_var;
			isl_seq_neg(bmap->div[div]+1, bmap->eq[done], 1+total);
			isl_int_set_si(bmap->div[div][1+1+last_var], 0);
			isl_int_set(bmap->div[div][0],
				    bmap->eq[done][1+last_var]);
			F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
		}
	}
	if (done == bmap->n_eq)
		return bmap;
	for (k = done; k < bmap->n_eq; ++k) {
		if (isl_int_is_zero(bmap->eq[k][0]))
			continue;
		return isl_basic_map_set_to_empty(bmap);
	}
	isl_basic_map_free_equality(bmap, bmap->n_eq-done);
	return bmap;
}

struct isl_basic_set *isl_basic_set_gauss(
	struct isl_basic_set *bset, int *progress)
{
	return (struct isl_basic_set*)isl_basic_map_gauss(
			(struct isl_basic_map *)bset, progress);
}

static unsigned int round_up(unsigned int v)
{
	int old_v = v;

	while (v) {
		old_v = v;
		v ^= v & -v;
	}
	return old_v << 1;
}

static int hash_index(int *index, unsigned int size, int bits,
			struct isl_basic_map *bmap, int k)
{
	int h;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	u_int32_t hash = isl_seq_hash(bmap->ineq[k]+1, total, bits);
	for (h = hash; index[h]; h = (h+1) % size)
		if (k != index[h]-1 &&
		    isl_seq_eq(bmap->ineq[k]+1,
			       bmap->ineq[index[h]-1]+1, total))
			break;
	return h;
}

static struct isl_basic_map *remove_duplicate_constraints(
	struct isl_basic_map *bmap, int *progress)
{
	unsigned int size;
	int *index;
	int k, l, h;
	int bits;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	isl_int sum;

	if (bmap->n_ineq <= 1)
		return bmap;

	size = round_up(4 * (bmap->n_ineq+1) / 3 - 1);
	bits = ffs(size) - 1;
	index = isl_alloc_array(ctx, int, size);
	memset(index, 0, size * sizeof(int));
	if (!index)
		return bmap;

	index[isl_seq_hash(bmap->ineq[0]+1, total, bits)] = 1;
	for (k = 1; k < bmap->n_ineq; ++k) {
		h = hash_index(index, size, bits, bmap, k);
		if (!index[h]) {
			index[h] = k+1;
			continue;
		}
		if (progress)
			*progress = 1;
		l = index[h] - 1;
		if (isl_int_lt(bmap->ineq[k][0], bmap->ineq[l][0]))
			swap_inequality(bmap, k, l);
		isl_basic_map_drop_inequality(bmap, k);
		--k;
	}
	isl_int_init(sum);
	for (k = 0; k < bmap->n_ineq-1; ++k) {
		isl_seq_neg(bmap->ineq[k]+1, bmap->ineq[k]+1, total);
		h = hash_index(index, size, bits, bmap, k);
		isl_seq_neg(bmap->ineq[k]+1, bmap->ineq[k]+1, total);
		if (!index[h])
			continue;
		l = index[h] - 1;
		isl_int_add(sum, bmap->ineq[k][0], bmap->ineq[l][0]);
		if (isl_int_is_pos(sum))
			continue;
		if (isl_int_is_zero(sum)) {
			/* We need to break out of the loop after these
			 * changes since the contents of the hash
			 * will no longer be valid.
			 * Plus, we probably we want to regauss first.
			 */
			isl_basic_map_drop_inequality(bmap, l);
			isl_basic_map_inequality_to_equality(bmap, k);
		} else
			bmap = isl_basic_map_set_to_empty(bmap);
		break;
	}
	isl_int_clear(sum);

	free(index);
	return bmap;
}

static void compute_elimination_index(struct isl_basic_map *bmap, int *elim)
{
	int d, i;
	unsigned total;

	total = bmap->nparam + bmap->n_in + bmap->n_out;
	for (d = 0; d < total; ++d)
		elim[d] = -1;
	for (d = total - 1, i = 0; d >= 0 && i < bmap->n_eq; ++i) {
		for (; d >= 0; --d) {
			if (isl_int_is_zero(bmap->eq[i][1+d]))
				continue;
			elim[d] = i;
			break;
		}
	}
}

static int reduced_using_equalities(isl_int *c, struct isl_vec *v,
	struct isl_basic_map *bmap, int *elim)
{
	int d, i;
	int copied = 0;
	unsigned total;

	total = bmap->nparam + bmap->n_in + bmap->n_out;
	for (d = total - 1; d >= 0; --d) {
		if (isl_int_is_zero(c[1+d]))
			continue;
		if (elim[d] == -1)
			continue;
		if (!copied) {
			isl_seq_cpy(v->block.data, c, 1 + total);
			copied = 1;
		}
		isl_seq_elim(v->block.data, bmap->eq[elim[d]],
				1 + d, 1 + total, NULL);
	}
	return copied;
}

/* Quick check to see if two basic maps are disjoint.
 * In particular, we reduce the equalities and inequalities of
 * one basic map in the context of the equalities of the other
 * basic map and check if we get a contradiction.
 */
int isl_basic_map_fast_is_disjoint(struct isl_basic_map *bmap1,
	struct isl_basic_map *bmap2)
{
	struct isl_vec *v = NULL;
	int *elim = NULL;
	unsigned total;
	int d, i;

	if (!bmap1 || !bmap2)
		return -1;
	isl_assert(bmap1->ctx, bmap1->nparam == bmap2->nparam, return -1);
	isl_assert(bmap1->ctx, bmap1->n_in == bmap2->n_in, return -1);
	isl_assert(bmap1->ctx, bmap1->n_out == bmap2->n_out, return -1);
	if (bmap1->n_div || bmap2->n_div)
		return 0;
	if (!bmap1->n_eq && !bmap2->n_eq)
		return 0;

	total = bmap1->nparam + bmap1->n_in + bmap1->n_out;
	if (total == 0)
		return 0;
	v = isl_vec_alloc(bmap1->ctx, 1 + total);
	if (!v)
		goto error;
	elim = isl_alloc_array(bmap1->ctx, int, total);
	if (!elim)
		goto error;
	compute_elimination_index(bmap1, elim);
	for (i = 0; i < bmap2->n_eq; ++i) {
		int reduced;
		reduced = reduced_using_equalities(bmap2->eq[i], v, bmap1, elim);
		if (reduced && !isl_int_is_zero(v->block.data[0]) &&
		    isl_seq_first_non_zero(v->block.data + 1, total) == -1)
			goto disjoint;
	}
	for (i = 0; i < bmap2->n_ineq; ++i) {
		int reduced;
		reduced = reduced_using_equalities(bmap2->ineq[i], v, bmap1, elim);
		if (reduced && isl_int_is_neg(v->block.data[0]) &&
		    isl_seq_first_non_zero(v->block.data + 1, total) == -1)
			goto disjoint;
	}
	compute_elimination_index(bmap2, elim);
	for (i = 0; i < bmap1->n_ineq; ++i) {
		int reduced;
		reduced = reduced_using_equalities(bmap1->ineq[i], v, bmap2, elim);
		if (reduced && isl_int_is_neg(v->block.data[0]) &&
		    isl_seq_first_non_zero(v->block.data + 1, total) == -1)
			goto disjoint;
	}
	isl_vec_free(bmap1->ctx, v);
	free(elim);
	return 0;
disjoint:
	isl_vec_free(bmap1->ctx, v);
	free(elim);
	return 1;
error:
	isl_vec_free(bmap1->ctx, v);
	free(elim);
	return -1;
}

int isl_basic_set_fast_is_disjoint(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2)
{
	return isl_basic_map_fast_is_disjoint((struct isl_basic_map *)bset1,
					      (struct isl_basic_map *)bset2);
}

int isl_map_fast_is_disjoint(struct isl_map *map1, struct isl_map *map2)
{
	int i, j;

	if (!map1 || !map2)
		return -1;

	if (isl_map_fast_is_equal(map1, map2))
		return 0;

	for (i = 0; i < map1->n; ++i) {
		for (j = 0; j < map2->n; ++j) {
			int d = isl_basic_map_fast_is_disjoint(map1->p[i],
							       map2->p[j]);
			if (d != 1)
				return d;
		}
	}
	return 1;
}

int isl_set_fast_is_disjoint(struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_fast_is_disjoint((struct isl_map *)set1,
					(struct isl_map *)set2);
}

static struct isl_basic_map *remove_duplicate_divs(
	struct isl_basic_map *bmap, int *progress)
{
	unsigned int size;
	int *index;
	int k, l, h;
	int bits;
	struct isl_blk eq;
	unsigned total_var = bmap->nparam + bmap->n_in + bmap->n_out;
	unsigned total = total_var + bmap->n_div;
	struct isl_ctx *ctx;

	if (bmap->n_div <= 1)
		return bmap;

	ctx = bmap->ctx;
	for (k = bmap->n_div - 1; k >= 0; --k)
		if (!isl_int_is_zero(bmap->div[k][0]))
			break;
	if (k <= 0)
		return bmap;

	size = round_up(4 * bmap->n_div / 3 - 1);
	bits = ffs(size) - 1;
	index = isl_alloc_array(ctx, int, size);
	memset(index, 0, size * sizeof(int));
	if (!index)
		return bmap;
	eq = isl_blk_alloc(ctx, 1+total);
	if (isl_blk_is_error(eq))
		goto out;

	isl_seq_clr(eq.data, 1+total);
	index[isl_seq_hash(bmap->div[k], 2+total, bits)] = k + 1;
	for (--k; k >= 0; --k) {
		u_int32_t hash;

		if (isl_int_is_zero(bmap->div[k][0]))
			continue;

		hash = isl_seq_hash(bmap->div[k], 2+total, bits);
		for (h = hash; index[h]; h = (h+1) % size)
			if (isl_seq_eq(bmap->div[k],
				       bmap->div[index[h]-1], 2+total))
				break;
		if (index[h]) {
			*progress = 1;
			l = index[h] - 1;
			isl_int_set_si(eq.data[1+total_var+k], -1);
			isl_int_set_si(eq.data[1+total_var+l], 1);
			eliminate_div(bmap, eq.data, l);
			isl_int_set_si(eq.data[1+total_var+k], 0);
			isl_int_set_si(eq.data[1+total_var+l], 0);
		}
		index[h] = k+1;
	}

	isl_blk_free(ctx, eq);
out:
	free(index);
	return bmap;
}

/* Elimininate divs based on equalities
 */
static struct isl_basic_map *eliminate_divs_eq(
		struct isl_basic_map *bmap, int *progress)
{
	int d;
	int i;
	int modified = 0;
	unsigned off;

	if (!bmap)
		return NULL;

	off = 1 + bmap->nparam + bmap->n_in + bmap->n_out;

	for (d = bmap->n_div - 1; d >= 0 ; --d) {
		for (i = 0; i < bmap->n_eq; ++i) {
			if (!isl_int_is_one(bmap->eq[i][off + d]) &&
			    !isl_int_is_negone(bmap->eq[i][off + d]))
				continue;
			modified = 1;
			*progress = 1;
			eliminate_div(bmap, bmap->eq[i], d);
			isl_basic_map_drop_equality(bmap, i);
			break;
		}
	}
	if (modified)
		return eliminate_divs_eq(bmap, progress);
	return bmap;
}

static struct isl_basic_map *normalize_constraints(struct isl_basic_map *bmap)
{
	int i;
	isl_int gcd;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;

	isl_int_init(gcd);
	for (i = bmap->n_eq - 1; i >= 0; --i) {
		isl_seq_gcd(bmap->eq[i]+1, total, &gcd);
		if (isl_int_is_zero(gcd)) {
			if (!isl_int_is_zero(bmap->eq[i][0])) {
				bmap = isl_basic_map_set_to_empty(bmap);
				break;
			}
			isl_basic_map_drop_equality(bmap, i);
			continue;
		}
		if (F_ISSET(bmap, ISL_BASIC_MAP_RATIONAL))
			isl_int_gcd(gcd, gcd, bmap->eq[i][0]);
		if (isl_int_is_one(gcd))
			continue;
		if (!isl_int_is_divisible_by(bmap->eq[i][0], gcd)) {
			bmap = isl_basic_map_set_to_empty(bmap);
			break;
		}
		isl_seq_scale_down(bmap->eq[i], bmap->eq[i], gcd, 1+total);
	}

	for (i = bmap->n_ineq - 1; i >= 0; --i) {
		isl_seq_gcd(bmap->ineq[i]+1, total, &gcd);
		if (isl_int_is_zero(gcd)) {
			if (isl_int_is_neg(bmap->ineq[i][0])) {
				bmap = isl_basic_map_set_to_empty(bmap);
				break;
			}
			isl_basic_map_drop_inequality(bmap, i);
			continue;
		}
		if (F_ISSET(bmap, ISL_BASIC_MAP_RATIONAL))
			isl_int_gcd(gcd, gcd, bmap->ineq[i][0]);
		if (isl_int_is_one(gcd))
			continue;
		isl_int_fdiv_q(bmap->ineq[i][0], bmap->ineq[i][0], gcd);
		isl_seq_scale_down(bmap->ineq[i]+1, bmap->ineq[i]+1, gcd, total);
	}
	isl_int_clear(gcd);

	return bmap;
}


/* Eliminate the specified variables from the constraints using
 * Fourier-Motzkin.  The variables themselves are not removed.
 */
struct isl_basic_map *isl_basic_map_eliminate_vars(
	struct isl_basic_map *bmap, int pos, unsigned n)
{
	int d;
	int i, j, k;
	unsigned total;
	unsigned extra;

	if (!bmap)
		return NULL;
	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	extra = bmap->extra - bmap->n_div;

	bmap = isl_basic_map_cow(bmap);
	for (d = pos + n - 1; d >= pos; --d) {
		int n_lower, n_upper;
		if (!bmap)
			return NULL;
		for (i = 0; i < bmap->n_eq; ++i) {
			if (isl_int_is_zero(bmap->eq[i][1+d]))
				continue;
			eliminate_var_using_equality(bmap, d, bmap->eq[i], NULL);
			isl_basic_map_drop_equality(bmap, i);
			break;
		}
		if (i < bmap->n_eq)
			continue;
		n_lower = 0;
		n_upper = 0;
		for (i = 0; i < bmap->n_ineq; ++i) {
			if (isl_int_is_pos(bmap->ineq[i][1+d]))
				n_lower++;
			else if (isl_int_is_neg(bmap->ineq[i][1+d]))
				n_upper++;
		}
		bmap = isl_basic_map_extend(bmap,
				bmap->nparam, bmap->n_in, bmap->n_out, 0,
				0, n_lower * n_upper);
		for (i = bmap->n_ineq - 1; i >= 0; --i) {
			int last;
			if (isl_int_is_zero(bmap->ineq[i][1+d]))
				continue;
			last = -1;
			for (j = 0; j < i; ++j) {
				if (isl_int_is_zero(bmap->ineq[j][1+d]))
					continue;
				last = j;
				if (isl_int_sgn(bmap->ineq[i][1+d]) ==
				    isl_int_sgn(bmap->ineq[j][1+d]))
					continue;
				k = isl_basic_map_alloc_inequality(bmap);
				if (k < 0)
					goto error;
				isl_seq_cpy(bmap->ineq[k], bmap->ineq[i],
						1+total);
				isl_seq_clr(bmap->ineq[k]+1+total, extra);
				isl_seq_elim(bmap->ineq[k], bmap->ineq[j],
						1+d, 1+total, NULL);
			}
			isl_basic_map_drop_inequality(bmap, i);
			i = last + 1;
		}
		if (n_lower > 0 && n_upper > 0) {
			bmap = normalize_constraints(bmap);
			bmap = remove_duplicate_constraints(bmap, NULL);
			bmap = isl_basic_map_gauss(bmap, NULL);
			bmap = isl_basic_map_convex_hull(bmap);
			if (!bmap)
				goto error;
			if (F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
				break;
		}
	}
	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_eliminate_vars(
	struct isl_basic_set *bset, unsigned pos, unsigned n)
{
	return (struct isl_basic_set *)isl_basic_map_eliminate_vars(
			(struct isl_basic_map *)bset, pos, n);
}

/* Eliminate the specified n dimensions starting at first from the
 * constraints using Fourier-Motzkin, The dimensions themselves
 * are not removed.
 */
struct isl_set *isl_set_eliminate_dims(struct isl_set *set,
	unsigned first, unsigned n)
{
	int i;

	if (n == 0)
		return set;

	set = isl_set_cow(set);
	if (!set)
		return NULL;
	isl_assert(set->ctx, first+n <= set->dim, goto error);
	
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_eliminate_vars(set->p[i],
							set->nparam + first, n);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

/* Project out n dimensions starting at first using Fourier-Motzkin */
struct isl_set *isl_set_remove_dims(struct isl_set *set,
	unsigned first, unsigned n)
{
	set = isl_set_eliminate_dims(set, first, n);
	set = isl_set_drop_dims(set, first, n);
	return set;
}

/* Project out n inputs starting at first using Fourier-Motzkin */
struct isl_map *isl_map_remove_inputs(struct isl_map *map,
	unsigned first, unsigned n)
{
	int i;

	if (n == 0)
		return map;

	map = isl_map_cow(map);
	if (!map)
		return NULL;
	isl_assert(map->ctx, first+n <= map->n_in, goto error);
	
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_eliminate_vars(map->p[i],
							map->nparam + first, n);
		if (!map->p[i])
			goto error;
	}
	map = isl_map_drop_inputs(map, first, n);
	return map;
error:
	isl_map_free(map);
	return NULL;
}

/* Project out n dimensions starting at first using Fourier-Motzkin */
struct isl_basic_set *isl_basic_set_remove_dims(struct isl_basic_set *bset,
	unsigned first, unsigned n)
{
	bset = isl_basic_set_eliminate_vars(bset, bset->nparam + first, n);
	bset = isl_basic_set_drop_dims(bset, first, n);
	return bset;
}

/* Elimininate divs based on inequalities
 */
static struct isl_basic_map *eliminate_divs_ineq(
		struct isl_basic_map *bmap, int *progress)
{
	int d;
	int i;
	unsigned off;
	struct isl_ctx *ctx;

	if (!bmap)
		return NULL;

	ctx = bmap->ctx;
	off = 1 + bmap->nparam + bmap->n_in + bmap->n_out;

	for (d = bmap->n_div - 1; d >= 0 ; --d) {
		for (i = 0; i < bmap->n_eq; ++i)
			if (!isl_int_is_zero(bmap->eq[i][off + d]))
				break;
		if (i < bmap->n_eq)
			continue;
		for (i = 0; i < bmap->n_ineq; ++i)
			if (isl_int_abs_gt(bmap->ineq[i][off + d], ctx->one))
				break;
		if (i < bmap->n_ineq)
			continue;
		*progress = 1;
		bmap = isl_basic_map_eliminate_vars(bmap, (off-1)+d, 1);
		if (F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
			break;
		bmap = isl_basic_map_drop_div(bmap, d);
		if (!bmap)
			break;
	}
	return bmap;
}

struct isl_basic_map *isl_basic_map_simplify(struct isl_basic_map *bmap)
{
	int progress = 1;
	if (!bmap)
		return NULL;
	while (progress) {
		progress = 0;
		bmap = normalize_constraints(bmap);
		bmap = eliminate_divs_eq(bmap, &progress);
		bmap = eliminate_divs_ineq(bmap, &progress);
		bmap = isl_basic_map_gauss(bmap, &progress);
		bmap = remove_duplicate_divs(bmap, &progress);
		bmap = remove_duplicate_constraints(bmap, &progress);
	}
	return bmap;
}

struct isl_basic_set *isl_basic_set_simplify(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_simplify((struct isl_basic_map *)bset);
}

static void dump_term(struct isl_basic_map *bmap,
			isl_int c, int pos, FILE *out)
{
	unsigned in = bmap->n_in;
	unsigned dim = bmap->n_in + bmap->n_out;
	if (!pos)
		isl_int_print(out, c, 0);
	else {
		if (!isl_int_is_one(c))
			isl_int_print(out, c, 0);
		if (pos < 1 + bmap->nparam)
			fprintf(out, "p%d", pos - 1);
		else if (pos < 1 + bmap->nparam + in)
			fprintf(out, "i%d", pos - 1 - bmap->nparam);
		else if (pos < 1 + bmap->nparam + dim)
			fprintf(out, "o%d", pos - 1 - bmap->nparam - in);
		else
			fprintf(out, "e%d", pos - 1 - bmap->nparam - dim);
	}
}

static void dump_constraint_sign(struct isl_basic_map *bmap, isl_int *c,
				int sign, FILE *out)
{
	int i;
	int first;
	unsigned len = 1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	isl_int v;

	isl_int_init(v);
	for (i = 0, first = 1; i < len; ++i) {
		if (isl_int_sgn(c[i]) * sign <= 0)
			continue;
		if (!first)
			fprintf(out, " + ");
		first = 0;
		isl_int_abs(v, c[i]);
		dump_term(bmap, v, i, out);
	}
	isl_int_clear(v);
	if (first)
		fprintf(out, "0");
}

static void dump_constraint(struct isl_basic_map *bmap, isl_int *c,
				const char *op, FILE *out, int indent)
{
	int i;

	fprintf(out, "%*s", indent, "");

	dump_constraint_sign(bmap, c, 1, out);
	fprintf(out, " %s ", op);
	dump_constraint_sign(bmap, c, -1, out);

	fprintf(out, "\n");

	for (i = bmap->n_div; i < bmap->extra; ++i) {
		if (isl_int_is_zero(c[1+bmap->nparam+bmap->n_in+bmap->n_out+i]))
			continue;
		fprintf(out, "%*s", indent, "");
		fprintf(out, "ERROR: unused div coefficient not zero\n");
	}
}

static void dump_constraints(struct isl_basic_map *bmap,
				isl_int **c, unsigned n,
				const char *op, FILE *out, int indent)
{
	int i;

	for (i = 0; i < n; ++i)
		dump_constraint(bmap, c[i], op, out, indent);
}

static void dump_affine(struct isl_basic_map *bmap, isl_int *exp, FILE *out)
{
	int j;
	int first = 1;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;

	for (j = 0; j < 1 + total; ++j) {
		if (isl_int_is_zero(exp[j]))
			continue;
		if (!first && isl_int_is_pos(exp[j]))
			fprintf(out, "+");
		dump_term(bmap, exp[j], j, out);
		first = 0;
	}
}

static void dump(struct isl_basic_map *bmap, FILE *out, int indent)
{
	int i;

	dump_constraints(bmap, bmap->eq, bmap->n_eq, "=", out, indent);
	dump_constraints(bmap, bmap->ineq, bmap->n_ineq, ">=", out, indent);

	for (i = 0; i < bmap->n_div; ++i) {
		fprintf(out, "%*s", indent, "");
		fprintf(out, "e%d = [(", i);
		dump_affine(bmap, bmap->div[i]+1, out);
		fprintf(out, ")/");
		isl_int_print(out, bmap->div[i][0], 0);
		fprintf(out, "]\n");
	}
}

void isl_basic_set_dump(struct isl_basic_set *bset, FILE *out, int indent)
{
	if (!bset) {
		fprintf(out, "null basic set\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, nparam: %d, dim: %d, extra: %d, flags: %x\n",
			bset->ref, bset->nparam, bset->dim, bset->extra,
			bset->flags);
	dump((struct isl_basic_map *)bset, out, indent);
}

void isl_basic_map_dump(struct isl_basic_map *bmap, FILE *out, int indent)
{
	if (!bmap) {
		fprintf(out, "null basic map\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, nparam: %d, in: %d, out: %d, extra: %d, flags: %x\n",
		bmap->ref,
		bmap->nparam, bmap->n_in, bmap->n_out, bmap->extra, bmap->flags);
	dump(bmap, out, indent);
}

int isl_inequality_negate(struct isl_basic_map *bmap, unsigned pos)
{
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	if (!bmap)
		return -1;
	isl_assert(bmap->ctx, pos < bmap->n_ineq, return -1);
	isl_seq_neg(bmap->ineq[pos], bmap->ineq[pos], 1 + total);
	isl_int_sub_ui(bmap->ineq[pos][0], bmap->ineq[pos][0], 1);
	F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	return 0;
}

struct isl_set *isl_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, int n, unsigned flags)
{
	struct isl_set *set;

	isl_assert(ctx, n >= 0, return NULL);
	set = isl_alloc(ctx, struct isl_set,
			sizeof(struct isl_set) +
			n * sizeof(struct isl_basic_set *));
	if (!set)
		return NULL;

	set->ctx = ctx;
	isl_ctx_ref(ctx);
	set->ref = 1;
	set->size = n;
	set->n = 0;
	set->nparam = nparam;
	set->zero = 0;
	set->dim = dim;
	set->flags = flags;
	return set;
}

struct isl_set *isl_set_dup(struct isl_set *set)
{
	int i;
	struct isl_set *dup;

	if (!set)
		return NULL;

	dup = isl_set_alloc(set->ctx, set->nparam, set->dim, set->n, set->flags);
	if (!dup)
		return NULL;
	for (i = 0; i < set->n; ++i)
		dup = isl_set_add(dup,
				  isl_basic_set_dup(set->p[i]));
	return dup;
}

struct isl_set *isl_set_from_basic_set(struct isl_basic_set *bset)
{
	struct isl_set *set;

	if (!bset)
		return NULL;

	set = isl_set_alloc(bset->ctx, bset->nparam, bset->dim, 1, ISL_MAP_DISJOINT);
	if (!set) {
		isl_basic_set_free(bset);
		return NULL;
	}
	return isl_set_add(set, bset);
}

struct isl_map *isl_map_from_basic_map(struct isl_basic_map *bmap)
{
	struct isl_map *map;

	if (!bmap)
		return NULL;

	map = isl_map_alloc(bmap->ctx, bmap->nparam, bmap->n_in, bmap->n_out, 1,
				ISL_MAP_DISJOINT);
	if (!map) {
		isl_basic_map_free(bmap);
		return NULL;
	}
	return isl_map_add(map, bmap);
}

struct isl_set *isl_set_add(struct isl_set *set, struct isl_basic_set *bset)
{
	if (!bset || !set)
		goto error;
	isl_assert(set->ctx, set->nparam == bset->nparam, goto error);
	isl_assert(set->ctx, set->dim == bset->dim, goto error);
	isl_assert(set->ctx, set->n < set->size, goto error);
	set->p[set->n] = bset;
	set->n++;
	return set;
error:
	if (set)
		isl_set_free(set);
	if (bset)
		isl_basic_set_free(bset);
	return NULL;
}

void isl_set_free(struct isl_set *set)
{
	int i;

	if (!set)
		return;

	if (--set->ref > 0)
		return;

	isl_ctx_deref(set->ctx);
	for (i = 0; i < set->n; ++i)
		isl_basic_set_free(set->p[i]);
	free(set);
}

void isl_set_dump(struct isl_set *set, FILE *out, int indent)
{
	int i;

	if (!set) {
		fprintf(out, "null set\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, n: %d, nparam: %d, dim: %d, flags: %x\n",
			set->ref, set->n, set->nparam, set->dim, set->flags);
	for (i = 0; i < set->n; ++i) {
		fprintf(out, "%*s", indent, "");
		fprintf(out, "basic set %d:\n", i);
		isl_basic_set_dump(set->p[i], out, indent+4);
	}
}

void isl_map_dump(struct isl_map *map, FILE *out, int indent)
{
	int i;

	if (!map) {
		fprintf(out, "null map\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, n: %d, nparam: %d, in: %d, out: %d, flags: %x\n",
			map->ref, map->n, map->nparam, map->n_in, map->n_out,
			map->flags);
	for (i = 0; i < map->n; ++i) {
		fprintf(out, "%*s", indent, "");
		fprintf(out, "basic map %d:\n", i);
		isl_basic_map_dump(map->p[i], out, indent+4);
	}
}

struct isl_basic_map *isl_basic_map_intersect_domain(
		struct isl_basic_map *bmap, struct isl_basic_set *bset)
{
	struct isl_basic_map *bmap_domain;

	if (!bmap || !bset)
		goto error;

	isl_assert(bset->ctx, bset->dim == bmap->n_in, goto error);
	isl_assert(bset->ctx, bset->nparam == bmap->nparam, goto error);

	bmap = isl_basic_map_extend(bmap,
			bset->nparam, bmap->n_in, bmap->n_out,
			bset->n_div, bset->n_eq, bset->n_ineq);
	if (!bmap)
		goto error;
	bmap_domain = isl_basic_map_from_basic_set(bset,
						bset->dim, 0);
	bmap = add_constraints(bmap, bmap_domain, 0, 0);

	bmap = isl_basic_map_simplify(bmap);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_intersect_range(
		struct isl_basic_map *bmap, struct isl_basic_set *bset)
{
	struct isl_basic_map *bmap_range;

	if (!bmap || !bset)
		goto error;

	isl_assert(bset->ctx, bset->dim == bmap->n_out, goto error);
	isl_assert(bset->ctx, bset->nparam == bmap->nparam, goto error);

	bmap = isl_basic_map_extend(bmap,
			bset->nparam, bmap->n_in, bmap->n_out,
			bset->n_div, bset->n_eq, bset->n_ineq);
	if (!bmap)
		goto error;
	bmap_range = isl_basic_map_from_basic_set(bset,
						0, bset->dim);
	bmap = add_constraints(bmap, bmap_range, 0, 0);

	bmap = isl_basic_map_simplify(bmap);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_intersect(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	if (!bmap1 || !bmap2)
		goto error;

	isl_assert(bmap1->ctx, bmap1->nparam == bmap2->nparam, goto error);
	isl_assert(bmap1->ctx, bmap1->n_in == bmap2->n_in, goto error);
	isl_assert(bmap1->ctx, bmap1->n_out == bmap2->n_out, goto error);

	bmap1 = isl_basic_map_extend(bmap1,
			bmap1->nparam, bmap1->n_in, bmap1->n_out,
			bmap2->n_div, bmap2->n_eq, bmap2->n_ineq);
	if (!bmap1)
		goto error;
	bmap1 = add_constraints(bmap1, bmap2, 0, 0);

	bmap1 = isl_basic_map_simplify(bmap1);
	return isl_basic_map_finalize(bmap1);
error:
	isl_basic_map_free(bmap1);
	isl_basic_map_free(bmap2);
	return NULL;
}

struct isl_basic_set *isl_basic_set_intersect(
		struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	return (struct isl_basic_set *)
		isl_basic_map_intersect(
			(struct isl_basic_map *)bset1,
			(struct isl_basic_map *)bset2);
}

struct isl_map *isl_map_intersect(struct isl_map *map1, struct isl_map *map2)
{
	unsigned flags = 0;
	struct isl_map *result;
	int i, j;

	if (!map1 || !map2)
		goto error;

	if (F_ISSET(map1, ISL_MAP_DISJOINT) &&
	    F_ISSET(map2, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	result = isl_map_alloc(map1->ctx, map1->nparam, map1->n_in, map1->n_out,
				map1->n * map2->n, flags);
	if (!result)
		goto error;
	for (i = 0; i < map1->n; ++i)
		for (j = 0; j < map2->n; ++j) {
			struct isl_basic_map *part;
			part = isl_basic_map_intersect(
				    isl_basic_map_copy(map1->p[i]),
				    isl_basic_map_copy(map2->p[j]));
			if (isl_basic_map_is_empty(part))
				isl_basic_map_free(part);
			else
				result = isl_map_add(result, part);
			if (!result)
				goto error;
		}
	isl_map_free(map1);
	isl_map_free(map2);
	return result;
error:
	isl_map_free(map1);
	isl_map_free(map2);
	return NULL;
}

struct isl_set *isl_set_intersect(struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_intersect((struct isl_map *)set1,
				  (struct isl_map *)set2);
}

struct isl_basic_map *isl_basic_map_reverse(struct isl_basic_map *bmap)
{
	struct isl_basic_set *bset;
	unsigned in;

	if (!bmap)
		return NULL;
	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		return NULL;
	in = bmap->n_in;
	bset = isl_basic_set_from_basic_map(bmap);
	bset = isl_basic_set_swap_vars(bset, in);
	return isl_basic_map_from_basic_set(bset, bset->dim-in, in);
}

/* Turn final n dimensions into existentially quantified variables.
 */
struct isl_basic_set *isl_basic_set_project_out(
		struct isl_basic_set *bset, unsigned n, unsigned flags)
{
	int i;
	size_t row_size;
	isl_int **new_div;
	isl_int *old;

	if (!bset)
		return NULL;

	isl_assert(bset->ctx, n <= bset->dim, goto error);

	if (n == 0)
		return bset;

	bset = isl_basic_set_cow(bset);

	row_size = 1 + bset->nparam + bset->dim + bset->extra;
	old = bset->block2.data;
	bset->block2 = isl_blk_extend(bset->ctx, bset->block2,
					(bset->extra + n) * (1 + row_size));
	if (!bset->block2.data)
		goto error;
	new_div = isl_alloc_array(ctx, isl_int *, bset->extra + n);
	if (!new_div)
		goto error;
	for (i = 0; i < n; ++i) {
		new_div[i] = bset->block2.data +
				(bset->extra + i) * (1 + row_size);
		isl_seq_clr(new_div[i], 1 + row_size);
	}
	for (i = 0; i < bset->extra; ++i)
		new_div[n + i] = bset->block2.data + (bset->div[i] - old);
	free(bset->div);
	bset->div = new_div;
	bset->n_div += n;
	bset->extra += n;
	bset->dim -= n;
	bset = isl_basic_set_simplify(bset);
	return isl_basic_set_finalize(bset);
error:
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_apply_range(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	struct isl_basic_set *bset;
	unsigned n_in, n_out;

	if (!bmap1 || !bmap2)
		goto error;

	isl_assert(bmap->ctx, bmap1->n_out == bmap2->n_in, goto error);
	isl_assert(bmap->ctx, bmap1->nparam == bmap2->nparam, goto error);

	n_in = bmap1->n_in;
	n_out = bmap2->n_out;

	bmap2 = isl_basic_map_reverse(bmap2);
	if (!bmap2)
		goto error;
	bmap1 = isl_basic_map_extend(bmap1, bmap1->nparam,
			bmap1->n_in + bmap2->n_in, bmap1->n_out,
			bmap2->extra, bmap2->n_eq, bmap2->n_ineq);
	if (!bmap1)
		goto error;
	bmap1 = add_constraints(bmap1, bmap2, bmap1->n_in - bmap2->n_in, 0);
	bmap1 = isl_basic_map_simplify(bmap1);
	bset = isl_basic_set_from_basic_map(bmap1);
	bset = isl_basic_set_project_out(bset,
						bset->dim - (n_in + n_out), 0);
	return isl_basic_map_from_basic_set(bset, n_in, n_out);
error:
	isl_basic_map_free(bmap1);
	isl_basic_map_free(bmap2);
	return NULL;
}

struct isl_basic_set *isl_basic_set_apply(
		struct isl_basic_set *bset, struct isl_basic_map *bmap)
{
	if (!bset || !bmap)
		goto error;

	isl_assert(bset->ctx, bset->dim == bmap->n_in, goto error);

	return (struct isl_basic_set *)
		isl_basic_map_apply_range((struct isl_basic_map *)bset, bmap);
error:
	isl_basic_set_free(bset);
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_apply_domain(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	if (!bmap1 || !bmap2)
		goto error;

	isl_assert(ctx, bmap1->n_in == bmap2->n_in, goto error);
	isl_assert(ctx, bmap1->nparam == bmap2->nparam, goto error);

	bmap1 = isl_basic_map_reverse(bmap1);
	bmap1 = isl_basic_map_apply_range(bmap1, bmap2);
	return isl_basic_map_reverse(bmap1);
error:
	isl_basic_map_free(bmap1);
	isl_basic_map_free(bmap2);
	return NULL;
}

static struct isl_basic_map *var_equal(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	int i;
	i = isl_basic_map_alloc_equality(bmap);
	if (i < 0)
		goto error;
	isl_seq_clr(bmap->eq[i],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->eq[i][1+bmap->nparam+pos], -1);
	isl_int_set_si(bmap->eq[i][1+bmap->nparam+bmap->n_in+pos], 1);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *var_more(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	int i;
	i = isl_basic_map_alloc_inequality(bmap);
	if (i < 0)
		goto error;
	isl_seq_clr(bmap->ineq[i],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->ineq[i][0], -1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+pos], -1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+bmap->n_in+pos], 1);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

static struct isl_basic_map *var_less(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	int i;
	i = isl_basic_map_alloc_inequality(bmap);
	if (i < 0)
		goto error;
	isl_seq_clr(bmap->ineq[i],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->ineq[i][0], -1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+pos], 1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+bmap->n_in+pos], -1);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_equal(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, unsigned n_equal)
{
	int i;
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_alloc(ctx, nparam, in, out, 0, n_equal, 0);
	if (!bmap)
		return NULL;
	for (i = 0; i < n_equal && bmap; ++i)
		bmap = var_equal(ctx, bmap, i);
	return isl_basic_map_finalize(bmap);
}

struct isl_basic_map *isl_basic_map_less_at(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, unsigned pos)
{
	int i;
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_alloc(ctx, nparam, in, out, 0, pos, 1);
	if (!bmap)
		return NULL;
	for (i = 0; i < pos && bmap; ++i)
		bmap = var_equal(ctx, bmap, i);
	if (bmap)
		bmap = var_less(ctx, bmap, pos);
	return isl_basic_map_finalize(bmap);
}

struct isl_basic_map *isl_basic_map_more_at(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, unsigned pos)
{
	int i;
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_alloc(ctx, nparam, in, out, 0, pos, 1);
	if (!bmap)
		return NULL;
	for (i = 0; i < pos && bmap; ++i)
		bmap = var_equal(ctx, bmap, i);
	if (bmap)
		bmap = var_more(ctx, bmap, pos);
	return isl_basic_map_finalize(bmap);
}

struct isl_basic_map *isl_basic_map_from_basic_set(
		struct isl_basic_set *bset, unsigned n_in, unsigned n_out)
{
	struct isl_basic_map *bmap;

	bset = isl_basic_set_cow(bset);
	if (!bset)
		return NULL;

	isl_assert(ctx, bset->dim == n_in + n_out, goto error);
	bmap = (struct isl_basic_map *) bset;
	bmap->n_in = n_in;
	bmap->n_out = n_out;
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_basic_set *isl_basic_set_from_basic_map(struct isl_basic_map *bmap)
{
	if (!bmap)
		goto error;
	if (bmap->n_in == 0)
		return (struct isl_basic_set *)bmap;
	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		goto error;
	bmap->n_out += bmap->n_in;
	bmap->n_in = 0;
	bmap = isl_basic_map_finalize(bmap);
	return (struct isl_basic_set *)bmap;
error:
	return NULL;
}

/* For a div d = floor(f/m), add the constraints
 *
 *		f - m d >= 0
 *		-(f-(n-1)) + m d >= 0
 *
 * Note that the second constraint is the negation of
 *
 *		f - m d >= n
 */
static int add_div_constraints(struct isl_basic_map *bmap, unsigned div)
{
	int i, j;
	unsigned div_pos = 1 + bmap->nparam + bmap->n_in + bmap->n_out + div;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra;

	i = isl_basic_map_alloc_inequality(bmap);
	if (i < 0)
		return -1;
	isl_seq_cpy(bmap->ineq[i], bmap->div[div]+1, 1+total);
	isl_int_neg(bmap->ineq[i][div_pos], bmap->div[div][0]);

	j = isl_basic_map_alloc_inequality(bmap);
	if (j < 0)
		return -1;
	isl_seq_neg(bmap->ineq[j], bmap->ineq[i], 1 + total);
	isl_int_add(bmap->ineq[j][0], bmap->ineq[j][0], bmap->ineq[j][div_pos]);
	isl_int_sub_ui(bmap->ineq[j][0], bmap->ineq[j][0], 1);
	return j;
}

struct isl_basic_set *isl_basic_map_underlying_set(
		struct isl_basic_map *bmap)
{
	if (!bmap)
		goto error;
	if (bmap->nparam == 0 && bmap->n_in == 0 && bmap->n_div == 0)
		return (struct isl_basic_set *)bmap;
	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		goto error;
	bmap->n_out += bmap->nparam + bmap->n_in + bmap->n_div;
	bmap->nparam = 0;
	bmap->n_in = 0;
	bmap->extra -= bmap->n_div;
	bmap->n_div = 0;
	bmap = isl_basic_map_finalize(bmap);
	return (struct isl_basic_set *)bmap;
error:
	return NULL;
}

struct isl_basic_map *isl_basic_map_overlying_set(
	struct isl_basic_set *bset, struct isl_basic_map *like)
{
	struct isl_basic_map *bmap;
	struct isl_ctx *ctx;
	unsigned total;
	int i, k;

	if (!bset || !like)
		goto error;
	ctx = bset->ctx;
	isl_assert(ctx, bset->dim ==
			like->nparam + like->n_in + like->n_out + like->n_div,
			goto error);
	if (like->nparam == 0 && like->n_in == 0 && like->n_div == 0) {
		isl_basic_map_free(like);
		return (struct isl_basic_map *)bset;
	}
	bset = isl_basic_set_cow(bset);
	if (!bset)
		goto error;
	total = bset->dim + bset->extra;
	bmap = (struct isl_basic_map *)bset;
	bmap->nparam = like->nparam;
	bmap->n_in = like->n_in;
	bmap->n_out = like->n_out;
	bmap->extra += like->n_div;
	if (bmap->extra) {
		unsigned ltotal;
		ltotal = total - bmap->extra + like->extra;
		if (ltotal > total)
			ltotal = total;
		bmap->block2 = isl_blk_extend(ctx, bmap->block2,
					bmap->extra * (1 + 1 + total));
		if (isl_blk_is_error(bmap->block2))
			goto error;
		bmap->div = isl_realloc_array(ctx, bmap->div, isl_int *,
						bmap->extra);
		if (!bmap->div)
			goto error;
		bmap = isl_basic_map_extend(bmap, bmap->nparam,
			bmap->n_in, bmap->n_out, 0, 0, 2 * like->n_div);
		for (i = 0; i < like->n_div; ++i) {
			k = isl_basic_map_alloc_div(bmap);
			if (k < 0)
				goto error;
			isl_seq_cpy(bmap->div[k], like->div[i], 1 + 1 + ltotal);
			isl_seq_clr(bmap->div[k]+1+1+ltotal, total - ltotal);
			if (add_div_constraints(bmap, k) < 0)
				goto error;
		}
	}
	isl_basic_map_free(like);
	bmap = isl_basic_map_finalize(bmap);
	return bmap;
error:
	isl_basic_map_free(like);
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_basic_set *isl_basic_set_from_underlying_set(
	struct isl_basic_set *bset, struct isl_basic_set *like)
{
	return (struct isl_basic_set *)
		isl_basic_map_overlying_set(bset, (struct isl_basic_map *)like);
}

struct isl_set *isl_set_from_underlying_set(
	struct isl_set *set, struct isl_basic_set *like)
{
	int i;

	if (!set || !like)
		goto error;
	isl_assert(set->ctx, set->dim == like->nparam + like->dim + like->n_div,
		    goto error);
	if (like->nparam == 0 && like->n_div == 0) {
		isl_basic_set_free(like);
		return set;
	}
	set = isl_set_cow(set);
	if (!set)
		goto error;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_from_underlying_set(set->p[i],
						      isl_basic_set_copy(like));
		if (!set->p[i])
			goto error;
	}
	set->nparam = like->nparam;
	set->dim = like->dim;
	isl_basic_set_free(like);
	return set;
error:
	isl_basic_set_free(like);
	isl_set_free(set);
	return NULL;
}

struct isl_set *isl_map_underlying_set(struct isl_map *map)
{
	int i;

	map = isl_map_align_divs(map);
	map = isl_map_cow(map);
	if (!map)
		return NULL;

	for (i = 0; i < map->n; ++i) {
		map->p[i] = (struct isl_basic_map *)
				isl_basic_map_underlying_set(map->p[i]);
		if (!map->p[i])
			goto error;
	}
	if (map->n == 0)
		map->n_out += map->nparam + map->n_in;
	else
		map->n_out = map->p[0]->n_out;
	map->nparam = 0;
	map->n_in = 0;
	return (struct isl_set *)map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_set *isl_set_to_underlying_set(struct isl_set *set)
{
	return (struct isl_set *)isl_map_underlying_set((struct isl_map *)set);
}

struct isl_basic_set *isl_basic_map_domain(struct isl_basic_map *bmap)
{
	struct isl_basic_set *domain;
	unsigned n_out;
	if (!bmap)
		return NULL;
	n_out = bmap->n_out;
	domain = isl_basic_set_from_basic_map(bmap);
	return isl_basic_set_project_out(domain, n_out, 0);
}

struct isl_basic_set *isl_basic_map_range(struct isl_basic_map *bmap)
{
	return isl_basic_map_domain(isl_basic_map_reverse(bmap));
}

struct isl_set *isl_map_range(struct isl_map *map)
{
	int i;
	struct isl_set *set;

	if (!map)
		goto error;
	map = isl_map_cow(map);
	if (!map)
		goto error;

	set = (struct isl_set *) map;
	set->zero = 0;
	for (i = 0; i < map->n; ++i) {
		set->p[i] = isl_basic_map_range(map->p[i]);
		if (!set->p[i])
			goto error;
	}
	F_CLR(set, ISL_MAP_DISJOINT);
	F_CLR(set, ISL_SET_NORMALIZED);
	return set;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_map *isl_map_from_set(struct isl_set *set,
		unsigned n_in, unsigned n_out)
{
	int i;
	struct isl_map *map = NULL;

	if (!set)
		return NULL;
	isl_assert(set->ctx, set->dim == n_in + n_out, goto error);
	set = isl_set_cow(set);
	if (!set)
		return NULL;
	map = (struct isl_map *)set;
	for (i = 0; i < set->n; ++i) {
		map->p[i] = isl_basic_map_from_basic_set(
				set->p[i], n_in, n_out);
		if (!map->p[i])
			goto error;
	}
	map->n_in = n_in;
	map->n_out = n_out;
	return map;
error:
	isl_set_free(set);
	return NULL;
}

struct isl_set *isl_set_from_map(struct isl_map *map)
{
	int i;
	struct isl_set *set = NULL;

	if (!map)
		return NULL;
	map = isl_map_cow(map);
	if (!map)
		return NULL;
	map->n_out += map->n_in;
	map->n_in = 0;
	set = (struct isl_set *)map;
	for (i = 0; i < map->n; ++i) {
		set->p[i] = isl_basic_set_from_basic_map(map->p[i]);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_map *isl_map_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out, int n,
		unsigned flags)
{
	struct isl_map *map;

	map = isl_alloc(ctx, struct isl_map,
			sizeof(struct isl_map) +
			n * sizeof(struct isl_basic_map *));
	if (!map)
		return NULL;

	map->ctx = ctx;
	isl_ctx_ref(ctx);
	map->ref = 1;
	map->size = n;
	map->n = 0;
	map->nparam = nparam;
	map->n_in = in;
	map->n_out = out;
	map->flags = flags;
	return map;
}

struct isl_basic_map *isl_basic_map_empty(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_alloc(ctx, nparam, in, out, 0, 1, 0);
	bmap = isl_basic_map_set_to_empty(bmap);
	return bmap;
}

struct isl_basic_set *isl_basic_set_empty(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim)
{
	struct isl_basic_set *bset;
	bset = isl_basic_set_alloc(ctx, nparam, dim, 0, 1, 0);
	bset = isl_basic_set_set_to_empty(bset);
	return bset;
}

struct isl_basic_map *isl_basic_map_universe(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out)
{
	struct isl_basic_map *bmap;
	bmap = isl_basic_map_alloc(ctx, nparam, in, out, 0, 0, 0);
	return bmap;
}

struct isl_basic_set *isl_basic_set_universe(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim)
{
	struct isl_basic_set *bset;
	bset = isl_basic_set_alloc(ctx, nparam, dim, 0, 0, 0);
	return bset;
}

struct isl_map *isl_map_empty(struct isl_ctx *ctx,
		unsigned nparam, unsigned in, unsigned out)
{
	return isl_map_alloc(ctx, nparam, in, out, 0, ISL_MAP_DISJOINT);
}

struct isl_set *isl_set_empty(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim)
{
	return isl_set_alloc(ctx, nparam, dim, 0, ISL_MAP_DISJOINT);
}

struct isl_map *isl_map_dup(struct isl_map *map)
{
	int i;
	struct isl_map *dup;

	if (!map)
		return NULL;
	dup = isl_map_alloc(map->ctx, map->nparam, map->n_in, map->n_out, map->n,
				map->flags);
	for (i = 0; i < map->n; ++i)
		dup = isl_map_add(dup,
				  isl_basic_map_dup(map->p[i]));
	return dup;
}

struct isl_map *isl_map_add(struct isl_map *map, struct isl_basic_map *bmap)
{
	if (!bmap || !map)
		goto error;
	isl_assert(map->ctx, map->nparam == bmap->nparam, goto error);
	isl_assert(map->ctx, map->n_in == bmap->n_in, goto error);
	isl_assert(map->ctx, map->n_out == bmap->n_out, goto error);
	isl_assert(map->ctx, map->n < map->size, goto error);
	map->p[map->n] = bmap;
	map->n++;
	F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	if (map)
		isl_map_free(map);
	if (bmap)
		isl_basic_map_free(bmap);
	return NULL;
}

void isl_map_free(struct isl_map *map)
{
	int i;

	if (!map)
		return;

	if (--map->ref > 0)
		return;

	isl_ctx_deref(map->ctx);
	for (i = 0; i < map->n; ++i)
		isl_basic_map_free(map->p[i]);
	free(map);
}

struct isl_map *isl_map_extend(struct isl_map *base,
		unsigned nparam, unsigned n_in, unsigned n_out)
{
	int i;

	base = isl_map_cow(base);
	if (!base)
		return NULL;

	isl_assert(base->ctx, base->nparam <= nparam, goto error);
	isl_assert(base->ctx, base->n_in <= n_in, goto error);
	isl_assert(base->ctx, base->n_out <= n_out, goto error);
	base->nparam = nparam;
	base->n_in = n_in;
	base->n_out = n_out;
	for (i = 0; i < base->n; ++i) {
		base->p[i] = isl_basic_map_extend(base->p[i],
				nparam, n_in, n_out, 0, 0, 0);
		if (!base->p[i])
			goto error;
	}
	return base;
error:
	isl_map_free(base);
	return NULL;
}

struct isl_set *isl_set_extend(struct isl_set *base,
		unsigned nparam, unsigned dim)
{
	return (struct isl_set *)isl_map_extend((struct isl_map *)base,
							nparam, 0, dim);
}

static struct isl_basic_map *isl_basic_map_fix_var(struct isl_basic_map *bmap,
		unsigned var, int value)
{
	int j;

	bmap = isl_basic_map_cow(bmap);
	bmap = isl_basic_map_extend(bmap,
			bmap->nparam, bmap->n_in, bmap->n_out, 0, 1, 0);
	j = isl_basic_map_alloc_equality(bmap);
	if (j < 0)
		goto error;
	isl_seq_clr(bmap->eq[j],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->eq[j][1 + var], -1);
	isl_int_set_si(bmap->eq[j][0], value);
	bmap = isl_basic_map_simplify(bmap);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_fix_input_si(struct isl_basic_map *bmap,
		unsigned input, int value)
{
	if (!bmap)
		return NULL;
	isl_assert(bmap->ctx, input < bmap->n_in, goto error);
	return isl_basic_map_fix_var(bmap, bmap->nparam + input, value);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_fix_dim_si(struct isl_basic_set *bset,
		unsigned dim, int value)
{
	if (!bset)
		return NULL;
	isl_assert(bset->ctx, dim < bset->dim, goto error);
	return (struct isl_basic_set *)
		isl_basic_map_fix_var((struct isl_basic_map *)bset,
						bset->nparam + dim, value);
error:
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_map *isl_map_fix_input_si(struct isl_map *map,
		unsigned input, int value)
{
	int i;

	map = isl_map_cow(map);
	if (!map)
		return NULL;

	isl_assert(ctx, input < map->n_in, goto error);
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_fix_input_si(map->p[i], input, value);
		if (!map->p[i])
			goto error;
	}
	F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_set *isl_set_fix_dim_si(struct isl_set *set, unsigned dim, int value)
{
	int i;

	set = isl_set_cow(set);
	if (!set)
		return NULL;

	isl_assert(set->ctx, dim < set->dim, goto error);
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_fix_dim_si(set->p[i], dim, value);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

struct isl_basic_set *isl_basic_set_lower_bound_dim(struct isl_basic_set *bset,
	unsigned dim, isl_int value)
{
	int j;

	bset = isl_basic_set_cow(bset);
	bset = isl_basic_set_extend(bset, bset->nparam, bset->dim, 0, 0, 1);
	j = isl_basic_set_alloc_inequality(bset);
	if (j < 0)
		goto error;
	isl_seq_clr(bset->ineq[j], 1 + bset->nparam + bset->dim + bset->extra);
	isl_int_set_si(bset->ineq[j][1 + bset->nparam + dim], 1);
	isl_int_neg(bset->ineq[j][0], value);
	bset = isl_basic_set_simplify(bset);
	return isl_basic_set_finalize(bset);
error:
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_set *isl_set_lower_bound_dim(struct isl_set *set, unsigned dim,
					isl_int value)
{
	int i;

	set = isl_set_cow(set);
	if (!set)
		return NULL;

	isl_assert(set->ctx, dim < set->dim, goto error);
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_lower_bound_dim(set->p[i], dim, value);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

struct isl_map *isl_map_reverse(struct isl_map *map)
{
	int i;
	unsigned t;

	map = isl_map_cow(map);
	if (!map)
		return NULL;

	t = map->n_in;
	map->n_in = map->n_out;
	map->n_out = t;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_reverse(map->p[i]);
		if (!map->p[i])
			goto error;
	}
	F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_map *isl_basic_map_lexmax(
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty)
{
	return isl_pip_basic_map_lexmax(bmap, dom, empty);
}

struct isl_map *isl_basic_map_lexmin(
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty)
{
	return isl_pip_basic_map_lexmin(bmap, dom, empty);
}

struct isl_set *isl_basic_set_lexmin(struct isl_basic_set *bset)
{
	struct isl_basic_map *bmap = NULL;
	struct isl_basic_set *dom = NULL;
	struct isl_map *min;

	if (!bset)
		goto error;
	bmap = isl_basic_map_from_basic_set(bset, 0, bset->dim);
	if (!bmap)
		goto error;
	dom = isl_basic_set_alloc(bmap->ctx, bmap->nparam, 0, 0, 0, 0);
	if (!dom)
		goto error;
	min = isl_basic_map_lexmin(bmap, dom, NULL);
	return isl_map_range(min);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_map *isl_basic_map_compute_divs(struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;
	if (bmap->n_div == 0)
		return isl_map_from_basic_map(bmap);
	return isl_pip_basic_map_compute_divs(bmap);
}

struct isl_map *isl_map_compute_divs(struct isl_map *map)
{
	int i;
	struct isl_map *res;

	if (!map)
		return NULL;
	if (map->n == 0)
		return map;
	res = isl_basic_map_compute_divs(isl_basic_map_copy(map->p[0]));
	for (i = 1 ; i < map->n; ++i) {
		struct isl_map *r2;
		r2 = isl_basic_map_compute_divs(isl_basic_map_copy(map->p[i]));
		if (F_ISSET(map, ISL_MAP_DISJOINT))
			res = isl_map_union_disjoint(res, r2);
		else
			res = isl_map_union(res, r2);
	}
	isl_map_free(map);

	return res;
}

struct isl_set *isl_basic_set_compute_divs(struct isl_basic_set *bset)
{
	return (struct isl_set *)
		isl_basic_map_compute_divs((struct isl_basic_map *)bset);
}

struct isl_set *isl_set_compute_divs(struct isl_set *set)
{
	return (struct isl_set *)
		isl_map_compute_divs((struct isl_map *)set);
}

struct isl_set *isl_map_domain(struct isl_map *map)
{
	int i;
	struct isl_set *set;

	if (!map)
		goto error;

	map = isl_map_cow(map);
	if (!map)
		return NULL;

	set = (struct isl_set *)map;
	set->dim = map->n_in;
	set->zero = 0;
	for (i = 0; i < map->n; ++i) {
		set->p[i] = isl_basic_map_domain(map->p[i]);
		if (!set->p[i])
			goto error;
	}
	F_CLR(set, ISL_MAP_DISJOINT);
	F_CLR(set, ISL_SET_NORMALIZED);
	return set;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_map *isl_map_union_disjoint(
			struct isl_map *map1, struct isl_map *map2)
{
	int i;
	unsigned flags = 0;
	struct isl_map *map = NULL;

	if (!map1 || !map2)
		goto error;

	if (map1->n == 0) {
		isl_map_free(map1);
		return map2;
	}
	if (map2->n == 0) {
		isl_map_free(map2);
		return map1;
	}

	isl_assert(map1->ctx, map1->nparam == map2->nparam, goto error);
	isl_assert(map1->ctx, map1->n_in == map2->n_in, goto error);
	isl_assert(map1->ctx, map1->n_out == map2->n_out, goto error);

	if (F_ISSET(map1, ISL_MAP_DISJOINT) &&
	    F_ISSET(map2, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	map = isl_map_alloc(map1->ctx, map1->nparam, map1->n_in, map1->n_out,
				map1->n + map2->n, flags);
	if (!map)
		goto error;
	for (i = 0; i < map1->n; ++i) {
		map = isl_map_add(map,
				  isl_basic_map_copy(map1->p[i]));
		if (!map)
			goto error;
	}
	for (i = 0; i < map2->n; ++i) {
		map = isl_map_add(map,
				  isl_basic_map_copy(map2->p[i]));
		if (!map)
			goto error;
	}
	isl_map_free(map1);
	isl_map_free(map2);
	return map;
error:
	isl_map_free(map);
	isl_map_free(map1);
	isl_map_free(map2);
	return NULL;
}

struct isl_map *isl_map_union(struct isl_map *map1, struct isl_map *map2)
{
	map1 = isl_map_union_disjoint(map1, map2);
	if (!map1)
		return NULL;
	if (map1->n > 1)
		F_CLR(map1, ISL_MAP_DISJOINT);
	return map1;
}

struct isl_set *isl_set_union_disjoint(
			struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_union_disjoint(
			(struct isl_map *)set1, (struct isl_map *)set2);
}

struct isl_set *isl_set_union(struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_union((struct isl_map *)set1, (struct isl_map *)set2);
}

struct isl_map *isl_map_intersect_range(
		struct isl_map *map, struct isl_set *set)
{
	unsigned flags = 0;
	struct isl_map *result;
	int i, j;

	if (!map || !set)
		goto error;

	if (F_ISSET(map, ISL_MAP_DISJOINT) &&
	    F_ISSET(set, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	result = isl_map_alloc(map->ctx, map->nparam, map->n_in, map->n_out,
				map->n * set->n, flags);
	if (!result)
		goto error;
	for (i = 0; i < map->n; ++i)
		for (j = 0; j < set->n; ++j) {
			result = isl_map_add(result,
			    isl_basic_map_intersect_range(
				isl_basic_map_copy(map->p[i]),
				isl_basic_set_copy(set->p[j])));
			if (!result)
				goto error;
		}
	isl_map_free(map);
	isl_set_free(set);
	return result;
error:
	isl_map_free(map);
	isl_set_free(set);
	return NULL;
}

struct isl_map *isl_map_intersect_domain(
		struct isl_map *map, struct isl_set *set)
{
	return isl_map_reverse(
		isl_map_intersect_range(isl_map_reverse(map), set));
}

struct isl_map *isl_map_apply_domain(
		struct isl_map *map1, struct isl_map *map2)
{
	if (!map1 || !map2)
		goto error;
	map1 = isl_map_reverse(map1);
	map1 = isl_map_apply_range(map1, map2);
	return isl_map_reverse(map1);
error:
	isl_map_free(map1);
	isl_map_free(map2);
	return NULL;
}

struct isl_map *isl_map_apply_range(
		struct isl_map *map1, struct isl_map *map2)
{
	struct isl_map *result;
	int i, j;

	if (!map1 || !map2)
		goto error;

	isl_assert(map1->ctx, map1->nparam == map2->nparam, goto error);
	isl_assert(map1->ctx, map1->n_out == map2->n_in, goto error);

	result = isl_map_alloc(map1->ctx, map1->nparam, map1->n_in, map2->n_out,
				map1->n * map2->n, 0);
	if (!result)
		goto error;
	for (i = 0; i < map1->n; ++i)
		for (j = 0; j < map2->n; ++j) {
			result = isl_map_add(result,
			    isl_basic_map_apply_range(
				isl_basic_map_copy(map1->p[i]),
				isl_basic_map_copy(map2->p[j])));
			if (!result)
				goto error;
		}
	isl_map_free(map1);
	isl_map_free(map2);
	if (result && result->n <= 1)
		F_SET(result, ISL_MAP_DISJOINT);
	return result;
error:
	isl_map_free(map1);
	isl_map_free(map2);
	return NULL;
}

/*
 * returns range - domain
 */
struct isl_basic_set *isl_basic_map_deltas(struct isl_basic_map *bmap)
{
	struct isl_basic_set *bset;
	unsigned dim;
	int i;

	if (!bmap)
		goto error;
	isl_assert(bmap->ctx, bmap->n_in == bmap->n_out, goto error);
	dim = bmap->n_in;
	bset = isl_basic_set_from_basic_map(bmap);
	bset = isl_basic_set_extend(bset, bset->nparam, 3*dim, 0,
					dim, 0);
	bset = isl_basic_set_swap_vars(bset, 2*dim);
	for (i = 0; i < dim; ++i) {
		int j = isl_basic_map_alloc_equality(
					    (struct isl_basic_map *)bset);
		if (j < 0)
			goto error;
		isl_seq_clr(bset->eq[j],
			    1 + bset->nparam + bset->dim + bset->extra);
		isl_int_set_si(bset->eq[j][1+bset->nparam+i], 1);
		isl_int_set_si(bset->eq[j][1+bset->nparam+dim+i], 1);
		isl_int_set_si(bset->eq[j][1+bset->nparam+2*dim+i], -1);
	}
	return isl_basic_set_project_out(bset, 2*dim, 0);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

/*
 * returns range - domain
 */
struct isl_set *isl_map_deltas(struct isl_map *map)
{
	int i;
	struct isl_set *result;

	if (!map)
		return NULL;

	isl_assert(map->ctx, map->n_in == map->n_out, goto error);
	result = isl_set_alloc(map->ctx, map->nparam, map->n_in, map->n, map->flags);
	if (!result)
		goto error;
	for (i = 0; i < map->n; ++i)
		result = isl_set_add(result,
			  isl_basic_map_deltas(isl_basic_map_copy(map->p[i])));
	isl_map_free(map);
	return result;
error:
	isl_map_free(map);
	return NULL;
}

/* If the only constraints a div d=floor(f/m)
 * appears in are its two defining constraints
 *
 *	f - m d >=0
 *	-(f - (m - 1)) + m d >= 0
 *
 * then it can safely be removed.
 */
static int div_is_redundant(struct isl_basic_map *bmap, int div)
{
	int i;
	unsigned pos = 1 + bmap->nparam + bmap->n_in + bmap->n_out + div;

	for (i = 0; i < bmap->n_eq; ++i)
		if (!isl_int_is_zero(bmap->eq[i][pos]))
			return 0;

	for (i = 0; i < bmap->n_ineq; ++i) {
		if (isl_int_is_zero(bmap->ineq[i][pos]))
			continue;
		if (isl_int_eq(bmap->ineq[i][pos], bmap->div[div][0])) {
			int neg;
			isl_int_sub(bmap->div[div][1],
					bmap->div[div][1], bmap->div[div][0]);
			isl_int_add_ui(bmap->div[div][1], bmap->div[div][1], 1);
			neg = isl_seq_is_neg(bmap->ineq[i], bmap->div[div]+1, pos);
			isl_int_sub_ui(bmap->div[div][1], bmap->div[div][1], 1);
			isl_int_add(bmap->div[div][1],
					bmap->div[div][1], bmap->div[div][0]);
			if (!neg)
				return 0;
			if (isl_seq_first_non_zero(bmap->ineq[i]+pos+1,
						    bmap->n_div-div-1) != -1)
				return 0;
		} else if (isl_int_abs_eq(bmap->ineq[i][pos], bmap->div[div][0])) {
			if (!isl_seq_eq(bmap->ineq[i], bmap->div[div]+1, pos))
				return 0;
			if (isl_seq_first_non_zero(bmap->ineq[i]+pos+1,
						    bmap->n_div-div-1) != -1)
				return 0;
		} else
			return 0;
	}

	for (i = 0; i < bmap->n_div; ++i)
		if (!isl_int_is_zero(bmap->div[i][1+pos]))
			return 0;

	return 1;
}

/*
 * Remove divs that don't occur in any of the constraints or other divs.
 * These can arise when dropping some of the variables in a quast
 * returned by piplib.
 */
static struct isl_basic_map *remove_redundant_divs(struct isl_basic_map *bmap)
{
	int i;

	if (!bmap)
		return NULL;

	for (i = bmap->n_div-1; i >= 0; --i) {
		if (!div_is_redundant(bmap, i))
			continue;
		bmap = isl_basic_map_drop_div(bmap, i);
	}
	return bmap;
}

struct isl_basic_map *isl_basic_map_finalize(struct isl_basic_map *bmap)
{
	bmap = remove_redundant_divs(bmap);
	if (!bmap)
		return NULL;
	F_SET(bmap, ISL_BASIC_SET_FINAL);
	return bmap;
}

struct isl_basic_set *isl_basic_set_finalize(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_finalize((struct isl_basic_map *)bset);
}

struct isl_set *isl_set_finalize(struct isl_set *set)
{
	int i;

	if (!set)
		return NULL;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_finalize(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

struct isl_map *isl_map_finalize(struct isl_map *map)
{
	int i;

	if (!map)
		return NULL;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_finalize(map->p[i]);
		if (!map->p[i])
			goto error;
	}
	F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_basic_map *isl_basic_map_identity(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim)
{
	struct isl_basic_map *bmap;
	int i;

	bmap = isl_basic_map_alloc(ctx, nparam, dim, dim, 0, dim, 0);
	if (!bmap)
		goto error;

	for (i = 0; i < dim; ++i) {
		int j = isl_basic_map_alloc_equality(bmap);
		if (j < 0)
			goto error;
		isl_seq_clr(bmap->eq[j],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
		isl_int_set_si(bmap->eq[j][1+nparam+i], 1);
		isl_int_set_si(bmap->eq[j][1+nparam+dim+i], -1);
	}
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_map *isl_map_identity(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim)
{
	struct isl_map *map = isl_map_alloc(ctx, nparam, dim, dim, 1,
						ISL_MAP_DISJOINT);
	if (!map)
		goto error;
	map = isl_map_add(map,
		isl_basic_map_identity(ctx, nparam, dim));
	return map;
error:
	isl_map_free(map);
	return NULL;
}

int isl_set_is_equal(struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_is_equal((struct isl_map *)set1, (struct isl_map *)set2);
}

int isl_set_is_subset(struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_is_subset(
			(struct isl_map *)set1, (struct isl_map *)set2);
}

int isl_basic_map_is_subset(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	int is_subset;
	struct isl_map *map1;
	struct isl_map *map2;

	if (!bmap1 || !bmap2)
		return -1;

	map1 = isl_map_from_basic_map(isl_basic_map_copy(bmap1));
	map2 = isl_map_from_basic_map(isl_basic_map_copy(bmap2));

	is_subset = isl_map_is_subset(map1, map2);

	isl_map_free(map1);
	isl_map_free(map2);

	return is_subset;
}

int isl_basic_map_is_equal(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	int is_subset;

	if (!bmap1 || !bmap2)
		return -1;
	is_subset = isl_basic_map_is_subset(bmap1, bmap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_basic_map_is_subset(bmap2, bmap1);
	return is_subset;
}

int isl_basic_set_is_equal(
		struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	return isl_basic_map_is_equal(
		(struct isl_basic_map *)bset1, (struct isl_basic_map *)bset2);
}

int isl_map_is_empty(struct isl_map *map)
{
	int i;
	int is_empty;

	if (!map)
		return -1;
	for (i = 0; i < map->n; ++i) {
		is_empty = isl_basic_map_is_empty(map->p[i]);
		if (is_empty < 0)
			return -1;
		if (!is_empty)
			return 0;
	}
	return 1;
}

int isl_set_is_empty(struct isl_set *set)
{
	return isl_map_is_empty((struct isl_map *)set);
}

int isl_map_is_subset(struct isl_map *map1, struct isl_map *map2)
{
	int i;
	int is_subset = 0;
	struct isl_map *diff;

	if (!map1 || !map2)
		return -1;

	if (isl_map_is_empty(map1))
		return 1;

	if (isl_map_is_empty(map2))
		return 0;

	diff = isl_map_subtract(isl_map_copy(map1), isl_map_copy(map2));
	if (!diff)
		return -1;

	is_subset = isl_map_is_empty(diff);
	isl_map_free(diff);

	return is_subset;
}

int isl_map_is_equal(struct isl_map *map1, struct isl_map *map2)
{
	int is_subset;

	if (!map1 || !map2)
		return -1;
	is_subset = isl_map_is_subset(map1, map2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_map_is_subset(map2, map1);
	return is_subset;
}

int isl_basic_map_is_strict_subset(
		struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	int is_subset;

	if (!bmap1 || !bmap2)
		return -1;
	is_subset = isl_basic_map_is_subset(bmap1, bmap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_basic_map_is_subset(bmap2, bmap1);
	if (is_subset == -1)
		return is_subset;
	return !is_subset;
}

static int basic_map_contains(struct isl_basic_map *bmap, struct isl_vec *vec)
{
	int i;
	unsigned total;
	isl_int s;

	total = 1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	if (total != vec->size)
		return -1;

	isl_int_init(s);

	for (i = 0; i < bmap->n_eq; ++i) {
		isl_seq_inner_product(vec->block.data, bmap->eq[i], total, &s);
		if (!isl_int_is_zero(s)) {
			isl_int_clear(s);
			return 0;
		}
	}

	for (i = 0; i < bmap->n_ineq; ++i) {
		isl_seq_inner_product(vec->block.data, bmap->ineq[i], total, &s);
		if (isl_int_is_neg(s)) {
			isl_int_clear(s);
			return 0;
		}
	}

	isl_int_clear(s);

	return 1;
}

int isl_basic_map_is_universe(struct isl_basic_map *bmap)
{
	if (!bmap)
		return -1;
	return bmap->n_eq == 0 && bmap->n_ineq == 0;
}

int isl_basic_map_is_empty(struct isl_basic_map *bmap)
{
	struct isl_basic_set *bset = NULL;
	struct isl_vec *sample = NULL;
	int empty;
	unsigned total;

	if (!bmap)
		return -1;

	if (F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
		return 1;

	if (F_ISSET(bmap, ISL_BASIC_MAP_RATIONAL)) {
		struct isl_basic_map *copy = isl_basic_map_copy(bmap);
		copy = isl_basic_map_convex_hull(copy);
		empty = F_ISSET(copy, ISL_BASIC_MAP_EMPTY);
		isl_basic_map_free(copy);
		return empty;
	}

	total = 1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	if (bmap->sample && bmap->sample->size == total) {
		int contains = basic_map_contains(bmap, bmap->sample);
		if (contains < 0)
			return -1;
		if (contains)
			return 0;
	}
	bset = isl_basic_map_underlying_set(isl_basic_map_copy(bmap));
	if (!bset)
		return -1;
	sample = isl_basic_set_sample(bset);
	if (!sample)
		return -1;
	empty = sample->size == 0;
	if (bmap->sample)
		isl_vec_free(bmap->ctx, bmap->sample);
	bmap->sample = sample;

	return empty;
}

struct isl_map *isl_basic_map_union(
	struct isl_basic_map *bmap1, struct isl_basic_map *bmap2)
{
	struct isl_map *map;
	if (!bmap1 || !bmap2)
		return NULL;

	isl_assert(bmap1->ctx, bmap1->nparam == bmap2->nparam, goto error);
	isl_assert(bmap1->ctx, bmap1->n_in == bmap2->n_in, goto error);
	isl_assert(bmap1->ctx, bmap1->n_out == bmap2->n_out, goto error);

	map = isl_map_alloc(bmap1->ctx, bmap1->nparam,
				bmap1->n_in, bmap1->n_out, 2, 0);
	if (!map)
		goto error;
	map = isl_map_add(map, bmap1);
	map = isl_map_add(map, bmap2);
	return map;
error:
	isl_basic_map_free(bmap1);
	isl_basic_map_free(bmap2);
	return NULL;
}

struct isl_set *isl_basic_set_union(
		struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	return (struct isl_set *)isl_basic_map_union(
					    (struct isl_basic_map *)bset1,
					    (struct isl_basic_map *)bset2);
}

/* Order divs such that any div only depends on previous divs */
static struct isl_basic_map *order_divs(struct isl_basic_map *bmap)
{
	int i;
	unsigned off = bmap->nparam + bmap->n_in + bmap->n_out;

	for (i = 0; i < bmap->n_div; ++i) {
		int pos;
		pos = isl_seq_first_non_zero(bmap->div[i]+1+1+off+i,
							    bmap->n_div-i);
		if (pos == -1)
			continue;
		swap_div(bmap, i, pos);
		--i;
	}
	return bmap;
}

static int find_div(struct isl_basic_map *dst,
			struct isl_basic_map *src, unsigned div)
{
	int i;

	unsigned total = src->nparam + src->n_in + src->n_out + src->n_div;

	for (i = 0; i < dst->n_div; ++i)
		if (isl_seq_eq(dst->div[i], src->div[div], 1+1+total) &&
		    isl_seq_first_non_zero(dst->div[i]+1+1+total,
						dst->n_div - src->n_div) == -1)
			return i;
	return -1;
}

struct isl_basic_map *isl_basic_map_align_divs(
		struct isl_basic_map *dst, struct isl_basic_map *src)
{
	int i;
	unsigned total = src->nparam + src->n_in + src->n_out + src->n_div;

	if (src->n_div == 0)
		return dst;

	src = order_divs(src);
	dst = isl_basic_map_extend(dst, dst->nparam, dst->n_in,
			dst->n_out, src->n_div, 0, 2 * src->n_div);
	if (!dst)
		return NULL;
	for (i = 0; i < src->n_div; ++i) {
		int j = find_div(dst, src, i);
		if (j < 0) {
			j = isl_basic_map_alloc_div(dst);
			if (j < 0)
				goto error;
			isl_seq_cpy(dst->div[j], src->div[i], 1+1+total);
			isl_seq_clr(dst->div[j]+1+1+total,
						    dst->extra - src->n_div);
			if (add_div_constraints(dst, j) < 0)
				goto error;
		}
		if (j != i)
			swap_div(dst, i, j);
	}
	return dst;
error:
	isl_basic_map_free(dst);
	return NULL;
}

struct isl_map *isl_map_align_divs(struct isl_map *map)
{
	int i;

	map = isl_map_compute_divs(map);
	map = isl_map_cow(map);
	if (!map)
		return NULL;

	for (i = 1; i < map->n; ++i)
		map->p[0] = isl_basic_map_align_divs(map->p[0], map->p[i]);
	for (i = 1; i < map->n; ++i)
		map->p[i] = isl_basic_map_align_divs(map->p[i], map->p[0]);

	F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
}

static struct isl_map *add_cut_constraint(struct isl_map *dst,
		struct isl_basic_map *src, isl_int *c,
		unsigned len, int oppose)
{
	struct isl_basic_map *copy = NULL;
	int is_empty;
	int k;
	unsigned total;

	copy = isl_basic_map_copy(src);
	copy = isl_basic_map_cow(copy);
	if (!copy)
		goto error;
	copy = isl_basic_map_extend(copy,
		copy->nparam, copy->n_in, copy->n_out, 0, 0, 1);
	k = isl_basic_map_alloc_inequality(copy);
	if (k < 0)
		goto error;
	if (oppose)
		isl_seq_neg(copy->ineq[k], c, len);
	else
		isl_seq_cpy(copy->ineq[k], c, len);
	total = 1 + copy->nparam + copy->n_in + copy->n_out + copy->extra;
	isl_seq_clr(copy->ineq[k]+len, total - len);
	isl_inequality_negate(copy, k);
	copy = isl_basic_map_simplify(copy);
	copy = isl_basic_map_finalize(copy);
	is_empty = isl_basic_map_is_empty(copy);
	if (is_empty < 0)
		goto error;
	if (!is_empty)
		dst = isl_map_add(dst, copy);
	else
		isl_basic_map_free(copy);
	return dst;
error:
	isl_basic_map_free(copy);
	isl_map_free(dst);
	return NULL;
}

static struct isl_map *subtract(struct isl_map *map, struct isl_basic_map *bmap)
{
	int i, j, k;
	unsigned flags = 0;
	struct isl_map *rest = NULL;
	unsigned max;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;

	assert(bmap);

	if (!map)
		goto error;

	if (F_ISSET(map, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	max = map->n * (2 * bmap->n_eq + bmap->n_ineq);
	rest = isl_map_alloc(map->ctx, map->nparam, map->n_in, map->n_out,
				max, flags);
	if (!rest)
		goto error;

	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_align_divs(map->p[i], bmap);
		if (!map->p[i])
			goto error;
	}

	for (j = 0; j < map->n; ++j)
		map->p[j] = isl_basic_map_cow(map->p[j]);

	for (i = 0; i < bmap->n_eq; ++i) {
		for (j = 0; j < map->n; ++j) {
			rest = add_cut_constraint(rest,
				map->p[j], bmap->eq[i], 1+total, 0);
			if (!rest)
				goto error;

			rest = add_cut_constraint(rest,
				map->p[j], bmap->eq[i], 1+total, 1);
			if (!rest)
				goto error;

			map->p[j] = isl_basic_map_extend(map->p[j],
				map->p[j]->nparam, map->p[j]->n_in,
				map->p[j]->n_out, 0, 1, 0);
			if (!map->p[j])
				goto error;
			k = isl_basic_map_alloc_equality(map->p[j]);
			if (k < 0)
				goto error;
			isl_seq_cpy(map->p[j]->eq[k], bmap->eq[i], 1+total);
			isl_seq_clr(map->p[j]->eq[k]+1+total,
					map->p[j]->extra - bmap->n_div);
		}
	}

	for (i = 0; i < bmap->n_ineq; ++i) {
		for (j = 0; j < map->n; ++j) {
			rest = add_cut_constraint(rest,
				map->p[j], bmap->ineq[i], 1+total, 0);
			if (!rest)
				goto error;

			map->p[j] = isl_basic_map_extend(map->p[j],
				map->p[j]->nparam, map->p[j]->n_in,
				map->p[j]->n_out, 0, 0, 1);
			if (!map->p[j])
				goto error;
			k = isl_basic_map_alloc_inequality(map->p[j]);
			if (k < 0)
				goto error;
			isl_seq_cpy(map->p[j]->ineq[k], bmap->ineq[i], 1+total);
			isl_seq_clr(map->p[j]->ineq[k]+1+total,
					map->p[j]->extra - bmap->n_div);
		}
	}

	isl_map_free(map);
	return rest;
error:
	isl_map_free(map);
	isl_map_free(rest);
	return NULL;
}

struct isl_map *isl_map_subtract(struct isl_map *map1, struct isl_map *map2)
{
	int i;
	if (!map1 || !map2)
		goto error;

	isl_assert(map1->ctx, map1->nparam == map2->nparam, goto error);
	isl_assert(map1->ctx, map1->n_in == map2->n_in, goto error);
	isl_assert(map1->ctx, map1->n_out == map2->n_out, goto error);

	if (isl_map_is_empty(map2)) {
		isl_map_free(map2);
		return map1;
	}

	map1 = isl_map_compute_divs(map1);
	map2 = isl_map_compute_divs(map2);
	if (!map1 || !map2)
		goto error;

	for (i = 0; map1 && i < map2->n; ++i)
		map1 = subtract(map1, map2->p[i]);

	isl_map_free(map2);
	return map1;
error:
	isl_map_free(map1);
	isl_map_free(map2);
	return NULL;
}

struct isl_set *isl_set_subtract(struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_subtract(
			(struct isl_map *)set1, (struct isl_map *)set2);
}

struct isl_set *isl_set_apply(struct isl_set *set, struct isl_map *map)
{
	if (!set || !map)
		goto error;
	isl_assert(set->ctx, set->dim == map->n_in, goto error);
	map = isl_map_intersect_domain(map, set);
	set = isl_map_range(map);
	return set;
error:
	isl_set_free(set);
	isl_map_free(map);
	return NULL;
}

/* There is no need to cow as removing empty parts doesn't change
 * the meaning of the set.
 */
struct isl_map *isl_map_remove_empty_parts(struct isl_map *map)
{
	int i;

	if (!map)
		return NULL;

	for (i = map->n-1; i >= 0; --i) {
		if (!F_ISSET(map->p[i], ISL_BASIC_MAP_EMPTY))
			continue;
		isl_basic_map_free(map->p[i]);
		if (i != map->n-1) {
			F_CLR(map, ISL_MAP_NORMALIZED);
			map->p[i] = map->p[map->n-1];
		}
		map->n--;
	}

	return map;
}

struct isl_set *isl_set_remove_empty_parts(struct isl_set *set)
{
	return (struct isl_set *)
		isl_map_remove_empty_parts((struct isl_map *)set);
}

struct isl_basic_set *isl_set_copy_basic_set(struct isl_set *set)
{
	struct isl_basic_set *bset;
	if (!set || set->n == 0)
		return NULL;
	bset = set->p[set->n-1];
	isl_assert(set->ctx, F_ISSET(bset, ISL_BASIC_SET_FINAL), return NULL);
	return isl_basic_set_copy(bset);
}

struct isl_set *isl_set_drop_basic_set(struct isl_set *set,
						struct isl_basic_set *bset)
{
	int i;

	if (!set || !bset)
		goto error;
	for (i = set->n-1; i >= 0; --i) {
		if (set->p[i] != bset)
			continue;
		set = isl_set_cow(set);
		if (!set)
			goto error;
		isl_basic_set_free(set->p[i]);
		if (i != set->n-1) {
			F_CLR(set, ISL_SET_NORMALIZED);
			set->p[i] = set->p[set->n-1];
		}
		set->n--;
		return set;
	}
	isl_basic_set_free(bset);
	return set;
error:
	isl_set_free(set);
	isl_basic_set_free(bset);
	return NULL;
}

/* Given two _disjoint_ basic sets bset1 and bset2, check whether
 * for any common value of the parameters and dimensions preceding dim
 * in both basic sets, the values of dimension pos in bset1 are
 * smaller or larger then those in bset2.
 *
 * Returns
 *	 1 if bset1 follows bset2
 *	-1 if bset1 precedes bset2
 *	 0 if bset1 and bset2 are incomparable
 *	-2 if some error occurred.
 */
int isl_basic_set_compare_at(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2, int pos)
{
	struct isl_basic_map *bmap1 = NULL;
	struct isl_basic_map *bmap2 = NULL;
	struct isl_ctx *ctx;
	struct isl_vec *obj;
	unsigned total;
	isl_int num, den;
	enum isl_lp_result res;
	int cmp;

	if (!bset1 || !bset2)
		return -2;

	bmap1 = isl_basic_map_from_basic_set(isl_basic_set_copy(bset1),
						pos, bset1->dim - pos);
	bmap2 = isl_basic_map_from_basic_set(isl_basic_set_copy(bset2),
						pos, bset2->dim - pos);
	if (!bmap1 || !bmap2)
		goto error;
	bmap1 = isl_basic_map_extend(bmap1, bmap1->nparam,
			bmap1->n_in, bmap1->n_out + bmap2->n_out,
			bmap2->n_div, bmap2->n_eq, bmap2->n_ineq);
	if (!bmap1)
		goto error;
	total = bmap1->nparam + bmap1->n_in + bmap1->n_out + bmap1->n_div;
	ctx = bmap1->ctx;
	obj = isl_vec_alloc(ctx, total);
	isl_seq_clr(obj->block.data, total);
	isl_int_set_si(obj->block.data[bmap1->nparam+bmap1->n_in], 1);
	isl_int_set_si(obj->block.data[bmap1->nparam+bmap1->n_in+
					bmap1->n_out-bmap2->n_out], -1);
	if (!obj)
		goto error;
	bmap1 = add_constraints(bmap1, bmap2, 0, bmap1->n_out - bmap2->n_out);
	isl_int_init(num);
	isl_int_init(den);
	res = isl_solve_lp(bmap1, 0, obj->block.data, ctx->one, &num, &den);
	if (res == isl_lp_empty)
		cmp = 0;
	else if (res == isl_lp_ok && isl_int_is_pos(num))
		cmp = 1;
	else if ((res == isl_lp_ok && isl_int_is_neg(num)) ||
		  res == isl_lp_unbounded)
		cmp = -1;
	else
		cmp = -2;
	isl_int_clear(num);
	isl_int_clear(den);
	isl_basic_map_free(bmap1);
	isl_vec_free(ctx, obj);
	return cmp;
error:
	isl_basic_map_free(bmap1);
	isl_basic_map_free(bmap2);
	return -2;
}

static int isl_basic_map_fast_has_fixed_var(struct isl_basic_map *bmap,
	unsigned pos, isl_int *val)
{
	int i;
	int d;
	unsigned total;

	if (!bmap)
		return -1;
	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	for (i = 0, d = total-1; i < bmap->n_eq && d+1 > pos; ++i) {
		for (; d+1 > pos; --d)
			if (!isl_int_is_zero(bmap->eq[i][1+d]))
				break;
		if (d != pos)
			continue;
		if (isl_seq_first_non_zero(bmap->eq[i]+1, d) != -1)
			return 0;
		if (isl_seq_first_non_zero(bmap->eq[i]+1+d+1, total-d-1) != -1)
			return 0;
		if (!isl_int_is_one(bmap->eq[i][1+d]))
			return 0;
		if (val)
			isl_int_neg(*val, bmap->eq[i][0]);
		return 1;
	}
	return 0;
}

static int isl_map_fast_has_fixed_var(struct isl_map *map,
	unsigned pos, isl_int *val)
{
	int i;
	isl_int v;
	isl_int tmp;
	int fixed;

	if (!map)
		return -1;
	if (map->n == 0)
		return 0;
	if (map->n == 1)
		return isl_basic_map_fast_has_fixed_var(map->p[0], pos, val); 
	isl_int_init(v);
	isl_int_init(tmp);
	fixed = isl_basic_map_fast_has_fixed_var(map->p[0], pos, &v); 
	for (i = 1; fixed == 1 && i < map->n; ++i) {
		fixed = isl_basic_map_fast_has_fixed_var(map->p[i], pos, &tmp); 
		if (fixed == 1 && isl_int_ne(tmp, v))
			fixed = 0;
	}
	if (val)
		isl_int_set(*val, v);
	isl_int_clear(tmp);
	isl_int_clear(v);
	return fixed;
}

static int isl_set_fast_has_fixed_var(struct isl_set *set, unsigned pos,
	isl_int *val)
{
	return isl_map_fast_has_fixed_var((struct isl_map *)set, pos, val);
}

/* Check if dimension dim has fixed value and if so and if val is not NULL,
 * then return this fixed value in *val.
 */
int isl_set_fast_dim_is_fixed(struct isl_set *set, unsigned dim, isl_int *val)
{
	return isl_set_fast_has_fixed_var(set, set->nparam + dim, val);
}

/* Check if input variable in has fixed value and if so and if val is not NULL,
 * then return this fixed value in *val.
 */
int isl_map_fast_input_is_fixed(struct isl_map *map, unsigned in, isl_int *val)
{
	return isl_map_fast_has_fixed_var(map, map->nparam + in, val);
}

/* Check if dimension dim has an (obvious) fixed lower bound and if so
 * and if val is not NULL, then return this lower bound in *val.
 */
int isl_basic_set_fast_dim_has_fixed_lower_bound(struct isl_basic_set *bset,
	unsigned dim, isl_int *val)
{
	int i, i_eq = -1, i_ineq = -1;
	isl_int *c;
	unsigned total;

	if (!bset)
		return -1;
	total = bset->nparam + bset->dim + bset->n_div;
	for (i = 0; i < bset->n_eq; ++i) {
		if (isl_int_is_zero(bset->eq[i][1+bset->nparam+dim]))
			continue;
		if (i_eq != -1)
			return 0;
		i_eq = i;
	}
	for (i = 0; i < bset->n_ineq; ++i) {
		if (!isl_int_is_pos(bset->ineq[i][1+bset->nparam+dim]))
			continue;
		if (i_eq != -1 || i_ineq != -1)
			return 0;
		i_ineq = i;
	}
	if (i_eq == -1 && i_ineq == -1)
		return 0;
	c = i_eq != -1 ? bset->eq[i_eq] : bset->ineq[i_ineq];
	/* The coefficient should always be one due to normalization. */
	if (!isl_int_is_one(c[1+bset->nparam+dim]))
		return 0;
	if (isl_seq_first_non_zero(c+1, bset->nparam+dim) != -1)
		return 0;
	if (isl_seq_first_non_zero(c+1+bset->nparam+dim+1,
					total - bset->nparam - dim - 1) != -1)
		return 0;
	if (val)
		isl_int_neg(*val, c[0]);
	return 1;
}

int isl_set_fast_dim_has_fixed_lower_bound(struct isl_set *set,
	unsigned dim, isl_int *val)
{
	int i;
	isl_int v;
	isl_int tmp;
	int fixed;

	if (!set)
		return -1;
	if (set->n == 0)
		return 0;
	if (set->n == 1)
		return isl_basic_set_fast_dim_has_fixed_lower_bound(set->p[0],
								dim, val);
	isl_int_init(v);
	isl_int_init(tmp);
	fixed = isl_basic_set_fast_dim_has_fixed_lower_bound(set->p[0],
								dim, &v);
	for (i = 1; fixed == 1 && i < set->n; ++i) {
		fixed = isl_basic_set_fast_dim_has_fixed_lower_bound(set->p[i],
								dim, &tmp);
		if (fixed == 1 && isl_int_ne(tmp, v))
			fixed = 0;
	}
	if (val)
		isl_int_set(*val, v);
	isl_int_clear(tmp);
	isl_int_clear(v);
	return fixed;
}

/* Remove all information from bset that is redundant in the context
 * of context.  In particular, equalities that are linear combinations
 * of those in context are remobed.  Then the inequalities that are
 * redundant in the context of the equalities and inequalities of
 * context are removed.
 */
static struct isl_basic_set *uset_gist(struct isl_basic_set *bset,
	struct isl_basic_set *context)
{
	int i;
	isl_int opt;
	struct isl_basic_set *combined;
	struct isl_ctx *ctx;
	enum isl_lp_result res = isl_lp_ok;

	if (!bset || !context)
		goto error;

	if (context->n_eq > 0) {
		struct isl_mat *T;
		struct isl_mat *T2;
		struct isl_ctx *ctx = context->ctx;
		context = isl_basic_set_remove_equalities(context, &T, &T2);
		if (!context)
			goto error;
		bset = isl_basic_set_preimage(ctx, bset, T);
		bset = uset_gist(bset, context);
		bset = isl_basic_set_preimage(ctx, bset, T2);
		return bset;
	}
	combined = isl_basic_set_extend(isl_basic_set_copy(bset),
			0, bset->dim, 0, context->n_eq, context->n_ineq);
	context = set_add_constraints(combined, context, 0);
	if (!context)
		goto error;
	ctx = context->ctx;
	isl_int_init(opt);
	for (i = bset->n_ineq-1; i >= 0; --i) {
		set_swap_inequality(context, i, context->n_ineq-1);
		context->n_ineq--;
		res = isl_solve_lp((struct isl_basic_map *)context, 0,
			context->ineq[context->n_ineq]+1, ctx->one, &opt, NULL);
		context->n_ineq++;
		set_swap_inequality(context, i, context->n_ineq-1);
		if (res == isl_lp_unbounded)
			continue;
		if (res == isl_lp_error)
			break;
		if (res == isl_lp_empty) {
			bset = isl_basic_set_set_to_empty(bset);
			break;
		}
		isl_int_add(opt, opt, context->ineq[i][0]);
		if (!isl_int_is_neg(opt)) {
			isl_basic_set_drop_inequality(context, i);
			isl_basic_set_drop_inequality(bset, i);
		}
	}
	isl_int_clear(opt);
	if (res == isl_lp_error)
		goto error;
	isl_basic_set_free(context);
	return bset;
error:
	isl_basic_set_free(context);
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_gist(struct isl_basic_map *bmap,
	struct isl_basic_map *context)
{
	struct isl_basic_set *bset;

	if (!bmap || !context)
		goto error;

	context = isl_basic_map_align_divs(context, bmap);
	bmap = isl_basic_map_align_divs(bmap, context);

	bset = uset_gist(isl_basic_map_underlying_set(isl_basic_map_copy(bmap)),
			 isl_basic_map_underlying_set(context));

	return isl_basic_map_overlying_set(bset, bmap);
error:
	isl_basic_map_free(bmap);
	isl_basic_map_free(context);
	return NULL;
}

struct isl_map *isl_map_gist(struct isl_map *map, struct isl_basic_map *context)
{
	int i;

	map = isl_map_cow(map);
	if (!map || !context)
		return NULL;
	isl_assert(map->ctx, map->nparam == context->nparam, goto error);
	isl_assert(map->ctx, map->n_in == context->n_in, goto error);
	isl_assert(map->ctx, map->n_out == context->n_out, goto error);
	for (i = 0; i < map->n; ++i)
		context = isl_basic_map_align_divs(context, map->p[i]);
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_gist(map->p[i],
						isl_basic_map_copy(context));
		if (!map->p[i])
			goto error;
	}
	isl_basic_map_free(context);
	F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	isl_basic_map_free(context);
	return NULL;
}

struct isl_set *isl_set_gist(struct isl_set *set, struct isl_basic_set *context)
{
	return (struct isl_set *)isl_map_gist((struct isl_map *)set,
					(struct isl_basic_map *)context);
}

struct constraint {
	unsigned	size;
	isl_int		*c;
};

static int qsort_constraint_cmp(const void *p1, const void *p2)
{
	const struct constraint *c1 = (const struct constraint *)p1;
	const struct constraint *c2 = (const struct constraint *)p2;
	unsigned size = isl_min(c1->size, c2->size);
	return isl_seq_cmp(c1->c, c2->c, size);
}

static struct isl_basic_map *isl_basic_map_sort_constraints(
	struct isl_basic_map *bmap)
{
	int i;
	struct constraint *c;
	unsigned total;

	if (!bmap)
		return NULL;
	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	c = isl_alloc_array(bmap->ctx, struct constraint, bmap->n_ineq);
	if (!c)
		goto error;
	for (i = 0; i < bmap->n_ineq; ++i) {
		c[i].size = total;
		c[i].c = bmap->ineq[i];
	}
	qsort(c, bmap->n_ineq, sizeof(struct constraint), qsort_constraint_cmp);
	for (i = 0; i < bmap->n_ineq; ++i)
		bmap->ineq[i] = c[i].c;
	free(c);
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_normalize(struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;
	if (F_ISSET(bmap, ISL_BASIC_MAP_NORMALIZED))
		return bmap;
	bmap = isl_basic_map_convex_hull(bmap);
	bmap = isl_basic_map_sort_constraints(bmap);
	F_SET(bmap, ISL_BASIC_MAP_NORMALIZED);
	return bmap;
}

static int isl_basic_map_fast_cmp(const struct isl_basic_map *bmap1,
	const struct isl_basic_map *bmap2)
{
	int i, cmp;
	unsigned total;

	if (bmap1 == bmap2)
		return 0;
	if (bmap1->nparam != bmap2->nparam)
		return bmap1->nparam - bmap2->nparam;
	if (bmap1->n_in != bmap2->n_in)
		return bmap1->n_in - bmap2->n_in;
	if (bmap1->n_out != bmap2->n_out)
		return bmap1->n_out - bmap2->n_out;
	if (F_ISSET(bmap1, ISL_BASIC_MAP_EMPTY) &&
	    F_ISSET(bmap2, ISL_BASIC_MAP_EMPTY))
		return 0;
	if (F_ISSET(bmap1, ISL_BASIC_MAP_EMPTY))
		return 1;
	if (F_ISSET(bmap2, ISL_BASIC_MAP_EMPTY))
		return -1;
	if (bmap1->n_eq != bmap2->n_eq)
		return bmap1->n_eq - bmap2->n_eq;
	if (bmap1->n_ineq != bmap2->n_ineq)
		return bmap1->n_ineq - bmap2->n_ineq;
	if (bmap1->n_div != bmap2->n_div)
		return bmap1->n_div - bmap2->n_div;
	total = bmap1->nparam + bmap1->n_in + bmap1->n_out + bmap1->n_div;
	for (i = 0; i < bmap1->n_eq; ++i) {
		cmp = isl_seq_cmp(bmap1->eq[i], bmap2->eq[i], 1+total);
		if (cmp)
			return cmp;
	}
	for (i = 0; i < bmap1->n_ineq; ++i) {
		cmp = isl_seq_cmp(bmap1->ineq[i], bmap2->ineq[i], 1+total);
		if (cmp)
			return cmp;
	}
	for (i = 0; i < bmap1->n_div; ++i) {
		cmp = isl_seq_cmp(bmap1->div[i], bmap2->div[i], 1+1+total);
		if (cmp)
			return cmp;
	}
	return 0;
}

static int isl_basic_map_fast_is_equal(struct isl_basic_map *bmap1,
	struct isl_basic_map *bmap2)
{
	return isl_basic_map_fast_cmp(bmap1, bmap2) == 0;
}

static int qsort_bmap_cmp(const void *p1, const void *p2)
{
	const struct isl_basic_map *bmap1 = *(const struct isl_basic_map **)p1;
	const struct isl_basic_map *bmap2 = *(const struct isl_basic_map **)p2;

	return isl_basic_map_fast_cmp(bmap1, bmap2);
}

/* We normalize in place, but if anything goes wrong we need
 * to return NULL, so we need to make sure we don't change the
 * meaning of any possible other copies of map.
 */
struct isl_map *isl_map_normalize(struct isl_map *map)
{
	int i, j;
	struct isl_basic_map *bmap;

	if (!map)
		return NULL;
	if (F_ISSET(map, ISL_MAP_NORMALIZED))
		return map;
	for (i = 0; i < map->n; ++i) {
		bmap = isl_basic_map_normalize(isl_basic_map_copy(map->p[i]));
		if (!bmap)
			goto error;
		isl_basic_map_free(map->p[i]);
		map->p[i] = bmap;
	}
	qsort(map->p, map->n, sizeof(struct isl_basic_map *), qsort_bmap_cmp);
	F_SET(map, ISL_MAP_NORMALIZED);
	map = isl_map_remove_empty_parts(map);
	if (!map)
		return NULL;
	for (i = map->n - 1; i >= 1; --i) {
		if (!isl_basic_map_fast_is_equal(map->p[i-1], map->p[i]))
			continue;
		isl_basic_map_free(map->p[i-1]);
		for (j = i; j < map->n; ++j)
			map->p[j-1] = map->p[j];
		map->n--;
	}
	return map;
error:
	isl_map_free(map);
	return NULL;

}

struct isl_set *isl_set_normalize(struct isl_set *set)
{
	return (struct isl_set *)isl_map_normalize((struct isl_map *)set);
}

int isl_map_fast_is_equal(struct isl_map *map1, struct isl_map *map2)
{
	int i;
	int equal;

	if (!map1 || !map2)
		return -1;

	if (map1 == map2)
		return 1;
	if (map1->nparam != map2->nparam)
		return 0;
	if (map1->n_in != map2->n_in)
		return 0;
	if (map1->n_out != map2->n_out)
		return 0;

	map1 = isl_map_copy(map1);
	map2 = isl_map_copy(map2);
	map1 = isl_map_normalize(map1);
	map2 = isl_map_normalize(map2);
	if (!map1 || !map2)
		goto error;
	equal = map1->n == map2->n;
	for (i = 0; equal && i < map1->n; ++i) {
		equal = isl_basic_map_fast_is_equal(map1->p[i], map2->p[i]);
		if (equal < 0)
			goto error;
	}
	isl_map_free(map1);
	isl_map_free(map2);
	return equal;
error:
	isl_map_free(map1);
	isl_map_free(map2);
	return -1;
}

int isl_set_fast_is_equal(struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_fast_is_equal((struct isl_map *)set1,
						(struct isl_map *)set2);
}

/* Return an interval that ranges from min to max (inclusive)
 */
struct isl_basic_set *isl_basic_set_interval(struct isl_ctx *ctx,
	isl_int min, isl_int max)
{
	int k;
	struct isl_basic_set *bset = NULL;

	bset = isl_basic_set_alloc(ctx, 0, 1, 0, 0, 2);
	if (!bset)
		goto error;

	k = isl_basic_set_alloc_inequality(bset);
	if (k < 0)
		goto error;
	isl_int_set_si(bset->ineq[k][1], 1);
	isl_int_neg(bset->ineq[k][0], min);

	k = isl_basic_set_alloc_inequality(bset);
	if (k < 0)
		goto error;
	isl_int_set_si(bset->ineq[k][1], -1);
	isl_int_set(bset->ineq[k][0], max);

	return bset;
error:
	isl_basic_set_free(bset);
	return NULL;
}

/* Return the Cartesian product of the basic sets in list (in the given order).
 */
struct isl_basic_set *isl_basic_set_product(struct isl_basic_set_list *list)
{
	int i;
	unsigned dim;
	unsigned nparam;
	unsigned extra;
	unsigned n_eq;
	unsigned n_ineq;
	struct isl_basic_set *product = NULL;

	if (!list)
		goto error;
	isl_assert(list->ctx, list->n > 0, goto error);
	isl_assert(list->ctx, list->p[0], goto error);
	nparam = list->p[0]->nparam;
	dim = list->p[0]->dim;
	extra = list->p[0]->n_div;
	n_eq = list->p[0]->n_eq;
	n_ineq = list->p[0]->n_ineq;
	for (i = 1; i < list->n; ++i) {
		isl_assert(list->ctx, list->p[i], goto error);
		isl_assert(list->ctx, nparam == list->p[i]->nparam, goto error);
		dim += list->p[i]->dim;
		extra += list->p[i]->n_div;
		n_eq += list->p[i]->n_eq;
		n_ineq += list->p[i]->n_ineq;
	}
	product = isl_basic_set_alloc(list->ctx, nparam, dim, extra,
					n_eq, n_ineq);
	if (!product)
		goto error;
	dim = 0;
	for (i = 0; i < list->n; ++i) {
		set_add_constraints(product,
					isl_basic_set_copy(list->p[i]), dim);
		dim += list->p[i]->dim;
	}
	isl_basic_set_list_free(list);
	return product;
error:
	isl_basic_set_free(product);
	isl_basic_set_list_free(list);
	return NULL;
}
