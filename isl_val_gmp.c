#include <isl/val_gmp.h>
#include <isl_val_private.h>

/* Return a reference to an isl_val representing the integer "z".
 */
__isl_give isl_val *isl_val_int_from_gmp(isl_ctx *ctx, mpz_t z)
{
	isl_val *v;

	v = isl_val_alloc(ctx);
	if (!v)
		return NULL;

	isl_int_set(v->n, z);
	isl_int_set_si(v->d, 1);

	return v;
}

/* Return a reference to an isl_val representing the rational value "n"/"d".
 */
__isl_give isl_val *isl_val_from_gmp(isl_ctx *ctx, const mpz_t n, const mpz_t d)
{
	isl_val *v;

	v = isl_val_alloc(ctx);
	if (!v)
		return NULL;

	isl_int_set(v->n, n);
	isl_int_set(v->d, d);

	return isl_val_normalize(v);
}

/* Extract the numerator of a rational value "v" in "z".
 *
 * If "v" is not a rational value, then the result is undefined.
 */
int isl_val_get_num_gmp(__isl_keep isl_val *v, mpz_t z)
{
	if (!v)
		return -1;
	if (!isl_val_is_rat(v))
		isl_die(isl_val_get_ctx(v), isl_error_invalid,
			"expecting rational value", return -1);
	mpz_set(z, v->n);
	return 0;
}

/* Extract the denominator of a rational value "v" in "z".
 *
 * If "v" is not a rational value, then the result is undefined.
 */
int isl_val_get_den_gmp(__isl_keep isl_val *v, mpz_t z)
{
	if (!v)
		return -1;
	if (!isl_val_is_rat(v))
		isl_die(isl_val_get_ctx(v), isl_error_invalid,
			"expecting rational value", return -1);
	mpz_set(z, v->d);
	return 0;
}
