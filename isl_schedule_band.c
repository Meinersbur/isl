/*
 * Copyright 2013-2014 Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege,
 * Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_schedule_band.h>
#include <isl_schedule_private.h>

isl_ctx *isl_schedule_band_get_ctx(__isl_keep isl_schedule_band *band)
{
	return band ? isl_multi_union_pw_aff_get_ctx(band->mupa) : NULL;
}

/* Return a new uninitialized isl_schedule_band.
 */
static __isl_give isl_schedule_band *isl_schedule_band_alloc(isl_ctx *ctx)
{
	isl_schedule_band *band;

	band = isl_calloc_type(ctx, isl_schedule_band);
	if (!band)
		return NULL;

	band->ref = 1;

	return band;
}

/* Return a new isl_schedule_band with partial schedule "mupa".
 * First replace "mupa" by its greatest integer part to ensure
 * that the schedule is always integral.
 * The band is not marked permutable and the dimensions are not
 * marked coincident.
 */
__isl_give isl_schedule_band *isl_schedule_band_from_multi_union_pw_aff(
	__isl_take isl_multi_union_pw_aff *mupa)
{
	isl_ctx *ctx;
	isl_schedule_band *band;

	mupa = isl_multi_union_pw_aff_floor(mupa);
	if (!mupa)
		return NULL;
	ctx = isl_multi_union_pw_aff_get_ctx(mupa);
	band = isl_schedule_band_alloc(ctx);
	if (!band)
		goto error;

	band->n = isl_multi_union_pw_aff_dim(mupa, isl_dim_set);
	band->coincident = isl_calloc_array(ctx, int, band->n);
	band->mupa = mupa;

	if (band->n && !band->coincident)
		return isl_schedule_band_free(band);

	return band;
error:
	isl_multi_union_pw_aff_free(mupa);
	return NULL;
}

/* Create a duplicate of the given isl_schedule_band.
 */
__isl_give isl_schedule_band *isl_schedule_band_dup(
	__isl_keep isl_schedule_band *band)
{
	int i;
	isl_ctx *ctx;
	isl_schedule_band *dup;

	if (!band)
		return NULL;

	ctx = isl_schedule_band_get_ctx(band);
	dup = isl_schedule_band_alloc(ctx);
	if (!dup)
		return NULL;

	dup->n = band->n;
	dup->coincident = isl_alloc_array(ctx, int, band->n);
	if (band->n && !dup->coincident)
		return isl_schedule_band_free(dup);

	for (i = 0; i < band->n; ++i)
		dup->coincident[i] = band->coincident[i];
	dup->permutable = band->permutable;

	dup->mupa = isl_multi_union_pw_aff_copy(band->mupa);
	if (!dup->mupa)
		return isl_schedule_band_free(dup);

	return dup;
}

/* Return an isl_schedule_band that is equal to "band" and that has only
 * a single reference.
 */
__isl_give isl_schedule_band *isl_schedule_band_cow(
	__isl_take isl_schedule_band *band)
{
	if (!band)
		return NULL;

	if (band->ref == 1)
		return band;
	band->ref--;
	return isl_schedule_band_dup(band);
}

/* Return a new reference to "band".
 */
__isl_give isl_schedule_band *isl_schedule_band_copy(
	__isl_keep isl_schedule_band *band)
{
	if (!band)
		return NULL;

	band->ref++;
	return band;
}

/* Free a reference to "band" and return NULL.
 */
__isl_null isl_schedule_band *isl_schedule_band_free(
	__isl_take isl_schedule_band *band)
{
	if (!band)
		return NULL;

	if (--band->ref > 0)
		return NULL;

	isl_multi_union_pw_aff_free(band->mupa);
	free(band->coincident);
	free(band);

	return NULL;
}

/* Return the number of scheduling dimensions in the band.
 */
int isl_schedule_band_n_member(__isl_keep isl_schedule_band *band)
{
	return band ? band->n : 0;
}

/* Is the given scheduling dimension coincident within the band and
 * with respect to the coincidence constraints?
 */
int isl_schedule_band_member_get_coincident(__isl_keep isl_schedule_band *band,
	int pos)
{
	if (!band)
		return -1;

	if (pos < 0 || pos >= band->n)
		isl_die(isl_schedule_band_get_ctx(band), isl_error_invalid,
			"invalid member position", return -1);

	return band->coincident[pos];
}

/* Mark the given scheduling dimension as being coincident or not
 * according to "coincident".
 */
__isl_give isl_schedule_band *isl_schedule_band_member_set_coincident(
	__isl_take isl_schedule_band *band, int pos, int coincident)
{
	if (!band)
		return NULL;
	if (isl_schedule_band_member_get_coincident(band, pos) == coincident)
		return band;
	band = isl_schedule_band_cow(band);
	if (!band)
		return NULL;

	if (pos < 0 || pos >= band->n)
		isl_die(isl_schedule_band_get_ctx(band), isl_error_invalid,
			"invalid member position",
			isl_schedule_band_free(band));

	band->coincident[pos] = coincident;

	return band;
}

/* Is the schedule band mark permutable?
 */
int isl_schedule_band_get_permutable(__isl_keep isl_schedule_band *band)
{
	if (!band)
		return -1;
	return band->permutable;
}

/* Mark the schedule band permutable or not according to "permutable"?
 */
__isl_give isl_schedule_band *isl_schedule_band_set_permutable(
	__isl_take isl_schedule_band *band, int permutable)
{
	if (!band)
		return NULL;
	if (band->permutable == permutable)
		return band;
	band = isl_schedule_band_cow(band);
	if (!band)
		return NULL;

	band->permutable = permutable;

	return band;
}

/* Return the schedule space of the band.
 */
__isl_give isl_space *isl_schedule_band_get_space(
	__isl_keep isl_schedule_band *band)
{
	if (!band)
		return NULL;
	return isl_multi_union_pw_aff_get_space(band->mupa);
}

/* Return the schedule of the band in isolation.
 */
__isl_give isl_multi_union_pw_aff *isl_schedule_band_get_partial_schedule(
	__isl_keep isl_schedule_band *band)
{
	return band ? isl_multi_union_pw_aff_copy(band->mupa) : NULL;
}
