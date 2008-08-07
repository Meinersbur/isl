#include "isl_map.h"
#include "isl_lp.h"
#include "isl_piplib.h"
#include "isl_map_piplib.h"

enum isl_lp_result isl_pip_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt)
{
	enum isl_lp_result res = isl_lp_ok;
	PipMatrix	*domain = NULL;
	PipOptions	*options;
	PipQuast   	*sol;
	unsigned	 total;

	total = bmap->nparam + bmap->n_in + bmap->n_out + bmap->n_div;
	domain = isl_basic_map_to_pip(bmap, 0, 1, 0);
	if (!domain)
		goto error;
	entier_set_si(domain->p[0][1], -1);
	isl_seq_cpy_to_pip(domain->p[0]+2, f, total);

	options = pip_options_init();
	if (!options)
		goto error;
	options->Urs_unknowns = -1;
	options->Maximize = maximize;
	options->Nq = 0;
	sol = pip_solve(domain, NULL, -1, options);
	pip_options_free(options);
	if (!sol)
		goto error;

	if (!sol->list)
		res = isl_lp_empty;
	else if (entier_zero_p(sol->list->vector->the_deno[0]))
		res = isl_lp_unbounded;
	else {
		if (maximize)
			mpz_fdiv_q(*opt, sol->list->vector->the_vector[0],
					 sol->list->vector->the_deno[0]);
		else
			mpz_cdiv_q(*opt, sol->list->vector->the_vector[0],
					 sol->list->vector->the_deno[0]);
	}
	pip_matrix_free(domain);
	pip_quast_free(sol);
	return res;
error:
	if (domain)
		pip_matrix_free(domain);
	return isl_lp_error;
}
