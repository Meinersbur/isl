/*
 * Copyright 2005-2007 Universiteit Leiden
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, Leiden Institute of Advanced Computer Science,
 * Universiteit Leiden, Niels Bohrweg 1, 2333 CA Leiden, The Netherlands
 * and K.U.Leuven, Departement Computerwetenschappen, Celestijnenlaan 200A,
 * B-3001 Leuven, Belgium
 * and INRIA Saclay - Ile-de-France, Parc Club Orsay Universite,
 * ZAC des vignes, 4 rue Jacques Monod, 91893 Orsay, France 
 */

#include <isl_flow.h>

/* A private structure to keep track of a mapping together with
 * a user-specified identifier.
 */
struct isl_labeled_map {
	struct isl_map	*map;
	void		*data;
};

/* A structure containing the input for dependence analysis:
 * - a sink
 * - n_source (<= max_source) sources
 * - a function for determining the relative order of sources and sink
 */
struct isl_access_info {
	struct isl_labeled_map	sink;
	isl_access_level_before	level_before;
	int		    	max_source;
	int		    	n_source;
	struct isl_labeled_map	source[1];
};

/* A structure containing the output of dependence analysis:
 * - n_source flow dependences
 * - a subset of the sink for which no source could be found
 */
struct isl_flow {
	struct isl_set		*no_source;
	int			n_source;
	struct isl_labeled_map	*dep;
};

/* Construct an isl_access_info structure and fill it up with
 * the given data.  The number of sources is set to 0.
 */
__isl_give isl_access_info *isl_access_info_alloc(__isl_take isl_map *sink,
	void *sink_user, isl_access_level_before fn, int max_source)
{
	struct isl_access_info *acc;

	if (!sink)
		return NULL;

	isl_assert(sink->ctx, max_source >= 0, goto error);

	acc = isl_alloc(sink->ctx, struct isl_access_info,
			sizeof(struct isl_access_info) +
			(max_source - 1) * sizeof(struct isl_labeled_map));
	if (!acc)
		goto error;

	acc->sink.map = sink;
	acc->sink.data = sink_user;
	acc->level_before = fn;
	acc->max_source = max_source;
	acc->n_source = 0;

	return acc;
error:
	isl_map_free(sink);
	return NULL;
}

/* Free the given isl_access_info structure.
 * This function is static because the user is expected to call
 * isl_access_info_compute_flow on any isl_access_info structure
 * he creates.
 */
static isl_access_info_free(__isl_take isl_access_info *acc)
{
	int i;

	if (!acc)
		return;
	isl_map_free(acc->sink.map);
	for (i = 0; i < acc->n_source; ++i)
		isl_map_free(acc->source[i].map);
	free(acc);
}

/* Add another source to an isl_access_info structure.
 * This function may be called at most max_source times on a
 * given isl_access_info structure, with max_source as specified
 * in the call to isl_access_info_alloc that constructed the structure.
 */
__isl_give isl_access_info *isl_access_info_add_source(
	__isl_take isl_access_info *acc, __isl_take isl_map *source,
	void *source_user)
{
	if (!acc)
		return NULL;
	isl_assert(acc->sink.map->ctx, acc->n_source < acc->max_source, goto error);

	acc->source[acc->n_source].map = source;
	acc->source[acc->n_source].data = source_user;
	acc->n_source++;

	return acc;
error:
	isl_map_free(source);
	isl_access_info_free(acc);
	return NULL;
}

/* A temporary structure used while sorting the accesses in an isl_access_info.
 */
struct isl_access_sort_info {
	struct isl_map		*source_map;
	void			*source_data;
	struct isl_access_info	*acc;
};

/* Return -n, 0 or n (with n a positive value), depending on whether
 * the source access identified by p1 should be sorted before, together
 * or after that identified by p2.
 *
 * If p1 and p2 share a different number of levels with the sink,
 * then the one with the lowest number of shared levels should be
 * sorted first.
 * If they both share no levels, then the order is irrelevant.
 * Otherwise, if p1 appears before p2, then it should be sorted first.
 */
static int access_sort_cmp(const void *p1, const void *p2)
{
	const struct isl_access_sort_info *i1, *i2;
	int level1, level2;
	i1 = (const struct isl_access_sort_info *) p1;
	i2 = (const struct isl_access_sort_info *) p2;

	level1 = i1->acc->level_before(i1->source_data, i1->acc->sink.data);
	level2 = i2->acc->level_before(i2->source_data, i2->acc->sink.data);

	if (level1 != level2 || !level1)
		return level1 - level2;

	level1 = i1->acc->level_before(i1->source_data, i2->source_data);

	return (level1 % 2) ? -1 : 1;
}

/* Sort the source accesses in order of increasing number of shared
 * levels with the sink access.
 * Source accesses with the same number of shared levels are sorted
 * in their textual order.
 */
static __isl_give isl_access_info *isl_access_info_sort_sources(
	__isl_take isl_access_info *acc)
{
	int i;
	struct isl_access_sort_info *array;

	if (!acc)
		return NULL;
	if (acc->n_source <= 1)
		return acc;

	array = isl_alloc_array(acc->sink.map->ctx,
				struct isl_access_sort_info, acc->n_source);
	if (!array)
		goto error;

	for (i = 0; i < acc->n_source; ++i) {
		array[i].source_map = acc->source[i].map;
		array[i].source_data = acc->source[i].data;
		array[i].acc = acc;
	}

	qsort(array, acc->n_source, sizeof(struct isl_access_sort_info),
		access_sort_cmp);

	for (i = 0; i < acc->n_source; ++i) {
		acc->source[i].map = array[i].source_map;
		acc->source[i].data = array[i].source_data;
	}

	free(array);

	return acc;
error:
	isl_access_info_free(acc);
	return NULL;
}

/* Initialize an empty isl_flow structure corresponding to a given
 * isl_access_info structure.
 * This function is private as isl_flow structures are only supposed
 * to be created by isl_access_info_compute_flow.
 */
static __isl_give isl_flow *isl_flow_alloc(__isl_keep isl_access_info *acc)
{
	int i;
	struct isl_ctx *ctx;
	struct isl_flow *dep;

	if (!acc)
		return NULL;

	ctx = acc->sink.map->ctx;
	dep = isl_calloc_type(ctx, struct isl_flow);
	if (!dep)
		return NULL;

	dep->dep = isl_alloc_array(ctx, struct isl_labeled_map, acc->n_source);
	if (!dep->dep)
		goto error;

	dep->n_source = acc->n_source;
	for (i = 0; i < acc->n_source; ++i) {
		struct isl_dim *dim;
		dim = isl_dim_join(isl_dim_copy(acc->source[i].map->dim),
			    isl_dim_reverse(isl_dim_copy(acc->sink.map->dim)));
		dep->dep[i].map = isl_map_empty(dim);
		dep->dep[i].data = acc->source[i].data;
	}

	return dep;
error:
	isl_flow_free(dep);
	return NULL;
}

/* Iterate over all sources and for each resulting flow dependence
 * that is not empty, call the user specfied function.
 * The second argument in this function call identifies the source,
 * while the third argument correspond to the final argument of
 * the isl_flow_foreach call.
 */
int isl_flow_foreach(__isl_keep isl_flow *deps,
	int (*fn)(__isl_take isl_map *dep, void *dep_user, void *user),
	void *user)
{
	int i;

	if (!deps)
		return -1;

	for (i = 0; i < deps->n_source; ++i) {
		if (isl_map_fast_is_empty(deps->dep[i].map))
			continue;
		if (fn(isl_map_copy(deps->dep[i].map), deps->dep[i].data, user) < 0)
			return -1;
	}

	return 0;
}

/* Return a copy of the subset of the sink for which no source could be found.
 */
__isl_give isl_set *isl_flow_get_no_source(__isl_keep isl_flow *deps)
{
	if (!deps)
		return NULL;
	
	return isl_set_copy(deps->no_source);
}

void isl_flow_free(__isl_take isl_flow *deps)
{
	int i;

	if (!deps)
		return;
	isl_set_free(deps->no_source);
	if (deps->dep) {
		for (i = 0; i < deps->n_source; ++i)
			isl_map_free(deps->dep[i].map);
		free(deps->dep);
	}
	free(deps);
}

/* Return a map that enforces that the domain iteration occurs after
 * the range iteration at the given level.
 * If level is odd, then the domain iteration should occur after
 * the target iteration in their shared level/2 outermost loops.
 * In this case we simply need to enforce that these outermost
 * loop iterations are the same.
 * If level is even, then the loop iterator of the domain should
 * be greater than the loop iterator of the range at the last
 * of the level/2 shared loops, i.e., loop level/2 - 1.
 */
static __isl_give isl_map *after_at_level(struct isl_dim *dim, int level)
{
	struct isl_basic_map *bmap;

	if (level % 2)
		bmap = isl_basic_map_equal(dim, level/2);
	else
		bmap = isl_basic_map_more_at(dim, level/2 - 1);

	return isl_map_from_basic_map(bmap);
}

/* Compute the last iteration of source j that precedes the sink at the given
 * level for sink iterations in set_C.
 * The subset of set_C for which no such iteration can be found is returned
 * in *empty.
 */
static struct isl_map *last_source(struct isl_access_info *acc, 
				    struct isl_set *set_C,
				    int j, int level, struct isl_set **empty)
{
	struct isl_map *read_map;
	struct isl_map *write_map;
	struct isl_map *dep_map;
	struct isl_map *after;
	struct isl_map *result;

	read_map = isl_map_copy(acc->sink.map);
	write_map = isl_map_copy(acc->source[j].map);
	write_map = isl_map_reverse(write_map);
	dep_map = isl_map_apply_range(read_map, write_map);
	after = after_at_level(isl_dim_copy(dep_map->dim), level);
	dep_map = isl_map_intersect(dep_map, after);
	result = isl_map_partial_lexmax(dep_map, set_C, empty);
	result = isl_map_reverse(result);

	return result;
}

/* For a given mapping between iterations of source j and iterations
 * of the sink, compute the last iteration of source k preceding
 * the sink at level before_level for any of the sink iterations,
 * but following the corresponding iteration of source j at level
 * after_level.
 */
static struct isl_map *last_later_source(struct isl_access_info *acc,
					 struct isl_map *old_map,
					 int j, int before_level,
					 int k, int after_level,
					 struct isl_set **empty)
{
	struct isl_dim *dim;
	struct isl_set *set_C;
	struct isl_map *read_map;
	struct isl_map *write_map;
	struct isl_map *dep_map;
	struct isl_map *after_write;
	struct isl_map *before_read;
	struct isl_map *result;

	set_C = isl_map_range(isl_map_copy(old_map));
	read_map = isl_map_copy(acc->sink.map);
	write_map = isl_map_copy(acc->source[k].map);

	write_map = isl_map_reverse(write_map);
	dep_map = isl_map_apply_range(read_map, write_map);
	dim = isl_dim_join(isl_dim_copy(acc->source[k].map->dim),
		    isl_dim_reverse(isl_dim_copy(acc->source[j].map->dim)));
	after_write = after_at_level(dim, after_level);
	after_write = isl_map_apply_range(after_write, old_map);
	after_write = isl_map_reverse(after_write);
	dep_map = isl_map_intersect(dep_map, after_write);
	before_read = after_at_level(isl_dim_copy(dep_map->dim), before_level);
	dep_map = isl_map_intersect(dep_map, before_read);
	result = isl_map_partial_lexmax(dep_map, set_C, empty);
	result = isl_map_reverse(result);

	return result;
}

/* Given a shared_level between two accesses, return 1 if the
 * the first can precede the second at the requested target_level.
 * If the target level is odd, i.e., refers to a statement level
 * dimension, then first needs to precede second at the requested
 * level, i.e., shared_level must be equal to target_level.
 * If the target level is odd, then the two loops should share
 * at least the requested number of outer loops.
 */
static int can_precede_at_level(int shared_level, int target_level)
{
	if (shared_level < target_level)
		return 0;
	if ((target_level % 2) && shared_level > target_level)
		return 0;
	return 1;
}

/* Given a possible flow dependence temp_rel[j] between source j and the sink
 * at level sink_level, remove those elements for which
 * there is an iteration of another source k < j that is closer to the sink.
 * The flow dependences temp_rel[k] are updated with the improved sources.
 * Any improved source needs to precede the sink at the same level
 * and needs to follow source j at the same or a deeper level.
 * The lower this level, the later the execution date of source k.
 * We therefore consider lower levels first.
 *
 * If temp_rel[j] is empty, then there can be no improvement and
 * we return immediately.
 */
static int intermediate_sources(__isl_keep isl_access_info *acc,
	struct isl_map **temp_rel, int j, int sink_level)
{
	int k, level;
	int depth = 2 * isl_map_dim(acc->source[j].map, isl_dim_in) + 1;

	if (isl_map_fast_is_empty(temp_rel[j]))
		return 0;

	for (k = j - 1; k >= 0; --k) {
		int plevel, plevel2;
		plevel = acc->level_before(acc->source[k].data, acc->sink.data);
		if (!can_precede_at_level(plevel, sink_level))
			continue;

		plevel2 = acc->level_before(acc->source[j].data,
						acc->source[k].data);

		for (level = sink_level; level <= depth; ++level) {
			struct isl_map *T;
			struct isl_set *trest;
			struct isl_map *copy;

			if (!can_precede_at_level(plevel2, level))
				continue;

			copy = isl_map_copy(temp_rel[j]);
			T = last_later_source(acc, copy, j, sink_level, k,
					      level, &trest);
			if (isl_map_fast_is_empty(T)) {
				isl_set_free(trest);
				isl_map_free(T);
				continue;
			}
			temp_rel[j] = isl_map_intersect_range(temp_rel[j], trest);
			temp_rel[k] = isl_map_union_disjoint(temp_rel[k], T);
		}
	}

	return 0;
}

/* Given a "sink" access, a list of n "source" accesses,
 * compute for each iteration of the sink access,
 * the source access in the list that last accessed the
 * same element accessed by the sink access before this sink access.
 * Each access is given as a map from the loop iterators
 * to the array indices.
 * The result is a list of n relations between source and sink
 * iterations and a subset of the domain of the sink access,
 * corresponding to those iterations that access an element
 * not previously accessed.
 *
 * The algorithm considers all levels in which a source may precede the sink,
 * where a level may either be a statement level or a loop level.
 * The outermost statement level is 1, the first loop level is 2, etc...
 * The algorithm basically does the following:
 * for all levels l of the read access from innermost to outermost
 *	for all sources w that may precede the sink access at that level
 *	    compute the last iteration of the source that precedes the sink access
 *					    at that level
 *	    add result to possible last accesses at level l of source w
 *	    for all sources w2 that we haven't considered yet at this level that may
 *					    also precede the sink access
 *		for all levels l2 of w from l to innermost
 *		    for all possible last accesses dep of w at l
 *			compute last iteration of w2 between the source and sink
 *								of dep
 *			add result to possible last accesses at level l of write w2
 *			and replace possible last accesses dep by the remainder
 */
__isl_give isl_flow *isl_access_info_compute_flow(__isl_take isl_access_info *acc)
{
	struct isl_ctx *ctx;
	struct isl_set *todo;
	int level, j;
	int depth;
	struct isl_map **temp_rel;
	struct isl_flow *res;

	acc = isl_access_info_sort_sources(acc);

	res = isl_flow_alloc(acc);
	if (!res)
		goto error;
	ctx = acc->sink.map->ctx;

	depth = 2 * isl_map_dim(acc->sink.map, isl_dim_in) + 1;
	todo = isl_map_domain(isl_map_copy(acc->sink.map));
	if (isl_set_fast_is_empty(todo))
		goto done;

	temp_rel = isl_alloc_array(ctx, struct isl_map *, acc->n_source);

	for (level = depth; level >= 1; --level) {
		for (j = acc->n_source-1; j >=0; --j)
			temp_rel[j] = isl_map_empty_like(res->dep[j].map);

		for (j = acc->n_source - 1; j >= 0; --j) {
			struct isl_map *T;
			struct isl_set *rest;
			int plevel;

			plevel = acc->level_before(acc->source[j].data,
						     acc->sink.data);
			if (!can_precede_at_level(plevel, level))
				continue;

			T = last_source(acc, todo, j, level, &rest);
			temp_rel[j] = isl_map_union_disjoint(temp_rel[j], T);
			todo = rest;

			intermediate_sources(acc, temp_rel, j, level);

			if (isl_set_fast_is_empty(todo))
				break;
		}
		for (j = j - 1; j >= 0; --j) {
			int plevel;

			plevel = acc->level_before(acc->source[j].data,
						     acc->sink.data);
			if (!can_precede_at_level(plevel, level))
				continue;

			intermediate_sources(acc, temp_rel, j, level);
		}

		for (j = acc->n_source - 1; j >= 0; --j)
			res->dep[j].map = isl_map_union_disjoint(res->dep[j].map,
							     temp_rel[j]);
		if (isl_set_fast_is_empty(todo))
			break;
	}

	free(temp_rel);
done:
	res->no_source = todo;
	isl_access_info_free(acc);
	return res;
error:
	isl_access_info_free(acc);
	return NULL;
}
