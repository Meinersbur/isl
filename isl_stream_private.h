#include <isl_int.h>
#include <isl/stream.h>

struct isl_token {
	int type;

	unsigned int on_new_line : 1;
	unsigned is_keyword : 1;
	int line;
	int col;

	union {
		isl_int	v;
		char	*s;
		isl_map *map;
		isl_pw_aff *pwaff;
	} u;
};

struct isl_token *isl_token_new(isl_ctx *ctx,
	int line, int col, unsigned on_new_line);

struct isl_stream {
	struct isl_ctx	*ctx;
	FILE        	*file;
	const char  	*str;
	int	    	line;
	int	    	col;
	int	    	eof;

	char	    	*buffer;
	size_t	    	size;
	size_t	    	len;
	int	    	c;
	int		un[5];
	int		n_un;

	struct isl_token	*tokens[5];
	int	    	n_token;

	struct isl_hash_table	*keywords;
	enum isl_token_type	 next_type;
};
