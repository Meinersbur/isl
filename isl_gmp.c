#include "isl_int.h"

uint32_t isl_gmp_hash(mpz_t v, uint32_t hash)
{
	int sa = v[0]._mp_size;
	int abs_sa = sa < 0 ? -sa : sa;
	unsigned char *data = (unsigned char *)v[0]._mp_d;
	unsigned char *end = data + abs_sa * sizeof(v[0]._mp_d[0]);

	if (sa < 0) {
		hash *= 16777619;
		hash ^= 0xFF;
	}
	for (; data < end; ++data) {
		hash *= 16777619;
		hash ^= *data;
	}
	return hash;
}
