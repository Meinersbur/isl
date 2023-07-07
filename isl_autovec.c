/*
 * Copyright 2023      zhaosiying12138@IAYJT_LAS
 *
 * Use of this software is governed by the MIT license
 *
 * Written by zhaosiying12138, Institute of Advanced YanJia
 *  Technology - LiuYueCity Academy of Science 
 */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include <isl_aff_private.h>
#include <isl_space_private.h>
#include <isl/id.h>
#include <isl/set.h>
#include <isl/flow.h>
#include <isl_constraint_private.h>
#include <isl/polynomial.h>
#include <isl/union_set.h>
#include <isl/union_map.h>
#include <isl_factorization.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl_options_private.h>
#include <isl_vertices_private.h>
#include <isl/ast_build.h>
#include <isl/val.h>
#include <isl/ilp.h>
#include <isl_ast_build_expr.h>
#include <isl/options.h>

static int zsy_compute_max_common_loops(__isl_keep isl_basic_map *bmap, __isl_keep isl_union_map *S)
{
	isl_basic_set *bmap_domain = isl_basic_map_domain(isl_basic_map_copy(bmap));
	isl_size sched1_dim = isl_basic_set_dim(bmap_domain, isl_dim_set);
	isl_union_map *sched_domain = isl_union_map_intersect_domain(isl_union_map_copy(S), isl_union_set_from_basic_set(bmap_domain));
	isl_union_set *sched_domain_range = isl_union_map_range(sched_domain);
	isl_basic_set_list *sched1_list = isl_union_set_get_basic_set_list(sched_domain_range);
	assert(isl_basic_set_list_n_basic_set(sched1_list) == 1);
	isl_basic_set *sched1 = isl_basic_set_list_get_basic_set(sched1_list, 0);

	isl_basic_set *bmap_range = isl_basic_map_range(isl_basic_map_copy(bmap));
	isl_size sched2_dim = isl_basic_set_dim(bmap_range, isl_dim_set);
	isl_union_map *sched_range = isl_union_map_intersect_domain(isl_union_map_copy(S), isl_union_set_from_basic_set(bmap_range));
	isl_union_set *sched_range_range = isl_union_map_range(sched_range);
	isl_basic_set_list *sched2_list = isl_union_set_get_basic_set_list(sched_range_range);
	assert(isl_basic_set_list_n_basic_set(sched2_list) == 1);
	isl_basic_set *sched2 = isl_basic_set_list_get_basic_set(sched2_list, 0);
	
	isl_size sched_dim_min = sched1_dim < sched2_dim ? sched1_dim : sched2_dim;
	//printf("sched1_dim = %d, sched2_dim = %d\n", sched1_dim, sched2_dim);

	int max_common_loops = 0;
	isl_int tmp_val1, tmp_val2;
	isl_int_init(tmp_val1);
	isl_int_init(tmp_val2);
	for (int i = 0; i < sched_dim_min * 2; i += 2) {
		assert(isl_basic_set_plain_dim_is_fixed(sched1, i, &tmp_val1));
		assert(isl_basic_set_plain_dim_is_fixed(sched2, i, &tmp_val2));
		if (isl_int_eq(tmp_val1, tmp_val2)) {
			max_common_loops++;
			continue;
		} else {
			break;
		}
	}

	return max_common_loops;
}

static int zsy_compute_loop_carried(__isl_keep isl_basic_map *bmap, __isl_keep isl_union_map *S)
{
	isl_size common_loop_size = zsy_compute_max_common_loops(bmap, S);
	isl_basic_map *bmap_tmp = isl_basic_map_copy(bmap);
	isl_size dim_in_size = isl_basic_map_dim(bmap_tmp, isl_dim_in);
	isl_size dim_out_size = isl_basic_map_dim(bmap_tmp, isl_dim_out);
	isl_int tmp_val;
	isl_int_init(tmp_val);
	int i;

	bmap_tmp = isl_basic_map_project_out(bmap_tmp, isl_dim_in,  common_loop_size, dim_in_size - common_loop_size);
	bmap_tmp = isl_basic_map_project_out(bmap_tmp, isl_dim_out, common_loop_size, dim_out_size - common_loop_size);
	isl_basic_set *delta = isl_basic_map_deltas(bmap_tmp);
	//isl_basic_set_dump(delta);

	for (i = 0; i < common_loop_size; i++) {
		if (isl_basic_set_plain_dim_is_fixed(delta, i, &tmp_val)) {
			if (isl_int_is_zero(tmp_val)) {
				continue;
			}
		}
		break;
	}
	if (i == common_loop_size)
		i = -1;
	else
		++i;
	isl_basic_map_dump(bmap);
	printf("max_common_loops = %d, loop_carried = %d\n", common_loop_size, i);

	isl_basic_set_free(delta);
	return i;
}

static void zsy_dump_dependence(__isl_keep isl_union_map *dep, __isl_keep isl_union_map *S)
{
	isl_map_list *map_list = isl_union_map_get_map_list(dep);
	for (int i = 0; i < isl_map_list_n_map(map_list); i++) {
		isl_map *tmp_map = isl_map_list_get_map(map_list, i);
		isl_basic_map_list *tmp_bmap_list = isl_map_get_basic_map_list(tmp_map);
		for (int j = 0; j < isl_basic_map_list_n_basic_map(tmp_bmap_list); j++) {
			isl_basic_map *tmp_bmap = isl_basic_map_list_get_basic_map(tmp_bmap_list, j);
			zsy_compute_loop_carried(tmp_bmap, S);
		}
	}
}

int zsy_test_autovec(isl_ctx *ctx)
{
	const char *con, *d, *w, *r, *s;
	isl_set *CON;
	isl_union_set *D, *delta;
	isl_set *delta_set, *delta_set_lexmin, *delta_set_lexmax;
	isl_union_map *W, *R, *W_rev, *R_rev, *S, *S_lt_S;
	isl_union_map *empty;
	isl_union_map *dep_raw, *dep_war, *dep_waw, *dep;
	isl_union_map *validity, *proximity, *coincidence;
	isl_union_map *schedule, *schedule_isl, *schedule_paper, *schedule_test;
	isl_schedule_constraints *sc;
	isl_schedule *sched;
	isl_ast_build *build;
	isl_ast_node *tree;

	isl_options_set_schedule_serialize_sccs(ctx, 1);
	con = "{ : }";
	d = "[N] -> { S1[i] : 1 <= i <= 100; S2[i, j] : 1 <= i, j <= 100; "
					" S3[i, j, k] : 1 <= i, j, k <= 100; S4[i, j] : 1 <= i, j <= 100 } ";
	w = "[N] -> { S1[i] -> X[i]; S2[i, j] -> B[j]; S3[i, j, k] -> A[j + 1, k]; S4[i, j] -> Y[i + j] }";
	r = "[N] -> { S1[i] -> Y[i]; S2[i, j] -> A[j, N]; S3[i, j, k] -> B[j]; S3[i, j, k] -> C[j, k]; S4[i, j] -> A[j + 1, N] }";
	s = "[N] -> { S1[i] -> [0, i, 0, 0, 0, 0, 0]; S2[i, j] -> [0, i, 1, j, 0, 0, 0]; "
					" S3[i, j, k] -> [0, i, 1, j, 1, k, 0]; S4[i, j] -> [0, i, 1, j, 2, 0, 0] }";

	CON = isl_set_read_from_str(ctx, con);
	D = isl_union_set_read_from_str(ctx, d);
	W = isl_union_map_read_from_str(ctx, w);
	R = isl_union_map_read_from_str(ctx, r);
	S = isl_union_map_read_from_str(ctx, s);

	W = isl_union_map_intersect_domain(W, isl_union_set_copy(D));
	R = isl_union_map_intersect_domain(R, isl_union_set_copy(D));
	S = isl_union_map_intersect_domain(S, isl_union_set_copy(D));

	build = isl_ast_build_from_context(isl_set_read_from_str(ctx, con));
	tree = isl_ast_build_node_from_schedule_map(build, isl_union_map_copy(S));
	printf("Before Transform:\n%s\n", isl_ast_node_to_C_str(tree));

	W_rev = isl_union_map_reverse(isl_union_map_copy(W));
	R_rev = isl_union_map_reverse(isl_union_map_copy(R));
	S_lt_S = isl_union_map_lex_lt_union_map(isl_union_map_copy(S), isl_union_map_copy(S));
	dep_raw = isl_union_map_apply_range(isl_union_map_copy(W), isl_union_map_copy(R_rev));
	dep_raw = isl_union_map_intersect(dep_raw, isl_union_map_copy(S_lt_S));
	dep_waw = isl_union_map_apply_range(isl_union_map_copy(W), isl_union_map_copy(W_rev));
	dep_waw = isl_union_map_intersect(dep_waw, isl_union_map_copy(S_lt_S));
	dep_war = isl_union_map_apply_range(isl_union_map_copy(R), isl_union_map_copy(W_rev));
	dep_war = isl_union_map_intersect(dep_war, isl_union_map_copy(S_lt_S));

#if 1
	printf("\nRAW Dependence:\n");
	zsy_dump_dependence(dep_raw, S);
	printf("\nWAW Dependence:\n");
	zsy_dump_dependence(dep_waw, S);
	printf("\nWAR Dependence:\n");
	zsy_dump_dependence(dep_war, S);
#endif

	dep = isl_union_map_union(isl_union_map_copy(dep_waw), isl_union_map_copy(dep_war));
	dep = isl_union_map_union(dep, isl_union_map_copy(dep_raw));

#if 0
	isl_basic_map *t_map = isl_basic_map_read_from_str(ctx, " [N] -> { S3[i, j, k] -> S2[i', j'] : (k = N and j' = 1 + j and N > 0 and N <= 100 and i > 0 and i <= 100 and j > 0 and j <= 99 and i' > 0 and i' <= 100 and i' > i) } ");
	zsy_compute_loop_carried(t_map, S);
	

	t_map = isl_basic_map_read_from_str(ctx, " [N] -> { S3[i, j, k] -> S2[i', j'] : (k = N and i' = i and j' = 1 + j and N > 0 and N <= 100 and i > 0 and i <= 100 and j > 0 and j <= 99) } ");
	zsy_compute_loop_carried(t_map, S);

	t_map = isl_basic_map_read_from_str(ctx, " [N] -> { S3[i, j, k] -> S4[i', j'] : (k = N and i' = i and j' = j and N > 0 and N <= 100 and i > 0 and i <= 100 and j > 0 and j <= 100) } ");
	zsy_compute_loop_carried(t_map, S);
#endif

	validity = isl_union_map_copy(dep);
	coincidence = isl_union_map_copy(dep);
	proximity = isl_union_map_copy(dep);

	sc = isl_schedule_constraints_on_domain(isl_union_set_copy(D));
	sc = isl_schedule_constraints_set_context(sc, CON);
	sc = isl_schedule_constraints_set_validity(sc, validity);
	sc = isl_schedule_constraints_set_coincidence(sc, coincidence);
	sc = isl_schedule_constraints_set_proximity(sc, proximity);
	sched = isl_schedule_constraints_compute_schedule(sc);
	isl_schedule_dump(sched);
	schedule = isl_schedule_get_map(sched);
	printf("Schedule:\n");
	isl_union_map_dump(schedule);

//	tree = isl_ast_build_node_from_schedule_map(build, schedule);
//	printf("After Transform:\n%s\n", isl_ast_node_to_C_str(tree));
//	isl_ast_node_free(tree);


	return 0;
}

int main(int argc, char **argv)
{
	int i;
	struct isl_ctx *ctx;
	struct isl_options *options;

	options = isl_options_new_with_defaults();
	assert(options);
	argc = isl_options_parse(options, argc, argv, ISL_ARG_ALL);
	ctx = isl_ctx_alloc_with_options(&isl_options_args, options);

	printf("AutoVectorization written by zhaosiying12138@Institute of Advanced YanJia"
				" Technology, LiuYueCity Academy of Science\n");
	zsy_test_autovec(ctx);

	isl_ctx_free(ctx);
	return 0;
error:
	isl_ctx_free(ctx);
	return -1;
}
