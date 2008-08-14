#include <string.h>
#include <strings.h>
#include "isl_ctx.h"
#include "isl_blk.h"
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

static void dup_constraints(struct isl_ctx *ctx,
		struct isl_basic_map *dst, struct isl_basic_map *src)
{
	int i;
	unsigned total = src->nparam + src->n_in + src->n_out + src->n_div;

	for (i = 0; i < src->n_eq; ++i) {
		int j = isl_basic_map_alloc_equality(ctx, dst);
		isl_seq_cpy(dst->eq[j], src->eq[i], 1+total);
	}

	for (i = 0; i < src->n_ineq; ++i) {
		int j = isl_basic_map_alloc_inequality(ctx, dst);
		isl_seq_cpy(dst->ineq[j], src->ineq[i], 1+total);
	}

	for (i = 0; i < src->n_div; ++i) {
		int j = isl_basic_map_alloc_div(ctx, dst);
		isl_seq_cpy(dst->div[j], src->div[i], 1+1+total);
	}
	F_SET(dst, ISL_BASIC_SET_FINAL);
}

struct isl_basic_map *isl_basic_map_dup(struct isl_ctx *ctx,
					struct isl_basic_map *bmap)
{
	struct isl_basic_map *dup;

	dup = isl_basic_map_alloc(ctx, bmap->nparam,
			bmap->n_in, bmap->n_out,
			bmap->n_div, bmap->n_eq, bmap->n_ineq);
	if (!dup)
		return NULL;
	dup_constraints(ctx, dup, bmap);
	dup->sample = isl_vec_copy(ctx, bmap->sample);
	return dup;
}

struct isl_basic_set *isl_basic_set_dup(struct isl_ctx *ctx,
					struct isl_basic_set *bset)
{
	struct isl_basic_map *dup;

	dup = isl_basic_map_dup(ctx, (struct isl_basic_map *)bset);
	return (struct isl_basic_set *)dup;
}

struct isl_basic_set *isl_basic_set_copy(struct isl_ctx *ctx,
					struct isl_basic_set *bset)
{
	if (!bset)
		return NULL;

	if (F_ISSET(bset, ISL_BASIC_SET_FINAL)) {
		bset->ref++;
		return bset;
	}
	return isl_basic_set_dup(ctx, bset);
}

struct isl_set *isl_set_copy(struct isl_ctx *ctx, struct isl_set *set)
{
	if (!set)
		return NULL;

	set->ref++;
	return set;
}

struct isl_basic_map *isl_basic_map_copy(struct isl_ctx *ctx,
					struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;

	if (F_ISSET(bmap, ISL_BASIC_SET_FINAL)) {
		bmap->ref++;
		return bmap;
	}
	return isl_basic_map_dup(ctx, bmap);
}

struct isl_map *isl_map_copy(struct isl_ctx *ctx, struct isl_map *map)
{
	if (!map)
		return NULL;

	map->ref++;
	return map;
}

void isl_basic_map_free(struct isl_ctx *ctx, struct isl_basic_map *bmap)
{
	if (!bmap)
		return;

	if (--bmap->ref > 0)
		return;

	free(bmap->div);
	isl_blk_free(ctx, bmap->block2);
	free(bmap->eq);
	isl_blk_free(ctx, bmap->block);
	isl_vec_free(ctx, bmap->sample);
	free(bmap);
}

void isl_basic_set_free(struct isl_ctx *ctx, struct isl_basic_set *bset)
{
	isl_basic_map_free(ctx, (struct isl_basic_map *)bset);
}

int isl_basic_map_alloc_equality(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	if (!bmap)
		return -1;
	isl_assert(ctx, bmap->n_eq + bmap->n_ineq < bmap->c_size, return -1);
	isl_assert(ctx, bmap->eq + bmap->n_eq <= bmap->ineq, return -1);
	if (bmap->eq + bmap->n_eq == bmap->ineq) {
		isl_int *t;
		int j = isl_basic_map_alloc_inequality(ctx, bmap);
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

int isl_basic_set_alloc_equality(struct isl_ctx *ctx,
		struct isl_basic_set *bset)
{
	return isl_basic_map_alloc_equality(ctx, (struct isl_basic_map *)bset);
}

int isl_basic_map_free_equality(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned n)
{
	isl_assert(ctx, n <= bmap->n_eq, return -1);
	bmap->n_eq -= n;
	return 0;
}

int isl_basic_map_drop_equality(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	isl_int *t;
	isl_assert(ctx, pos < bmap->n_eq, return -1);

	if (pos != bmap->n_eq - 1) {
		t = bmap->eq[pos];
		bmap->eq[pos] = bmap->eq[bmap->n_eq - 1];
		bmap->eq[bmap->n_eq - 1] = t;
	}
	bmap->n_eq--;
	return 0;
}

void isl_basic_map_inequality_to_equality(struct isl_ctx *ctx,
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
}

int isl_basic_map_alloc_inequality(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	isl_assert(ctx, (bmap->ineq - bmap->eq) + bmap->n_ineq < bmap->c_size,
			return -1);
	F_CLR(bmap, ISL_BASIC_MAP_NO_IMPLICIT);
	return bmap->n_ineq++;
}

int isl_basic_set_alloc_inequality(struct isl_ctx *ctx,
		struct isl_basic_set *bset)
{
	return isl_basic_map_alloc_inequality(ctx, (struct isl_basic_map *)bset);
}

int isl_basic_map_free_inequality(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned n)
{
	isl_assert(ctx, n <= bmap->n_ineq, return -1);
	bmap->n_ineq -= n;
	return 0;
}

int isl_basic_map_drop_inequality(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	isl_int *t;
	isl_assert(ctx, pos < bmap->n_ineq, return -1);

	if (pos != bmap->n_ineq - 1) {
		t = bmap->ineq[pos];
		bmap->ineq[pos] = bmap->ineq[bmap->n_ineq - 1];
		bmap->ineq[bmap->n_ineq - 1] = t;
	}
	bmap->n_ineq--;
	return 0;
}

int isl_basic_map_alloc_div(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	isl_assert(ctx, bmap->n_div < bmap->extra, return -1);
	return bmap->n_div++;
}

int isl_basic_map_free_div(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned n)
{
	isl_assert(ctx, n <= bmap->n_div, return -1);
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

static struct isl_basic_map *add_constraints(struct isl_ctx *ctx,
		struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2, unsigned i_pos, unsigned o_pos)
{
	int i;
	unsigned div_off = bmap1->n_div;

	for (i = 0; i < bmap2->n_eq; ++i) {
		int i1 = isl_basic_map_alloc_equality(ctx, bmap1);
		if (i1 < 0)
			goto error;
		copy_constraint(bmap1, bmap1->eq[i1], bmap2, bmap2->eq[i],
				i_pos, o_pos, div_off);
	}

	for (i = 0; i < bmap2->n_ineq; ++i) {
		int i1 = isl_basic_map_alloc_inequality(ctx, bmap1);
		if (i1 < 0)
			goto error;
		copy_constraint(bmap1, bmap1->ineq[i1], bmap2, bmap2->ineq[i],
				i_pos, o_pos, div_off);
	}

	for (i = 0; i < bmap2->n_div; ++i) {
		int i1 = isl_basic_map_alloc_div(ctx, bmap1);
		if (i1 < 0)
			goto error;
		copy_div(bmap1, bmap1->div[i1], bmap2, bmap2->div[i],
			 i_pos, o_pos, div_off);
	}

	isl_basic_map_free(ctx, bmap2);

	return bmap1;

error:
	isl_basic_map_free(ctx, bmap1);
	isl_basic_map_free(ctx, bmap2);
	return NULL;
}

struct isl_basic_map *isl_basic_map_extend(struct isl_ctx *ctx,
		struct isl_basic_map *base,
		unsigned nparam, unsigned n_in, unsigned n_out, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	struct isl_basic_map *ext;
	int dims_ok;

	base = isl_basic_map_cow(ctx, base);
	if (!base)
		goto error;

	dims_ok = base && base->nparam == nparam &&
		  base->n_in == n_in && base->n_out == n_out &&
		  base->extra >= base->n_div + extra;

	if (dims_ok && n_eq == 0 && n_ineq == 0)
		return base;

	if (base) {
		isl_assert(ctx, base->nparam <= nparam, goto error);
		isl_assert(ctx, base->n_in <= n_in, goto error);
		isl_assert(ctx, base->n_out <= n_out, goto error);
		extra += base->extra;
		n_eq += base->n_eq;
		n_ineq += base->n_ineq;
	}

	ext = isl_basic_map_alloc(ctx, nparam, n_in, n_out,
					extra, n_eq, n_ineq);
	if (!base)
		return ext;
	if (!ext)
		goto error;

	ext = add_constraints(ctx, ext, base, 0, 0);

	return ext;

error:
	isl_basic_map_free(ctx, base);
	return NULL;
}

struct isl_basic_set *isl_basic_set_extend(struct isl_ctx *ctx,
		struct isl_basic_set *base,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq)
{
	return (struct isl_basic_set *)
		isl_basic_map_extend(ctx, (struct isl_basic_map *)base,
					nparam, 0, dim, extra, n_eq, n_ineq);
}

struct isl_basic_set *isl_basic_set_cow(struct isl_ctx *ctx,
		struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_cow(ctx, (struct isl_basic_map *)bset);
}

struct isl_basic_map *isl_basic_map_cow(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;

	if (bmap->ref > 1) {
		bmap->ref--;
		bmap = isl_basic_map_dup(ctx, bmap);
	}
	F_CLR(bmap, ISL_BASIC_SET_FINAL);
	return bmap;
}

static struct isl_set *isl_set_cow(struct isl_ctx *ctx, struct isl_set *set)
{
	if (!set)
		return NULL;

	if (set->ref == 1)
		return set;
	set->ref--;
	return isl_set_dup(ctx, set);
}

struct isl_map *isl_map_cow(struct isl_ctx *ctx, struct isl_map *map)
{
	if (!map)
		return NULL;

	if (map->ref == 1)
		return map;
	map->ref--;
	return isl_map_dup(ctx, map);
}

static void swap_vars(struct isl_blk blk, isl_int *a,
			unsigned a_len, unsigned b_len)
{
	isl_seq_cpy(blk.data, a+a_len, b_len);
	isl_seq_cpy(blk.data+b_len, a, a_len);
	isl_seq_cpy(a, blk.data, b_len+a_len);
}

struct isl_basic_set *isl_basic_set_swap_vars(struct isl_ctx *ctx,
		struct isl_basic_set *bset, unsigned n)
{
	int i;
	struct isl_blk blk;

	if (!bset)
		goto error;

	isl_assert(ctx, n <= bset->dim, goto error);

	if (n == bset->dim)
		return bset;

	bset = isl_basic_set_cow(ctx, bset);
	if (!bset)
		return NULL;

	blk = isl_blk_alloc(ctx, bset->dim);
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

	isl_blk_free(ctx, blk);

	return bset;

error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}

struct isl_set *isl_set_swap_vars(struct isl_ctx *ctx,
		struct isl_set *set, unsigned n)
{
	int i;
	set = isl_set_cow(ctx, set);
	if (!set)
		return NULL;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_swap_vars(ctx, set->p[i], n);
		if (!set->p[i]) {
			isl_set_free(ctx, set);
			return NULL;
		}
	}
	return set;
}

static void constraint_drop_vars(isl_int *c, unsigned n, unsigned rem)
{
	isl_seq_cpy(c, c + n, rem);
	isl_seq_clr(c + rem, n);
}

struct isl_basic_set *isl_basic_set_drop_vars(struct isl_ctx *ctx,
		struct isl_basic_set *bset, unsigned first, unsigned n)
{
	int i;

	if (!bset)
		goto error;

	isl_assert(ctx, first + n <= bset->dim, goto error);

	if (n == 0)
		return bset;

	bset = isl_basic_set_cow(ctx, bset);
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

	return isl_basic_set_finalize(ctx, bset);
error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}

/*
 * We don't cow, as the div is assumed to be redundant.
 */
static struct isl_basic_map *isl_basic_map_drop_div(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned div)
{
	int i;
	unsigned pos;

	if (!bmap)
		goto error;

	pos = 1 + bmap->nparam + bmap->n_in + bmap->n_out + div;

	isl_assert(ctx, div < bmap->n_div, goto error);

	for (i = 0; i < bmap->n_eq; ++i)
		constraint_drop_vars(bmap->eq[i]+pos, 1, bmap->extra-div-1);

	for (i = 0; i < bmap->n_ineq; ++i) {
		if (!isl_int_is_zero(bmap->ineq[i][pos])) {
			isl_basic_map_drop_inequality(ctx, bmap, i);
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
	isl_basic_map_free_div(ctx, bmap, 1);

	return bmap;
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

static void eliminate_div(struct isl_ctx *ctx, struct isl_basic_map *bmap,
				isl_int *eq, unsigned div)
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

	isl_basic_map_drop_div(ctx, bmap, div);
}

struct isl_basic_map *isl_basic_map_set_to_empty(
		struct isl_ctx *ctx, struct isl_basic_map *bmap)
{
	int i = 0;
	unsigned total;
	if (!bmap)
		goto error;
	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra;
	isl_basic_map_free_div(ctx, bmap, bmap->n_div);
	isl_basic_map_free_inequality(ctx, bmap, bmap->n_ineq);
	if (bmap->n_eq > 0)
		isl_basic_map_free_equality(ctx, bmap, bmap->n_eq-1);
	else {
		isl_basic_map_alloc_equality(ctx, bmap);
		if (i < 0)
			goto error;
	}
	isl_int_set_si(bmap->eq[i][0], 1);
	isl_seq_clr(bmap->eq[i]+1, total);
	F_SET(bmap, ISL_BASIC_MAP_EMPTY);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_set_to_empty(
		struct isl_ctx *ctx, struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_set_to_empty(ctx, (struct isl_basic_map *)bset);
}

static void swap_equality(struct isl_basic_map *bmap, int a, int b)
{
	isl_int *t = bmap->eq[a];
	bmap->eq[a] = bmap->eq[b];
	bmap->eq[b] = t;
}

static void swap_inequality(struct isl_basic_map *bmap, int a, int b)
{
	isl_int *t = bmap->ineq[a];
	bmap->ineq[a] = bmap->ineq[b];
	bmap->ineq[b] = t;
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
}

struct isl_basic_map *isl_basic_map_gauss(struct isl_ctx *ctx,
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

		for (k = 0; k < bmap->n_eq; ++k) {
			if (k == done)
				continue;
			if (isl_int_is_zero(bmap->eq[k][1+last_var]))
				continue;
			if (progress)
				*progress = 1;
			isl_seq_elim(bmap->eq[k], bmap->eq[done],
					1+last_var, 1+total, NULL);
		}

		for (k = 0; k < bmap->n_ineq; ++k) {
			if (isl_int_is_zero(bmap->ineq[k][1+last_var]))
				continue;
			if (progress)
				*progress = 1;
			isl_seq_elim(bmap->ineq[k], bmap->eq[done],
					1+last_var, 1+total, NULL);
		}

		for (k = 0; k < bmap->n_div; ++k) {
			if (isl_int_is_zero(bmap->div[k][0]))
				continue;
			if (isl_int_is_zero(bmap->div[k][1+1+last_var]))
				continue;
			if (progress)
				*progress = 1;
			isl_seq_elim(bmap->div[k]+1, bmap->eq[done],
					1+last_var, 1+total, &bmap->div[k][0]);
		}

		if (last_var >= total_var &&
		    isl_int_is_zero(bmap->div[last_var - total_var][0])) {
			unsigned div = last_var - total_var;
			isl_seq_neg(bmap->div[div]+1, bmap->eq[done], 1+total);
			isl_int_set_si(bmap->div[div][1+1+last_var], 0);
			isl_int_set(bmap->div[div][0],
				    bmap->eq[done][1+last_var]);
		}
	}
	if (done == bmap->n_eq)
		return bmap;
	for (k = done; k < bmap->n_eq; ++k) {
		if (isl_int_is_zero(bmap->eq[k][0]))
			continue;
		return isl_basic_map_set_to_empty(ctx, bmap);
	}
	isl_basic_map_free_equality(ctx, bmap, bmap->n_eq-done);
	return bmap;
}

struct isl_basic_set *isl_basic_set_gauss(struct isl_ctx *ctx,
	struct isl_basic_set *bset, int *progress)
{
	return (struct isl_basic_set*)isl_basic_map_gauss(ctx,
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
	struct isl_ctx *ctx,
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
		*progress = 1;
		l = index[h] - 1;
		if (isl_int_lt(bmap->ineq[k][0], bmap->ineq[l][0]))
			swap_inequality(bmap, k, l);
		isl_basic_map_drop_inequality(ctx, bmap, k);
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
			isl_basic_map_drop_inequality(ctx, bmap, l);
			isl_basic_map_inequality_to_equality(ctx, bmap, k);
		} else
			bmap = isl_basic_map_set_to_empty(ctx, bmap);
		break;
	}
	isl_int_clear(sum);

	free(index);
	return bmap;
}

static struct isl_basic_map *remove_duplicate_divs(struct isl_ctx *ctx,
	struct isl_basic_map *bmap, int *progress)
{
	unsigned int size;
	int *index;
	int k, l, h;
	int bits;
	struct isl_blk eq;
	unsigned total_var = bmap->nparam + bmap->n_in + bmap->n_out;
	unsigned total = total_var + bmap->n_div;

	if (bmap->n_div <= 1)
		return bmap;

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
			eliminate_div(ctx, bmap, eq.data, l);
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

static struct isl_basic_map *eliminate_divs(
		struct isl_ctx *ctx, struct isl_basic_map *bmap,
		int *progress)
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
			eliminate_div(ctx, bmap, bmap->eq[i], d);
			isl_basic_map_drop_equality(ctx, bmap, i);
		}
	}
	if (modified)
		return eliminate_divs(ctx, bmap, progress);
	return bmap;
}

static struct isl_basic_map *normalize_constraints(
		struct isl_ctx *ctx, struct isl_basic_map *bmap)
{
	int i;
	isl_int gcd;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;

	isl_int_init(gcd);
	for (i = bmap->n_eq - 1; i >= 0; --i) {
		isl_seq_gcd(bmap->eq[i]+1, total, &gcd);
		if (isl_int_is_zero(gcd)) {
			if (!isl_int_is_zero(bmap->eq[i][0])) {
				bmap = isl_basic_map_set_to_empty(ctx, bmap);
				break;
			}
			isl_basic_map_drop_equality(ctx, bmap, i);
			continue;
		}
		if (isl_int_is_one(gcd))
			continue;
		if (!isl_int_is_divisible_by(bmap->eq[i][0], gcd)) {
			bmap = isl_basic_map_set_to_empty(ctx, bmap);
			break;
		}
		isl_seq_scale_down(bmap->eq[i], bmap->eq[i], gcd, 1+total);
	}

	for (i = bmap->n_ineq - 1; i >= 0; --i) {
		isl_seq_gcd(bmap->ineq[i]+1, total, &gcd);
		if (isl_int_is_zero(gcd)) {
			if (isl_int_is_neg(bmap->ineq[i][0])) {
				bmap = isl_basic_map_set_to_empty(ctx, bmap);
				break;
			}
			isl_basic_map_drop_inequality(ctx, bmap, i);
			continue;
		}
		if (isl_int_is_one(gcd))
			continue;
		isl_int_fdiv_q(bmap->ineq[i][0], bmap->ineq[i][0], gcd);
		isl_seq_scale_down(bmap->ineq[i]+1, bmap->ineq[i]+1, gcd, total);
	}
	isl_int_clear(gcd);

	return bmap;
}

struct isl_basic_map *isl_basic_map_simplify(
		struct isl_ctx *ctx, struct isl_basic_map *bmap)
{
	int progress = 1;
	if (!bmap)
		return NULL;
	while (progress) {
		progress = 0;
		bmap = normalize_constraints(ctx, bmap);
		bmap = eliminate_divs(ctx, bmap, &progress);
		bmap = isl_basic_map_gauss(ctx, bmap, &progress);
		bmap = remove_duplicate_divs(ctx, bmap, &progress);
		bmap = remove_duplicate_constraints(ctx, bmap, &progress);
	}
	return bmap;
}

struct isl_basic_set *isl_basic_set_simplify(
		struct isl_ctx *ctx, struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_simplify(ctx,
			(struct isl_basic_map *)bset);
}

static void dump_term(struct isl_basic_map *bmap,
			isl_int c, int pos, FILE *out)
{
	unsigned in = bmap->n_in;
	unsigned dim = bmap->n_in + bmap->n_out;
	if (!pos)
		isl_int_print(out, c);
	else {
		if (!isl_int_is_one(c))
			isl_int_print(out, c);
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
		isl_int_print(out, bmap->div[i][0]);
		fprintf(out, "]\n");
	}
}

void isl_basic_set_dump(struct isl_ctx *ctx, struct isl_basic_set *bset,
				FILE *out, int indent)
{
	if (!bset) {
		fprintf(out, "null basic set\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "nparam: %d, dim: %d, extra: %d\n",
			bset->nparam, bset->dim, bset->extra);
	dump((struct isl_basic_map *)bset, out, indent);
}

void isl_basic_map_dump(struct isl_ctx *ctx, struct isl_basic_map *bmap,
				FILE *out, int indent)
{
	if (!bmap) {
		fprintf(out, "null basic map\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, nparam: %d, in: %d, out: %d, extra: %d\n",
		bmap->ref,
		bmap->nparam, bmap->n_in, bmap->n_out, bmap->extra);
	dump(bmap, out, indent);
}

int isl_inequality_negate(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	isl_assert(ctx, pos < bmap->n_ineq, return -1);
	isl_seq_neg(bmap->ineq[pos], bmap->ineq[pos], 1 + total);
	isl_int_sub_ui(bmap->ineq[pos][0], bmap->ineq[pos][0], 1);
	return 0;
}

struct isl_set *isl_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, int n, unsigned flags)
{
	struct isl_set *set;

	set = isl_alloc(ctx, struct isl_set,
			sizeof(struct isl_set) +
			n * sizeof(struct isl_basic_set *));
	if (!set)
		return NULL;

	set->ref = 1;
	set->size = n;
	set->n = 0;
	set->nparam = nparam;
	set->zero = 0;
	set->dim = dim;
	set->flags = flags;
	return set;
}

struct isl_set *isl_set_dup(struct isl_ctx *ctx, struct isl_set *set)
{
	int i;
	struct isl_set *dup;

	dup = isl_set_alloc(ctx, set->nparam, set->dim, set->n, set->flags);
	if (!dup)
		return NULL;
	for (i = 0; i < set->n; ++i)
		dup = isl_set_add(ctx, dup,
				  isl_basic_set_dup(ctx, set->p[i]));
	return dup;
}

struct isl_set *isl_set_from_basic_set(struct isl_ctx *ctx,
				struct isl_basic_set *bset)
{
	struct isl_set *set;

	set = isl_set_alloc(ctx, bset->nparam, bset->dim, 1, ISL_MAP_DISJOINT);
	if (!set) {
		isl_basic_set_free(ctx, bset);
		return NULL;
	}
	return isl_set_add(ctx, set, bset);
}

struct isl_map *isl_map_from_basic_map(struct isl_ctx *ctx,
				struct isl_basic_map *bmap)
{
	struct isl_map *map;

	map = isl_map_alloc(ctx, bmap->nparam, bmap->n_in, bmap->n_out, 1,
				ISL_MAP_DISJOINT);
	if (!map) {
		isl_basic_map_free(ctx, bmap);
		return NULL;
	}
	return isl_map_add(ctx, map, bmap);
}

struct isl_set *isl_set_add(struct isl_ctx *ctx, struct isl_set *set,
				struct isl_basic_set *bset)
{
	if (!bset || !set)
		goto error;
	isl_assert(ctx, set->nparam == bset->nparam, goto error);
	isl_assert(ctx, set->dim == bset->dim, goto error);
	isl_assert(ctx, set->n < set->size, goto error);
	set->p[set->n] = bset;
	set->n++;
	return set;
error:
	if (set)
		isl_set_free(ctx, set);
	if (bset)
		isl_basic_set_free(ctx, bset);
	return NULL;
}

void isl_set_free(struct isl_ctx *ctx, struct isl_set *set)
{
	int i;

	if (!set)
		return;

	if (--set->ref > 0)
		return;

	for (i = 0; i < set->n; ++i)
		isl_basic_set_free(ctx, set->p[i]);
	free(set);
}

void isl_set_dump(struct isl_ctx *ctx, struct isl_set *set, FILE *out,
		  int indent)
{
	int i;

	if (!set) {
		fprintf(out, "null set\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, n: %d\n", set->ref, set->n);
	for (i = 0; i < set->n; ++i) {
		fprintf(out, "%*s", indent, "");
		fprintf(out, "basic set %d:\n", i);
		isl_basic_set_dump(ctx, set->p[i], out, indent+4);
	}
}

void isl_map_dump(struct isl_ctx *ctx, struct isl_map *map, FILE *out,
		  int indent)
{
	int i;

	if (!map) {
		fprintf(out, "null map\n");
		return;
	}

	fprintf(out, "%*s", indent, "");
	fprintf(out, "ref: %d, n: %d, nparam: %d, in: %d, out: %d\n",
			map->ref, map->n, map->nparam, map->n_in, map->n_out);
	for (i = 0; i < map->n; ++i) {
		fprintf(out, "%*s", indent, "");
		fprintf(out, "basic map %d:\n", i);
		isl_basic_map_dump(ctx, map->p[i], out, indent+4);
	}
}

struct isl_basic_map *isl_basic_map_intersect_domain(
		struct isl_ctx *ctx, struct isl_basic_map *bmap,
		struct isl_basic_set *bset)
{
	struct isl_basic_map *bmap_domain;

	if (!bmap || !bset)
		goto error;

	isl_assert(ctx, bset->dim == bmap->n_in, goto error);
	isl_assert(ctx, bset->nparam == bmap->nparam, goto error);

	bmap = isl_basic_map_extend(ctx, bmap,
			bset->nparam, bmap->n_in, bmap->n_out,
			bset->n_div, bset->n_eq, bset->n_ineq);
	if (!bmap)
		goto error;
	bmap_domain = isl_basic_map_from_basic_set(ctx, bset,
						bset->dim, 0);
	bmap = add_constraints(ctx, bmap, bmap_domain, 0, 0);

	bmap = isl_basic_map_simplify(ctx, bmap);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	isl_basic_set_free(ctx, bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_intersect_range(
		struct isl_ctx *ctx, struct isl_basic_map *bmap,
		struct isl_basic_set *bset)
{
	struct isl_basic_map *bmap_range;

	if (!bmap || !bset)
		goto error;

	isl_assert(ctx, bset->dim == bmap->n_out, goto error);
	isl_assert(ctx, bset->nparam == bmap->nparam, goto error);

	bmap = isl_basic_map_extend(ctx, bmap,
			bset->nparam, bmap->n_in, bmap->n_out,
			bset->n_div, bset->n_eq, bset->n_ineq);
	if (!bmap)
		goto error;
	bmap_range = isl_basic_map_from_basic_set(ctx, bset,
						0, bset->dim);
	bmap = add_constraints(ctx, bmap, bmap_range, 0, 0);

	bmap = isl_basic_map_simplify(ctx, bmap);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	isl_basic_set_free(ctx, bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_intersect(
		struct isl_ctx *ctx, struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2)
{
	if (!bmap1 || !bmap2)
		goto error;

	isl_assert(ctx, bmap1->nparam == bmap2->nparam, goto error);
	isl_assert(ctx, bmap1->n_in == bmap2->n_in, goto error);
	isl_assert(ctx, bmap1->n_out == bmap2->n_out, goto error);

	bmap1 = isl_basic_map_extend(ctx, bmap1,
			bmap1->nparam, bmap1->n_in, bmap1->n_out,
			bmap2->n_div, bmap2->n_eq, bmap2->n_ineq);
	if (!bmap1)
		goto error;
	bmap1 = add_constraints(ctx, bmap1, bmap2, 0, 0);

	bmap1 = isl_basic_map_simplify(ctx, bmap1);
	return isl_basic_map_finalize(ctx, bmap1);
error:
	isl_basic_map_free(ctx, bmap1);
	isl_basic_map_free(ctx, bmap2);
	return NULL;
}

struct isl_basic_set *isl_basic_set_intersect(
		struct isl_ctx *ctx, struct isl_basic_set *bset1,
		struct isl_basic_set *bset2)
{
	return (struct isl_basic_set *)
		isl_basic_map_intersect(ctx,
			(struct isl_basic_map *)bset1,
			(struct isl_basic_map *)bset2);
}

struct isl_map *isl_map_intersect(struct isl_ctx *ctx, struct isl_map *map1,
		struct isl_map *map2)
{
	unsigned flags = 0;
	struct isl_map *result;
	int i, j;

	if (!map1 || !map2)
		goto error;

	if (F_ISSET(map1, ISL_MAP_DISJOINT) &&
	    F_ISSET(map2, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	result = isl_map_alloc(ctx, map1->nparam, map1->n_in, map1->n_out,
				map1->n * map2->n, flags);
	if (!result)
		goto error;
	for (i = 0; i < map1->n; ++i)
		for (j = 0; j < map2->n; ++j) {
			struct isl_basic_map *part;
			part = isl_basic_map_intersect(ctx,
				    isl_basic_map_copy(ctx, map1->p[i]),
				    isl_basic_map_copy(ctx, map2->p[j]));
			if (isl_basic_map_is_empty(ctx, part))
				isl_basic_map_free(ctx, part);
			else
				result = isl_map_add(ctx, result, part);
			if (!result)
				goto error;
		}
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return result;
error:
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return NULL;
}

struct isl_basic_map *isl_basic_map_reverse(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	struct isl_basic_set *bset;
	unsigned in;

	if (!bmap)
		return NULL;
	bmap = isl_basic_map_cow(ctx, bmap);
	if (!bmap)
		return NULL;
	in = bmap->n_in;
	bset = isl_basic_set_from_basic_map(ctx, bmap);
	bset = isl_basic_set_swap_vars(ctx, bset, in);
	return isl_basic_map_from_basic_set(ctx, bset, bset->dim-in, in);
}

/* Turn final n dimensions into existentially quantified variables.
 */
struct isl_basic_set *isl_basic_set_project_out(
		struct isl_ctx *ctx, struct isl_basic_set *bset,
		unsigned n, unsigned flags)
{
	int i;
	size_t row_size;
	isl_int **new_div;
	isl_int *old;

	if (!bset)
		return NULL;

	isl_assert(ctx, n <= bset->dim, goto error);

	if (n == 0)
		return bset;

	bset = isl_basic_set_cow(ctx, bset);

	row_size = 1 + bset->nparam + bset->dim + bset->extra;
	old = bset->block2.data;
	bset->block2 = isl_blk_extend(ctx, bset->block2,
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
	bset = isl_basic_set_simplify(ctx, bset);
	return isl_basic_set_finalize(ctx, bset);
error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}

struct isl_basic_map *isl_basic_map_apply_range(
		struct isl_ctx *ctx, struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2)
{
	struct isl_basic_set *bset;
	unsigned n_in, n_out;

	if (!bmap1 || !bmap2)
		goto error;

	isl_assert(ctx, bmap1->n_out == bmap2->n_in, goto error);
	isl_assert(ctx, bmap1->nparam == bmap2->nparam, goto error);

	n_in = bmap1->n_in;
	n_out = bmap2->n_out;

	bmap2 = isl_basic_map_reverse(ctx, bmap2);
	if (!bmap2)
		goto error;
	bmap1 = isl_basic_map_extend(ctx, bmap1, bmap1->nparam,
			bmap1->n_in + bmap2->n_in, bmap1->n_out,
			bmap2->extra, bmap2->n_eq, bmap2->n_ineq);
	if (!bmap1)
		goto error;
	bmap1 = add_constraints(ctx, bmap1, bmap2, bmap1->n_in - bmap2->n_in, 0);
	bmap1 = isl_basic_map_simplify(ctx, bmap1);
	bset = isl_basic_set_from_basic_map(ctx, bmap1);
	bset = isl_basic_set_project_out(ctx, bset,
						bset->dim - (n_in + n_out), 0);
	return isl_basic_map_from_basic_set(ctx, bset, n_in, n_out);
error:
	isl_basic_map_free(ctx, bmap1);
	isl_basic_map_free(ctx, bmap2);
	return NULL;
}

struct isl_basic_map *isl_basic_map_apply_domain(
		struct isl_ctx *ctx, struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2)
{
	if (!bmap1 || !bmap2)
		goto error;

	isl_assert(ctx, bmap1->n_in == bmap2->n_in, goto error);
	isl_assert(ctx, bmap1->nparam == bmap2->nparam, goto error);

	bmap1 = isl_basic_map_reverse(ctx, bmap1);
	bmap1 = isl_basic_map_apply_range(ctx, bmap1, bmap2);
	return isl_basic_map_reverse(ctx, bmap1);
error:
	isl_basic_map_free(ctx, bmap1);
	isl_basic_map_free(ctx, bmap2);
	return NULL;
}

static struct isl_basic_map *var_equal(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	int i;
	i = isl_basic_map_alloc_equality(ctx, bmap);
	if (i < 0)
		goto error;
	isl_seq_clr(bmap->eq[i],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->eq[i][1+bmap->nparam+pos], -1);
	isl_int_set_si(bmap->eq[i][1+bmap->nparam+bmap->n_in+pos], 1);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

static struct isl_basic_map *var_more(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	int i;
	i = isl_basic_map_alloc_inequality(ctx, bmap);
	if (i < 0)
		goto error;
	isl_seq_clr(bmap->ineq[i],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->ineq[i][0], -1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+pos], -1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+bmap->n_in+pos], 1);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

static struct isl_basic_map *var_less(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned pos)
{
	int i;
	i = isl_basic_map_alloc_inequality(ctx, bmap);
	if (i < 0)
		goto error;
	isl_seq_clr(bmap->ineq[i],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->ineq[i][0], -1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+pos], 1);
	isl_int_set_si(bmap->ineq[i][1+bmap->nparam+bmap->n_in+pos], -1);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
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
	return isl_basic_map_finalize(ctx, bmap);
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
	return isl_basic_map_finalize(ctx, bmap);
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
	return isl_basic_map_finalize(ctx, bmap);
}

struct isl_basic_map *isl_basic_map_from_basic_set(
		struct isl_ctx *ctx, struct isl_basic_set *bset,
		unsigned n_in, unsigned n_out)
{
	struct isl_basic_map *bmap;

	bset = isl_basic_set_cow(ctx, bset);
	if (!bset)
		return NULL;

	isl_assert(ctx, bset->dim == n_in + n_out, goto error);
	bmap = (struct isl_basic_map *) bset;
	bmap->n_in = n_in;
	bmap->n_out = n_out;
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_set_free(ctx, bset);
	return NULL;
}

struct isl_basic_set *isl_basic_set_from_basic_map(
		struct isl_ctx *ctx, struct isl_basic_map *bmap)
{
	if (!bmap)
		goto error;
	if (bmap->n_in == 0)
		return (struct isl_basic_set *)bmap;
	bmap = isl_basic_map_cow(ctx, bmap);
	if (!bmap)
		goto error;
	bmap->n_out += bmap->n_in;
	bmap->n_in = 0;
	bmap = isl_basic_map_finalize(ctx, bmap);
	return (struct isl_basic_set *)bmap;
error:
	return NULL;
}

static struct isl_basic_set *isl_basic_map_underlying_set(
		struct isl_ctx *ctx, struct isl_basic_map *bmap)
{
	if (!bmap)
		goto error;
	if (bmap->nparam == 0 && bmap->n_in == 0 && bmap->n_div == 0)
		return (struct isl_basic_set *)bmap;
	bmap = isl_basic_map_cow(ctx, bmap);
	if (!bmap)
		goto error;
	bmap->n_out += bmap->nparam + bmap->n_in + bmap->n_div;
	bmap->nparam = 0;
	bmap->n_in = 0;
	bmap->n_div = 0;
	bmap = isl_basic_map_finalize(ctx, bmap);
	return (struct isl_basic_set *)bmap;
error:
	return NULL;
}

struct isl_basic_set *isl_basic_map_domain(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	struct isl_basic_set *domain;
	unsigned n_out;
	if (!bmap)
		return NULL;
	n_out = bmap->n_out;
	domain = isl_basic_set_from_basic_map(ctx, bmap);
	return isl_basic_set_project_out(ctx, domain, n_out, 0);
}

struct isl_basic_set *isl_basic_map_range(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	return isl_basic_map_domain(ctx,
			isl_basic_map_reverse(ctx, bmap));
}

struct isl_set *isl_map_range(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;
	struct isl_set *set;

	if (!map)
		goto error;
	map = isl_map_cow(ctx, map);
	if (!map)
		goto error;

	set = (struct isl_set *) map;
	set->zero = 0;
	for (i = 0; i < map->n; ++i) {
		set->p[i] = isl_basic_map_range(ctx, map->p[i]);
		if (!set->p[i])
			goto error;
	}
	F_CLR(set, ISL_MAP_DISJOINT);
	return set;
error:
	isl_map_free(ctx, map);
	return NULL;
}

struct isl_map *isl_map_from_set(
		struct isl_ctx *ctx, struct isl_set *set,
		unsigned n_in, unsigned n_out)
{
	int i;
	struct isl_map *map = NULL;

	if (!set)
		return NULL;
	isl_assert(ctx, set->dim == n_in + n_out, goto error);
	set = isl_set_cow(ctx, set);
	if (!set)
		return NULL;
	map = (struct isl_map *)set;
	for (i = 0; i < set->n; ++i) {
		map->p[i] = isl_basic_map_from_basic_set(ctx,
				set->p[i], n_in, n_out);
		if (!map->p[i])
			goto error;
	}
	map->n_in = n_in;
	map->n_out = n_out;
	return map;
error:
	isl_set_free(ctx, set);
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
	bmap = isl_basic_map_set_to_empty(ctx, bmap);
	return bmap;
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

struct isl_map *isl_map_dup(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;
	struct isl_map *dup;

	dup = isl_map_alloc(ctx, map->nparam, map->n_in, map->n_out, map->n,
				map->flags);
	for (i = 0; i < map->n; ++i)
		dup = isl_map_add(ctx, dup,
				  isl_basic_map_dup(ctx, map->p[i]));
	return dup;
}

struct isl_map *isl_map_add(struct isl_ctx *ctx, struct isl_map *map,
				struct isl_basic_map *bmap)
{
	if (!bmap || !map)
		goto error;
	isl_assert(ctx, map->nparam == bmap->nparam, goto error);
	isl_assert(ctx, map->n_in == bmap->n_in, goto error);
	isl_assert(ctx, map->n_out == bmap->n_out, goto error);
	isl_assert(ctx, map->n < map->size, goto error);
	map->p[map->n] = bmap;
	map->n++;
	return map;
error:
	if (map)
		isl_map_free(ctx, map);
	if (bmap)
		isl_basic_map_free(ctx, bmap);
	return NULL;
}

void isl_map_free(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;

	if (!map)
		return;

	if (--map->ref > 0)
		return;

	for (i = 0; i < map->n; ++i)
		isl_basic_map_free(ctx, map->p[i]);
	free(map);
}

struct isl_map *isl_map_extend(struct isl_ctx *ctx, struct isl_map *base,
		unsigned nparam, unsigned n_in, unsigned n_out)
{
	int i;

	base = isl_map_cow(ctx, base);
	if (!base)
		return NULL;

	isl_assert(ctx, base->nparam <= nparam, goto error);
	isl_assert(ctx, base->n_in <= n_in, goto error);
	isl_assert(ctx, base->n_out <= n_out, goto error);
	base->nparam = nparam;
	base->n_in = n_in;
	base->n_out = n_out;
	for (i = 0; i < base->n; ++i) {
		base->p[i] = isl_basic_map_extend(ctx, base->p[i],
				nparam, n_in, n_out, 0, 0, 0);
		if (!base->p[i])
			goto error;
	}
	return base;
error:
	isl_map_free(ctx, base);
	return NULL;
}

struct isl_basic_map *isl_basic_map_fix_input_si(struct isl_ctx *ctx,
		struct isl_basic_map *bmap,
		unsigned input, int value)
{
	int j;

	bmap = isl_basic_map_cow(ctx, bmap);
	if (!bmap)
		return NULL;
	isl_assert(ctx, input < bmap->n_in, goto error);

	bmap = isl_basic_map_extend(ctx, bmap,
			bmap->nparam, bmap->n_in, bmap->n_out, 0, 1, 0);
	j = isl_basic_map_alloc_equality(ctx, bmap);
	if (j < 0)
		goto error;
	isl_seq_clr(bmap->eq[j],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
	isl_int_set_si(bmap->eq[j][1+bmap->nparam+input], -1);
	isl_int_set_si(bmap->eq[j][0], value);
	bmap = isl_basic_map_simplify(ctx, bmap);
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

struct isl_map *isl_map_fix_input_si(struct isl_ctx *ctx, struct isl_map *map,
		unsigned input, int value)
{
	int i;

	map = isl_map_cow(ctx, map);
	if (!map)
		return NULL;

	isl_assert(ctx, input < map->n_in, goto error);
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_fix_input_si(ctx, map->p[i],
								input, value);
		if (!map->p[i])
			goto error;
	}
	return map;
error:
	isl_map_free(ctx, map);
	return NULL;
}

struct isl_map *isl_map_reverse(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;
	unsigned t;

	map = isl_map_cow(ctx, map);
	if (!map)
		return NULL;

	t = map->n_in;
	map->n_in = map->n_out;
	map->n_out = t;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_reverse(ctx, map->p[i]);
		if (!map->p[i])
			goto error;
	}
	return map;
error:
	isl_map_free(ctx, map);
	return NULL;
}

struct isl_map *isl_basic_map_lexmax(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty)
{
	return isl_pip_basic_map_lexmax(ctx, bmap, dom, empty);
}

struct isl_map *isl_basic_map_lexmin(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty)
{
	return isl_pip_basic_map_lexmin(ctx, bmap, dom, empty);
}

struct isl_set *isl_basic_set_lexmin(struct isl_ctx *ctx,
		struct isl_basic_set *bset)
{
	struct isl_basic_map *bmap = NULL;
	struct isl_basic_set *dom = NULL;
	struct isl_map *min;

	if (!bset)
		goto error;
	bmap = isl_basic_map_from_basic_set(ctx, bset, 0, bset->dim);
	if (!bmap)
		goto error;
	dom = isl_basic_set_alloc(ctx, bmap->nparam, 0, 0, 0, 0);
	if (!dom)
		goto error;
	min = isl_basic_map_lexmin(ctx, bmap, dom, NULL);
	return isl_map_range(ctx, min);
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

struct isl_map *isl_basic_map_compute_divs(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	if (!bmap)
		return NULL;
	if (bmap->n_div == 0)
		return isl_map_from_basic_map(ctx, bmap);
	return isl_pip_basic_map_compute_divs(ctx, bmap);
}

struct isl_map *isl_map_compute_divs(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;
	struct isl_map *res;

	if (!map)
		return NULL;
	if (map->n == 0)
		return map;
	res = isl_basic_map_compute_divs(ctx,
		isl_basic_map_copy(ctx, map->p[0]));
	for (i = 1 ; i < map->n; ++i) {
		struct isl_map *r2;
		r2 = isl_basic_map_compute_divs(ctx,
			isl_basic_map_copy(ctx, map->p[i]));
		if (F_ISSET(map, ISL_MAP_DISJOINT))
			res = isl_map_union_disjoint(ctx, res, r2);
		else
			res = isl_map_union(ctx, res, r2);
	}
	isl_map_free(ctx, map);

	return res;
}

struct isl_set *isl_basic_set_compute_divs(struct isl_ctx *ctx,
		struct isl_basic_set *bset)
{
	return (struct isl_set *)
		isl_basic_map_compute_divs(ctx,
			(struct isl_basic_map *)bset);
}

struct isl_set *isl_map_domain(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;
	struct isl_set *set;

	if (!map)
		goto error;

	map = isl_map_cow(ctx, map);
	if (!map)
		return NULL;

	set = (struct isl_set *)map;
	set->dim = map->n_in;
	set->zero = 0;
	for (i = 0; i < map->n; ++i) {
		set->p[i] = isl_basic_map_domain(ctx, map->p[i]);
		if (!set->p[i])
			goto error;
	}
	F_CLR(set, ISL_MAP_DISJOINT);
	return set;
error:
	isl_map_free(ctx, map);
	return NULL;
}

struct isl_map *isl_map_union_disjoint(struct isl_ctx *ctx,
			struct isl_map *map1, struct isl_map *map2)
{
	int i;
	unsigned flags = 0;
	struct isl_map *map = NULL;

	if (!map1 || !map2)
		goto error;

	if (map1->n == 0) {
		isl_map_free(ctx, map1);
		return map2;
	}
	if (map2->n == 0) {
		isl_map_free(ctx, map2);
		return map1;
	}

	isl_assert(ctx, map1->nparam == map2->nparam, goto error);
	isl_assert(ctx, map1->n_in == map2->n_in, goto error);
	isl_assert(ctx, map1->n_out == map2->n_out, goto error);

	if (F_ISSET(map1, ISL_MAP_DISJOINT) &&
	    F_ISSET(map2, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	map = isl_map_alloc(ctx, map1->nparam, map1->n_in, map1->n_out,
				map1->n + map2->n, flags);
	if (!map)
		goto error;
	for (i = 0; i < map1->n; ++i) {
		map = isl_map_add(ctx, map,
				  isl_basic_map_copy(ctx, map1->p[i]));
		if (!map)
			goto error;
	}
	for (i = 0; i < map2->n; ++i) {
		map = isl_map_add(ctx, map,
				  isl_basic_map_copy(ctx, map2->p[i]));
		if (!map)
			goto error;
	}
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return map;
error:
	isl_map_free(ctx, map);
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return NULL;
}

struct isl_map *isl_map_union(struct isl_ctx *ctx,
			struct isl_map *map1, struct isl_map *map2)
{
	map1 = isl_map_union_disjoint(ctx, map1, map2);
	if (!map1)
		return NULL;
	if (map1->n > 1)
		F_CLR(map1, ISL_MAP_DISJOINT);
	return map1;
}

struct isl_set *isl_set_union_disjoint(struct isl_ctx *ctx,
			struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_union_disjoint(ctx,
			(struct isl_map *)set1, (struct isl_map *)set2);
}

struct isl_set *isl_set_union(struct isl_ctx *ctx,
			struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_union(ctx,
			(struct isl_map *)set1, (struct isl_map *)set2);
}

struct isl_map *isl_map_intersect_range(
		struct isl_ctx *ctx, struct isl_map *map,
		struct isl_set *set)
{
	unsigned flags = 0;
	struct isl_map *result;
	int i, j;

	if (!map || !set)
		goto error;

	if (F_ISSET(map, ISL_MAP_DISJOINT) &&
	    F_ISSET(set, ISL_MAP_DISJOINT))
		FL_SET(flags, ISL_MAP_DISJOINT);

	result = isl_map_alloc(ctx, map->nparam, map->n_in, map->n_out,
				map->n * set->n, flags);
	if (!result)
		goto error;
	for (i = 0; i < map->n; ++i)
		for (j = 0; j < set->n; ++j) {
			result = isl_map_add(ctx, result,
			    isl_basic_map_intersect_range(ctx,
				isl_basic_map_copy(ctx, map->p[i]),
				isl_basic_set_copy(ctx, set->p[j])));
			if (!result)
				goto error;
		}
	isl_map_free(ctx, map);
	isl_set_free(ctx, set);
	return result;
error:
	isl_map_free(ctx, map);
	isl_set_free(ctx, set);
	return NULL;
}

struct isl_map *isl_map_intersect_domain(
		struct isl_ctx *ctx, struct isl_map *map,
		struct isl_set *set)
{
	return isl_map_reverse(ctx,
		isl_map_intersect_range(ctx, isl_map_reverse(ctx, map), set));
}

struct isl_map *isl_map_apply_domain(
		struct isl_ctx *ctx, struct isl_map *map1,
		struct isl_map *map2)
{
	if (!map1 || !map2)
		goto error;
	map1 = isl_map_reverse(ctx, map1);
	map1 = isl_map_apply_range(ctx, map1, map2);
	return isl_map_reverse(ctx, map1);
error:
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return NULL;
}

struct isl_map *isl_map_apply_range(
		struct isl_ctx *ctx, struct isl_map *map1,
		struct isl_map *map2)
{
	struct isl_map *result;
	int i, j;

	if (!map1 || !map2)
		goto error;

	isl_assert(ctx, map1->nparam == map2->nparam, goto error);
	isl_assert(ctx, map1->n_out == map2->n_in, goto error);

	result = isl_map_alloc(ctx, map1->nparam, map1->n_in, map2->n_out,
				map1->n * map2->n, 0);
	if (!result)
		goto error;
	for (i = 0; i < map1->n; ++i)
		for (j = 0; j < map2->n; ++j) {
			result = isl_map_add(ctx, result,
			    isl_basic_map_apply_range(ctx,
				isl_basic_map_copy(ctx, map1->p[i]),
				isl_basic_map_copy(ctx, map2->p[j])));
			if (!result)
				goto error;
		}
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return result;
error:
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return NULL;
}

/*
 * returns range - domain
 */
struct isl_basic_set *isl_basic_map_deltas(struct isl_ctx *ctx,
				struct isl_basic_map *bmap)
{
	struct isl_basic_set *bset;
	unsigned dim;
	int i;

	if (!bmap)
		goto error;
	isl_assert(ctx, bmap->n_in == bmap->n_out, goto error);
	dim = bmap->n_in;
	bset = isl_basic_set_from_basic_map(ctx, bmap);
	bset = isl_basic_set_extend(ctx, bset, bset->nparam, 3*dim, 0,
					dim, 0);
	bset = isl_basic_set_swap_vars(ctx, bset, 2*dim);
	for (i = 0; i < dim; ++i) {
		int j = isl_basic_map_alloc_equality(ctx,
					    (struct isl_basic_map *)bset);
		if (j < 0)
			goto error;
		isl_seq_clr(bset->eq[j],
			    1 + bset->nparam + bset->dim + bset->extra);
		isl_int_set_si(bset->eq[j][1+bset->nparam+i], 1);
		isl_int_set_si(bset->eq[j][1+bset->nparam+dim+i], 1);
		isl_int_set_si(bset->eq[j][1+bset->nparam+2*dim+i], -1);
	}
	return isl_basic_set_project_out(ctx, bset, 2*dim, 0);
error:
	isl_basic_map_free(ctx, bmap);
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
static int div_is_redundant(struct isl_ctx *ctx, struct isl_basic_map *bmap,
				int div)
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
static struct isl_basic_map *remove_redundant_divs(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	int i;

	for (i = bmap->n_div-1; i >= 0; --i) {
		if (!div_is_redundant(ctx, bmap, i))
			continue;
		bmap = isl_basic_map_drop_div(ctx, bmap, i);
	}
	return bmap;
}

struct isl_basic_map *isl_basic_map_finalize(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	bmap = remove_redundant_divs(ctx, bmap);
	if (!bmap)
		return NULL;
	F_SET(bmap, ISL_BASIC_SET_FINAL);
	return bmap;
}

struct isl_basic_set *isl_basic_set_finalize(struct isl_ctx *ctx,
		struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_finalize(ctx,
			(struct isl_basic_map *)bset);
}

struct isl_set *isl_set_finalize(struct isl_ctx *ctx, struct isl_set *set)
{
	int i;

	if (!set)
		return NULL;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_finalize(ctx, set->p[i]);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(ctx, set);
	return NULL;
}

struct isl_map *isl_map_finalize(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;

	if (!map)
		return NULL;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_finalize(ctx, map->p[i]);
		if (!map->p[i])
			goto error;
	}
	return map;
error:
	isl_map_free(ctx, map);
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
		int j = isl_basic_map_alloc_equality(ctx, bmap);
		if (j < 0)
			goto error;
		isl_seq_clr(bmap->eq[j],
		    1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra);
		isl_int_set_si(bmap->eq[j][1+nparam+i], 1);
		isl_int_set_si(bmap->eq[j][1+nparam+dim+i], -1);
	}
	return isl_basic_map_finalize(ctx, bmap);
error:
	isl_basic_map_free(ctx, bmap);
	return NULL;
}

struct isl_map *isl_map_identity(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim)
{
	struct isl_map *map = isl_map_alloc(ctx, nparam, dim, dim, 1,
						ISL_MAP_DISJOINT);
	if (!map)
		goto error;
	map = isl_map_add(ctx, map,
		isl_basic_map_identity(ctx, nparam, dim));
	return map;
error:
	isl_map_free(ctx, map);
	return NULL;
}

int isl_set_is_equal(struct isl_ctx *ctx,
		struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_is_equal(ctx,
			(struct isl_map *)set1, (struct isl_map *)set2);
}

int isl_set_is_subset(struct isl_ctx *ctx,
		struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_is_subset(ctx,
			(struct isl_map *)set1, (struct isl_map *)set2);
}

int isl_basic_map_is_subset(struct isl_ctx *ctx,
		struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2)
{
	int is_subset;
	struct isl_map *map1;
	struct isl_map *map2;

	if (!bmap1 || !bmap2)
		return -1;

	map1 = isl_map_from_basic_map(ctx,
			isl_basic_map_copy(ctx, bmap1));
	map2 = isl_map_from_basic_map(ctx,
			isl_basic_map_copy(ctx, bmap2));

	is_subset = isl_map_is_subset(ctx, map1, map2);

	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);

	return is_subset;
}

int isl_map_is_empty(struct isl_ctx *ctx, struct isl_map *map)
{
	int i;
	int is_empty;

	if (!map)
		return -1;
	for (i = 0; i < map->n; ++i) {
		is_empty = isl_basic_map_is_empty(ctx, map->p[i]);
		if (is_empty < 0)
			return -1;
		if (!is_empty)
			return 0;
	}
	return 1;
}

int isl_set_is_empty(struct isl_ctx *ctx, struct isl_set *set)
{
	return isl_map_is_empty(ctx, (struct isl_map *)set);
}

int isl_map_is_subset(struct isl_ctx *ctx, struct isl_map *map1,
		struct isl_map *map2)
{
	int i;
	int is_subset = 0;
	struct isl_map *diff;

	if (!map1 || !map2)
		return -1;

	if (isl_map_is_empty(ctx, map1))
		return 1;

	if (isl_map_is_empty(ctx, map2))
		return 0;

	diff = isl_map_subtract(ctx, isl_map_copy(ctx, map1),
				     isl_map_copy(ctx, map2));
	if (!diff)
		return -1;

	is_subset = isl_map_is_empty(ctx, diff);
	isl_map_free(ctx, diff);

	return is_subset;
}

int isl_map_is_equal(struct isl_ctx *ctx,
		struct isl_map *map1, struct isl_map *map2)
{
	int is_subset;

	if (!map1 || !map2)
		return -1;
	is_subset = isl_map_is_subset(ctx, map1, map2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_map_is_subset(ctx, map2, map1);
	return is_subset;
}

int isl_basic_map_is_strict_subset(struct isl_ctx *ctx,
		struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2)
{
	int is_subset;

	if (!bmap1 || !bmap2)
		return -1;
	is_subset = isl_basic_map_is_subset(ctx, bmap1, bmap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_basic_map_is_subset(ctx, bmap2, bmap1);
	if (is_subset == -1)
		return is_subset;
	return !is_subset;
}

static int basic_map_contains(struct isl_ctx *ctx, struct isl_basic_map *bmap,
					 struct isl_vec *vec)
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

int isl_basic_map_is_empty(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
{
	struct isl_basic_set *bset = NULL;
	struct isl_vec *sample = NULL;
	int empty;
	unsigned total;

	if (!bmap)
		return -1;

	if (F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
		return 1;

	total = 1 + bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	if (bmap->sample && bmap->sample->size == total) {
		int contains = basic_map_contains(ctx, bmap, bmap->sample);
		if (contains < 0)
			return -1;
		if (contains)
			return 0;
	}
	bset = isl_basic_map_underlying_set(ctx,
			isl_basic_map_copy(ctx, bmap));
	if (!bset)
		return -1;
	sample = isl_basic_set_sample(ctx, bset);
	if (!sample)
		return -1;
	empty = sample->size == 0;
	if (bmap->sample)
		isl_vec_free(ctx, bmap->sample);
	bmap->sample = sample;

	return empty;
}

struct isl_map *isl_basic_map_union(
		struct isl_ctx *ctx, struct isl_basic_map *bmap1,
		struct isl_basic_map *bmap2)
{
	struct isl_map *map;
	if (!bmap1 || !bmap2)
		return NULL;

	isl_assert(ctx, bmap1->nparam == bmap2->nparam, goto error);
	isl_assert(ctx, bmap1->n_in == bmap2->n_in, goto error);
	isl_assert(ctx, bmap1->n_out == bmap2->n_out, goto error);

	map = isl_map_alloc(ctx, bmap1->nparam,
				bmap1->n_in, bmap1->n_out, 2, 0);
	if (!map)
		goto error;
	map = isl_map_add(ctx, map, bmap1);
	map = isl_map_add(ctx, map, bmap2);
	return map;
error:
	isl_basic_map_free(ctx, bmap1);
	isl_basic_map_free(ctx, bmap2);
	return NULL;
}

struct isl_set *isl_basic_set_union(
		struct isl_ctx *ctx, struct isl_basic_set *bset1,
		struct isl_basic_set *bset2)
{
	return (struct isl_set *)isl_basic_map_union(ctx,
					    (struct isl_basic_map *)bset1,
					    (struct isl_basic_map *)bset2);
}

/* Order divs such that any div only depends on previous divs */
static struct isl_basic_map *order_divs(struct isl_ctx *ctx,
		struct isl_basic_map *bmap)
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

/* For a div d = floor(f/m), add the constraints
 *
 *		f - m d >= 0
 *		-(f-(n-1)) + m d >= 0
 *
 * Note that the second constraint is the negation of
 *
 *		f - m d >= n
 */
static int add_div_constraints(struct isl_ctx *ctx,
		struct isl_basic_map *bmap, unsigned div)
{
	int i, j;
	unsigned div_pos = 1 + bmap->nparam + bmap->n_in + bmap->n_out + div;
	unsigned total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->extra;

	i = isl_basic_map_alloc_inequality(ctx, bmap);
	if (i < 0)
		return -1;
	isl_seq_cpy(bmap->ineq[i], bmap->div[div]+1, 1+total);
	isl_int_neg(bmap->ineq[i][div_pos], bmap->div[div][0]);

	j = isl_basic_map_alloc_inequality(ctx, bmap);
	if (j < 0)
		return -1;
	isl_seq_neg(bmap->ineq[j], bmap->ineq[i], 1 + total);
	isl_int_add(bmap->ineq[j][0], bmap->ineq[j][0], bmap->ineq[j][div_pos]);
	isl_int_sub_ui(bmap->ineq[j][0], bmap->ineq[j][0], 1);
	return j;
}

struct isl_basic_map *isl_basic_map_align_divs(struct isl_ctx *ctx,
		struct isl_basic_map *dst, struct isl_basic_map *src)
{
	int i;
	unsigned total = src->nparam + src->n_in + src->n_out + src->n_div;

	if (src->n_div == 0)
		return dst;

	src = order_divs(ctx, src);
	dst = isl_basic_map_extend(ctx, dst, dst->nparam, dst->n_in,
			dst->n_out, src->n_div, 0, 2 * src->n_div);
	if (!dst)
		return NULL;
	for (i = 0; i < src->n_div; ++i) {
		int j = find_div(dst, src, i);
		if (j < 0) {
			j = isl_basic_map_alloc_div(ctx, dst);
			if (j < 0)
				goto error;
			isl_seq_cpy(dst->div[j], src->div[i], 1+1+total);
			isl_seq_clr(dst->div[j]+1+1+total,
						    dst->extra - src->n_div);
			if (add_div_constraints(ctx, dst, j) < 0)
				goto error;
		}
		if (j != i)
			swap_div(dst, i, j);
	}
	return dst;
error:
	isl_basic_map_free(ctx, dst);
	return NULL;
}

static struct isl_map *add_cut_constraint(struct isl_ctx *ctx,
		struct isl_map *dst,
		struct isl_basic_map *src, isl_int *c,
		unsigned len, int oppose)
{
	struct isl_basic_map *copy = NULL;
	int is_empty;
	int k;
	unsigned total;

	copy = isl_basic_map_copy(ctx, src);
	copy = isl_basic_map_cow(ctx, copy);
	if (!copy)
		goto error;
	copy = isl_basic_map_extend(ctx, copy,
		copy->nparam, copy->n_in, copy->n_out, 0, 0, 1);
	k = isl_basic_map_alloc_inequality(ctx, copy);
	if (k < 0)
		goto error;
	if (oppose)
		isl_seq_neg(copy->ineq[k], c, len);
	else
		isl_seq_cpy(copy->ineq[k], c, len);
	total = 1 + copy->nparam + copy->n_in + copy->n_out + copy->extra;
	isl_seq_clr(copy->ineq[k]+len, total - len);
	isl_inequality_negate(ctx, copy, k);
	copy = isl_basic_map_simplify(ctx, copy);
	copy = isl_basic_map_finalize(ctx, copy);
	is_empty = isl_basic_map_is_empty(ctx, copy);
	if (is_empty < 0)
		goto error;
	if (!is_empty)
		dst = isl_map_add(ctx, dst, copy);
	else
		isl_basic_map_free(ctx, copy);
	return dst;
error:
	isl_basic_map_free(ctx, copy);
	isl_map_free(ctx, dst);
	return NULL;
}

static struct isl_map *subtract(struct isl_ctx *ctx, struct isl_map *map,
		struct isl_basic_map *bmap)
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
	rest = isl_map_alloc(ctx, map->nparam, map->n_in, map->n_out,
				max, flags);
	if (!rest)
		goto error;

	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_align_divs(ctx, map->p[i], bmap);
		if (!map->p[i])
			goto error;
	}

	for (j = 0; j < map->n; ++j)
		map->p[j] = isl_basic_map_cow(ctx, map->p[j]);

	for (i = 0; i < bmap->n_eq; ++i) {
		for (j = 0; j < map->n; ++j) {
			rest = add_cut_constraint(ctx, rest,
				map->p[j], bmap->eq[i], 1+total, 0);
			if (!rest)
				goto error;

			rest = add_cut_constraint(ctx, rest,
				map->p[j], bmap->eq[i], 1+total, 1);
			if (!rest)
				goto error;

			map->p[j] = isl_basic_map_extend(ctx, map->p[j],
				map->p[j]->nparam, map->p[j]->n_in,
				map->p[j]->n_out, 0, 1, 0);
			if (!map->p[j])
				goto error;
			k = isl_basic_map_alloc_equality(ctx, map->p[j]);
			if (k < 0)
				goto error;
			isl_seq_cpy(map->p[j]->eq[k], bmap->eq[i], 1+total);
			isl_seq_clr(map->p[j]->eq[k]+1+total,
					map->p[j]->extra - bmap->n_div);
		}
	}

	for (i = 0; i < bmap->n_ineq; ++i) {
		for (j = 0; j < map->n; ++j) {
			rest = add_cut_constraint(ctx, rest,
				map->p[j], bmap->ineq[i], 1+total, 0);
			if (!rest)
				goto error;

			map->p[j] = isl_basic_map_extend(ctx, map->p[j],
				map->p[j]->nparam, map->p[j]->n_in,
				map->p[j]->n_out, 0, 0, 1);
			if (!map->p[j])
				goto error;
			k = isl_basic_map_alloc_inequality(ctx, map->p[j]);
			if (k < 0)
				goto error;
			isl_seq_cpy(map->p[j]->ineq[k], bmap->ineq[i], 1+total);
			isl_seq_clr(map->p[j]->ineq[k]+1+total,
					map->p[j]->extra - bmap->n_div);
		}
	}

	isl_map_free(ctx, map);
	return rest;
error:
	isl_map_free(ctx, map);
	isl_map_free(ctx, rest);
	return NULL;
}

struct isl_map *isl_map_subtract(struct isl_ctx *ctx, struct isl_map *map1,
		struct isl_map *map2)
{
	int i;
	if (!map1 || !map2)
		goto error;

	isl_assert(ctx, map1->nparam == map2->nparam, goto error);
	isl_assert(ctx, map1->n_in == map2->n_in, goto error);
	isl_assert(ctx, map1->n_out == map2->n_out, goto error);

	if (isl_map_is_empty(ctx, map2)) {
		isl_map_free(ctx, map2);
		return map1;
	}

	map1 = isl_map_compute_divs(ctx, map1);
	map2 = isl_map_compute_divs(ctx, map2);
	if (!map1 || !map2)
		goto error;

	for (i = 0; map1 && i < map2->n; ++i)
		map1 = subtract(ctx, map1, map2->p[i]);

	isl_map_free(ctx, map2);
	return map1;
error:
	isl_map_free(ctx, map1);
	isl_map_free(ctx, map2);
	return NULL;
}

struct isl_set *isl_set_subtract(struct isl_ctx *ctx, struct isl_set *set1,
		struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_subtract(ctx,
			(struct isl_map *)set1, (struct isl_map *)set2);
}

struct isl_set *isl_set_apply(struct isl_ctx *ctx, struct isl_set *set,
		struct isl_map *map)
{
	isl_assert(ctx, set->dim == map->n_in, goto error);
	map = isl_map_intersect_domain(ctx, map, set);
	set = isl_map_range(ctx, map);
	return set;
error:
	isl_set_free(ctx, set);
	isl_map_free(ctx, map);
	return NULL;
}
