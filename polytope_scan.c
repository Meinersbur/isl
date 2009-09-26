#include <assert.h>
#include "isl_basis_reduction.h"
#include "isl_equalities.h"
#include "isl_seq.h"
#include "isl_tab.h"
#include "isl_vec.h"

/* The input of this program is the same as that of the "polytope_scan"
 * program from the barvinok distribution.
 *
 * Constraints of set is PolyLib format.
 *
 * The input set is assumed to be bounded.
 */

static struct isl_mat *isl_basic_set_samples(struct isl_basic_set *bset);

static struct isl_mat *samples_eq(struct isl_basic_set *bset)
{
	struct isl_mat *T;
	struct isl_mat *samples;

	bset = isl_basic_set_remove_equalities(bset, &T, NULL);
	samples = isl_basic_set_samples(bset);
	return isl_mat_product(samples, isl_mat_transpose(T));
}

/* Add the current sample value of the tableau "tab" to the list
 * in "samples".
 */
static struct isl_mat *add_solution(struct isl_mat *samples,
	struct isl_tab *tab)
{
	struct isl_vec *sample;

	if (!samples || !tab)
		goto error;
	samples = isl_mat_extend(samples, samples->n_row + 1, samples->n_col);
	if (!samples)
		return NULL;
	sample = isl_tab_get_sample_value(tab);
	if (!sample)
		goto error;
	isl_seq_cpy(samples->row[samples->n_row - 1], sample->el, sample->size);
	isl_vec_free(sample);
	return samples;
error:
	isl_mat_free(samples);
	return NULL;
}

/* Look for and return all integer points in "bset", which is assumed
 * to be unbounded.
 *
 * We first compute a reduced basis for the set and then scan
 * the set in the directions of this basis.
 * We basically perform a depth first search, where in each level i
 * we compute the range in the i-th basis vector direction, given
 * fixed values in the directions of the previous basis vector.
 * We then add an equality to the tableau fixing the value in the
 * direction of the current basis vector to each value in the range
 * in turn and then continue to the next level.
 *
 * The search is implemented iteratively.  "level" identifies the current
 * basis vector.  "init" is true if we want the first value at the current
 * level and false if we want the next value.
 * Solutions are added in the leaves of the search tree, i.e., after
 * we have fixed a value in each direction of the basis.
 */
static struct isl_mat *isl_basic_set_samples(struct isl_basic_set *bset)
{
	unsigned dim;
	struct isl_mat *B = NULL;
	struct isl_tab *tab = NULL;
	struct isl_vec *min;
	struct isl_vec *max;
	struct isl_mat *samples;
	struct isl_tab_undo **snap;
	int level;
	int init;
	enum isl_lp_result res;

	if (bset->n_eq)
		return samples_eq(bset);

	dim = isl_basic_set_total_dim(bset);

	min = isl_vec_alloc(bset->ctx, dim);
	max = isl_vec_alloc(bset->ctx, dim);
	samples = isl_mat_alloc(bset->ctx, 0, 1 + dim);
	snap = isl_alloc_array(bset->ctx, struct isl_tab_undo *, dim);

	if (!min || !max || !samples || !snap)
		goto error;

	tab = isl_tab_from_basic_set(bset);
	if (!tab)
		goto error;

	tab->basis = isl_mat_identity(bset->ctx, dim);
	if (1)
		tab = isl_tab_compute_reduced_basis(tab);
	if (!tab)
		goto error;
	B = isl_mat_lin_to_aff(isl_mat_copy(tab->basis));
	if (!B)
		goto error;

	level = 0;
	init = 1;

	while (level >= 0) {
		int empty = 0;
		if (init) {
			res = isl_tab_min(tab, B->row[1 + level],
				    bset->ctx->one, &min->el[level], NULL, 0);
			if (res == isl_lp_empty)
				empty = 1;
			if (res == isl_lp_error || res == isl_lp_unbounded)
				goto error;
			isl_seq_neg(B->row[1 + level] + 1,
				    B->row[1 + level] + 1, dim);
			res = isl_tab_min(tab, B->row[1 + level],
				    bset->ctx->one, &max->el[level], NULL, 0);
			isl_seq_neg(B->row[1 + level] + 1,
				    B->row[1 + level] + 1, dim);
			isl_int_neg(max->el[level], max->el[level]);
			if (res == isl_lp_empty)
				empty = 1;
			if (res == isl_lp_error || res == isl_lp_unbounded)
				goto error;
			snap[level] = isl_tab_snap(tab);
		} else
			isl_int_add_ui(min->el[level], min->el[level], 1);

		if (empty || isl_int_gt(min->el[level], max->el[level])) {
			level--;
			init = 0;
			if (level >= 0)
				isl_tab_rollback(tab, snap[level]);
			continue;
		}
		isl_int_neg(B->row[1 + level][0], min->el[level]);
		tab = isl_tab_add_valid_eq(tab, B->row[1 + level]);
		isl_int_set_si(B->row[1 + level][0], 0);
		if (level < dim - 1) {
			++level;
			init = 1;
			continue;
		}
		samples = add_solution(samples, tab);
		init = 0;
		isl_tab_rollback(tab, snap[level]);
	}

	isl_tab_free(tab);
	free(snap);
	isl_vec_free(min);
	isl_vec_free(max);
	isl_basic_set_free(bset);
	isl_mat_free(B);
	return samples;
error:
	isl_tab_free(tab);
	free(snap);
	isl_mat_free(samples);
	isl_vec_free(min);
	isl_vec_free(max);
	isl_basic_set_free(bset);
	isl_mat_free(B);
	return NULL;
}

int main(int argc, char **argv)
{
	struct isl_ctx *ctx = isl_ctx_alloc();
	struct isl_basic_set *bset;
	struct isl_mat *samples;

	bset = isl_basic_set_read_from_file(ctx, stdin, 0, ISL_FORMAT_POLYLIB);
	samples = isl_basic_set_samples(bset);
	isl_mat_dump(samples, stdout, 0);
	isl_mat_free(samples);
	isl_ctx_free(ctx);

	return 0;
}
