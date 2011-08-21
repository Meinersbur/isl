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
	return band ? isl_union_map_get_ctx(band->map) : NULL;
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

	isl_union_map_free(band->map);
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
	isl_union_map *prefix;
	isl_band *a;

	if (!band)
		return NULL;

	prefix = isl_union_map_copy(band->map);
	prefix = isl_union_map_from_domain(isl_union_map_domain(prefix));

	for (a = band->parent; a; a = a->parent) {
		isl_union_map *partial = isl_union_map_copy(a->map);
		prefix = isl_union_map_flat_range_product(partial, prefix);
	}

	return prefix;
}

/* Return the schedule of the band in isolation.
 */
__isl_give isl_union_map *isl_band_get_partial_schedule(
	__isl_keep isl_band *band)
{
	return band ? isl_union_map_copy(band->map) : NULL;
}

/* Return the schedule for the forest underneath the given band.
 */
__isl_give isl_union_map *isl_band_get_suffix_schedule(
	__isl_keep isl_band *band)
{
	isl_union_map *suffix;

	if (!band)
		return NULL;

	if (!isl_band_has_children(band)) {
		suffix = isl_union_map_copy(band->map);
		suffix = isl_union_map_from_domain(isl_union_map_domain(suffix));
	} else {
		int i, n;
		isl_band_list *children;

		suffix = isl_union_map_empty(isl_union_map_get_space(band->map));
		children = isl_band_get_children(band);
		n = isl_band_list_n_band(children);
		for (i = 0; i < n; ++i) {
			isl_band *child;
			isl_union_map *partial_i;
			isl_union_map *suffix_i;

			child = isl_band_list_get_band(children, i);
			partial_i = isl_band_get_partial_schedule(child);
			suffix_i = isl_band_get_suffix_schedule(child);
			suffix_i = isl_union_map_flat_range_product(partial_i,
								    suffix_i);
			suffix = isl_union_map_union(suffix, suffix_i);

			isl_band_free(child);
		}
		isl_band_list_free(children);
	}

	return suffix;
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
