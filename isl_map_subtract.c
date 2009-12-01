#include "isl_seq.h"
#include "isl_set.h"
#include "isl_map.h"
#include "isl_map_private.h"

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
	copy = isl_basic_map_extend_constraints(copy, 0, 1);
	k = isl_basic_map_alloc_inequality(copy);
	if (k < 0)
		goto error;
	if (oppose)
		isl_seq_neg(copy->ineq[k], c, len);
	else
		isl_seq_cpy(copy->ineq[k], c, len);
	total = 1 + isl_basic_map_total_dim(copy);
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
	unsigned total = isl_basic_map_total_dim(bmap);

	map = isl_map_cow(map);

	if (!map || !bmap)
		goto error;

	if (ISL_F_ISSET(map, ISL_MAP_DISJOINT))
		ISL_FL_SET(flags, ISL_MAP_DISJOINT);

	max = map->n * (2 * bmap->n_eq + bmap->n_ineq);
	rest = isl_map_alloc_dim(isl_dim_copy(map->dim), max, flags);
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

			map->p[j] = isl_basic_map_extend_constraints(map->p[j],
				1, 0);
			if (!map->p[j])
				goto error;
			k = isl_basic_map_alloc_equality(map->p[j]);
			if (k < 0)
				goto error;
			isl_seq_cpy(map->p[j]->eq[k], bmap->eq[i], 1+total);
			isl_seq_clr(map->p[j]->eq[k]+1+total,
					map->p[j]->n_div - bmap->n_div);
		}
	}

	for (i = 0; i < bmap->n_ineq; ++i) {
		for (j = 0; j < map->n; ++j) {
			rest = add_cut_constraint(rest,
				map->p[j], bmap->ineq[i], 1+total, 0);
			if (!rest)
				goto error;

			map->p[j] = isl_basic_map_extend_constraints(map->p[j],
				0, 1);
			if (!map->p[j])
				goto error;
			k = isl_basic_map_alloc_inequality(map->p[j]);
			if (k < 0)
				goto error;
			isl_seq_cpy(map->p[j]->ineq[k], bmap->ineq[i], 1+total);
			isl_seq_clr(map->p[j]->ineq[k]+1+total,
					map->p[j]->n_div - bmap->n_div);
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

	isl_assert(map1->ctx, isl_dim_equal(map1->dim, map2->dim), goto error);

	if (isl_map_is_empty(map2)) {
		isl_map_free(map2);
		return map1;
	}

	map1 = isl_map_compute_divs(map1);
	map2 = isl_map_compute_divs(map2);
	if (!map1 || !map2)
		goto error;

	map1 = isl_map_remove_empty_parts(map1);
	map2 = isl_map_remove_empty_parts(map2);

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

/* Return 1 if "bmap" contains a single element.
 */
int isl_basic_map_is_singleton(__isl_keep isl_basic_map *bmap)
{
	if (!bmap)
		return -1;
	if (bmap->n_div)
		return 0;
	if (bmap->n_ineq)
		return 0;
	return bmap->n_eq == isl_basic_map_total_dim(bmap);
}

/* Return 1 if "map" contains a single element.
 */
int isl_map_is_singleton(__isl_keep isl_map *map)
{
	if (!map)
		return -1;
	if (map->n != 1)
		return 0;

	return isl_basic_map_is_singleton(map->p[0]);
}

/* Given a singleton basic map, extract the single element
 * as an isl_vec.
 */
static __isl_give isl_vec *singleton_extract_point(__isl_keep isl_basic_map *bmap)
{
	int i, j;
	unsigned dim;
	struct isl_vec *point;
	isl_int m;

	if (!bmap)
		return NULL;

	dim = isl_basic_map_total_dim(bmap);
	isl_assert(bmap->ctx, bmap->n_eq == dim, return NULL);
	point = isl_vec_alloc(bmap->ctx, 1 + dim);
	if (!point)
		return NULL;

	isl_int_init(m);

	isl_int_set_si(point->el[0], 1);
	for (j = 0; j < bmap->n_eq; ++j) {
		int s;
		int i = dim - 1 - j;
		isl_assert(bmap->ctx,
		    isl_seq_first_non_zero(bmap->eq[j] + 1, i) == -1,
		    goto error);
		isl_assert(bmap->ctx,
		    isl_int_is_one(bmap->eq[j][1 + i]) ||
		    isl_int_is_negone(bmap->eq[j][1 + i]),
		    goto error);
		isl_assert(bmap->ctx,
		    isl_seq_first_non_zero(bmap->eq[j]+1+i+1, dim-i-1) == -1,
		    goto error);

		isl_int_gcd(m, point->el[0], bmap->eq[j][1 + i]);
		isl_int_divexact(m, bmap->eq[j][1 + i], m);
		isl_int_abs(m, m);
		isl_seq_scale(point->el, point->el, m, 1 + i);
		isl_int_divexact(m, point->el[0], bmap->eq[j][1 + i]);
		isl_int_neg(m, m);
		isl_int_mul(point->el[1 + i], m, bmap->eq[j][0]);
	}

	isl_int_clear(m);
	return point;
error:
	isl_int_clear(m);
	isl_vec_free(point);
	return NULL;
}

/* Return 1 if "bmap" contains the point "point".
 * "bmap" is assumed to have known divs.
 * The point is first extended with the divs and then passed
 * to basic_map_contains.
 */
static int basic_map_contains_point(__isl_keep isl_basic_map *bmap,
	__isl_keep isl_vec *point)
{
	int i;
	struct isl_vec *vec;
	unsigned dim;
	int contains;

	if (!bmap || !point)
		return -1;
	if (bmap->n_div == 0)
		return isl_basic_map_contains(bmap, point);

	dim = isl_basic_map_total_dim(bmap) - bmap->n_div;
	vec = isl_vec_alloc(bmap->ctx, 1 + dim + bmap->n_div);
	if (!vec)
		return -1;

	isl_seq_cpy(vec->el, point->el, point->size);
	for (i = 0; i < bmap->n_div; ++i) {
		isl_seq_inner_product(bmap->div[i] + 1, vec->el,
					1 + dim + i, &vec->el[1+dim+i]);
		isl_int_fdiv_q(vec->el[1+dim+i], vec->el[1+dim+i],
				bmap->div[i][0]);
	}

	contains = isl_basic_map_contains(bmap, vec);

	isl_vec_free(vec);
	return contains;
}

/* Return 1 is the singleton map "map1" is a subset of "map2",
 * i.e., if the single element of "map1" is also an element of "map2".
 */
static int map_is_singleton_subset(__isl_keep isl_map *map1,
	__isl_keep isl_map *map2)
{
	int i;
	int is_subset = 0;
	struct isl_vec *point;

	if (!map1 || !map2)
		return -1;
	if (map1->n != 1)
		return -1;

	point = singleton_extract_point(map1->p[0]);
	if (!point)
		return -1;

	for (i = 0; i < map2->n; ++i) {
		is_subset = basic_map_contains_point(map2->p[i], point);
		if (is_subset)
			break;
	}

	isl_vec_free(point);
	return is_subset;
}

int isl_map_is_subset(struct isl_map *map1, struct isl_map *map2)
{
	int is_subset = 0;
	struct isl_map *diff;

	if (!map1 || !map2)
		return -1;

	if (isl_map_is_empty(map1))
		return 1;

	if (isl_map_is_empty(map2))
		return 0;

	if (isl_map_fast_is_universe(map2))
		return 1;

	map2 = isl_map_compute_divs(isl_map_copy(map2));
	if (isl_map_is_singleton(map1)) {
		is_subset = map_is_singleton_subset(map1, map2);
		isl_map_free(map2);
		return is_subset;
	}
	diff = isl_map_subtract(isl_map_copy(map1), map2);
	if (!diff)
		return -1;

	is_subset = isl_map_is_empty(diff);
	isl_map_free(diff);

	return is_subset;
}

int isl_set_is_subset(struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_is_subset(
			(struct isl_map *)set1, (struct isl_map *)set2);
}
