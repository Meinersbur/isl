/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <assert.h>
#include "isl_sample.h"
#include <isl/vec.h>
#include "isl_map_private.h"

int main(int argc, char **argv)
{
	struct isl_ctx *ctx = isl_ctx_alloc();
	struct isl_basic_set *bset;
	struct isl_vec *sample;

	bset = isl_basic_set_read_from_file(ctx, stdin, 0);
	sample = isl_basic_set_sample_vec(isl_basic_set_copy(bset));
	isl_vec_dump(sample, stdout, 0);
	assert(sample);
	if (sample->size > 0)
		assert(isl_basic_set_contains(bset, sample));
	isl_basic_set_free(bset);
	isl_vec_free(sample);
	isl_ctx_free(ctx);

	return 0;
}
