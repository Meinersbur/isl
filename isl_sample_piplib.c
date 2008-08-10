#include "isl_vec.h"
#include "isl_piplib.h"
#include "isl_sample_piplib.h"

struct isl_vec *isl_pip_basic_set_sample(struct isl_ctx *ctx,
	struct isl_basic_set *bset)
{
	PipOptions	*options = NULL;
	PipMatrix	*domain = NULL;
	PipQuast	*sol = NULL;
	struct isl_basic_map *bmap = (struct isl_basic_map *)bset;
	struct isl_vec *vec = NULL;
	unsigned	dim;

	if (!bmap)
		goto error;
	dim = bmap->nparam + bmap->n_in + bmap->n_out;
	domain = isl_basic_map_to_pip(bmap, 0, 0, 0);
	if (!domain)
		goto error;

	options = pip_options_init();
	if (!options)
		goto error;
	options->Simplify = 1;
	options->Urs_unknowns = -1;
	sol = pip_solve(domain, NULL, -1, options);
	if (!sol)
		goto error;
	if (!sol->list)
		vec = isl_vec_alloc(ctx, 0);
	else {
		PipList *l;
		int i;
		vec = isl_vec_alloc(ctx, 1 + dim);
		if (!vec)
			goto error;
		isl_int_set_si(vec->block.data[0], 1);
		for (i = 0, l = sol->list; l && i < dim; ++i, l = l->next) {
			isl_seq_cpy_from_pip(&vec->block.data[1+i],
					&l->vector->the_vector[0], 1);
			if (entier_zero_p(l->vector->the_deno[0]))
				isl_int_set_si(vec->block.data[0], 0);
		}
		if (i != dim)
			goto error;
	}

	pip_quast_free(sol);
	pip_options_free(options);
	pip_matrix_free(domain);

	isl_basic_map_free(ctx, bmap);
	return vec;
error:
	isl_vec_free(ctx, vec);
	isl_basic_map_free(ctx, bmap);
	if (sol)
		pip_quast_free(sol);
	if (domain)
		pip_matrix_free(domain);
	return NULL;
}
