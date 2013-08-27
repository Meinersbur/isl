#ifndef ISL_SCHEDLUE_PRIVATE_H
#define ISL_SCHEDLUE_PRIVATE_H

#include <isl/aff.h>
#include <isl/schedule.h>

enum isl_edge_type {
	isl_edge_validity = 0,
	isl_edge_first = isl_edge_validity,
	isl_edge_coincidence,
	isl_edge_condition,
	isl_edge_conditional_validity,
	isl_edge_proximity,
	isl_edge_last = isl_edge_proximity
};

/* The constraints that need to be satisfied by a schedule on "domain".
 *
 * "validity" constraints map domain elements i to domain elements
 * that should be scheduled after i.  (Hard constraint)
 * "proximity" constraints map domain elements i to domains elements
 * that should be scheduled as early as possible after i (or before i).
 * (Soft constraint)
 *
 * "condition" and "conditional_validity" constraints map possibly "tagged"
 * domain elements i -> s to "tagged" domain elements j -> t.
 * The elements of the "conditional_validity" constraints, but without the
 * tags (i.e., the elements i -> j) are treated as validity constraints,
 * except that during the construction of a tilable band,
 * the elements of the "conditional_validity" constraints may be violated
 * provided that all adjacent elements of the "condition" constraints
 * are local within the band.
 * A dependence is local within a band if domain and range are mapped
 * to the same schedule point by the band.
 */
struct isl_schedule_constraints {
	isl_union_set *domain;

	isl_union_map *constraint[isl_edge_last + 1];
};

/* The schedule for an individual domain, plus information about the bands
 * and scheduling dimensions.
 * In particular, we keep track of the number of bands and for each
 * band, the starting position of the next band.  The first band starts at
 * position 0.
 * For each scheduling dimension, we keep track of whether it satisfies
 * the coincidence constraints (within its band).
 */
struct isl_schedule_node {
	isl_multi_aff *sched;
	int	 n_band;
	int	*band_end;
	int	*band_id;
	int	*coincident;
};

/* Information about the computed schedule.
 * n is the number of nodes/domains/statements.
 * n_band is the maximal number of bands.
 * n_total_row is the number of coordinates of the schedule.
 * dim contains a description of the parameters.
 * band_forest points to a band forest representation of the schedule
 * and may be NULL if the forest hasn't been created yet.
 */
struct isl_schedule {
	int ref;

	int n;
	int n_band;
	int n_total_row;
	isl_space *dim;

	isl_band_list *band_forest;

	struct isl_schedule_node node[1];
};

#endif
