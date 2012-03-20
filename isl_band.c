/*
 * Copyright 2011      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 */

#include <isl_band_private.h>
#include <isl_schedule_private.h>
#include <isl_list_private.h>

isl_ctx *isl_band_get_ctx(__isl_keep isl_band *band)
{
	return band ? isl_union_pw_multi_aff_get_ctx(band->pma) : NULL;
}

__isl_give isl_band *isl_band_alloc(isl_ctx *ctx)
{
	isl_band *band;

	band = isl_calloc_type(ctx, isl_band);
	if (!band)
		return NULL;

	band->ref = 1;

	return band;
}

/* Create a duplicate of the given band.  The duplicate refers
 * to the same schedule and parent as the input, but does not
 * increment their reference counts.
 */
__isl_give isl_band *isl_band_dup(__isl_keep isl_band *band)
{
	int i;
	isl_ctx *ctx;
	isl_band *dup;

	if (!band)
		return NULL;

	ctx = isl_band_get_ctx(band);
	dup = isl_band_alloc(ctx);
	if (!dup)
		return NULL;

	dup->n = band->n;
	dup->zero = isl_alloc_array(ctx, int, band->n);
	if (!dup->zero)
		goto error;

	for (i = 0; i < band->n; ++i)
		dup->zero[i] = band->zero[i];

	dup->pma = isl_union_pw_multi_aff_copy(band->pma);
	dup->schedule = band->schedule;
	dup->parent = band->parent;

	if (!dup->pma)
		goto error;

	return dup;
error:
	isl_band_free(dup);
	return NULL;
}

/* We not only increment the reference count of the band,
 * but also that of the schedule that contains this band.
 * This ensures that the schedule won't disappear while there
 * is still a reference to the band outside of the schedule.
 * There is no need to increment the reference count of the parent
 * band as the parent band is part of the same schedule.
 */
__isl_give isl_band *isl_band_copy(__isl_keep isl_band *band)
{
	if (!band)
		return NULL;

	band->ref++;
	band->schedule->ref++;
	return band;
}

/* If this is not the last reference to the band (the one from within the
 * schedule), then we also need to decrement the reference count of the
 * containing schedule as it was incremented in isl_band_copy.
 */
void *isl_band_free(__isl_take isl_band *band)
{
	if (!band)
		return NULL;

	if (--band->ref > 0)
		return isl_schedule_free(band->schedule);

	isl_union_pw_multi_aff_free(band->pma);
	isl_band_list_free(band->children);
	free(band->zero);
	free(band);

	return NULL;
}

int isl_band_has_children(__isl_keep isl_band *band)
{
	if (!band)
		return -1;

	return band->children != NULL;
}

__isl_give isl_band_list *isl_band_get_children(
	__isl_keep isl_band *band)
{
	if (!band)
		return NULL;
	if (!band->children)
		isl_die(isl_band_get_ctx(band), isl_error_invalid,
			"band has no children", return NULL);
	return isl_band_list_dup(band->children);
}

int isl_band_n_member(__isl_keep isl_band *band)
{
	return band ? band->n : 0;
}

/* Is the given scheduling dimension zero distance within the band and
 * with respect to the proximity dependences.
 */
int isl_band_member_is_zero_distance(__isl_keep isl_band *band, int pos)
{
	if (!band)
		return -1;

	if (pos < 0 || pos >= band->n)
		isl_die(isl_band_get_ctx(band), isl_error_invalid,
			"invalid member position", return -1);

	return band->zero[pos];
}

/* Return the schedule that leads up to this band.
 */
__isl_give isl_union_map *isl_band_get_prefix_schedule(
	__isl_keep isl_band *band)
{
	isl_union_set *domain;
	isl_union_pw_multi_aff *prefix;
	isl_band *a;

	if (!band)
		return NULL;

	prefix = isl_union_pw_multi_aff_copy(band->pma);
	domain = isl_union_pw_multi_aff_domain(prefix);
	prefix = isl_union_pw_multi_aff_from_domain(domain);

	for (a = band->parent; a; a = a->parent) {
		isl_union_pw_multi_aff *partial;

		partial = isl_union_pw_multi_aff_copy(a->pma);
		prefix = isl_union_pw_multi_aff_flat_range_product(partial,
								   prefix);
	}

	return isl_union_map_from_union_pw_multi_aff(prefix);
}

/* Return the schedule of the band in isolation.
 */
__isl_give isl_union_pw_multi_aff *
isl_band_get_partial_schedule_union_pw_multi_aff(__isl_keep isl_band *band)
{
	return band ? isl_union_pw_multi_aff_copy(band->pma) : NULL;
}

/* Return the schedule of the band in isolation.
 */
__isl_give isl_union_map *isl_band_get_partial_schedule(
	__isl_keep isl_band *band)
{
	isl_union_pw_multi_aff *sched;

	sched = isl_band_get_partial_schedule_union_pw_multi_aff(band);
	return isl_union_map_from_union_pw_multi_aff(sched);
}

__isl_give isl_union_pw_multi_aff *
isl_band_get_suffix_schedule_union_pw_multi_aff(__isl_keep isl_band *band);

/* Return the schedule for the given band list.
 * For each band in the list, the schedule is composed of the partial
 * and suffix schedules of that band.
 */
__isl_give isl_union_pw_multi_aff *
isl_band_list_get_suffix_schedule_union_pw_multi_aff(
	__isl_keep isl_band_list *list)
{
	isl_ctx *ctx;
	int i, n;
	isl_space *space;
	isl_union_pw_multi_aff *suffix;

	if (!list)
		return NULL;

	ctx = isl_band_list_get_ctx(list);
	space = isl_space_alloc(ctx, 0, 0, 0);
	suffix = isl_union_pw_multi_aff_empty(space);
	n = isl_band_list_n_band(list);
	for (i = 0; i < n; ++i) {
		isl_band *el;
		isl_union_pw_multi_aff *partial;
		isl_union_pw_multi_aff *suffix_i;

		el = isl_band_list_get_band(list, i);
		partial = isl_band_get_partial_schedule_union_pw_multi_aff(el);
		suffix_i = isl_band_get_suffix_schedule_union_pw_multi_aff(el);
		suffix_i = isl_union_pw_multi_aff_flat_range_product(
				partial, suffix_i);
		suffix = isl_union_pw_multi_aff_add(suffix, suffix_i);

		isl_band_free(el);
	}

	return suffix;
}

/* Return the schedule for the given band list.
 * For each band in the list, the schedule is composed of the partial
 * and suffix schedules of that band.
 */
__isl_give isl_union_map *isl_band_list_get_suffix_schedule(
	__isl_keep isl_band_list *list)
{
	isl_union_pw_multi_aff *suffix;

	suffix = isl_band_list_get_suffix_schedule_union_pw_multi_aff(list);
	return isl_union_map_from_union_pw_multi_aff(suffix);
}

/* Return the schedule for the forest underneath the given band.
 */
__isl_give isl_union_pw_multi_aff *
isl_band_get_suffix_schedule_union_pw_multi_aff(__isl_keep isl_band *band)
{
	isl_union_pw_multi_aff *suffix;

	if (!band)
		return NULL;

	if (!isl_band_has_children(band)) {
		isl_union_set *domain;

		suffix = isl_union_pw_multi_aff_copy(band->pma);
		domain = isl_union_pw_multi_aff_domain(suffix);
		suffix = isl_union_pw_multi_aff_from_domain(domain);
	} else {
		isl_band_list *list;

		list = isl_band_get_children(band);
		suffix =
		    isl_band_list_get_suffix_schedule_union_pw_multi_aff(list);
		isl_band_list_free(list);
	}

	return suffix;
}

/* Return the schedule for the forest underneath the given band.
 */
__isl_give isl_union_map *isl_band_get_suffix_schedule(
	__isl_keep isl_band *band)
{
	isl_union_pw_multi_aff *suffix;

	suffix = isl_band_get_suffix_schedule_union_pw_multi_aff(band);
	return isl_union_map_from_union_pw_multi_aff(suffix);
}

__isl_give isl_printer *isl_printer_print_band(__isl_take isl_printer *p,
	__isl_keep isl_band *band)
{
	isl_union_map *prefix, *partial, *suffix;

	prefix = isl_band_get_prefix_schedule(band);
	partial = isl_band_get_partial_schedule(band);
	suffix = isl_band_get_suffix_schedule(band);

	p = isl_printer_print_str(p, "(");
	p = isl_printer_print_union_map(p, prefix);
	p = isl_printer_print_str(p, ",");
	p = isl_printer_print_union_map(p, partial);
	p = isl_printer_print_str(p, ",");
	p = isl_printer_print_union_map(p, suffix);
	p = isl_printer_print_str(p, ")");

	isl_union_map_free(prefix);
	isl_union_map_free(partial);
	isl_union_map_free(suffix);

	return p;
}
