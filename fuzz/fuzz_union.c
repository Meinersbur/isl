#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "isl/ctx.h"
#include "isl/union_set.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
	char* data_cstr = strndup(data, size);

	if (data_cstr == NULL) {
		return 0;
	}

	isl_ctx* ctx = isl_ctx_alloc();
	isl_union_map_read_from_str(ctx, data_cstr);
	isl_union_set_read_from_str(ctx, data_cstr);

	if (size > 10) {
		data += 5;
		size -= 5;
		char *data_cstr2 = strndup(data, size);

		isl_union_set* tmp_set = isl_union_set_read_from_str(ctx, data_cstr);
		isl_union_map* tmp_map = isl_union_map_read_from_str(ctx, data_cstr2);

		isl_union_map_intersect_domain(tmp_map, tmp_set);
		free(data_cstr2);
	}

	isl_ctx_free(ctx);
	free(data_cstr);

	return 0;
}
