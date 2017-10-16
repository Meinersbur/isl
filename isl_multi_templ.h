#include <isl/space.h>

#include <isl_multi_macro.h>

/* A multiple expression with base expressions of type EL.
 *
 * "space" is the space in which the multiple expression lives.
 * "n" is the number of base expression and is equal
 * to the output or set dimension of "space".
 * "p" is an array of size "n" of base expressions.
 * The array is only accessible when n > 0.
 */
struct MULTI(BASE) {
	int ref;
	isl_space *space;

	int n;
	struct {
		EL *p[1];
	} u;
};

__isl_give MULTI(BASE) *CAT(MULTI(BASE),_alloc)(__isl_take isl_space *space);
