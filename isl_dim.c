/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 * and INRIA Saclay - Ile-de-France, Parc Club Orsay Universite,
 * ZAC des vignes, 4 rue Jacques Monod, 91893 Orsay, France 
 */

#include <stdlib.h>
#include <isl_dim_private.h>
#include "isl_name.h"
#include <isl_reordering.h>

isl_ctx *isl_dim_get_ctx(__isl_keep isl_dim *dim)
{
	return dim ? dim->ctx : NULL;
}

struct isl_dim *isl_dim_alloc(struct isl_ctx *ctx,
			unsigned nparam, unsigned n_in, unsigned n_out)
{
	struct isl_dim *dim;

	dim = isl_alloc_type(ctx, struct isl_dim);
	if (!dim)
		return NULL;

	dim->ctx = ctx;
	isl_ctx_ref(ctx);
	dim->ref = 1;
	dim->nparam = nparam;
	dim->n_in = n_in;
	dim->n_out = n_out;

	dim->tuple_name[0] = NULL;
	dim->tuple_name[1] = NULL;

	dim->nested[0] = NULL;
	dim->nested[1] = NULL;

	dim->n_name = 0;
	dim->names = NULL;

	return dim;
}

struct isl_dim *isl_dim_set_alloc(struct isl_ctx *ctx,
			unsigned nparam, unsigned dim)
{
	return isl_dim_alloc(ctx, nparam, 0, dim);
}

static unsigned global_pos(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos)
{
	struct isl_ctx *ctx = dim->ctx;

	switch (type) {
	case isl_dim_param:
		isl_assert(ctx, pos < dim->nparam, return isl_dim_total(dim));
		return pos;
	case isl_dim_in:
		isl_assert(ctx, pos < dim->n_in, return isl_dim_total(dim));
		return pos + dim->nparam;
	case isl_dim_out:
		isl_assert(ctx, pos < dim->n_out, return isl_dim_total(dim));
		return pos + dim->nparam + dim->n_in;
	default:
		isl_assert(ctx, 0, return isl_dim_total(dim));
	}
	return isl_dim_total(dim);
}

/* Extend length of names array to the total number of dimensions.
 */
static __isl_give isl_dim *extend_names(__isl_take isl_dim *dim)
{
	struct isl_name **names;
	int i;

	if (isl_dim_total(dim) <= dim->n_name)
		return dim;

	if (!dim->names) {
		dim->names = isl_calloc_array(dim->ctx,
				struct isl_name *, isl_dim_total(dim));
		if (!dim->names)
			goto error;
	} else {
		names = isl_realloc_array(dim->ctx, dim->names,
				struct isl_name *, isl_dim_total(dim));
		if (!names)
			goto error;
		dim->names = names;
		for (i = dim->n_name; i < isl_dim_total(dim); ++i)
			dim->names[i] = NULL;
	}

	dim->n_name = isl_dim_total(dim);

	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

static struct isl_dim *set_name(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos,
				 struct isl_name *name)
{
	struct isl_ctx *ctx = dim->ctx;
	dim = isl_dim_cow(dim);

	if (!dim)
		goto error;

	pos = global_pos(dim, type, pos);
	if (pos == isl_dim_total(dim))
		goto error;

	if (pos >= dim->n_name) {
		if (!name)
			return dim;
		dim = extend_names(dim);
		if (!dim)
			goto error;
	}

	dim->names[pos] = name;

	return dim;
error:
	isl_name_free(ctx, name);
	isl_dim_free(dim);
	return NULL;
}

static struct isl_name *get_name(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos)
{
	if (!dim)
		return NULL;

	pos = global_pos(dim, type, pos);
	if (pos == isl_dim_total(dim))
		return NULL;
	if (pos >= dim->n_name)
		return NULL;
	return dim->names[pos];
}

static unsigned offset(struct isl_dim *dim, enum isl_dim_type type)
{
	switch (type) {
	case isl_dim_param:	return 0;
	case isl_dim_in:	return dim->nparam;
	case isl_dim_out:	return dim->nparam + dim->n_in;
	default:		return 0;
	}
}

static unsigned n(struct isl_dim *dim, enum isl_dim_type type)
{
	switch (type) {
	case isl_dim_param:	return dim->nparam;
	case isl_dim_in:	return dim->n_in;
	case isl_dim_out:	return dim->n_out;
	case isl_dim_all:	return dim->nparam + dim->n_in + dim->n_out;
	default:		return 0;
	}
}

unsigned isl_dim_size(struct isl_dim *dim, enum isl_dim_type type)
{
	if (!dim)
		return 0;
	return n(dim, type);
}

unsigned isl_dim_offset(__isl_keep isl_dim *dim, enum isl_dim_type type)
{
	if (!dim)
		return 0;
	return offset(dim, type);
}

static struct isl_dim *copy_names(struct isl_dim *dst,
	enum isl_dim_type dst_type, unsigned offset, struct isl_dim *src,
	enum isl_dim_type src_type)
{
	int i;
	struct isl_name *name;

	if (!dst)
		return NULL;

	for (i = 0; i < n(src, src_type); ++i) {
		name = get_name(src, src_type, i);
		if (!name)
			continue;
		dst = set_name(dst, dst_type, offset + i,
					isl_name_copy(dst->ctx, name));
		if (!dst)
			return NULL;
	}
	return dst;
}

struct isl_dim *isl_dim_dup(struct isl_dim *dim)
{
	struct isl_dim *dup;
	if (!dim)
		return NULL;
	dup = isl_dim_alloc(dim->ctx, dim->nparam, dim->n_in, dim->n_out);
	if (dim->tuple_name[0] &&
	    !(dup->tuple_name[0] = isl_name_copy(dim->ctx, dim->tuple_name[0])))
		goto error;
	if (dim->tuple_name[1] &&
	    !(dup->tuple_name[1] = isl_name_copy(dim->ctx, dim->tuple_name[1])))
		goto error;
	if (dim->nested[0] && !(dup->nested[0] = isl_dim_copy(dim->nested[0])))
		goto error;
	if (dim->nested[1] && !(dup->nested[1] = isl_dim_copy(dim->nested[1])))
		goto error;
	if (!dim->names)
		return dup;
	dup = copy_names(dup, isl_dim_param, 0, dim, isl_dim_param);
	dup = copy_names(dup, isl_dim_in, 0, dim, isl_dim_in);
	dup = copy_names(dup, isl_dim_out, 0, dim, isl_dim_out);
	return dup;
error:
	isl_dim_free(dup);
	return NULL;
}

struct isl_dim *isl_dim_cow(struct isl_dim *dim)
{
	if (!dim)
		return NULL;

	if (dim->ref == 1)
		return dim;
	dim->ref--;
	return isl_dim_dup(dim);
}

struct isl_dim *isl_dim_copy(struct isl_dim *dim)
{
	if (!dim)
		return NULL;

	dim->ref++;
	return dim;
}

void isl_dim_free(struct isl_dim *dim)
{
	int i;

	if (!dim)
		return;

	if (--dim->ref > 0)
		return;

	isl_name_free(dim->ctx, dim->tuple_name[0]);
	isl_name_free(dim->ctx, dim->tuple_name[1]);

	isl_dim_free(dim->nested[0]);
	isl_dim_free(dim->nested[1]);

	for (i = 0; i < dim->n_name; ++i)
		isl_name_free(dim->ctx, dim->names[i]);
	free(dim->names);
	isl_ctx_deref(dim->ctx);
	
	free(dim);
}

static int name_ok(isl_ctx *ctx, const char *s)
{
	char *p;
	long dummy;

	dummy = strtol(s, &p, 0);
	if (p != s)
		isl_die(ctx, isl_error_invalid, "name looks like a number",
			return 0);

	return 1;
}

__isl_give isl_dim *isl_dim_set_tuple_name(__isl_take isl_dim *dim,
	enum isl_dim_type type, const char *s)
{
	struct isl_name *name;

	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;
	if (type != isl_dim_in && type != isl_dim_out)
		isl_die(dim->ctx, isl_error_invalid,
			"only input, output and set tuples can have names",
			goto error);
	if (!s) {
		name = NULL;
	} else {
		if (!name_ok(dim->ctx, s))
			goto error;
		name = isl_name_get(dim->ctx, s);
		if (!name)
			goto error;
	}

	isl_name_free(dim->ctx, dim->tuple_name[type - isl_dim_in]);
	dim->tuple_name[type - isl_dim_in] = name;

	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

const char *isl_dim_get_tuple_name(__isl_keep isl_dim *dim,
	 enum isl_dim_type type)
{
	struct isl_name *name;
	if (!dim)
		return NULL;
	if (type != isl_dim_in && type != isl_dim_out)
		return NULL;
	name = dim->tuple_name[type - isl_dim_in];
	return name ? name->name : NULL;
}

struct isl_dim *isl_dim_set_name(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos,
				 const char *s)
{
	struct isl_name *name;

	if (!dim)
		return NULL;
	if (!name_ok(dim->ctx, s))
		goto error;
	isl_name_free(dim->ctx, get_name(dim, type, pos));
	name = isl_name_get(dim->ctx, s);
	if (!name)
		goto error;
	return set_name(dim, type, pos, name);
error:
	isl_dim_free(dim);
	return NULL;
}

const char *isl_dim_get_name(struct isl_dim *dim,
				 enum isl_dim_type type, unsigned pos)
{
	struct isl_name *name = get_name(dim, type, pos);
	return name ? name->name : NULL;
}

static struct isl_name *tuple_name(__isl_keep isl_dim *dim,
	enum isl_dim_type type)
{
	if (!dim)
		return NULL;
	if (type == isl_dim_in)
		return dim->tuple_name[0];
	if (type == isl_dim_out)
		return dim->tuple_name[1];
	return NULL;
}

static __isl_keep isl_dim *nested(__isl_keep isl_dim *dim,
	enum isl_dim_type type)
{
	if (!dim)
		return NULL;
	if (type == isl_dim_in)
		return dim->nested[0];
	if (type == isl_dim_out)
		return dim->nested[1];
	return NULL;
}

int isl_dim_tuple_match(__isl_keep isl_dim *dim1, enum isl_dim_type dim1_type,
			__isl_keep isl_dim *dim2, enum isl_dim_type dim2_type)
{
	struct isl_name *name1, *name2;
	isl_dim *nested1, *nested2;

	if (n(dim1, dim1_type) != n(dim2, dim2_type))
		return 0;
	name1 = tuple_name(dim1, dim1_type);
	name2 = tuple_name(dim2, dim2_type);
	if (!name1 ^ !name2)
		return 0;
	if (name1 && name1->name != name2->name)
		return 0;
	nested1 = nested(dim1, dim1_type);
	nested2 = nested(dim2, dim2_type);
	if (!nested1 ^ !nested2)
		return 0;
	if (nested1 && !isl_dim_equal(nested1, nested2))
		return 0;
	return 1;
}

static int match(struct isl_dim *dim1, enum isl_dim_type dim1_type,
		struct isl_dim *dim2, enum isl_dim_type dim2_type)
{
	int i;

	if (!isl_dim_tuple_match(dim1, dim1_type, dim2, dim2_type))
		return 0;

	if (!dim1->names && !dim2->names)
		return 1;

	for (i = 0; i < n(dim1, dim1_type); ++i) {
		if (get_name(dim1, dim1_type, i) !=
		    get_name(dim2, dim2_type, i))
			return 0;
	}
	return 1;
}

int isl_dim_match(struct isl_dim *dim1, enum isl_dim_type dim1_type,
		struct isl_dim *dim2, enum isl_dim_type dim2_type)
{
	return match(dim1, dim1_type, dim2, dim2_type);
}

static void get_names(struct isl_dim *dim, enum isl_dim_type type,
	unsigned first, unsigned n, struct isl_name **names)
{
	int i;

	for (i = 0; i < n ; ++i)
		names[i] = get_name(dim, type, first+i);
}

struct isl_dim *isl_dim_extend(struct isl_dim *dim,
			unsigned nparam, unsigned n_in, unsigned n_out)
{
	struct isl_name **names = NULL;

	if (!dim)
		return NULL;
	if (dim->nparam == nparam && dim->n_in == n_in && dim->n_out == n_out)
		return dim;

	isl_assert(dim->ctx, dim->nparam <= nparam, goto error);
	isl_assert(dim->ctx, dim->n_in <= n_in, goto error);
	isl_assert(dim->ctx, dim->n_out <= n_out, goto error);

	dim = isl_dim_cow(dim);

	if (dim->names) {
		names = isl_calloc_array(dim->ctx, struct isl_name *,
					 nparam + n_in + n_out);
		if (!names)
			goto error;
		get_names(dim, isl_dim_param, 0, dim->nparam, names);
		get_names(dim, isl_dim_in, 0, dim->n_in, names + nparam);
		get_names(dim, isl_dim_out, 0, dim->n_out,
				names + nparam + n_in);
		free(dim->names);
		dim->names = names;
		dim->n_name = nparam + n_in + n_out;
	}
	dim->nparam = nparam;
	dim->n_in = n_in;
	dim->n_out = n_out;

	return dim;
error:
	free(names);
	isl_dim_free(dim);
	return NULL;
}

struct isl_dim *isl_dim_add(struct isl_dim *dim, enum isl_dim_type type,
	unsigned n)
{
	if (!dim)
		return NULL;
	dim = isl_dim_reset(dim, type);
	switch (type) {
	case isl_dim_param:
		dim = isl_dim_extend(dim,
					dim->nparam + n, dim->n_in, dim->n_out);
		if (dim && dim->nested[0] &&
		    !(dim->nested[0] = isl_dim_add(dim->nested[0],
						    isl_dim_param, n)))
			goto error;
		if (dim && dim->nested[1] &&
		    !(dim->nested[1] = isl_dim_add(dim->nested[1],
						    isl_dim_param, n)))
			goto error;
		return dim;
	case isl_dim_in:
		return isl_dim_extend(dim,
					dim->nparam, dim->n_in + n, dim->n_out);
	case isl_dim_out:
		return isl_dim_extend(dim,
					dim->nparam, dim->n_in, dim->n_out + n);
	default:
		isl_die(dim->ctx, isl_error_invalid,
			"cannot add dimensions of specified type", goto error);
	}
error:
	isl_dim_free(dim);
	return NULL;
}

static int valid_dim_type(enum isl_dim_type type)
{
	switch (type) {
	case isl_dim_param:
	case isl_dim_in:
	case isl_dim_out:
		return 1;
	default:
		return 0;
	}
}

__isl_give isl_dim *isl_dim_insert(__isl_take isl_dim *dim,
	enum isl_dim_type type, unsigned pos, unsigned n)
{
	struct isl_name **names = NULL;

	if (!dim)
		return NULL;
	if (n == 0)
		return isl_dim_reset(dim, type);

	if (!valid_dim_type(type))
		isl_die(dim->ctx, isl_error_invalid,
			"cannot insert dimensions of specified type",
			goto error);

	isl_assert(dim->ctx, pos <= isl_dim_size(dim, type), goto error);

	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;

	if (dim->names) {
		enum isl_dim_type t;
		int off;
		int s[3];
		int *size = s - isl_dim_param;
		names = isl_calloc_array(dim->ctx, struct isl_name *,
				     dim->nparam + dim->n_in + dim->n_out + n);
		if (!names)
			goto error;
		off = 0;
		size[isl_dim_param] = dim->nparam;
		size[isl_dim_in] = dim->n_in;
		size[isl_dim_out] = dim->n_out;
		for (t = isl_dim_param; t <= isl_dim_out; ++t) {
			if (t != type) {
				get_names(dim, t, 0, size[t], names + off);
				off += size[t];
			} else {
				get_names(dim, t, 0, pos, names + off);
				off += pos + n;
				get_names(dim, t, pos, size[t]-pos, names+off);
				off += size[t] - pos;
			}
		}
		free(dim->names);
		dim->names = names;
		dim->n_name = dim->nparam + dim->n_in + dim->n_out + n;
	}
	switch (type) {
	case isl_dim_param:	dim->nparam += n; break;
	case isl_dim_in:	dim->n_in += n; break;
	case isl_dim_out:	dim->n_out += n; break;
	default:		;
	}
	dim = isl_dim_reset(dim, type);

	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_dim *isl_dim_move(__isl_take isl_dim *dim,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	int i;

	if (!dim)
		return NULL;
	if (n == 0)
		return dim;

	isl_assert(dim->ctx, src_pos + n <= isl_dim_size(dim, src_type),
		goto error);

	if (dst_type == src_type && dst_pos == src_pos)
		return dim;

	isl_assert(dim->ctx, dst_type != src_type, goto error);

	dim = isl_dim_reset(dim, src_type);
	dim = isl_dim_reset(dim, dst_type);

	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;

	if (dim->names) {
		struct isl_name **names;
		enum isl_dim_type t;
		int off;
		int s[3];
		int *size = s - isl_dim_param;
		names = isl_calloc_array(dim->ctx, struct isl_name *,
					 dim->nparam + dim->n_in + dim->n_out);
		if (!names)
			goto error;
		off = 0;
		size[isl_dim_param] = dim->nparam;
		size[isl_dim_in] = dim->n_in;
		size[isl_dim_out] = dim->n_out;
		for (t = isl_dim_param; t <= isl_dim_out; ++t) {
			if (t == dst_type) {
				get_names(dim, t, 0, dst_pos, names + off);
				off += dst_pos;
				get_names(dim, src_type, src_pos, n, names+off);
				off += n;
				get_names(dim, t, dst_pos, size[t] - dst_pos,
						names + off);
				off += size[t] - dst_pos;
			} else if (t == src_type) {
				get_names(dim, t, 0, src_pos, names + off);
				off += src_pos;
				get_names(dim, t, src_pos + n,
					    size[t] - src_pos - n, names + off);
				off += size[t] - src_pos - n;
			} else {
				get_names(dim, t, 0, size[t], names + off);
				off += size[t];
			}
		}
		free(dim->names);
		dim->names = names;
		dim->n_name = dim->nparam + dim->n_in + dim->n_out;
	}

	switch (dst_type) {
	case isl_dim_param:	dim->nparam += n; break;
	case isl_dim_in:	dim->n_in += n; break;
	case isl_dim_out:	dim->n_out += n; break;
	default:		;
	}

	switch (src_type) {
	case isl_dim_param:	dim->nparam -= n; break;
	case isl_dim_in:	dim->n_in -= n; break;
	case isl_dim_out:	dim->n_out -= n; break;
	default:		;
	}

	if (dst_type != isl_dim_param && src_type != isl_dim_param)
		return dim;

	for (i = 0; i < 2; ++i) {
		if (!dim->nested[i])
			continue;
		dim->nested[i] = isl_dim_replace(dim->nested[i],
						 isl_dim_param, dim);
		if (!dim->nested[i])
			goto error;
	}

	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

struct isl_dim *isl_dim_join(struct isl_dim *left, struct isl_dim *right)
{
	struct isl_dim *dim;

	if (!left || !right)
		goto error;

	isl_assert(left->ctx, match(left, isl_dim_param, right, isl_dim_param),
			goto error);
	isl_assert(left->ctx,
		isl_dim_tuple_match(left, isl_dim_out, right, isl_dim_in),
		goto error);

	dim = isl_dim_alloc(left->ctx, left->nparam, left->n_in, right->n_out);
	if (!dim)
		goto error;

	dim = copy_names(dim, isl_dim_param, 0, left, isl_dim_param);
	dim = copy_names(dim, isl_dim_in, 0, left, isl_dim_in);
	dim = copy_names(dim, isl_dim_out, 0, right, isl_dim_out);

	if (dim && left->tuple_name[0] &&
	    !(dim->tuple_name[0] = isl_name_copy(dim->ctx, left->tuple_name[0])))
		goto error;
	if (dim && right->tuple_name[1] &&
	    !(dim->tuple_name[1] = isl_name_copy(dim->ctx, right->tuple_name[1])))
		goto error;
	if (dim && left->nested[0] &&
	    !(dim->nested[0] = isl_dim_copy(left->nested[0])))
		goto error;
	if (dim && right->nested[1] &&
	    !(dim->nested[1] = isl_dim_copy(right->nested[1])))
		goto error;

	isl_dim_free(left);
	isl_dim_free(right);

	return dim;
error:
	isl_dim_free(left);
	isl_dim_free(right);
	return NULL;
}

struct isl_dim *isl_dim_product(struct isl_dim *left, struct isl_dim *right)
{
	isl_dim *dom1, *dom2, *nest1, *nest2;

	if (!left || !right)
		goto error;

	isl_assert(left->ctx, match(left, isl_dim_param, right, isl_dim_param),
			goto error);

	dom1 = isl_dim_domain(isl_dim_copy(left));
	dom2 = isl_dim_domain(isl_dim_copy(right));
	nest1 = isl_dim_wrap(isl_dim_join(isl_dim_reverse(dom1), dom2));

	dom1 = isl_dim_range(left);
	dom2 = isl_dim_range(right);
	nest2 = isl_dim_wrap(isl_dim_join(isl_dim_reverse(dom1), dom2));

	return isl_dim_join(isl_dim_reverse(nest1), nest2);
error:
	isl_dim_free(left);
	isl_dim_free(right);
	return NULL;
}

__isl_give isl_dim *isl_dim_range_product(__isl_take isl_dim *left,
	__isl_take isl_dim *right)
{
	isl_dim *dom, *ran1, *ran2, *nest;

	if (!left || !right)
		goto error;

	isl_assert(left->ctx, match(left, isl_dim_param, right, isl_dim_param),
			goto error);
	if (!isl_dim_match(left, isl_dim_in, right, isl_dim_in))
		isl_die(left->ctx, isl_error_invalid,
			"domains need to match", goto error);

	dom = isl_dim_domain(isl_dim_copy(left));

	ran1 = isl_dim_range(left);
	ran2 = isl_dim_range(right);
	nest = isl_dim_wrap(isl_dim_join(isl_dim_reverse(ran1), ran2));

	return isl_dim_join(isl_dim_reverse(dom), nest);
error:
	isl_dim_free(left);
	isl_dim_free(right);
	return NULL;
}

__isl_give isl_dim *isl_dim_map_from_set(__isl_take isl_dim *dim)
{
	struct isl_name **names = NULL;

	if (!dim)
		return NULL;
	isl_assert(dim->ctx, dim->n_in == 0, goto error);
	if (dim->n_out == 0 && !isl_dim_is_named_or_nested(dim, isl_dim_out))
		return dim;
	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;
	if (dim->names) {
		names = isl_calloc_array(dim->ctx, struct isl_name *,
					dim->nparam + dim->n_out + dim->n_out);
		if (!names)
			goto error;
		get_names(dim, isl_dim_param, 0, dim->nparam, names);
		get_names(dim, isl_dim_out, 0, dim->n_out, names + dim->nparam);
	}
	dim->n_in = dim->n_out;
	if (names) {
		free(dim->names);
		dim->names = names;
		dim->n_name = dim->nparam + dim->n_out + dim->n_out;
		dim = copy_names(dim, isl_dim_out, 0, dim, isl_dim_in);
	}
	isl_name_free(dim->ctx, dim->tuple_name[0]);
	dim->tuple_name[0] = isl_name_copy(dim->ctx, dim->tuple_name[1]);
	isl_dim_free(dim->nested[0]);
	dim->nested[0] = isl_dim_copy(dim->nested[1]);
	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

static struct isl_dim *set_names(struct isl_dim *dim, enum isl_dim_type type,
	unsigned first, unsigned n, struct isl_name **names)
{
	int i;

	for (i = 0; i < n ; ++i)
		dim = set_name(dim, type, first+i, names[i]);

	return dim;
}

struct isl_dim *isl_dim_reverse(struct isl_dim *dim)
{
	unsigned t;
	isl_dim *nested;
	struct isl_name **names = NULL;
	struct isl_name *name;

	if (!dim)
		return NULL;
	if (match(dim, isl_dim_in, dim, isl_dim_out))
		return dim;

	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;

	name = dim->tuple_name[0];
	dim->tuple_name[0] = dim->tuple_name[1];
	dim->tuple_name[1] = name;

	nested = dim->nested[0];
	dim->nested[0] = dim->nested[1];
	dim->nested[1] = nested;

	if (dim->names) {
		names = isl_alloc_array(dim->ctx, struct isl_name *,
					dim->n_in + dim->n_out);
		if (!names)
			goto error;
		get_names(dim, isl_dim_in, 0, dim->n_in, names);
		get_names(dim, isl_dim_out, 0, dim->n_out, names + dim->n_in);
	}

	t = dim->n_in;
	dim->n_in = dim->n_out;
	dim->n_out = t;

	if (dim->names) {
		dim = set_names(dim, isl_dim_out, 0, dim->n_out, names);
		dim = set_names(dim, isl_dim_in, 0, dim->n_in, names + dim->n_out);
		free(names);
	}

	return dim;
error:
	free(names);
	isl_dim_free(dim);
	return NULL;
}

struct isl_dim *isl_dim_drop(struct isl_dim *dim, enum isl_dim_type type,
		unsigned first, unsigned num)
{
	int i;

	if (!dim)
		return NULL;

	if (num == 0)
		return isl_dim_reset(dim, type);

	if (!valid_dim_type(type))
		isl_die(dim->ctx, isl_error_invalid,
			"cannot drop dimensions of specified type", goto error);

	isl_assert(dim->ctx, first + num <= n(dim, type), goto error);
	dim = isl_dim_cow(dim);
	if (!dim)
		goto error;
	if (dim->names) {
		dim = extend_names(dim);
		if (!dim)
			goto error;
		for (i = 0; i < num; ++i)
			isl_name_free(dim->ctx, get_name(dim, type, first+i));
		for (i = first+num; i < n(dim, type); ++i)
			set_name(dim, type, i - num, get_name(dim, type, i));
		switch (type) {
		case isl_dim_param:
			get_names(dim, isl_dim_in, 0, dim->n_in,
				dim->names + offset(dim, isl_dim_in) - num);
		case isl_dim_in:
			get_names(dim, isl_dim_out, 0, dim->n_out,
				dim->names + offset(dim, isl_dim_out) - num);
		default:
			;
		}
		dim->n_name -= num;
	}
	switch (type) {
	case isl_dim_param:	dim->nparam -= num; break;
	case isl_dim_in:	dim->n_in -= num; break;
	case isl_dim_out:	dim->n_out -= num; break;
	default:		;
	}
	dim = isl_dim_reset(dim, type);
	if (type == isl_dim_param) {
		if (dim && dim->nested[0] &&
		    !(dim->nested[0] = isl_dim_drop(dim->nested[0],
						    isl_dim_param, first, num)))
			goto error;
		if (dim && dim->nested[1] &&
		    !(dim->nested[1] = isl_dim_drop(dim->nested[1],
						    isl_dim_param, first, num)))
			goto error;
	}
	return dim;
error:
	isl_dim_free(dim);
	return NULL;
}

struct isl_dim *isl_dim_drop_inputs(struct isl_dim *dim,
		unsigned first, unsigned n)
{
	if (!dim)
		return NULL;
	return isl_dim_drop(dim, isl_dim_in, first, n);
}

struct isl_dim *isl_dim_drop_outputs(struct isl_dim *dim,
		unsigned first, unsigned n)
{
	if (!dim)
		return NULL;
	return isl_dim_drop(dim, isl_dim_out, first, n);
}

struct isl_dim *isl_dim_domain(struct isl_dim *dim)
{
	if (!dim)
		return NULL;
	dim = isl_dim_drop_outputs(dim, 0, dim->n_out);
	return isl_dim_reverse(dim);
}

__isl_give isl_dim *isl_dim_from_domain(__isl_take isl_dim *dim)
{
	return isl_dim_reverse(dim);
}

struct isl_dim *isl_dim_range(struct isl_dim *dim)
{
	if (!dim)
		return NULL;
	return isl_dim_drop_inputs(dim, 0, dim->n_in);
}

__isl_give isl_dim *isl_dim_from_range(__isl_take isl_dim *dim)
{
	return dim;
}

__isl_give isl_dim *isl_dim_as_set_dim(__isl_take isl_dim *dim)
{
	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;

	dim->n_out += dim->n_in;
	dim->n_in = 0;
	dim = isl_dim_reset(dim, isl_dim_in);
	dim = isl_dim_reset(dim, isl_dim_out);

	return dim;
}

struct isl_dim *isl_dim_underlying(struct isl_dim *dim, unsigned n_div)
{
	int i;

	if (!dim)
		return NULL;
	if (n_div == 0 &&
	    dim->nparam == 0 && dim->n_in == 0 && dim->n_name == 0)
		return isl_dim_reset(isl_dim_reset(dim, isl_dim_in), isl_dim_out);
	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;
	dim->n_out += dim->nparam + dim->n_in + n_div;
	dim->nparam = 0;
	dim->n_in = 0;

	for (i = 0; i < dim->n_name; ++i)
		isl_name_free(dim->ctx, get_name(dim, isl_dim_out, i));
	dim->n_name = 0;
	dim = isl_dim_reset(dim, isl_dim_in);
	dim = isl_dim_reset(dim, isl_dim_out);

	return dim;
}

unsigned isl_dim_total(struct isl_dim *dim)
{
	return dim ? dim->nparam + dim->n_in + dim->n_out : 0;
}

int isl_dim_equal(struct isl_dim *dim1, struct isl_dim *dim2)
{
	return match(dim1, isl_dim_param, dim2, isl_dim_param) &&
	       isl_dim_tuple_match(dim1, isl_dim_in, dim2, isl_dim_in) &&
	       isl_dim_tuple_match(dim1, isl_dim_out, dim2, isl_dim_out);
}

int isl_dim_compatible(struct isl_dim *dim1, struct isl_dim *dim2)
{
	return dim1->nparam == dim2->nparam &&
	       dim1->n_in + dim1->n_out == dim2->n_in + dim2->n_out;
}

static uint32_t isl_hash_dim(uint32_t hash, __isl_keep isl_dim *dim)
{
	int i;
	struct isl_name *name;

	if (!dim)
		return hash;

	hash = isl_hash_builtin(hash, dim->nparam);
	hash = isl_hash_builtin(hash, dim->n_in);
	hash = isl_hash_builtin(hash, dim->n_out);

	for (i = 0; i < dim->nparam; ++i) {
		name = get_name(dim, isl_dim_param, i);
		hash = isl_hash_name(hash, name);
	}

	name = tuple_name(dim, isl_dim_in);
	hash = isl_hash_name(hash, name);
	name = tuple_name(dim, isl_dim_out);
	hash = isl_hash_name(hash, name);

	hash = isl_hash_dim(hash, dim->nested[0]);
	hash = isl_hash_dim(hash, dim->nested[1]);

	return hash;
}

uint32_t isl_dim_get_hash(__isl_keep isl_dim *dim)
{
	uint32_t hash;

	if (!dim)
		return 0;

	hash = isl_hash_init();
	hash = isl_hash_dim(hash, dim);

	return hash;
}

int isl_dim_is_wrapping(__isl_keep isl_dim *dim)
{
	if (!dim)
		return -1;

	if (dim->n_in != 0 || dim->tuple_name[0] || dim->nested[0])
		return 0;

	return dim->nested[1] != NULL;
}

__isl_give isl_dim *isl_dim_wrap(__isl_take isl_dim *dim)
{
	isl_dim *wrap;

	if (!dim)
		return NULL;

	wrap = isl_dim_alloc(dim->ctx, dim->nparam, 0, dim->n_in + dim->n_out);

	wrap = copy_names(wrap, isl_dim_param, 0, dim, isl_dim_param);
	wrap = copy_names(wrap, isl_dim_set, 0, dim, isl_dim_in);
	wrap = copy_names(wrap, isl_dim_set, dim->n_in, dim, isl_dim_out);

	if (!wrap)
		goto error;

	wrap->nested[1] = dim;

	return wrap;
error:
	isl_dim_free(dim);
	return NULL;
}

__isl_give isl_dim *isl_dim_unwrap(__isl_take isl_dim *dim)
{
	isl_dim *unwrap;

	if (!dim)
		return NULL;

	if (!isl_dim_is_wrapping(dim))
		isl_die(dim->ctx, isl_error_invalid, "not a wrapping dim",
			goto error);

	unwrap = isl_dim_copy(dim->nested[1]);
	isl_dim_free(dim);

	return unwrap;
error:
	isl_dim_free(dim);
	return NULL;
}

int isl_dim_is_named_or_nested(__isl_keep isl_dim *dim, enum isl_dim_type type)
{
	if (type != isl_dim_in && type != isl_dim_out)
		return 0;
	if (!dim)
		return -1;
	if (dim->tuple_name[type - isl_dim_in])
		return 1;
	if (dim->nested[type - isl_dim_in])
		return 1;
	return 0;
}

int isl_dim_may_be_set(__isl_keep isl_dim *dim)
{
	if (!dim)
		return -1;
	if (isl_dim_size(dim, isl_dim_in) != 0)
		return 0;
	if (isl_dim_is_named_or_nested(dim, isl_dim_in))
		return 0;
	return 1;
}

__isl_give isl_dim *isl_dim_reset(__isl_take isl_dim *dim,
	enum isl_dim_type type)
{
	if (!isl_dim_is_named_or_nested(dim, type))
		return dim;

	dim = isl_dim_cow(dim);
	if (!dim)
		return NULL;

	isl_name_free(dim->ctx, dim->tuple_name[type - isl_dim_in]);
	dim->tuple_name[type - isl_dim_in] = NULL;
	isl_dim_free(dim->nested[type - isl_dim_in]);
	dim->nested[type - isl_dim_in] = NULL;

	return dim;
}

__isl_give isl_dim *isl_dim_flatten(__isl_take isl_dim *dim)
{
	if (!dim)
		return NULL;
	if (!dim->nested[0] && !dim->nested[1])
		return dim;

	if (dim->nested[0])
		dim = isl_dim_reset(dim, isl_dim_in);
	if (dim && dim->nested[1])
		dim = isl_dim_reset(dim, isl_dim_out);

	return dim;
}

__isl_give isl_dim *isl_dim_flatten_range(__isl_take isl_dim *dim)
{
	if (!dim)
		return NULL;
	if (!dim->nested[1])
		return dim;

	return isl_dim_reset(dim, isl_dim_out);
}

/* Replace the dimensions of the given type of dst by those of src.
 */
__isl_give isl_dim *isl_dim_replace(__isl_take isl_dim *dst,
	enum isl_dim_type type, __isl_keep isl_dim *src)
{
	dst = isl_dim_cow(dst);

	if (!dst || !src)
		goto error;

	dst = isl_dim_drop(dst, type, 0, isl_dim_size(dst, type));
	dst = isl_dim_add(dst, type, isl_dim_size(src, type));
	dst = copy_names(dst, type, 0, src, type);

	if (dst && type == isl_dim_param) {
		int i;
		for (i = 0; i <= 1; ++i) {
			if (!dst->nested[i])
				continue;
			dst->nested[i] = isl_dim_replace(dst->nested[i],
							 type, src);
			if (!dst->nested[i])
				goto error;
		}
	}

	return dst;
error:
	isl_dim_free(dst);
	return NULL;
}

/* Given a dimension specification "dim" of a set, create a dimension
 * specification for the lift of the set.  In particular, the result
 * is of the form [dim -> local[..]], with n_local variables in the
 * range of the wrapped map.
 */
__isl_give isl_dim *isl_dim_lift(__isl_take isl_dim *dim, unsigned n_local)
{
	isl_dim *local_dim;

	if (!dim)
		return NULL;

	local_dim = isl_dim_dup(dim);
	local_dim = isl_dim_drop(local_dim, isl_dim_set, 0, dim->n_out);
	local_dim = isl_dim_add(local_dim, isl_dim_set, n_local);
	local_dim = isl_dim_set_tuple_name(local_dim, isl_dim_set, "local");
	dim = isl_dim_join(isl_dim_from_domain(dim),
			    isl_dim_from_range(local_dim));
	dim = isl_dim_wrap(dim);
	dim = isl_dim_set_tuple_name(dim, isl_dim_set, "lifted");

	return dim;
}

int isl_dim_can_zip(__isl_keep isl_dim *dim)
{
	if (!dim)
		return -1;

	return dim->nested[0] && dim->nested[1];
}

__isl_give isl_dim *isl_dim_zip(__isl_take isl_dim *dim)
{
	isl_dim *dom, *ran;
	isl_dim *dom_dom, *dom_ran, *ran_dom, *ran_ran;

	if (!isl_dim_can_zip(dim))
		isl_die(dim->ctx, isl_error_invalid, "dim cannot be zipped",
			goto error);

	if (!dim)
		return 0;
	dom = isl_dim_unwrap(isl_dim_domain(isl_dim_copy(dim)));
	ran = isl_dim_unwrap(isl_dim_range(dim));
	dom_dom = isl_dim_domain(isl_dim_copy(dom));
	dom_ran = isl_dim_range(dom);
	ran_dom = isl_dim_domain(isl_dim_copy(ran));
	ran_ran = isl_dim_range(ran);
	dom = isl_dim_join(isl_dim_from_domain(dom_dom),
			   isl_dim_from_range(ran_dom));
	ran = isl_dim_join(isl_dim_from_domain(dom_ran),
			   isl_dim_from_range(ran_ran));
	return isl_dim_join(isl_dim_from_domain(isl_dim_wrap(dom)),
			    isl_dim_from_range(isl_dim_wrap(ran)));
error:
	isl_dim_free(dim);
	return NULL;
}

int isl_dim_has_named_params(__isl_keep isl_dim *dim)
{
	int i;
	unsigned off;

	if (!dim)
		return -1;
	if (dim->nparam == 0)
		return 1;
	off = isl_dim_offset(dim, isl_dim_param);
	if (off + dim->nparam > dim->n_name)
		return 0;
	for (i = 0; i < dim->nparam; ++i)
		if (!dim->names[off + i])
			return 0;
	return 1;
}

/* Align the initial parameters of dim1 to match the order in dim2.
 */
__isl_give isl_dim *isl_dim_align_params(__isl_take isl_dim *dim1,
	__isl_take isl_dim *dim2)
{
	isl_reordering *exp;

	if (!isl_dim_has_named_params(dim1) || !isl_dim_has_named_params(dim2))
		isl_die(isl_dim_get_ctx(dim1), isl_error_invalid,
			"parameter alignment requires named parameters",
			goto error);

	exp = isl_parameter_alignment_reordering(dim1, dim2);
	isl_dim_free(dim1);
	isl_dim_free(dim2);
	if (!exp)
		return NULL;
	dim1 = isl_dim_copy(exp->dim);
	isl_reordering_free(exp);
	return dim1;
error:
	isl_dim_free(dim1);
	isl_dim_free(dim2);
	return NULL;
}
