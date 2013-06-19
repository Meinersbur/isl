#include <stdio.h>
/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl_int.h>

uint32_t isl_gmp_hash(mpz_t v, uint32_t hash)
{
	int sa = v[0]._mp_size;
	int abs_sa = sa < 0 ? -sa : sa;
	unsigned char *data = (unsigned char *)v[0]._mp_d;
	unsigned char *end = data + abs_sa * sizeof(v[0]._mp_d[0]);

	if (sa < 0)
		isl_hash_byte(hash, 0xFF);
	for (; data < end; ++data)
		isl_hash_byte(hash, *data);
	return hash;
}

/* This function tries to produce outputs that do not depend on
 * the version of GMP that is being used.
 *
 * In particular, when computing the extended gcd of -1 and 9,
 * some versions will produce
 *
 *	1 = -1 * -1 + 0 * 9
 *
 * while other versions will produce
 *
 *	1 = 8 * -1 + 1 * 9
 *
 * If configure detects that we are in the former case, then
 * mpz_gcdext will be called directly.  Otherwise, this function
 * is called and then we try to mimic the behavior of the other versions.
 */
void isl_gmp_gcdext(mpz_t G, mpz_t S, mpz_t T, mpz_t A, mpz_t B)
{
	if (mpz_divisible_p(B, A)) {
		mpz_set_si(S, mpz_sgn(A));
		mpz_set_si(T, 0);
		mpz_abs(G, A);
		return;
	}
	if (mpz_divisible_p(A, B)) {
		mpz_set_si(S, 0);
		mpz_set_si(T, mpz_sgn(B));
		mpz_abs(G, B);
		return;
	}
	mpz_gcdext(G, S, T, A, B);
}
