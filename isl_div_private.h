#include <isl/div.h>
#include <isl/map.h>

struct isl_div {
	int ref;
	struct isl_ctx *ctx;

	struct isl_basic_map	*bmap;
	isl_int			**line;
};
